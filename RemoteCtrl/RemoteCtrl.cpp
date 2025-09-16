// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含框架头文件
#include "RemoteCtrl.h"  // 包含当前项目头文件
#include "Command.h" 
#include "ServerSocket.h"  // 包含服务器套接字相关头文件
#include <conio.h>
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
bool Init()
{
	HMODULE hModule = ::GetModuleHandle(nullptr);  // 获取当前模块的句柄，若为 nullptr 表示获取失败
	if (hModule == nullptr) {
		wprintf(L"错误: GetModuleHandle 失败\n");  // 输出获取模块句柄失败的错误信息
		return false;  // 返回 false 表示初始化失败
	}
	if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))  // 初始化 MFC 库，若返回 false 表示初始化失败
	{
		wprintf(L"错误: MFC 初始化失败\n");  // 输出 MFC 初始化失败的错误信息
		return false;  // 返回 false 表示初始化失败
	}
	return true;
}

#define IOCP_LIST_EMPTY 0  // 定义宏，标识 IOCP 列表为空
#define IOCP_LIST_PUSH 1   // 定义宏，标识对 IOCP 列表执行推入操作
#define IOCP_LIST_POP 2    // 定义宏，标识对 IOCP 列表执行弹出操作

enum {
	IocpListEmpty,  // 枚举值，对应 IOCP 列表为空的状态
	IocpListPush,   // 枚举值，对应对 IOCP 列表执行推入操作的状态
	IocpListPop     // 枚举值，对应对 IOCP 列表执行弹出操作的状态
};

typedef struct IocpParam {
	int nOperator; // 操作标识，用于区分不同的操作类型
	std::string strData; // 存储相关的数据
	_beginthread_proc_type cbFunc; // 回调函数指针
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) { // IocpParam 结构体的构造函数，接收操作标识、数据以及可选的回调函数指针
		nOperator = op; // 将传入的操作标识赋值给结构体成员 nOperator
		strData = sData; // 将传入的数据赋值给结构体成员 strData
		cbFunc = cb; // 将传入的回调函数指针赋值给结构体成员 cbFunc
	}
	IocpParam()
	{
		nOperator = -1;
	}
} IOCP_PARAM;

void threadQueueEntry(HANDLE hIOCP) // 线程入口函数，接收一个 void* 类型的参数
{
	std::list<std::string> lstString;
	DWORD dwTransferred = 0; // 用于存储 I/O 操作传输的字节数
	ULONG_PTR CompletionKey = 0; // 用于存储完成键
	OVERLAPPED* pOverlapped = NULL; // 用于存储 OVERLAPPED 结构体指针
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) { // 从 I/O 完成端口获取完成状态，无限等待
		if ((dwTransferred == 0) || (CompletionKey == NULL)) { // 判断传输字节数为 0 且完成键为 NULL
			printf("thread is prepare to exit!\r\n"); // 输出线程准备退出的提示信息
			break; // 跳出循环
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey; // 将 CompletionKey 强制转换为 IOCP_PARAM* 类型的指针，用于后续操作 IOCP_PARAM 结构体数据
		if (pParam->nOperator == IocpListPush) { // 判断操作类型为推入列表
			lstString.push_back(pParam->strData); // 将数据推入字符串列表
		}
		else if (pParam->nOperator == IocpListPop) { // 判断操作类型为从列表弹出
			std::string* pStr = NULL;
			if (lstString.size() > 0) { // 列表不为空时
				pStr = new std::string(lstString.front()); // 新建字符串并复制列表头部元素
				lstString.pop_front(); // 弹出列表头部元素
			}
			if (pParam->cbFunc) { // 若存在回调函数
				pParam->cbFunc(pStr); // 调用回调函数并传入弹出的字符串指针
			}
		}
		else if (pParam->nOperator == IocpListEmpty) { // 判断操作类型为清空列表
			lstString.clear(); // 清空字符串列表
		}
		delete pParam;
	}
	_endthread(); // 结束当前线程
}

void func(void* arg) // 函数 func，接收 void* 类型的参数
{
	std::string* pstr = (std::string*)arg; // 将 void* 类型的 arg 强制转换为 std::string* 类型指针
	if (pstr != NULL) { // 判断指针是否非空
		printf("pop from list:%s\r\n", pstr->c_str()); // 输出从列表弹出的字符串内容
		delete pstr; // 释放 pstr 指向的内存
	}
	else { // 指针为空时的处理
		printf("list is empty,no data!\r\n"); // 输出列表为空、无数据的提示信息
	}
}

int main()  // 主函数
{
	if (!Init()) return 1;

	printf("press any key to exit ...\r\n"); // 输出提示信息，按任意键退出
	HANDLE hIOCP = INVALID_HANDLE_VALUE; // Input/Output Completion Port，初始化 IOCP 句柄为无效句柄值
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // 调用 CreateIoCompletionPort 函数创建 I/O 完成端口，第一个参数为 INVALID_HANDLE_VALUE 表示创建新的完成端口，最后一个参数 1 可指定关联的线程数等（具体含义依场景）
	HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP); // 调用 _beginthread 创建线程，入口函数为 threadQueueEntry，传递 hIOCP 作为参数

	ULONGLONG tick = GetTickCount64(); // 获取当前系统启动后的毫秒数（64 位）
	while (_kbhit() != 0) { // 当有键盘输入时进入循环 把请求和实现分离
		if (GetTickCount64() - tick > 1300) { // 如果距离上次记录的时间超过 1300 毫秒
			// 向 I/O 完成端口投递完成状态，携带新创建的 IOCP_PARAM（操作类型为 IocpListPush，数据为 "hello world"）
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world"), NULL);
		}
		if (GetTickCount64() - tick > 2000) { // 如果距离上次记录的时间超过 2000 毫秒
			// 向 I/O 完成端口投递完成状态，携带新创建的 IOCP_PARAM（操作类型为 IocpListPush，数据为 "hello world"）
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
			tick = GetTickCount64(); // 更新上次记录的时间为当前时间
		}
		Sleep(1); // 线程休眠 1 毫秒
	}

	if (hIOCP != NULL) { // 判断 hIOCP 是否不为空（即 I/O 完成端口有效）
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL); // 向 I/O 完成端口投递一个完成状态
		WaitForSingleObject(hThread, INFINITE); // 无限等待创建的线程结束
	}
	CloseHandle(hIOCP); // 关闭 I/O 完成端口的句柄
	printf("exit done!\r\n"); // 输出退出完成的提示信息
	::exit(0); // 调用全局的 exit 函数，以 0 为退出码终止程序

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