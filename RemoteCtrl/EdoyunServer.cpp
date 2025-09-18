#include "pch.h"
#include "Edoyunserver.h"
#pragma warning (disable:4407)
template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {  // AcceptOverlapped 类的构造函数
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);  // 初始化线程工作对象 m_worker，绑定当前对象和 AcceptWorker 方法
	m_operator = EAccept;  // 设置操作类型为 EAccept
	memset(&m_overlapped, 0, sizeof(m_overlapped));  // 将 m_overlapped 内存初始化为 0
	m_buffer.resize(1024);  // 调整 m_buffer 大小为 1024
	m_server = NULL;  // 将 m_server 置为 NULL
}

template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {  // 模板类 AcceptOverlapped 的 AcceptWorker 方法
	INT lLength = 0, rLength = 0;  // 定义存储本地、远程地址长度的变量
	if (*(LPDWORD)*m_client.get() > 0) {  // 若 m_client 智能指针指向对象的双字值大于 0
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength, // 本地地址
			(sockaddr**)m_client->GetRemoteAddr(), &rLength // 远程地址
		);
		if (!m_server->NewAccept())  // 调用服务器的 NewAccept 方法，若返回 false
		{
			return -2;  // 返回 -2
		}
	}
	return -1;  // 否则返回 -1
}EdoyunClient::EdoyunClient() : m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED()) {  // 构造函数，初始化成员变量，m_isbusy 为 false，新建 ACCEPTOVERLAPPED 对象给 m_overlapped
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);  // 创建支持重叠 I/O 的套接字
	m_buffer.resize(1024);  // 将 m_buffer 大小调整为 1024
	memset(&m_laddr, 0, sizeof(m_laddr));  // 初始化本地地址结构体 m_laddr 为全 0
	memset(&m_raddr, 0, sizeof(m_raddr));  // 初始化远程地址结构体 m_raddr 为全 0
}

void EdoyunClient::SetOverlapped(PCLIENT& ptr) {  // EdoyunClient 类的 SetOverlapped 方法，参数为 PCLIENT 类型引用 ptr
	m_overlapped->m_client = ptr;  // 将 ptr 赋值给 m_overlapped 的 m_client 成员
}

EdoyunClient::operator LPOVERLAPPED() {  // 类型转换运算符，将 EdoyunClient 对象转换为 LPOVERLAPPED 类型
	return &m_overlapped->m_overlapped;  // 返回 m_overlapped 中 m_overlapped 成员的地址
}
