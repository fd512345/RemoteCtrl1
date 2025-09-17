#pragma once                                                                   // 防止头文件重复包含

#include "pch.h"	                                                            // 预编译头文件
#include <atomic>                                                               // 包含原子操作库
#include <mutex>                                                                // 包含互斥量库
#include <list>                                                                 // 包含链表容器库
#include <process.h>                                                           // 包含线程操作函数库
#include <windows.h>                                                           // 包含Windows API头文件

template<class T>                                                              // 模板类声明，支持任意数据类型
class CEdoyunQueue                                                            // 线程安全队列类定义
{//线程安全的队列（利用IOCP实现）
public:
	enum {                                                                      // 操作类型枚举
		EQNone,                                                                 // 无操作
		EQPush,                                                                 // 入队操作
		EQPop,                                                                  // 出队操作
		EQSize,                                                                 // 获取大小操作
		EQClear                                                                 // 清空队列操作
	};

	typedef struct IocpParam {                                                  // IOCP操作参数结构体
		size_t nOperator; // 操作标识，用于区分不同的操作类型                   // 操作类型标识
		T Data; // 存储相关的数据                                               // 数据存储变量
		HANDLE hEvent;//pop操作需要的                                          // 事件句柄，用于同步

		IocpParam(int op, const T& data, HANDLE hEve = NULL) {                  // 结构体构造函数
			nOperator = op; // 将传入的操作标识赋值给结构体成员nOperator         // 初始化操作类型
			Data = data; // 将传入的数据赋值给结构体成员Data                     // 初始化数据
			hEvent = hEve;                                                      // 初始化事件句柄
		}
		IocpParam()                                                             // 默认构造函数
		{
			nOperator = EQNone;                                                 // 默认为无操作
		}
	} PPARAM;//Post Parameter 用于投递信息的结构体                              // 定义结构体别名

public:
	CEdoyunQueue() // 类的构造函数                                               // 构造函数
	{
		m_lock = false;                                                         // 初始化锁状态为未锁定
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // 创建IOCP端口
		m_hThread = INVALID_HANDLE_VALUE;                                       // 初始化线程句柄为无效
		if (m_hCompeletionPort != NULL) {                                       // 检查IOCP端口是否创建成功
			m_hThread = (HANDLE)_beginthread(                                   // 创建工作线程
				&CEdoyunQueue<T>::threadEntry,                                  // 线程入口函数
				0, this                                                        // 传递当前对象指针作为参数
			);
		}
	}

	~CEdoyunQueue() // 类的析构函数                                              // 析构函数
	{
		m_lock = true; // 修正变量名错误 m_lbck -> m_lock                        // 设置锁状态为锁定
		HANDLE hTemp = m_hCompeletionPort;                                      // 保存IOCP端口句柄
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);          // 投递退出信号
		WaitForSingleObject(m_hThread, INFINITE);                               // 等待工作线程结束
		m_hCompeletionPort = NULL;                                              // 置空IOCP端口句柄
		CloseHandle(hTemp);                                                     // 关闭IOCP端口
		CloseHandle(m_hThread);                                                 // 关闭线程句柄
	}

	bool PushBack(const T& data)                                                // 入队操作
	{
		PPARAM* pParam = new PPARAM(EQPush, data);                              // 创建入队操作参数
		if (m_lock)                                                             // 检查是否锁定
		{
			delete pParam;                                                      // 释放参数对象
			return false;                                                       // 返回失败
		}
		// 向IOCP端口投递入队操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (!ret)                                                               // 检查投递是否成功
			delete pParam;                                                      // 失败则释放参数
		return ret;                                                             // 返回操作结果
	}

	bool PopFront(T& data)                                                      // 出队操作
	{
		// 创建事件用于同步
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!hEvent)                                                            // 检查事件是否创建成功
			return false;                                                       // 失败返回

		PPARAM Param(EQPop, data, hEvent);                                      // 创建出队操作参数
		if (m_lock)                                                             // 检查是否锁定
		{
			CloseHandle(hEvent);                                                // 关闭事件句柄
			return false;                                                       // 返回失败
		}

		// 向IOCP端口投递出队操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (!ret)                                                               // 检查投递是否成功
		{
			CloseHandle(hEvent);                                                // 关闭事件句柄
			return false;                                                       // 返回失败
		}

		// 等待操作完成
		ret = (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0);
		if (ret)                                                                // 检查操作是否成功
		{
			data = Param.Data;                                                  // 获取出队数据
		}
		CloseHandle(hEvent);                                                    // 关闭事件句柄
		return ret;                                                             // 返回操作结果
	}

	size_t Size()                                                               // 获取队列大小
	{
		// 创建事件用于同步
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!hEvent)                                                            // 检查事件是否创建成功
			return -1;                                                          // 失败返回

		PPARAM Param(EQSize, T(), hEvent);                                      // 创建获取大小操作参数
		if (m_lock)                                                             // 检查是否锁定
		{
			CloseHandle(hEvent);                                                // 关闭事件句柄
			return -1;                                                          // 返回失败
		}

		// 向IOCP端口投递获取大小操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (!ret)                                                               // 检查投递是否成功
		{
			CloseHandle(hEvent);                                                // 关闭事件句柄
			return -1;                                                          // 返回失败
		}

		// 等待操作完成
		ret = (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0);
		CloseHandle(hEvent);                                                    // 关闭事件句柄

		if (ret)                                                                // 检查操作是否成功
		{
			return Param.nOperator;                                             // 返回队列大小
		}
		return -1;                                                              // 返回失败
	}

	bool Clear()                                                                // 清空队列
	{
		if (m_lock)                                                            // 检查是否锁定
			return false;                                                     // 返回失败

		PPARAM* pParam = new PPARAM(EQClear, T());                             // 创建清空操作参数
		// 向IOCP端口投递清空操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (!ret)                                                              // 检查投递是否成功
			delete pParam;                                                     // 失败则释放参数
		return ret;                                                            // 返回操作结果
	}

