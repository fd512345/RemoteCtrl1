// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含框架头文件
#include "RemoteCtrl.h"  // 包含当前项目头文件
#include "Command.h" 
#include "ServerSocket.h"  // 包含服务器套接字相关头文件
#include <conio.h>
#include "CEdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"
#ifdef _DEBUG
#define new DEBUG_NEW  // 调试模式下使用DEBUG_NEW宏
#endif 
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup" )  // 注释：设置子系统为windows及入口函数
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )  // 注释：设置子系统为windows及入口函数
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup" )  // 注释：设置子系统为console及入口函数
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )  // 注释：设置子系统为console及入口函数

#define INVOKE_PATH

// 唯一的应用程序对象   
CWinApp theApp;  // 定义MFC应用程序对象

using namespace std;  // 使用标准命名空间 




/*
1 bug测试/功能测试
2 关键因素的测试(内存泄漏、运行的稳定性、条件性)
3 压力测试(可靠性测试)
4 性能测试
*/
void iocp();

void udp_server();  // 声明 udp_server 函数，用于实现 UDP 服务器相关功能
void udp_client(bool ishost = true);  // 声明 udp_client 函数，用于实现 UDP 客户端相关功能

void initsock() {
	WSADATA wsa;  // 定义WSADATA结构体，用于存储Winsock初始化信息
	WSAStartup(MAKEWORD(2, 2), &wsa);  // 启动Winsock 2.2版本，参数为版本号和结构体指针
}

// 清理 Winsock 环境
void clearsock() {
	WSACleanup();  // 释放Winsock相关资源，结束Winsock的使用
}



int main(int argc, char* argv[])  // 主函数
{
	if (!CEdoyunTool::Init()) return 1; // 调用工具类初始化方法，若失败则返回 1
	initsock();  // 初始化 Winsock 环境
	if (argc == 1) {  // 检查命令行参数数量，如果没有额外参数     服务器
		char wstrDir[MAX_PATH];  // 定义存储当前目录的宽字符数组
		GetCurrentDirectoryA(MAX_PATH, wstrDir);  // 获取当前工作目录
		STARTUPINFOA si{};  // 定义进程启动信息结构体
		PROCESS_INFORMATION pi{};  // 定义进程信息结构体
		string strCmd = argv[0];  // 获取当前程序路径
		strCmd += " 1";  // 拼接命令行参数" 1"
		// 创建新进程，带有新控制台窗口，参数为当前程序路径加" 1"
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet) {  // 如果进程创建成功
			CloseHandle(pi.hThread);  // 关闭线程句柄
			CloseHandle(pi.hProcess);  // 关闭进程句柄
			TRACE("进程ID:%d\r\n", pi.dwProcessId);  // 输出进程ID
			TRACE("线程ID:%d\r\n", pi.dwThreadId);  // 输出线程ID
			strCmd += "2";  // 在现有命令后再拼接"2"，变为" 12"
			// 再次创建新进程，参数为当前程序路径加" 12"
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {  // 如果第二次进程创建成功
				CloseHandle(pi.hThread);  // 关闭线程句柄
				CloseHandle(pi.hProcess);  // 关闭进程句柄
				TRACE("进程ID:%d\r\n", pi.dwProcessId);  // 输出进程ID
				TRACE("线程ID:%d\r\n", pi.dwThreadId);  // 输出线程ID
				udp_server();  // 调用UDP服务器函数
			}
		}
	}
	else if (argc == 2)//主客户端
	{
		udp_client();
	}
	else//从客户端
	{
		udp_client(false);
	}
	clearsock();  // 清理 Winsock 环境

	//iocp();



	/*if (!Init()) return 1;
	CCommand cmd;  // 定义命令处理对象
	int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);  // 调用服务器对象的Run方法，传入NULL和cmd的地址，返回值存入ret
	switch (ret) {  // 根据ret的值进行分支判断
	case -1:  // 当ret为-1时，网络初始化异常的情况
		MessageBox(NULL, _T("网络初始化异常，未能成功初始， 请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);  // 弹出错误提示框
		exit(0);  // 退出程序
		break;  // 跳出case
	case -2:  // 当ret为-2时，多次接入用户失败的情况
		MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);  // 弹出错误提示框
		exit(0);  // 退出程序
		break;  // 跳出case
	}*/
	return 0;  // 返回结果
}

