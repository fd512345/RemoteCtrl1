#pragma once

#include "pch.h"	
#include <atomic>
#include <mutex>

template<class T>
class CEdoyunQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};

	typedef struct IocpParam {
		size_t nOperator; // 操作标识，用于区分不同的操作类型
		T Data; // 存储相关的数据
		HANDLE hEvent;//pop操作需要的

		IocpParam(int op, const T& data, HANDLE hEve = NULL) { // IocpParam 结构体的构造函数，接收操作标识、数据以及可选的回调函数指针
			nOperator = op; // 将传入的操作标识赋值给结构体成员 nOperator
			Data = data; // 将传入的数据赋值给结构体成员 Data
			hEvent = hEve;
		}
		IocpParam()
		{
			nOperator = EQNone;
		}
	} PPARAM;//Post Parameter 用于投递信息的结构体
public:
	CEdoyunQueue() // 类的构造函数
	{
		if (m_lock) return;
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // 创建 I/O 完成端口，最后一个参数 1 可指定关联的线程数等（具体含义依场景）
		m_hThread = INVALID_HANDLE_VALUE; // 初始化线程句柄为无效句柄值
		if (m_hCompletionPort != NULL) { // 判断 I/O 完成端口创建成功
			m_hThread = (HANDLE)_beginthread( // 创建线程，线程入口函数为类的静态成员函数 threadEntry，传递 m_hCompletionPort 作为参数
				&CEdo yunQueue<T>::threadEntry,
				0, m_hCompletionPort
			);
		}
	}
	~CEdoyunQueue() // 类的析构函数
	{
		m_lbck = true; // 设置锁标识为 true（假设用于同步等场景）
		HANDLE hTemp = m_hCompletionPort; // 临时保存 I/O 完成端口句柄
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); // 向 I/O 完成端口投递完成状态（传输字节数为 0，完成键和 OVERLAPPED 指针为 NULL）
		WaitForSingleObject(m_hThread, INFINITE); // 无限等待线程 m_hThread 结束
		m_hCompletionPort = NULL; // 将 I/O 完成端口句柄置空
		CloseHandle(hTemp); // 关闭临时保存的 I/O 完成端口句柄	
	}

	bool PushBack(const T& data) // 向队列尾部添加数据的方法，返回是否成功
	{
		IocpParam* pParam = new IocpParam(EQPush, data); // 新建 IocpParam 对象，操作类型为 EQPush，数据为 strData
		if (m_lock) 
		{
			delete pParam;
			return false;// 若处于锁定状态，返回 false
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL); // 向 I/O 完成端口投递完成状态，传输字节数为 pParam 指针大小，完成键为 pParam 指针，OVERLAPPED 指针为 NULL
		if (ret == false) delete pParam; // 若投递失败，释放 pParam 指向的内存
		return ret; // 返回投递操作是否成功的结果
	}
	bool PopFront(T& data) // 从队列头部弹出数据到 data，返回是否成功
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 创建一个自动重置、初始为未触发的事件
		IocpParam Param(EQPop, data, hEvent); // 新建 IocpParam 对象，操作类型为 EQPush，数据为 data，关联事件 hEvent
		if (m_lock)
		{
			if (hEvent) CloseHandle(hEvent); // 如果事件句柄 hEvent 有效，关闭该事件句柄
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&Param, NULL); // 向 I/O 完成端口投递完成状态
		if (ret == false) { // 若投递失败
			CloseHandle(hEvent); // 关闭事件句柄
			return false; // 返回 false
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; // 无限等待事件触发，判断是否等待成功
		if (ret) { // 若等待成功
			data = Param.Data; // 将 Param 中的数据赋值给传入的 data
		}
		return ret; // 返回操作是否成功的结果
	}
	size_t Size() // 获取队列大小的方法
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 创建一个自动重置、初始为未触发的事件
		IocpParam Param(EQSize, T(), hEvent); // 新建 IocpParam 对象，操作类型为 EQPush，数据为 data，关联事件 hEvent
		if (m_lock)
		{
			if (hEvent) CloseHandle(hEvent); // 如果事件句柄 hEvent 有效，关闭该事件句柄
			return -1;
		}// 若处于锁定状态，返回 false
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&Param, NULL); // 向 I/O 完成端口投递完成状态
		if (ret == false) { // 若投递失败
			CloseHandle(hEvent); // 关闭事件句柄
			return -1; // 返回 false
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; // 无限等待事件触发，判断是否等待成功
		if (ret) { // 若等待成功
			return Param.nOperator; // 将 Param 中的数据赋值给传入的 data
		}
		return -1; // 返回操作是否成功的结果
	}

	bool Clear()// 清空队列的方法
	{
		if (m_lock) return false; // 若处于锁定状态，返回 false
		IocpParam* pParam = new IocpParam(EQClear, T()); // 新建 IocpParam 对象，操作类型为 EQPush，数据为 strData
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL); // 向 I/O 完成端口投递完成状态，传输字节数为 pParam 指针大小，完成键为 pParam 指针，OVERLAPPED 指针为 NULL
		if (ret == false) delete pParam; // 若投递失败，释放 pParam 指向的内存
		return ret; // 返回投递操作是否成功的结果
	}
