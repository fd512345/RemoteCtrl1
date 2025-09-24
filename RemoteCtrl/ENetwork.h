#pragma once  // 防止头文件重复包含
#include "ESocket.h"  // 包含套接字相关头文件
#include "EdoyunThread.h"  // 包含线程相关头文件

/*
* 1 核心功能到底是什么？
* 2 业务逻辑是什么？
*/
class ENetwork  // 定义ENetwork类，暂为空类
{
};

typedef int(*AcceptFunc)(void* arg, ESOCKET& client);  // 定义AcceptFunc函数指针类型，处理客户端连接
typedef int(*RecvFunc)(void* arg, const EBuffer& buffer);  // 定义RecvFunc函数指针类型，处理接收数据
typedef int(*SendFunc)(void* arg, ESOCKET& client, int ret);  // 定义SendFunc函数指针类型，处理发送数据
typedef int(*RecvFromFunc)(void* arg, const EBuffer& buffer, ESockaddrIn& addr);  // 定义RecvFromFunc函数指针类型，用于处理从指定地址接收数据的逻辑，参数为void*类型参数、EBuffer类型的缓冲区、ESockaddrIn类型的地址，返回int
typedef int(*SendToFunc)(void* arg, const ESockaddrIn& addr, int ret);  // 定义SendToFunc函数指针类型，用于处理向指定地址发送数据的逻辑，参数为void*类型参数，返回int

class EServerParameter  // 定义EServerParameter类
{
public:
	EServerParameter(const std::string& ip = "0.0.0.0", short port = 9527,
		ETYPE type = ETYPE::ETypeTCP, AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL, SendFunc sendf = NULL, RecvFromFunc recvfromf = NULL,  // 定义RecvFromFunc类型的函数指针recvfromf并初始化为NULL
		SendToFunc sendtof = NULL);     // 定义SendToFunc类型的函数指针sendtof并初始化为NULL);  // 带更多参数（含回调函数）的构造函数声明
	//输入
	EServerParameter& operator<<(AcceptFunc func);  // 重载<<运算符，用于设置AcceptFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator<<(RecvFunc func);  // 重载<<运算符，用于设置RecvFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator<<(SendFunc func);  // 重载<<运算符，用于设置SendFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator<<(RecvFromFunc func);  // 重载<<运算符，用于设置RecvFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator<<(SendToFunc func);  // 重载<<运算符，用于设置SendFunc类型的回调函数，返回自身引用支持链式调用

	EServerParameter& operator<<(const std::string& ip);  // 重载<<运算符，用于设置IP地址，返回自身引用支持链式调用
	EServerParameter& operator<<(short port);  // 重载<<运算符，用于设置端口，返回自身引用支持链式调用
	EServerParameter& operator<<(ETYPE type);  // 重载<<运算符，用于设置类型，返回自身引用支持链式调用
	//输出
	EServerParameter& operator>>(AcceptFunc& func);  // 重载>>运算符，用于获取AcceptFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator>>(RecvFunc& func);  // 重载>>运算符，用于获取RecvFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator>>(SendFunc& func);  // 重载>>运算符，用于获取SendFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator>>(RecvFromFunc& func);  // 重载>>运算符，用于获取RecvFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator>>(SendToFunc& func);  // 重载>>运算符，用于获取SendFunc类型的回调函数，返回自身引用支持链式调用
	EServerParameter& operator>>(std::string& ip);// 重载>>运算符，用于获取IP地址，返回自身引用支持链式调用
	EServerParameter& operator>>(short& port);  // 重载>>运算符，用于获取端口，返回自身引用支持链式调用
	EServerParameter& operator>>(ETYPE& type);  // 重载>>运算符，用于获取类型，返回自身引用支持链式调用
	//复制构造函数  =重载运算符
	EServerParameter(const EServerParameter& param);  // 复制构造函数声明
	EServerParameter& operator=(const EServerParameter& param);  // 重载赋值运算符声明
	std::string m_ip;  // IP地址成员变量
	short m_port;  // 端口成员变量
	ETYPE m_type;  // 套接字类型成员变量
	AcceptFunc m_accept;  // 接收连接的回调函数指针成员变量
	RecvFunc m_recv;  // 接收数据的回调函数指针成员变量
	SendFunc m_send;  // 发送数据的回调函数指针成员变量
	RecvFromFunc m_recvfrom;  // 接收数据的回调函数指针成员变量
	SendToFunc m_sendto;  // 发送数据的回调函数指针成员变量
};

class EServer :public ThreadFuncBase// 定义EServer类
{
public:
	EServer(const EServerParameter& param);  // 构造函数，参数为IP、端口、套接字类型，注释：何时设置关键参数，是需要依据个人开发经验和实际需求去调整
	~EServer();
	int Invoke(void* arg);  // Invoke方法，执行相关逻辑
	int Send(ESOCKET& client, const EBuffer& buffer);  // Send方法，向客户端发送数据
	int Sendto(ESockaddrIn& addr, const EBuffer& buffer);  // Sendto方法，向客户端发送数据
	int Stop();
private:
	int threadfunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	EServerParameter m_params;  // 服务器参数成员变量
	void* m_args;  // 用户参数成员变量
	EdoyunThread m_thread;
	ESOCKET m_sock;  // 服务器套接字成员变量
	std::atomic<bool> m_stop;  // 服务器状态标记成员变量
};