class COverlapped { // 定义 COverlapped 类，用于封装重叠 I/O 相关数据
public:
	OVERLAPPED m_overlapped; // 系统重叠 I/O 结构，用于 Windows 重叠 I/O 操作
	DWORD m_operator; // 操作标识，可用于区分不同类型的 I/O 操作
	char m_buffer[4096]; // 数据缓冲区，用于存储 I/O 操作的数据
	COverlapped() { // 构造函数，初始化成员变量
		m_operator = 0; // 初始化操作标识为 0
		memset(&m_overlapped, 0, sizeof(m_overlapped)); // 将系统重叠 I/O 结构初始化为全 0
		memset(m_buffer, 0, sizeof(m_buffer)); // 将数据缓冲区初始化为全 0
	}
};

void iocp()
{
	EdoyunServer server; // 创建 EdoyunServer 类的对象 server
	server.StartService(); // 调用 server 的 StartService 方法，启动服务
	getchar(); // 等待用户输入一个字符，用于阻塞程序，防止服务启动后立即退出
}


// 初始化 Winsock 环境
void udp_server() {
	// 打印当前文件路径、代码行号、函数名
	printf("%s (%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);  // 创建一个 UDP 套接字
	if (sock == INVALID_SOCKET) {  // 检查套接字是否为无效套接字
		// 若套接字无效，打印当前文件路径、代码行号、函数名及错误提示
		printf("%s (%d):%s ERROR\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;  // 直接返回，不再执行后续代码
	}
	std::list<sockaddr_in> lstclients;  // 定义一个存储 sockaddr_in 类型元素的标准列表 lstclients，用于保存客户端地址信息
	sockaddr_in server, client;  // 定义服务器和客户端的地址结构体
	memset(&server, 0, sizeof(server));  // 将服务器地址结构体清零
	memset(&client, 0, sizeof(client));  // 将客户端地址结构体清零
	server.sin_family = AF_INET;  // 设置地址族为IPv4
	server.sin_port = htons(20000);  // 设置服务器端口为20000（网络字节序）
	server.sin_addr.s_addr = inet_addr("127.0.0.1");  // 设置服务器IP为127.0.0.1
	if (-1 == bind(sock, (sockaddr*)&server, sizeof(server))) {  // 绑定套接字到服务器地址，若失败进入分支
		// 打印当前文件、行号、函数名及错误码，关闭套接字并返回
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		closesocket(sock);
		return;
	}
	std::string buf;  // 定义用于接收数据的字符串缓冲区
	buf.resize(1024 * 256);  // 调整缓冲区大小为 1024*256 字节
	memset((char*)buf.c_str(), 0, buf.size());  // 将缓冲区初始化为全 0
	int len = sizeof(client);  // 用于存储客户端地址长度
	int ret = 0;  // 用于存储 recvfrom 和 sendto 的返回值
	while (!_kbhit()) {  // 当没有键盘输入时循环
		// 从套接字接收数据，存储到 buf，同时获取客户端地址
		ret = recvfrom(sock, (char*)buf.c_str(), buf.size(), 0, (sockaddr*)&client, &len);
		if (ret > 0) {  // 如果接收数据长度大于 0
			if (lstclients.size() <= 0) {  // 检查客户端列表 lstclients 大小是否小于等于 0
				lstclients.push_back(client);  // 将当前客户端地址添加到列表中
				// 打印当前文件路径、代码行号、函数名以及客户端的 IP 和端口（端口转为主机字节序）
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				// 向客户端发送数据，参数为套接字、数据、长度、标志、客户端地址、地址长度
				ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件、行号、函数名
			}
			else {
				// 将列表中第一个客户端地址拷贝到 buf 中
				memcpy((void*)buf.c_str(), &lstclients.front(), sizeof(lstclients.front()));
				// 向客户端发送列表中第一个客户端地址数据，参数为套接字、数据、长度、标志、客户端地址、地址长度
				ret = sendto(sock, buf.c_str(), sizeof(lstclients.front()), 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件、行号、函数名
			}
			// CEdoyunTool::Dump((BYTE*)buf.c_str(), ret);  // 注释：调用工具类的 Dump 方法（当前被注释）
		}
		else {
			// 打印当前文件、行号、函数名、Winsock 错误码以及 ret 的值，用于调试错误场景
			printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
		}
	}
	closesocket(sock);  // 关闭套接字
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件、行号、函数名
}

void udp_client(bool ishost) {
	Sleep(2000);  // 线程休眠2000毫秒（2秒）
	sockaddr_in server, client;  // 定义服务器和客户端的地址结构体
	int len = sizeof(client);  // 用于存储客户端地址长度
	server.sin_family = AF_INET;  // 设置地址族为IPv4
	server.sin_port = htons(20000);  // 设置服务器端口为20000（网络字节序）
	server.sin_addr.s_addr = inet_addr("127.0.0.1");  // 设置服务器IP为127.0.0.1
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);  // 创建UDP套接字
	if (sock == INVALID_SOCKET) {  // 检查套接字是否为无效套接字
		// 若套接字无效，打印当前文件路径、代码行号、函数名及错误提示并返回
		printf("%s(%d):%s ERROR!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) {  // 主客户端代码分支
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件、行号、函数名
		std::string msg = "hello world!\n";  // 定义要发送的消息
		// 向服务器发送消息，参数为套接字、消息内容、长度、标志、服务器地址、地址长度
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("host %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);  // 打印发送操作的返回值
		if (ret > 0) {  // 如果发送成功
			msg.resize(1024);  // 调整 msg 字符串的大小为 1024 字节
			memset((char*)msg.c_str(), 0, msg.size());  // 将 msg 对应的内存区域初始化为全 0
			// 从套接字接收数据到 msg 中，同时获取客户端地址信息
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			// 打印当前文件路径、代码行号、函数名、Winsock 错误码以及 recvfrom 的返回值，用于调试错误场景
			printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {  // 如果接收成功
				// 打印当前文件、行号、函数名以及客户端的IP和端口
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());  // 打印接收到的消息内容
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			// 打印当前文件路径、代码行号、函数名、Winsock 错误码以及 recvfrom 的返回值，用于调试错误场景
			printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {  // 如果接收成功
				// 打印当前文件、行号、函数名以及客户端的IP和端口
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());  // 打印接收到的消息内容
			}
		}
	}
	else {//从客户端代码分支
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件路径、代码行号、函数名
		std::string msg = "hello world!\n";  // 定义字符串msg并初始化为"hello world!\n"
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));  // 向服务器发送msg内容，返回值存入ret
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);  // 打印发送操作的返回值ret
		if (ret > 0) {  // 如果发送成功（ret>0）
			msg.resize(1024);  // 调整msg大小为1024字节
			memset((char*)msg.c_str(), 0, msg.size());  // 将msg对应的内存区域初始化为全0
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);  // 从套接字接收数据到msg，同时获取客户端地址
			printf("host %s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);  // 打印接收操作相关信息，包括错误码和ret
			if (ret > 0) {  // 如果接收成功（ret>0）
				sockaddr_in addr;  // 定义 sockaddr_in 类型的变量 addr，用于存储地址信息
				memcpy(&addr, msg.c_str(), sizeof(addr));  // 将 msg 中的内容拷贝到 addr 对应的内存区域，长度为 addr 的大小
				sockaddr_in* paddr = (sockaddr_in*)&addr;  // 将 addr 的地址转换为 sockaddr_in* 类型指针 paddr				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);  // 打印客户端的IP和端口
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());  // 打印msg的大小
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));  // 打印padd指向地址的IP和端口（端口转为主机字节序）
				msg = "hello, i am client!\r\n";  // 设置消息内容为"hello, i am client!\r\n"
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));// 向paddr指向的地址发送消息，参数为套接字、消息内容、长度、标志、目标地址指针、地址结构体大小
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));  // 打印padd指向地址的IP和端口（端口转为主机字节序）
				printf("host %s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);// 打印当前文件路径、代码行号、函数名、Winsock错误码以及sendto的返回值，带"host "前缀用于区分主机端
			}
		}
	}
	closesocket(sock);  // 关闭套接字
}