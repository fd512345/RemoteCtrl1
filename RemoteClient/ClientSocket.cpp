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
CClientSocket::CClientSocket(const CClientSocket& ss)
{  // 禁用拷贝构造函数
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();  // 获取ss.m_mapFunc的起始常量迭代器
	for (; it != ss.m_mapFunc.end(); it++) {  // 遍历ss.m_mapFunc
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));  // 将ss.m_mapFunc中的键值对插入到m_mapFunc中
	}
}
CClientSocket::CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE)
{// 私有构造函数（单例模式）
	if (InitSockEnv() == FALSE) {         // 初始化Socket环境
		MessageBox(NULL, _T("无法初始化网络环境，程序即将退出！"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);  // 创建一个事件对象m_eventInvoke，参数依次为：安全属性（NULL表示默认）、手动重置（TRUE）、初始状态非触发（FALSE）、事件名（NULL表示无命名）
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);  // 创建线程，参数依次为：安全属性（NULL）、堆栈大小（0表示默认）、线程入口函数（CClientSocket类的threadEntry静态方法）、传递给线程的参数（this指针，即当前对象）、创建标志（0）、接收线程ID的变量地址（&m_nThreadID），并将线程句柄赋值给m_hThread
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {  // 等待m_eventInvoke事件对象，超时时间100毫秒，若返回WAIT_TIMEOUT表示等待超时
		TRACE("网络消息处理线程启动失败了！\r\n");  // 输出调试信息，提示网络消息处理线程启动失败
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);         // 初始化缓冲区大小
	memset(m_buffer.data(), 0, BUFFER_SIZE);  // 清空缓冲区
	struct {
		UINT message;  // 消息类型，用于标识不同的消息
		MSGFUNC func;  // 函数指针，用于处理对应消息
	} funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		// {WM_SEND_PACK, /* 可添加更多消息与处理函数的对应项 */},
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {  // 遍历funcs数组，直到遇到message为0的元素
		// 向m_mapFunc中插入键值对（funcs[i].message为键，funcs[i].func为值），若插入失败（即该键已存在）
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			// 输出调试信息，提示插入失败，并显示相关的消息值、函数值和序号
			TRACE("插入失败，消息值：%d 函数值:%08X 序号:%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
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

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	// 根据isAutoClosed标志确定模式，若为true则模式为CSM_AUTOCLOSE，否则为0
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);  // 从pack中获取数据到strOut
	// 向线程m_nThreadID发送WM_SEND_PACK消息，附带新创建的PACKET_DATA对象（包含strOut的数据、长度和窗口句柄hWnd）
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);  // 动态创建PACKET_DATA对象，传入strOut的C字符串、长度、模式nMode和参数wParam
	// 向线程ID为m_nThreadID的线程发送WM_SEND_PACK消息，附带pData（转为WPARAM）和hWnd（转为LPARAM），并将发送结果赋值给ret
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {  // 如果消息发送失败
		delete pData;  // 释放之前创建的pData对象，防止内存泄漏
	}
	return ret;  // 返回消息发送结果ret
}

//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
//	{
//		//if (InitSocket() == false) return false;
//		// 调用 InitSocket 函数初始化套接字，若返回 false（初始化失败），则当前函数也返回 false			
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
//		TRACE("start thread\r\n");  // 输出调试信息“start thread”，用于在调试时跟踪线程启动的相关情况
//		// 调用 _beginthread 函数创建一个新线程，线程入口函数为 CClientSocket 类的 threadEntry 静态成员函数，
//		// 线程栈大小为 0（使用默认栈大小），传递当前对象（this 指针）作为线程函数的参数
//
//	}
//
//	m_lock.lock();
//	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
//	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
//	// 向 m_mapAck 中插入一个键为 head.hEvent（HANDLE 类型）、值为空 lstPacks
//	// auto 用于自动推导 pr 的类型（std::pair<std::map<HANDLE, std::list<CPacket>>::iterator, bool>）
//	m_lstSend.push_back(pack); // 将数据包 pack 加入待发送队列 m_lstSend
//	m_lock.unlock();
//	TRACE("cmd:%d event: %08X\r\n", pack.sCmd, pack.hEvent);  // 输出调试信息，格式为“cmd:命令值 event 事件句柄的8位十六进制值”，用于调试跟踪命令和事件相关信息
//	WaitForSingleObject(pack.hEvent, INFINITE); // 无限等待 pack 对应的事件对象
//	TRACE("cmd:%d event: %08X\r\n", pack.sCmd, pack.hEvent);  // 输出调试信息，格式为“cmd:命令值 event 事件句柄的8位十六进制值”，用于调试跟踪命令和事件相关信息
//	std::map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent); // 在 m_mapAck 中查找 pack.hEvent 对应的键值对
//
//	if (it != m_mapAck.end()) { // 如果找到对应的键值对
//		m_lock.lock();
//		m_mapAck.erase(it);// 从 m_mapAck 容器中删除由迭代器 it 指向的键值对
//		m_lock.unlock();
//		return true; // 返回 true 表示成功
//	}
//	return false; // 示例返回值，实际可根据函数逻辑调整返回结果
//}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg; // 将 void* 类型的 arg 强制转换为 CClientSocket* 类型并赋值给 thiz
	thiz->threadFunc2(); // 调用 thiz 指向的 CClientSocket 对象的 threadFunc2 成员函数
	_endthreadex(0);
	return 0;
}
//void CClientSocket::threadFunc()
//{
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE); // 调整 strBuffer 大小为 BUFFER_SIZE
//	char* pBuffer = (char*)strBuffer.c_str(); // 获取 strBuffer 的 C 风格字符串指针并转为 char*
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET) { // 当套接字有效时循环
//		if (m_lstSend.size() > 0) { // 如果待发送数据包列表不为空
//			TRACE("lstSend size: %d\r\n", m_lstSend.size());// 打印调试信息，输出待发送数据包列表 m_lstSend 的元素数量
//			m_lock.lock();
//			CPacket& head = m_lstSend.front(); // 获取列表头部的数据包引用
//			m_lock.unlock();
//			if (Send(head) == false) { // 调用 Send 函数发送该数据包，若发送失败
//				TRACE("发送失败！\r\n"); // 打印发送失败的调试信息
//
//				continue; // 继续循环，尝试后续操作或再次发送
//			}
//			std::map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent); // 在 m_mapAck 中查找 pack.hEvent 对应的键值对
//			if (it != m_mapAck.end()) {    // 判断迭代器it是否未指向m_mapAck的末尾（即是否找到对应元素）
//				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
//				// 在 m_mapAutoClosed 这个 std::map 容器中，查找键为 head.hEvent 的键值对，
//				// 并将找到的迭代器（若存在）或 end() 迭代器赋值给 it0
//				do
//				{
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//					TRACE("recv %d %d\r\n", length, index);
//					if ((length > 0) || (index > 0)) {
//						index += length; // 累加接收数据的长度到索引，方便后续处理
//						size_t size = (size_t)index; // 将索引转换为 size_t 类型，用于表示数据大小
//						CPacket pack((BYTE*)pBuffer, size); // 用接收到的数据和大小构造 CPacket 对象
//						if (size > 0) { // 如果有有效数据
//							// 向 m_mapAck 中键为 head.hEvent 的对应值（std::list<CPacket> 类型）中添加数据包 pack				
//							pack.hEvent = head.hEvent; // 将数据包的事件句柄设置为发送数据包的事件句柄
//							it->second.push_back(pack);
//							SetEvent(head.hEvent); // 触发数据包关联的事件，通知数据已准备好
//							memmove(pBuffer, pBuffer + size, index - size);
//							index -= size;
//							TRACE("SetEvent %d %d\r\n", pack.sCmd, it0->second);
//							if (it0->second)
//							{
//								SetEvent(head.hEvent);
//								break;
//							}
//						}
//					}
//					else if (length <= 0) { // 接收长度小于等于0且索引也小于等于0，说明可能连接有问题
//						CloseSocket(); // 关闭套接字
//						SetEvent(head.hEvent);//等到服务器关闭命令之后 再通知事件完成
//						if (it0 != m_mapAutoClosed.end()) {    // 判断迭代器it0是否未指向m_mapAutoClosed的末尾（即是否找到对应元素）
//							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);  // 输出调试信息，显示命令和对应的值
//						}
//						else {
//							TRACE("异常的情况，没有对应的pair\r\n");  // 输出异常情况的调试信息，表明没有找到对应的键值对
//						}
//					}
//				} while (it0->second == false);
//			}
//
//			m_lock.lock();
//			m_lstSend.pop_front(); // 从待发送列表中移除已处理的数据包
//			m_mapAutoClosed.erase(head.hEvent);        // 从m_mapAutoClosed中删除it0指向的元素
//			m_lock.unlock();
//			if (InitSocket() == false)
//				InitSocket();
//		}
//		Sleep(1);
//	}
//	CloseSocket(); // 关闭套接字连接
//}

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
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
{//TODO:定义一个消息的数据结构（数据和数据长度，模式） 回调消息的数据结构（HWND）
	PACKET_DATA data = *(PACKET_DATA*)wParam;  // 将wParam强制转换为PACKET_DATA*类型后解引用，赋值给PACKET_DATA类型的变量data，用于获取消息参数中传递的PACKET_DATA数据
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();  // 获取data.strData的长度，赋值给nTemp
	CPacket current((BYTE*)data.strData.c_str(), nTemp);  // 用data.strData的C字符串（转为BYTE*）和长度nTemp，构造CPacket对象current
	if (InitSocket() == true) {  // 初始化套接字成功
		PACKET_DATA data = *(PACKET_DATA*)wParam;
		delete (PACKET_DATA*)wParam;
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);  // 调用send函数，向m_sock对应的套接字发送数据，数据为data.strData的字符内容，数据长度为data.strData的大小，标志为0，返回值存入ret
		if (ret > 0) {  // 发送成功（发送字节数大于0）
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);  // 调整字符串缓冲区大小为BUFFER_SIZE
			char* pBuffer = (char*)strBuffer.c_str();  // 获取字符串缓冲区的字符指针
			while (m_sock != INVALID_SOCKET) {  // 当套接字有效时，持续接收数据
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);  // 从套接字m_sock接收数据到pBuffer + index位置，最多接收BUFFER_SIZE - index字节
				if (length > 0 || (index > 0)) {  // 若接收长度大于0，或已有数据（index>0）
					index += (size_t)length;  // 更新已接收数据的索引
					size_t nLen = index;  // 记录当前已接收数据总长度
					CPacket pack((BYTE*)pBuffer, nLen);  // 用接收的数据创建CPacket对象
					if (nLen > 0) {  // 若有有效数据
						TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nLen);  // 输出调试信息，显示确认包的命令pack.sCmd、窗口句柄hWnd（以8位十六进制格式）、索引index和长度nLen	
						TRACE("%04X\r\n", *(WORD*)pBuffer + nLen);  // 将pBuffer强制转换为WORD*类型后取值，再加上index和nLen，以4位十六进制格式输出结果，用于调试查看相关数据计算后的值
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);  // 向窗口hWnd发送WM_SEND_PACK_ACK消息，附带新创建的CPacket对象
						if (data.nMode & CSM_AUTOCLOSE) {  // 若数据模式包含自动关闭模式
							CloseSocket();  // 关闭套接字
							return;  // 结束当前处理流程
						}
						index -= nLen;  // 调整索引，为后续接收做准备
						memmove(pBuffer, pBuffer + nLen, index);  // 移动剩余数据到缓冲区起始位置
					}
				}
				else {  // 接收失败或对方关闭连接等情况
					//TODO：对方关闭了套接字，或者网络设备异常
					TRACE("recv failed length %d index %d cmd %d\r\n", length, index, current.sCmd);  // 输出调试信息，提示接收失败，并显示相关的长度length、索引index以及命令current.sCmd的值
					CloseSocket();  // 关闭套接字
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);  // 向窗口hWnd发送WM_SEND_PACK_ACK消息，参数为NULL
				}
			}
		}
		else {  // 发送失败
			CloseSocket();  // 关闭套接字
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), -1);  // 向窗口hWnd发送WM_SEND_PACK_ACK消息，参数为
		}
	}
	else {  // 初始化套接字失败
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);  // 向窗口hWnd发送WM_SEND_PACK_ACK消息，参数为NULL
	}
}
