#include "pch.h"
#include "Edoyunserver.h"
#include "CEdoyunQueue.h"
#include "EdoyunTool.h"
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
	if (m_client->GetBufferSize() > 0) {  // 若 m_client 智能指针指向对象的双字值大于 0
		sockaddr* plocal = NULL, * premote = NULL;  // 定义指向本地和远程套接字地址的指针，初始化为NULL
		GetAcceptExSockaddrs(*m_client, 0,  // 调用GetAcceptExSockaddrs函数，解析AcceptEx操作相关的套接字地址
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength, // 输出参数，用于获取本地地址及地址长度
			(sockaddr**)&premote, &rLength // 输出参数，用于获取远程地址及地址长度
		);
		memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));  // 将本地地址plocal的内容复制到m_client的本地地址存储区域
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));  // 将远程地址premote的内容复制到m_client的远程地址存储区域
		m_server->BindNewSocket(*m_client);  // 调用m_server对象的BindNewSocket方法，将*m_client对应的套接字进行绑定等相关操作
		int ret = WSARecv(
			(SOCKET)*m_client,          // 强制转换为SOCKET类型的客户端套接字，指定接收数据的套接字
			m_client->RecvWSABuffer(),  // 获取接收用的WSABUF结构体指针，描述接收缓冲区等信息
			1,                          // 要接收的WSABUF结构体数量
			*m_client,                  // 指向用于存储接收字节数的变量的指针
			&m_client->flags(),         // 指向接收操作标志的指针，用于获取操作相关标志
			m_client->RecvOverlapped(),                  // 指向WSAOVERLAPPED结构体的指针，若使用重叠I/O则非空（这里根据上下文推测传入对应重叠结构）
			NULL);                      // 完成例程的指针，重叠I/O时用于指定操作完成后的回调，这里为NULL表示非重叠或无需回调		
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {  // 若接收操作返回错误且错误不是重叠I/O操作挂起
			TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());  // 输出变量ret的值以及Winsock相关操作的最后一次错误码（均为十进制整数形式，换行）
		}		
		if (!m_server->NewAccept())  // 调用服务器的 NewAccept 方法，若返回 false
		{
			return -2;  // 返回 -2
		}
	}
	return -1;  // 否则返回 -1
}

template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = op;  // 初始化操作符成员m_operator为模板参数op
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);  // 初始化工作线程m_worker，绑定当前对象和SendWorker成员函数
	memset(&m_overlapped, 0, sizeof(m_overlapped));  // 将重叠I/O结构m_overlapped内存清零
	m_buffer.resize(1024 * 256);  // 调整发送缓冲区m_buffer大小为1024*256
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = op;  // 初始化操作符成员m_operator为模板参数op
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);  // 初始化工作线程m_worker，绑定当前对象和RecvWorker成员函数
	memset(&m_overlapped, 0, sizeof(m_overlapped));  // 将重叠I/O结构m_overlapped内存清零
	m_buffer.resize(1024 * 256);  // 调整接收缓冲区m_buffer大小为1024*256
}

EdoyunClient::EdoyunClient() : m_isbusy(false),
m_overlapped(new ACCEPTOVERLAPPED()), m_recv(new RECVOVERLAPPED()),
m_send(new SENDOVERLAPPED()), m_flags(0),
m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData) {  // 构造函数，初始化成员变量，m_isbusy 为 false，新建 ACCEPTOVERLAPPED 对象给 m_overlapped
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);  // 创建支持重叠 I/O 的套接字
	m_buffer.resize(1024);  // 将 m_buffer 大小调整为 1024
	memset(&m_laddr, 0, sizeof(m_laddr));  // 初始化本地地址结构体 m_laddr 为全 0
	memset(&m_raddr, 0, sizeof(m_raddr));  // 初始化远程地址结构体 m_raddr 为全 0
}

void EdoyunClient::SetOverlapped(PCLIENT& ptr) {  // EdoyunClient 类的 SetOverlapped 方法，参数为 PCLIENT 类型引用 ptr
	m_overlapped->m_client = ptr.get();  // 将 ptr 赋值给 m_overlapped 的 m_client 成员
	m_recv->m_client = ptr.get();  // 将ptr赋值给m_recv对象的m_client成员
	m_send->m_client = ptr.get();  // 将ptr赋值给m_send对象的m_client成员
}



