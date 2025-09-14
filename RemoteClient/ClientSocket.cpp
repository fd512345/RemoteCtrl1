#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

void Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}
bool CClientSocket::InitSocket()
{     // 初始化Socket并连接服务器
	if (m_sock != INVALID_SOCKET)CloseSocket();  // 关闭已有连接
	m_sock = socket(PF_INET, SOCK_STREAM, 0);  // 创建TCP Socket
	if (m_sock == -1)return false;        // 创建失败返回false
	sockaddr_in serv_adr;                 // 服务器地址结构体
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;        // IPv4协议
	TRACE("addr %08X nIP %08X\r\n", inet_addr("127.0.0.1"), m_nIP);  // 调试输出IP
	serv_adr.sin_addr.s_addr = htonl(m_nIP);  // 设置服务器IP（主机字节序转网络字节序）
	serv_adr.sin_port = htons(m_nPort);     // 设置服务器端口（主机字节序转网络字节序）
	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {  // IP地址无效
		AfxMessageBox("指定的IP地址不存在！");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));  // 连接服务器
	if (ret == -1) {                      // 连接失败
		AfxMessageBox("连接失败!");
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	TRACE("socket init done!\r\n");
	return true;                           // 连接成功
}
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
	{
		//if (InitSocket() == false) return false;
		// 调用 InitSocket 函数初始化套接字，若返回 false（初始化失败），则当前函数也返回 false			
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start thread\r\n");  // 输出调试信息“start thread”，用于在调试时跟踪线程启动的相关情况
		// 调用 _beginthread 函数创建一个新线程，线程入口函数为 CClientSocket 类的 threadEntry 静态成员函数，
		// 线程栈大小为 0（使用默认栈大小），传递当前对象（this 指针）作为线程函数的参数

	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	// 向 m_mapAck 中插入一个键为 head.hEvent（HANDLE 类型）、值为空 lstPacks
	// auto 用于自动推导 pr 的类型（std::pair<std::map<HANDLE, std::list<CPacket>>::iterator, bool>）
	m_lstSend.push_back(pack); // 将数据包 pack 加入待发送队列 m_lstSend
	m_lock.unlock();
	TRACE("cmd:%d event: %08X\r\n", pack.sCmd, pack.hEvent);  // 输出调试信息，格式为“cmd:命令值 event 事件句柄的8位十六进制值”，用于调试跟踪命令和事件相关信息
	WaitForSingleObject(pack.hEvent, INFINITE); // 无限等待 pack 对应的事件对象
	TRACE("cmd:%d event: %08X\r\n", pack.sCmd, pack.hEvent);  // 输出调试信息，格式为“cmd:命令值 event 事件句柄的8位十六进制值”，用于调试跟踪命令和事件相关信息
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent); // 在 m_mapAck 中查找 pack.hEvent 对应的键值对

	if (it != m_mapAck.end()) { // 如果找到对应的键值对
		m_lock.lock();
		m_mapAck.erase(it);// 从 m_mapAck 容器中删除由迭代器 it 指向的键值对
		m_lock.unlock();
		return true; // 返回 true 表示成功
	}
	return false; // 示例返回值，实际可根据函数逻辑调整返回结果
}
void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg; // 将 void* 类型的 arg 强制转换为 CClientSocket* 类型并赋值给 thiz
	thiz->threadFunc(); // 调用 thiz 指向的 CClientSocket 对象的 threadFunc 成员函数
}
void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE); // 调整 strBuffer 大小为 BUFFER_SIZE
	char* pBuffer = (char*)strBuffer.c_str(); // 获取 strBuffer 的 C 风格字符串指针并转为 char*
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) { // 当套接字有效时循环
		if (m_lstSend.size() > 0) { // 如果待发送数据包列表不为空
			TRACE("lstSend size: %d\r\n", m_lstSend.size());// 打印调试信息，输出待发送数据包列表 m_lstSend 的元素数量
			m_lock.lock();
			CPacket& head = m_lstSend.front(); // 获取列表头部的数据包引用
			m_lock.unlock();
			if (Send(head) == false) { // 调用 Send 函数发送该数据包，若发送失败
				TRACE("发送失败！\r\n"); // 打印发送失败的调试信息

				continue; // 继续循环，尝试后续操作或再次发送
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent); // 在 m_mapAck 中查找 pack.hEvent 对应的键值对
			if (it != m_mapAck.end()) {    // 判断迭代器it是否未指向m_mapAck的末尾（即是否找到对应元素）
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				// 在 m_mapAutoClosed 这个 std::map 容器中，查找键为 head.hEvent 的键值对，
				// 并将找到的迭代器（若存在）或 end() 迭代器赋值给 it0
				do
				{
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv %d %d\r\n", length, index);
					if ((length > 0) || (index > 0)) {
						index += length; // 累加接收数据的长度到索引，方便后续处理
						size_t size = (size_t)index; // 将索引转换为 size_t 类型，用于表示数据大小
						CPacket pack((BYTE*)pBuffer, size); // 用接收到的数据和大小构造 CPacket 对象
						if (size > 0) { // 如果有有效数据
							// 向 m_mapAck 中键为 head.hEvent 的对应值（std::list<CPacket> 类型）中添加数据包 pack				
							pack.hEvent = head.hEvent; // 将数据包的事件句柄设置为发送数据包的事件句柄
							it->second.push_back(pack);
							SetEvent(head.hEvent); // 触发数据包关联的事件，通知数据已准备好
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							TRACE("SetEvent %d %d\r\n", pack.sCmd, it0->second);
							if (it0->second)
							{
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0) { // 接收长度小于等于0且索引也小于等于0，说明可能连接有问题
						CloseSocket(); // 关闭套接字
						SetEvent(head.hEvent);//等到服务器关闭命令之后 再通知事件完成
						if (it0 != m_mapAutoClosed.end()) {    // 判断迭代器it0是否未指向m_mapAutoClosed的末尾（即是否找到对应元素）
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);  // 输出调试信息，显示命令和对应的值
						}
						else {
							TRACE("异常的情况，没有对应的pair\r\n");  // 输出异常情况的调试信息，表明没有找到对应的键值对
						}
					}
				} while (it0->second == false);
			}

			m_lock.lock();
			m_lstSend.pop_front(); // 从待发送列表中移除已处理的数据包
			m_mapAutoClosed.erase(head.hEvent);        // 从m_mapAutoClosed中删除it0指向的元素
			m_lock.unlock();
			if (InitSocket() == false)
				InitSocket();
		}
		Sleep(1);
	}
	CloseSocket(); // 关闭套接字连接
}
void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {  // 获取消息，若获取到有效消息则进入循环
		TranslateMessage(&msg);  // 转换消息（将虚拟键消息转换为字符消息）
		DispatchMessage(&msg);   // 分发消息（将消息发送到窗口过程处理）
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {  // 在m_mapFunc中查找当前消息对应的处理函数，若找到（即迭代器不等于end()）
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);  // 调用找到的成员函数处理该消息
		}
	}
}
bool CClientSocket::Send(const CPacket& pack)
{                // 发送数据包
	TRACE("m_sock = %d\r\n", m_sock);     // 调试输出Socket句柄
	if (m_sock == -1)return false;        // Socket无效返回false
	std::string strOut;			  // 用于存储序列化后的数据
	pack.Data(strOut);                  // 序列化数据包
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;  // 发送序列化后的数据包
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//TODO:定义一个消息的数据结构（数据和数据长度，模式） 回调消息的数据结构（HWND MESSAGE）
	if (InitSocket() == true) {  // 初始化套接字成功
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);  // 调用send函数发送数据
		if (ret > 0) {  // 发送成功（发送字节数大于0）
			// 可在此处添加发送成功后的逻辑，比如打印发送成功日志等
		}
		else {  // 发送失败
			CloseSocket();  // 关闭套接字
			//网络终止处理
		}
	}
	else {  // 初始化套接字失败
		//TODO:错误处理，比如弹出错误提示、记录错误日志等
	}
}
