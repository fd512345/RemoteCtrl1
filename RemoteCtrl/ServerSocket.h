#pragma once  // 防止头文件重复包含
#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含MFC框架头文件
#include <list>
#include "Packet.h"



typedef void (*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket&);

class CServerSocket  // 服务器Socket类（单例模式）
{
public:
	// 获取单例实例
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  // 懒汉式初始化
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	// 负责初始化服务器套接字、绑定端口并进入循环，持续接收客户端连接、处理命令并发送响应
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		//1 进度的可控性 2 对接的方便性 3 可行性评估，提早暴露风险
		// TODO: socket、bind、listen、accept、read、write、close
		//套接字初始化
		bool ret = InitSocket(port);  // 调用InitSocket函数初始化Socket，传入端口port，返回值存入ret
		if (ret == false) return -1;  // 若初始化失败，返回-1
		std::list<CPacket> lstPackets;
		m_callback = callback;  // 将传入的callback赋值给m_callback
		m_arg = arg;  // 将传入的arg赋值给m_arg
		int count = 0;  // 用于计数接受客户端失败的次数
		while (true) {  // 进入无限循环
			if (AcceptClient() == false) {  // 调用AcceptClient函数接受客户端连接，若失败
				if (count >= 3) {  // 若失败次数大于等于3
					return -2;  // 返回-2
				}
				count++;  // 失败次数加1
			}
			int ret = DealCommand();  // 调用DealCommand函数处理命令，返回值存入ret
			if (ret > 0) {  // 若处理命令返回值大于0
				m_callback(m_arg, ret, lstPackets, m_packet);  // 调用回调函数m_callback，传入m_arg和ret
				while (lstPackets.size() > 0) {  // 判断lstPackets容器中元素数量是否大于0
					Send(lstPackets.front());  // 调用Send函数发送lstPackets容器的第一个元素
					lstPackets.pop_front();  // 弹出lstPackets容器的第一个元素
				}
			}
			CloseClient();  // 调用CloseClient函数关闭客户端连接
		}
		return 0;
	}	// 接受客户端连接
protected:
	bool InitSocket(short port) {
		if (m_sock == -1)return false;  // 检查Socket是否有效
		sockaddr_in serv_adr;  // 服务器地址结构体
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;  // IPv4协议
		serv_adr.sin_addr.s_addr = INADDR_ANY;  // 绑定所有本地IP
		serv_adr.sin_port = htons(port);  // 绑定端口9527（网络字节序）
		// 绑定Socket到端口
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;  // 绑定失败
		}
		// 开始监听连接（最大1个等待连接）
		if (listen(m_sock, 1) == -1) {
			return false;  // 监听失败
		}
		return true;  // 初始化成功
	}

	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");  // 调试输出
		sockaddr_in client_adr;  // 客户端地址结构体
		int cli_sz = sizeof(client_adr);
		// 接受客户端连接，获取客户端Socket
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);  // 调试输出客户端Socket
		if (m_client == -1)return false;  // 接受连接失败
		return true;  // 接受连接成功
	}
#define BUFFER_SIZE 4096  // 接收缓冲区大小
	// 处理客户端命令（接收并解析数据包）
	int DealCommand() {
		if (m_client == -1)return -1;  // 客户端Socket无效
		char* buffer = new char[BUFFER_SIZE];  // 分配接收缓冲区
		if (buffer == NULL) {
			TRACE("内存溢出!\r\n");  // 内存分配失败
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);  // 清空缓冲区
		size_t index = 0;  // 缓冲区当前数据长度
		while (true) {
			// 接收客户端数据到缓冲区
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {  // 接收失败或连接关闭
				delete[]buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);  // 调试输出接收长度
			index += len;  // 更新缓冲区数据长度
			len = index;//保留 index 记录的总长度，同时用 len 接收解析后的有效数据包长度，确保缓冲区中剩余数据的处理逻辑正确。
			// 解析数据包
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {  // 解析成功
				// 移动未解析的数据到缓冲区头部
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;  // 更新剩余数据长度
				delete[]buffer;
				return m_packet.sCmd;  // 返回命令类型
			}
		}
		delete[]buffer;  // 释放缓冲区
		return -1;  // 处理失败
	}

	// 发送原始数据
	bool Send(const char* pData, int nSize) {
		if (m_client == -1)return false;  // 客户端Socket无效
		return send(m_client, pData, nSize, 0) > 0;  // 发送数据并返回结果
	}
	// 发送数据包
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;  // 客户端Socket无效
		//Dump((BYTE*)pack.Data(), pack.Size());  // 调试时打印数据包
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;  // 发送数据包
	}
	void CloseClient() {  // 关闭客户端连接
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;  // 标记为无效
		}
	}
private:
	SOCKET_CALLBACK m_callback;  // 定义一个类型为 SOCKET_CALLBACK 的变量 m_callback
	void* m_arg;  // 定义一个 void* 类型的变量 m_arg
	SOCKET m_client;  // 客户端Socket
	SOCKET m_sock;  // 服务器监听Socket
	CPacket m_packet;  // 当前处理的数据包
	CServerSocket& operator=(const CServerSocket& ss) {}  // 禁用赋值运算符
	CServerSocket(const CServerSocket& ss) {  // 禁用拷贝构造函数
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {  // 私有构造函数（单例模式）
		m_client = INVALID_SOCKET;  // 初始化客户端Socket为无效
		if (InitSockEnv() == FALSE) {  // 初始化Socket环境
			MessageBox(NULL, _T("无法初始化网络环境,程序即将退出!"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);  // 初始化失败则退出
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  // 创建TCP Socket
	}
	~CServerSocket() {  // 析构函数
		closesocket(m_sock);  // 关闭监听Socket
		WSACleanup();  // 清理Socket环境
	}
	// 初始化Socket环境（加载Winsock库）
	BOOL InitSockEnv() {
		WSADATA data;
		// 加载Winsock 1.1版本
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) {
			return FALSE;  // 加载失败
		}
		return TRUE;  // 加载成功
	}
	// 释放单例实例
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  // 释放实例
		}
	}
	static CServerSocket* m_instance;  // 单例实例指针
	class CHelper {  // 辅助类，用于自动释放单例
	public:
		CHelper() {  // 构造时创建单例
			CServerSocket::getInstance();
		}
		~CHelper() {  // 析构时释放单例
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;  // 辅助类实例（自动管理单例生命周期）
};