EdoyunClient::operator LPOVERLAPPED() {  // 类型转换运算符，将 EdoyunClient 对象转换为 LPOVERLAPPED 类型
	return &m_overlapped->m_overlapped;  // 返回 m_overlapped 中 m_overlapped 成员的地址
}

LPWSABUF EdoyunClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::RecvOverlapped() {  // 定义EdoyunClient类的RecvOverlapped成员函数，返回LPOVERLAPPED类型
	return &m_recv->m_overlapped;  // 返回m_recv对象中m_overlapped成员的地址，用于重叠I/O接收操作
}

LPWSABUF EdoyunClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::SendOverlapped() {  // 定义EdoyunClient类的SendOverlapped成员函数，返回LPOVERLAPPED类型
	return &m_send->m_overlapped;  // 返回m_send对象中m_overlapped成员的地址，用于重叠I/O发送操作
}

int EdoyunClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);  // 调用recv函数从套接字m_sock接收数据，存储到m_buffer中
	if (ret <= 0)return -1;
	m_used += (size_t)ret;
	//解析数据
	CEdoyunTool::Dump((BYTE*)m_buffer.data(), ret);  // 调用CEDoyunTool类的Dump静态方法，将m_buffer的数据（转换为BYTE*指针）和ret作为参数，用于数据的转储（如十六进制打印等调试操作）
	return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);  // 创建大小为nSize的char类型vector
	memcpy(data.data(), buffer, nSize);  // 将buffer指向的nSize字节数据复制到data的内存区域
	if (m_vecSend.PushBack(data)) {  // 调用m_vecSend的PushBack方法，将data加入容器
		return 0;  // 加入成功返回0
	}
	return -1;  // 加入失败返回-1
}

int EdoyunClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {                          // 检查发送缓冲区是否有数据需要发送
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);  // 调用WSAAsyncSelect模型下的WSASend函数发送数据
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {  // 判断发送是否失败且错误不是重叠I/O操作正在进行
			CEdoyunTool::ShowError();                     // 调用工具类显示错误信息
			return -1;                                    // 返回-1表示发送操作出错
		}
	}
	return 0;                                             // 返回0表示发送操作成功或无需发送数据
}

EdoyunServer::~EdoyunServer()
{
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();  // 获取m_client映射的起始迭代器
	for (; it != m_client.end(); it++) {                        // 遍历m_client映射的所有元素
		it->second.reset();                                     // 对每个元素的第二个值（PCLIENT类型）调用reset方法
	}
	m_client.clear();                                           // 清空m_client映射中的所有元素
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool EdoyunServer::StartService()
{
	CreateSocket();
	sockaddr_in addr; // 定义 IPv4 地址结构
	// 绑定套接字到指定地址和端口
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_sock); // 绑定失败则关闭套接字
		m_sock = INVALID_SOCKET; // 将套接字设为无效
		return false; // 函数返回
	}
	if (listen(m_sock, 3) == -1)
	{
		closesocket(m_sock); // 监听失败则关闭套接字
		m_sock = INVALID_SOCKET; // 将套接字设为无效
		return false; // 函数返回
	}

	// 1 创建 I/O 完成端口，第一个参数为无效句柄，第二个为 NULL（创建新端口），第三个为 0（无关联键），第四个为 4（并发线程数）
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) { // 判断 I/O 完成端口句柄是否为 NULL
		closesocket(m_sock); // 关闭套	接字 m_sock
		m_sock = INVALID_SOCKET; // 将套接字 m_sock 设为无效
		m_hIOCP = INVALID_HANDLE_VALUE; // 将 I/O 完成端口句柄设为无效
		return false; // 函数返回
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); // 2 将套接字 m_sock 与 I/O 完成端口 m_hIOCP 关联，传递当前对象指针 this 作为键，最后一个参数 0 表示默认并发数
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp)); // 调用线程池的 DispatchWorker 方法，分发一个 ThreadWorker 对象，该对象封装了当前 EdoyunServer 对象（this）和其 threadIocp 成员函数（转换为 FUNCTYPE 类型的成员函数指针），用于在线程池中执行 threadIocp 函数逻辑
	if (!NewAccept()) return false; // 调用 NewAccept 函数，若其返回值为 false（表示新接受连接操作失败），则当前函数返回 false
	return true;
}


