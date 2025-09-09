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
			//1 进度的可控性 2 对接的方便性 3 可行性评估，提早暴露风险
			// TODO: socket、bind、listen、accept、read、write、close
			//套接字初始化
			CCommand cmd;  // 定义命令处理对象
			CServerSocket* pserver = CServerSocket::getInstance();  // 获取服务器套接字实例
			int count = 0;  // 记录连接失败次数
			if (pserver->InitSocket() == false) {  // 初始化套接字失败
				MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);  // 显示错误消息框
				exit(0);  // 退出程序
			}
			while (CServerSocket::getInstance() != NULL) {  // 循环处理连接
				if (pserver->AcceptClient() == false) {  // 接受客户端连接失败
					if (count >= 3) {  // 失败次数超过3次
						MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);  // 显示错误消息框
						exit(0);  // 退出程序
					}
					MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);  // 显示错误消息框
					count++;  // 增加失败计数
				}
				TRACE("AcceptClient return true\r\n");  // 输出连接成功信息
				int ret = pserver->DealCommand();  // 处理命令
				TRACE("DealCommand ret %d\r\n", ret);  // 输出处理结果
				if (ret > 0) {  // 命令有效
					ret = cmd.ExecuteCommand(ret);  // 执行命令
					if (ret != 0) {  // 执行命令失败
						TRACE("执行命令失败：%d ret=%d\r\n", pserver->GetPacket().sCmd, ret);  // 输出错误信息
					}
					pserver->CloseClient();  // 关闭客户端连接
					TRACE("Command has done!\r\n");  // 输出命令完成信息
				}
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