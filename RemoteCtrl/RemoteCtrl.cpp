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
int main()  // 主函数
{
	if (!CEdoyunTool::Init()) return 1; // 调用工具类初始化方法，若失败则返回 1

	iocp();



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