void EdoyunServer::CreateSocket()
{
	WSADATA WSAData;  // 定义WSADATA结构体变量，用于接收Winsock初始化信息
	WSAStartup(MAKEWORD(2, 2), &WSAData);// 调用WSAStartup函数，初始化Winsock 2.2版本，将初始化信息存入WSAData
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));// 设置套接字选项，允许重用本地地址和端口 SO_REUSEADDR：具体选项，允许地址重用（解决端口占用问题）
}
int EdoyunServer::threadIocp()
{
	DWORD transferred = 0; // 用于存储传输的字节数
	ULONG_PTR CompletionKey = 0; // 用于存储完成键
	OVERLAPPED* lpOverlapped = NULL; // 用于存储重叠 I/O 结构指针
	// 从 I/O 完成端口获取完成的 I/O 操作状态，INFINITE 表示无限等待
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if ((CompletionKey != 0)) { // 判断传输字节数大于0且完成键非0
			// 通过 CONTAINING_RECORD 宏，从 OVERLAPPED 结构指针获取包含它的 EdoyunOverlapped 结构指针
			EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
			TRACE("pOverlapped->m_operator = %d \r\n", pOverlapped->m_operator);  // 输出pOverlapped对象中m_operator成员的值（十进制整数形式，换行）
			pOverlapped->m_server = this;  // 将当前对象的指针赋值给pOverlapped对象的m_server成员，建立关联
			switch (pOverlapped->m_operator) { // 根据操作类型枚举值进行分支处理
			case EAccept: { // 处理接受连接操作的情况
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped; // 将 pOverlapped 转换为 ACCEPTOVERLAPPED 类型指针
				m_pool.DispatchWorker(pOver->m_worker); // 调用线程池的 DispatchWorker 方法，分发 pOver 中的工作对象 m_worker
			}
						break;
			case ERecv: { // 处理接收数据操作的情况
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped; // 将 pOverlapped 转换为 RECVOVERLAPPED 类型指针
				m_pool.DispatchWorker(pOver->m_worker); // 调用线程池的 DispatchWorker 方法，分发 pOver 中的工作对象 m_worker
			}
					  break;
			case ESend: { // 处理发送数据操作的情况
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped; // 将 pOverlapped 转换为 SENDOverlapped 类型指针
				m_pool.DispatchWorker(pOver->m_worker); // 调用线程池的 DispatchWorker 方法，分发 pOver 中的工作对象 m_worker
			}
					  break;
			case EError: { // 处理错误情况
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped; // 将 pOverlapped 转换为 ERROROverlapped 类型指针
				m_pool.DispatchWorker(pOver->m_worker); // 调用线程池的 DispatchWorker 方法，分发 pOver 中的工作对象 m_worker
			}
					   break;

			}
		}
		else
		{
			return -1;
		}
	}
	return 0; // 函数返回 0
}

bool EdoyunServer::NewAccept()
{
	PCLIENT pClient(new EdoyunClient()); // 创建 EdoyunClient 对象的智能指针 pClient
	pClient->SetOverlapped(pClient); // 调用 pClient 的 SetOverlapped 方法，设置重叠 I/O 结构中关联的客户端智能指针为 pClient 自身
	m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient)); // 将客户端套接字（*pClient 转换为 SOCKET）和智能指针 pClient 组成键值对，插入到 m_client 容器中
	if (!AcceptEx(m_sock, // 调用 AcceptEx 函数接受客户端连接，若返回 FALSE 表示接受失败
		*pClient,
		*pClient,
		0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		*pClient, *pClient))
	{
		TRACE("%d\r\n", WSAGetLastError());  // 输出Winsock相关操作的最后一次错误码（以十进制整数形式，换行）
		if (WSAGetLastError() != WSA_IO_PENDING) {  // 检查Winsock最后一次错误是否不是重叠I/O操作正在进行
			closesocket(m_sock);                    // 关闭套接字m_sock
			m_sock = INVALID_SOCKET;                // 将套接字标记为无效
			m_hIOCP = INVALID_HANDLE_VALUE;         // 将IO完成端口句柄标记为无效
			return false;                           // 返回false表示操作失败
		}
	}
	return true;
}

void EdoyunServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);  // 调用CreateIoCompletionPort函数，将套接字s与I/O完成端口m_hIOCP关联，关联的完成键为当前对象指针（ULONG_PTR类型），最后一个参数0表示使用系统默认的并发线程数
}
