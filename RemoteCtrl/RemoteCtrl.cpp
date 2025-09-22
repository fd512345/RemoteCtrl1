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


int main(int argc, char* argv[])  // 主函数
{
	if (!CEdoyunTool::Init()) return 1; // 调用工具类初始化方法，若失败则返回 1

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


void udp_server() {
	// 打印当前文件路径、代码行号、函数名
	printf("%s (%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();  // 等待用户输入，暂停程序
}

void udp_client(bool ishost) {
	if (ishost) {
		// 若ishost为true，打印当前文件路径、代码行号、函数名
		printf("%s (%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
	else {
		// 若ishost为false，打印当前文件路径、代码行号、函数名
		printf("%s (%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
}