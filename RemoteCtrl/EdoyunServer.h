#pragma once
#include <MSWSock.h>
#include "EdoyunThread.h"
#include "CEdoyunQueue.h"
#include <winsock.h>
#include <map>



enum EdoyunOperator {
	ENone,    // 无操作状态
	EAccept,  // 接受连接操作
	ERecv,    // 接收数据操作
	ESend,    // 发送数据操作
	EError    // 错误状态
};


class EdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT; // 定义类型别名 PCLIENT，代表 EdoyunClient 类型的智能指针（std::shared_ptr），用于更方便地管理 EdoyunClient 对象的生命周期

class EdoyunOverlapped { // 定义 EdoyunOverlapped 类，用于封装 Windows 重叠 I/O 结构
public:
	OVERLAPPED m_overlapped; // 包含系统的 OVERLAPPED 结构，用于 Windows 重叠 I/O 操作
	DWORD m_operator;//各种操作 参见EdoyunOperator
	std::vector<char> m_buffer;//缓冲区
	ThreadWorker m_worker;//处理函数
	EdoyunServer* m_server;//服务器对象
};
template<EdoyunOperator> class AcceptOverlapped;// 模板参数，推测是与操作类型相关的模板
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

class EdoyunClient { // 定义 EdoyunClient 类，用于表示客户端
public:
	EdoyunClient();
	~EdoyunClient() { // 析构函数
		closesocket(m_sock); // 关闭套接字
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET() { // 类型转换运算符，将 EdoyunClient 对象转换为 SOCKET 类型
		return m_sock; // 返回套接字 m_sock
	}
	operator PVOID() { // 类型转换运算符，将当前对象转换为 PVOID（void*）类型
		return &m_buffer[0]; // 返回缓冲区 m_buffer 第一个元素的地址
	}
	operator LPOVERLAPPED();
	operator LPDWORD() { // 类型转换运算符，将当前对象转换为 LPDWORD（DWORD*）类型
		return &m_received; // 返回 m_received 成员变量的地址
	}
	sockaddr_in* GetLocalAddr() { return &m_laddr; }  // 获取本地地址，返回 m_laddr 的引用
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }  // 获取远程地址，返回 m_raddr 的引用
private:
private:
	SOCKET m_sock; // 客户端套接字
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer; // 数据缓冲区
	sockaddr_in m_laddr; // 地址结构，用于local端地址信息
	sockaddr_in m_raddr; // 地址结构，用于remote端地址信息
	bool m_isbusy;
};

template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class AcceptOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	AcceptOverlapped();
	int AcceptWorker();
	PCLIENT m_client;
};





