// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含框架头文件
#include "RemoteCtrl.h"  // 包含当前项目头文件
#include "Command.h" 
#include "ServerSocket.h"  // 包含服务器套接字相关头文件
#ifdef _DEBUG
#define new DEBUG_NEW  // 调试模式下使用DEBUG_NEW宏
#endif
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup" )  // 注释：设置子系统为windows及入口函数
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )  // 注释：设置子系统为windows及入口函数
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup" )  // 注释：设置子系统为console及入口函数
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )  // 注释：设置子系统为console及入口函数

// 唯一的应用程序对象   
CWinApp theApp;  // 定义MFC应用程序对象

using namespace std;  // 使用标准命名空间 


int main()  // 主函数
{
	int nRetCode = 0;  // 存储返回代码

	HMODULE hModule = ::GetModuleHandle(nullptr);  // 获取模块句柄

	if (hModule != nullptr)  // 模块句柄有效
	{
		// 初始化 MFC 并在失败时显示错误    
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))  // 初始化MFC失败
		{ 
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");  // 输出错误信息
			nRetCode = 1;  // 设置返回代码为错误
		}
		else  // MFC初始化成功
		{
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
			}
			
		}
	}
	else  // 模块句柄无效
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");  // 输出错误信息
		nRetCode = 1;  // 设置返回代码为错误
	}

	return nRetCode;  // 返回结果
}