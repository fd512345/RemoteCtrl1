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
void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg; // 将 void* 类型的 arg 强制转换为 CClientSocket* 类型并赋值给 thiz
	thiz->threadFunc(); // 调用 thiz 指向的 CClientSocket 对象的 threadFunc 成员函数
}
void CClientSocket::threadFunc()
{
	if (InitSocket() == false) {
		return;
	}
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE); // 调整 strBuffer 大小为 BUFFER_SIZE
	char* pBuffer = (char*)strBuffer.c_str(); // 获取 strBuffer 的 C 风格字符串指针并转为 char*
	int index = 0;
	while (m_sock != INVALID_SOCKET) { // 当套接字有效时循环
		if (m_lstSend.size() > 0) { // 如果待发送数据包列表不为空
			CPacket& head = m_lstSend.front(); // 获取列表头部的数据包引用
			if (Send(head) == false) { // 调用 Send 函数发送该数据包，若发送失败
				TRACE("发送失败！\r\n"); // 打印发送失败的调试信息
				continue; // 继续循环，尝试后续操作或再次发送
			}

			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			// 向 m_mapAck 中插入一个键为 head.hEvent（HANDLE 类型）、值为空 std::list<CPacket> 的键值对，
			// auto 用于自动推导 pr 的类型（std::pair<std::map<HANDLE, std::list<CPacket>>::iterator, bool>）
			int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
			if (length > 0 || index > 0) {
				index += length; // 累加接收数据的长度到索引，方便后续处理
				size_t size = (size_t)index; // 将索引转换为 size_t 类型，用于表示数据大小
				CPacket pack((BYTE*)pBuffer, size); // 用接收到的数据和大小构造 CPacket 对象
				if (size > 0) { // 如果有有效数据
					// 向 m_mapAck 中键为 head.hEvent 的对应值（std::list<CPacket> 类型）中添加数据包 pack				
					pack.hEvent = head.hEvent; // 将数据包的事件句柄设置为发送数据包的事件句柄
					pr.first->second.push_back(pack);
					SetEvent(head.hEvent); // 触发数据包关联的事件，通知数据已准备好
				}
			}
			else if (length <= 0 && index <= 0) { // 接收长度小于等于0且索引也小于等于0，说明可能连接有问题
				CloseSocket(); // 关闭套接字
			}
			m_lstSend.pop_front(); // 从待发送列表中移除已处理的数据包
		}
	}
}