template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class RecvOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	RecvOverlapped()
		: m_operator(ERecv), // 初始化操作类型为 EAccept（接受连接操作）
		m_worker(this, &RecvOverlapped::RecvWorker) { // 初始化工作对象 m_worker，关联当前对象和 AcceptWorker 成员函数
		memset(&m_overlapped, 0, sizeof(m_overlapped)); // 将重叠 I/O 结构 m_overlapped 内存清零
		m_buffer.resize(1024 * 256); // 调整缓冲区 m_buffer 大小为 1024*256
	}
	int RecvWorker() { // 定义 AcceptWorker 成员函数，用于处理接受连接相关工作
		// TODO: 此处待补充接受连接的具体逻辑
	}
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class SendOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	SendOverlapped()
		: m_operator(ESend), // 初始化操作类型为 ESend
		m_worker(this, &SendOverlapped::SendWorker) { // 初始化工作对象 m_worker，关联当前对象和 AcceptWorker 成员函数
		memset(&m_overlapped, 0, sizeof(m_overlapped)); // 将重叠 I/O 结构 m_overlapped 内存清零
		m_buffer.resize(1024 * 256); // 调整缓冲区 m_buffer 大小为 1024*256
	}
	int SendWorker() { // 定义 AcceptWorker 成员函数，用于处理接受连接相关工作
		// TODO: 此处待补充接受连接的具体逻辑
	}
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class ErrorOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	ErrorOverlapped()
		: m_operator(EError), // 初始化操作类型为 EError
		m_worker(this, &ErrorOverlapped::ErrorWorker) { // 初始化工作对象 m_worker，关联当前对象和 AcceptWorker 成员函数
		memset(&m_overlapped, 0, sizeof(m_overlapped)); // 将重叠 I/O 结构 m_overlapped 内存清零
		m_buffer.resize(1024); // 调整缓冲区 m_buffer 大小为 1024
	}
	int ErrorWorker() { // 定义 AcceptWorker 成员函数，用于处理接受连接相关工作
		// TODO: 此处待补充接受连接的具体逻辑
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;




class EdoyunServer : public ThreadFuncBase { // 定义 EdoyunServer 类，继承自 ThreadFuncBase
public:
	// 构造函数，默认 IP 为 "0.0.0.0"，默认端口为 9527，初始化线程池 m_pool 大小为 10
	EdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE; // 初始化 I/O 完成端口句柄为无效值
		// 创建支持重叠 I/O 的 TCP 套接字
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET; // 设置地址族为 IPv4
		m_addr.sin_port = htons(port); // 设置端口，主机字节序转网络字节序
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str()); // 设置 IP 地址

	}
	~EdoyunServer() {} // 析构函数，此处为空，可能后续补充资源释放逻辑
	bool StartService()
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

		// 创建 I/O 完成端口，第一个参数为无效句柄，第二个为 NULL（创建新端口），第三个为 0（无关联键），第四个为 4（并发线程数）
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) { // 判断 I/O 完成端口句柄是否为 NULL
			closesocket(m_sock); // 关闭套接字 m_sock
			m_sock = INVALID_SOCKET; // 将套接字 m_sock 设为无效
			m_hIOCP = INVALID_HANDLE_VALUE; // 将 I/O 完成端口句柄设为无效
			return false; // 函数返回
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); // 将套接字 m_sock 与 I/O 完成端口 m_hIOCP 关联，传递当前对象指针 this 作为键，最后一个参数 0 表示默认并发数
		m_pool.Invoke();
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp)); // 调用线程池的 DispatchWorker 方法，分发一个 ThreadWorker 对象，该对象封装了当前 EdoyunServer 对象（this）和其 threadIocp 成员函数（转换为 FUNCTYPE 类型的成员函数指针），用于在线程池中执行 threadIocp 函数逻辑
		if (!NewAccept()) return false; // 调用 NewAccept 函数，若其返回值为 false（表示新接受连接操作失败），则当前函数返回 false
		return true;
	}
	bool NewAccept()
	{
		PCLIENT pClient(new EdoyunClient()); // 创建 EdoyunClient 对象的智能指针 pClient
		pClient->SetOverlapped(pClient); // 调用 pClient 的 SetOverlapped 方法，设置重叠 I/O 结构中关联的客户端智能指针为 pClient 自身
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient)); // 将客户端套接字（*pClient 转换为 SOCKET）和智能指针 pClient 组成键值对，插入到 m_client 容器中
		if (!AcceptEx(m_sock, // 调用 AcceptEx 函数接受客户端连接，若返回 FALSE 表示接受失败
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient)) {
			closesocket(m_sock); // 关闭套接字 m_sock
			m_sock = INVALID_SOCKET; // 将套接字 m_sock 设为无效
			m_hIOCP = INVALID_HANDLE_VALUE; // 将 I/O 完成端口句柄设为无效
			return false; // 函数返回
		}
		return true;
	}

private:
	void CreateSocket()
	{
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));// 设置套接字选项，允许重用本地地址和端口 SO_REUSEADDR：具体选项，允许地址重用（解决端口占用问题）
	}
	int threadIocp()
	{
		DWORD transferred = 0; // 用于存储传输的字节数
		ULONG_PTR CompletionKey = 0; // 用于存储完成键
		OVERLAPPED* lpOverlapped = NULL; // 用于存储重叠 I/O 结构指针
		// 从 I/O 完成端口获取完成的 I/O 操作状态，INFINITY 表示无限等待
		if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (transferred > 0 && (CompletionKey != 0)) { // 判断传输字节数大于0且完成键非0
				// 通过 CONTAINING_RECORD 宏，从 OVERLAPPED 结构指针获取包含它的 EdoyunOverlapped 结构指针
				EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
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
private:
	EdoyunThreadPool m_pool; // 线程池，用于管理服务器线程
	HANDLE m_hIOCP; // I/O 完成端口句柄
	SOCKET m_sock; // 服务器套接字
	// 存储套接字与客户端对象智能指针的映射，键为套接字句柄，值为 EdoyunClient 智能指针
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<EdoyunClient>> m_client;
	CEdoyunQueue<EdoyunClient> m_lstClient; // 定义一个名为 m_lstClient 的队列对象，该队列存储 EdoyunClient 类型的元素，用于管理客户端相关的队列操作
};