private:
	static void threadEntry(void* arg)                                         // 线程入口函数
	{
		// 将参数转换为当前类指针
		CEdoyunQueue<T>* thiz = static_cast<CEdoyunQueue<T>*>(arg);
		if (thiz)                                                               // 检查指针有效性
			thiz->threadMain();                                                // 调用线程主函数
		_endthread();                                                           // 结束线程
	}

	void DealParam(PPARAM* pParam)                                             // 处理操作参数
	{
		if (!pParam) return;                                                   // 检查参数有效性

		switch (pParam->nOperator)                                             // 根据操作类型处理
		{
		case EQPush:                                                           // 入队操作
			m_lstData.push_back(pParam->Data);                                 // 将数据加入队列
			delete pParam;                                                     // 释放参数对象
			break;

		case EQPop:                                                            // 出队操作
			if (!m_lstData.empty()) {                                          // 检查队列是否为空
				pParam->Data = m_lstData.front();                              // 获取队首元素
				m_lstData.pop_front();                                         // 移除队首元素
			}
			if (pParam->hEvent != NULL)                                        // 检查事件句柄
				SetEvent(pParam->hEvent);                                      // 触发事件通知完成
			break;

		case EQSize:                                                           // 获取大小操作
			pParam->nOperator = m_lstData.size();                              // 保存队列大小
			if (pParam->hEvent != NULL)                                        // 检查事件句柄
				SetEvent(pParam->hEvent);                                      // 触发事件通知完成
			break;

		case EQClear:                                                          // 清空队列操作
			m_lstData.clear();                                                 // 清空队列
			delete pParam;                                                     // 释放参数对象
			break;

		default:                                                               // 未知操作
			OutputDebugStringA("unknown operator!\r\n");                        // 输出调试信息
			break;
		}
	}

	void threadMain()                                                          // 线程主函数
	{
		DWORD dwTransferred = 0;                                               // 传输字节数
		PPARAM* pParam = NULL;                                                 // 操作参数指针
		ULONG_PTR CompletionKey = 0;                                           // 完成键
		OVERLAPPED* pOverlapped = NULL;                                        // 重叠结构体指针

		// 从IOCP端口获取完成状态
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {              // 检查退出信号
				printf("thread is prepare to exit!\r\n");                      // 输出退出信息
				break;                                                         // 跳出循环
			}

			// 转换获取操作参数
			pParam = static_cast<PPARAM*>(reinterpret_cast<void*>(CompletionKey));
			DealParam(pParam);                                                 // 处理操作参数
		}

		// 处理剩余的队列项
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {              // 检查退出信号
				printf("thread is prepare to exit!\r\n");                      // 输出退出信息
				continue;                                                      // 继续处理下一个
			}

			// 转换获取操作参数
			pParam = static_cast<PPARAM*>(reinterpret_cast<void*>(CompletionKey));
			DealParam(pParam);                                                 // 处理操作参数
		}
	}

private:
	std::list<T> m_lstData;                                                    // 存储数据的链表
	HANDLE m_hCompeletionPort;                                                 // IOCP端口句柄
	HANDLE m_hThread;                                                          // 工作线程句柄
	std::atomic<bool> m_lock;                                                  // 原子变量，标记队列是否锁定
};