private:
	static void threadEntry(void* arg) // 静态线程入口函数
	{
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void DealParam(PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case EQPush: // 处理推入操作

			m_lstData.push_back(pParam->Data); // 将 pParam 中的字符串数据推入列表 m_lstData
			delete pParam; // 释放 pParam 指向的内存

			break;
		case EQPop:
			// 若操作类型为从 IOCP 列表弹出
			if (m_lstData.size() > 0) { // 列表非空时
				pParam->Data = m_lstData.front(); // 获取列表头部元素
				m_lstData.pop_front(); // 弹出列表头部元素
			}
			if (pParam->hEvent != NULL)  // 若存在回调函数
				SetEvent(pParam->hEvent);


			break;
		case EQSize:
			// 若操作类型为获取队列大小
			pParam->nOperator = m_lstData.size(); // 将队列 m_lstData 的大小赋值给 pParam 的 nOperator 成员
			if (pParam->hEvent != NULL)  // 若事件句柄非空
				SetEvent(pParam->hEvent); // 触发该事件


			break;
		case EQClear:
		{ // 判断操作类型为清空列表
			m_lstData.clear(); // 清空字符串列表
			delete pParam;
		}
		break;
		default:
			OutputDebugStringA("unknown operator!\r\n"); // 向调试器输出“unknown operator!”的调试信息
			break;
		}
	}
	void threadMain() // 线程主函数
	{
		DWORD dwTransferred = 0; // 用于存储传输的字节数
		PPARAM* pParam = NULL; // 指向 PPARAM 类型的指针，初始化为空
		ULONG_PTR CompletionKey = 0; // 用于存储完成键
		OVERLAPPED* pOverlapped = NULL; // 指向 OVERLAPPED 结构体的指针，初始化为空
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) { // 从 I/O 完成端口获取完成状态，无限等待
			if ((dwTransferred == 0) || (CompletionKey == NULL)) { // 判断传输字节数为 0 且完成键为 NULL
				printf("thread is prepare to exit!\r\n"); // 输出线程准备退出的提示信息
				break; // 跳出循环
			}
			pParam = (PPARAM*)CompletionKey; // 将 CompletionKey 强制转换为 IOCP_PARAM* 类型的指针，用于后续操作 IOCP_PARAM 结构体数据
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) { // 判断传输字节数为 0 且完成键为 NULL
				printf("thread is prepare to exit!\r\n"); // 输出线程准备退出的提示信息
				continue;
			}
			pParam = (PPARAM*)CompletionKey; // 将 CompletionKey 强制转换为 IOCP_PARAM* 类型的指针，用于后续操作 IOCP_PARAM 结构体数据
			DealParam(pParam);
		}
		CloseHandle(m_hCompeletionPort);
	}
private:
	std::list<T> m_lstData; // 存储数据的列表
	HANDLE m_hCompeletionPort; // I/O 完成端口句柄
	HANDLE m_hThread; // 线程句柄
	std::atomic<bool> m_lock;//队列正在析构

};

