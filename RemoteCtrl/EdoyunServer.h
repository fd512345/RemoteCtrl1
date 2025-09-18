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
	PCLIENT m_client;//对应的客户端
	WSABUF m_wsabuffer;
};

template<EdoyunOperator>class AcceptOverlapped;  // 前向声明模板类 AcceptOverlapped
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;  // 定义 ACCEPTOVERLAPPED 为 AcceptOverlapped<EAaccept> 的类型别名

template<EdoyunOperator>class RecvOverlapped;  // 前向声明模板类 RecvOverlapped
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;  // 定义 RECVOVERLAPPED 为 RecvOverlapped<ERecv> 的类型别名

template<EdoyunOperator>class SendOverlapped;  // 前向声明模板类 SendOverlapped
typedef SendOverlapped<ESend> SENDOVERLAPPED;  // 定义 SENDOVERLAPPED 为 SendOverlapped<ESend> 的类型别名


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
	LPWSABUF RecvWSABuffer();
	LPWSABUF SendWSABuffer();
	DWORD& flags() { return m_flags; }  // 返回m_flags的引用，可用于修改m_flags的值
	sockaddr_in* GetLocalAddr() { return &m_laddr; }  // 获取本地地址，返回 m_laddr 的引用
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }  // 获取远程地址，返回 m_raddr 的引用
	size_t GetBufferSize()const { return m_buffer.size(); }  // 常量成员函数，返回m_buffer的大小
	int Recv() {
		int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);  // 调用recv函数从套接字m_sock接收数据，存储到m_buffer中
		if (ret <= 0)return -1;
		m_used += (size_t)ret;
		//解析数据
		return 0;
	}
private:
private:
	SOCKET m_sock; // 客户端套接字
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;  // 定义一个 shared_ptr 智能指针 m_send，用于管理 SENDOVERLAPPED 类型的对象，实现自动内存管理 
	std::vector<char> m_buffer; // 数据缓冲区
	size_t m_used;//已使用的缓冲区大小
	sockaddr_in m_laddr; // 地址结构，用于local端地址信息
	sockaddr_in m_raddr; // 地址结构，用于remote端地址信息
	bool m_isbusy;
};

template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class AcceptOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	AcceptOverlapped();
	int AcceptWorker();
};





template<EdoyunOperator> // 模板参数，与操作类型相关的模板
class RecvOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	RecvOverlapped();
	int RecvWorker() { // 定义 AcceptWorker 成员函数，用于处理接受连接相关工作
		int ret = m_client->Recv();  // 调用m_client指向对象的Recv方法，接收返回值存入ret
		return ret;                  // 返回Recv方法的返回值
	}
};

template<EdoyunOperator> // 模板参数，推测是与操作类型相关的模板
class SendOverlapped : public EdoyunOverlapped, public ThreadFuncBase { // 定义 AcceptOverlapped 类，继承自 EdoyunOverlapped 和 ThreadFuncBase
public:
	SendOverlapped();
	int SendWorker() { // 定义 AcceptWorker 成员函数，用于处理接受连接相关工作
		return -1;
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
		return -1;
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
	bool StartService();
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
	int threadIocp();
private:
	EdoyunThreadPool m_pool; // 线程池，用于管理服务器线程
	HANDLE m_hIOCP; // I/O 完成端口句柄
	SOCKET m_sock; // 服务器套接字
	// 存储套接字与客户端对象智能指针的映射，键为套接字句柄，值为 EdoyunClient 智能指针
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<EdoyunClient>> m_client;
};

