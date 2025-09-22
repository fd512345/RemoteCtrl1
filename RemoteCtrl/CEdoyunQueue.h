#pragma once                                                               // 防止头文件重复包含
#include "pch.h"                                                           // 包含预编译头文件
#include <atomic>                                                          // 包含原子操作库，用于线程安全的变量操作
#include <list>                                                            // 包含链表容器，作为队列的底层存储
#include "EdoyunThread.h"                                                  // 包含自定义线程类头文件


//该头文件实现了一个轻量级的线程池框架，用于管理多个工作线程并分配任务执行。
//核心功能通过EdoyunThread（单个线程封装）和EdoyunThreadPool（线程池管理）两个类实现，配合ThreadWorker（任务封装）完成任务分发与执行

template<class T>                                                         // 定义模板类，支持任意数据类型的队列
class CEdoyunQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		EQNone,                                                           // 无操作
		EQPush,                                                           // 入队操作标识
		EQPop,                                                            // 出队操作标识
		EQSize,                                                           // 获取队列大小操作标识
		EQClear                                                           // 清空队列操作标识
	};
	typedef struct IocpParam {
		size_t nOperator;//操作                                            // 存储操作类型（EQPush/EQPop等）
		T Data;//数据                                                       // 存储操作涉及的数据
		HANDLE hEvent;//pop操作需要的                                       // 用于出队操作的事件句柄，通知操作完成
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {             // 构造函数，初始化操作类型、数据和事件句柄
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {                                                     // 默认构造函数，初始化操作类型为无操作
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体                           // 定义IOCP投递参数结构体别名

public:
	CEdoyunQueue() {                                                       // 构造函数
		m_lock = false;                                                    // 初始化析构锁为false（未析构）
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // 创建IOCP端口，用于线程间通信
		m_hThread = INVALID_HANDLE_VALUE;                                  // 初始化线程句柄为无效值
		if (m_hCompeletionPort != NULL) {                                  // 若IOCP端口创建成功
			m_hThread = (HANDLE)_beginthread(                               // 创建工作线程，执行threadEntry函数
				&CEdoyunQueue<T>::threadEntry,
				0, this
			);
		}
	}
	virtual ~CEdoyunQueue() {                                              // 析构函数
		if (m_lock)return;                                                 // 若已处于析构中，直接返回
		m_lock = true;                                                     // 标记为正在析构
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);     // 向IOCP端口投递退出信号
		WaitForSingleObject(m_hThread, INFINITE);                          // 等待工作线程结束
		if (m_hCompeletionPort != NULL) {                                  // 若IOCP端口有效
			HANDLE hTemp = m_hCompeletionPort;                             // 临时保存端口句柄
			m_hCompeletionPort = NULL;                                      // 置空成员变量
			CloseHandle(hTemp);                                             // 关闭IOCP端口
		}
	}
	bool PushBack(const T& data) {                                         // 入队操作
		IocpParam* pParam = new IocpParam(EQPush, data);                   // 创建入队操作参数
		if (m_lock) {                                                       // 若正在析构
			delete pParam;                                                  // 释放参数内存
			return false;                                                   // 返回失败
		}
		// 向IOCP端口投递入队操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;                                    // 若投递失败，释放参数内存
		return ret;                                                        // 返回操作结果
	}
	virtual bool PopFront(T& data) {                                        // 出队操作
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);               // 创建事件句柄，用于等待出队完成
		IocpParam Param(EQPop, data, hEvent);                               // 创建出队操作参数
		if (m_lock) {                                                       // 若正在析构
			if (hEvent)CloseHandle(hEvent);                                 // 关闭事件句柄
			return false;                                                   // 返回失败
		}
		// 向IOCP端口投递出队操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {                                                 // 若投递失败
			CloseHandle(hEvent);                                            // 关闭事件句柄
			return false;                                                   // 返回失败
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;       // 等待出队操作完成
		if (ret) {                                                          // 若操作完成
			data = Param.Data;                                              // 获取出队数据
		}
		return ret;                                                        // 返回操作结果
	}
	size_t Size() {                                                        // 获取队列大小
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);               // 创建事件句柄，用于等待结果
		IocpParam Param(EQSize, T(), hEvent);                              // 创建获取大小操作参数
		if (m_lock) {                                                       // 若正在析构
			if (hEvent)CloseHandle(hEvent);                                 // 关闭事件句柄
			return -1;                                                      // 返回无效值
		}
		// 向IOCP端口投递获取大小操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {                                                 // 若投递失败
			CloseHandle(hEvent);                                            // 关闭事件句柄
			return -1;                                                      // 返回无效值
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;       // 等待操作完成
		if (ret) {                                                          // 若操作完成
			return Param.nOperator;                                         // 返回队列大小（存储在nOperator中）
		}
		return -1;                                                          // 返回无效值
	}
	bool Clear() {                                                         // 清空队列
		if (m_lock)return false;                                           // 若正在析构，返回失败
		IocpParam* pParam = new IocpParam(EQClear, T());                   // 创建清空操作参数
		// 向IOCP端口投递清空操作
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;                                    // 若投递失败，释放参数内存
		return ret;                                                        // 返回操作结果
	}
protected:
	static void threadEntry(void* arg) {                                   // 线程入口函数
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;                     // 将参数转换为当前类指针
		thiz->threadMain();                                                // 调用线程主函数
		_endthread();                                                      // 结束线程
	}
	virtual void DealParam(PPARAM* pParam) {                                // 处理IOCP投递的参数
		switch (pParam->nOperator)                                         // 根据操作类型处理
		{
		case EQPush:                                                       // 入队操作
			m_lstData.push_back(pParam->Data);                              // 将数据加入链表
			delete pParam;                                                  // 释放参数内存
			break;
		case EQPop:                                                        // 出队操作
			if (m_lstData.size() > 0) {                                     // 若队列非空
				pParam->Data = m_lstData.front();                           // 获取队首数据
				m_lstData.pop_front();                                      // 移除队首元素
			}
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);            // 若有事件句柄，触发事件通知完成
			break;
		case EQSize:                                                       // 获取大小操作
			pParam->nOperator = m_lstData.size();                           // 将队列大小存入nOperator
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);                                   // 触发事件通知完成
			break;
		case EQClear:                                                       // 清空操作
			m_lstData.clear();                                              // 清空链表
			delete pParam;                                                  // 释放参数内存
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");                    // 输出未知操作调试信息
			break;
		}
	}
	virtual void threadMain() {                                             // 线程主函数
		DWORD dwTransferred = 0;                                            // 用于接收IOCP传输字节数
		PPARAM* pParam = NULL;                                              // 用于接收IOCP参数
		ULONG_PTR CompletionKey = 0;                                        // 用于接收完成键
		OVERLAPPED* pOverlapped = NULL;                                     // 用于接收重叠结构
		// 循环从IOCP端口获取完成通知
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {          // 若收到退出信号
				printf("thread is prepare to exit!\r\n");                   // 输出退出信息
				break;                                                      // 退出循环
			}

			pParam = (PPARAM*)CompletionKey;                                // 转换参数指针
			DealParam(pParam);                                              // 处理参数
		}
		// 处理剩余的IOCP通知
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, 0))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {          // 若收到退出信号
				printf("thread is prepare to exit!\r\n");                   // 输出退出信息
				continue;                                                   // 继续处理下一个
			}
			pParam = (PPARAM*)CompletionKey;                                // 转换参数指针
			DealParam(pParam);                                              // 处理参数
		}
		HANDLE hTemp = m_hCompeletionPort;                                 // 临时保存IOCP端口句柄
		m_hCompeletionPort = NULL;                                          // 置空成员变量
		CloseHandle(hTemp);                                                 // 关闭IOCP端口
	}
protected:
	std::list<T> m_lstData;                                                 // 底层存储链表
	HANDLE m_hCompeletionPort;                                             // IOCP端口句柄
	HANDLE m_hThread;                                                      // 工作线程句柄
	std::atomic<bool> m_lock;//队列正在析构                                // 原子变量，标记是否正在析构
};


<<<<<<< Updated upstream

template<class T>
class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CEdoyunQueue<T>(), m_base(obj), m_callback(callback)
=======
	template<class T>                                                         // 定义模板类，继承自CEdoyunQueue和ThreadFuncBase
	class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
>>>>>>> Stashed changes
	{
	public:
		typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);                  // 定义回调函数指针类型
		EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)             // 构造函数
			:CEdoyunQueue<T>(), m_base(obj), m_callback(callback)               // 初始化基类和成员变量
		{
			m_thread.Start();                                                  // 启动发送线程
			// 更新线程工作函数为当前类的threadTick
			m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&EdoyunSendQueue<T>::threadTick));
		}
		virtual ~EdoyunSendQueue() {                                           // 析构函数
			m_base = NULL;                                                     // 置空回调对象指针
			m_callback = NULL;                                                 // 置空回调函数指针
			m_thread.Stop();                                                   // 停止发送线程
		}

	protected:
		virtual bool PopFront(T& data) { return false; };                       // 重写基类出队方法，禁止外部直接调用
		bool PopFront() {                                                      // 内部出队方法
			// 创建出队操作参数
			typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T());
			if (CEdoyunQueue<T>::m_lock) {                                     // 若正在析构
				delete Param;                                                  // 释放参数内存
				return false;                                                   // 返回失败
			}
			// 向IOCP端口投递出队操作
			bool ret = PostQueuedCompletionStatus(CEdoyunQueue<T>::m_hCompeletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
			if (ret == false) {                                                 // 若投递失败
				delete Param;                                                  // 释放参数内存
				return false;                                                   // 返回失败
			}
			return ret;                                                        // 返回操作结果
		}
		int threadTick() {                                                     // 线程定时执行函数
			// 检查队列工作线程是否已结束
			if (WaitForSingleObject(CEdoyunQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)
				return -1;                                                      // 若已结束，返回-1
			if (CEdoyunQueue<T>::m_lstData.size() > 0) {                       // 若队列非空
				PopFront();                                                    // 执行出队操作
			}
			return 0;                                                          // 返回0表示继续执行
		}
		// 重写参数处理方法
		virtual void DealParam(typename CEdoyunQueue<T>::PPARAM* pParam) {
			switch (pParam->nOperator)                                         // 根据操作类型处理
			{
			case CEdoyunQueue<T>::EQPush:                                      // 入队操作
				CEdoyunQueue<T>::m_lstData.push_back(pParam->Data);             // 将数据加入链表
				delete pParam;                                                  // 释放参数内存
				break;
			case CEdoyunQueue<T>::EQPop:                                       // 出队操作
				if (CEdoyunQueue<T>::m_lstData.size() > 0) {                    // 若队列非空
					pParam->Data = CEdoyunQueue<T>::m_lstData.front();          // 获取队首数据
					// 调用回调函数处理数据，若成功则移除队首元素
					if ((m_base->*m_callback)(pParam->Data) == 0)
						CEdoyunQueue<T>::m_lstData.pop_front();
				}
				delete pParam;                                                  // 释放参数内存
				break;
			case CEdoyunQueue<T>::EQSize:                                      // 获取大小操作
				pParam->nOperator = CEdoyunQueue<T>::m_lstData.size();          // 存储队列大小
				if (pParam->hEvent != NULL)
					SetEvent(pParam->hEvent);                                   // 触发事件通知完成
				break;
			case CEdoyunQueue<T>::EQClear:                                      // 清空操作
				CEdoyunQueue<T>::m_lstData.clear();                             // 清空链表
				delete pParam;                                                  // 释放参数内存
				break;
			default:
				OutputDebugStringA("unknown operator!\r\n");                    // 输出未知操作调试信息
				break;
			}
		}
	private:
		ThreadFuncBase* m_base;                                                // 回调函数所属对象
		EDYCALLBACK m_callback;                                                // 数据处理回调函数
		EdoyunThread m_thread;                                                 // 发送线程对象
	};

	// 定义发送队列回调函数类型别名（针对char向量类型）
	typedef EdoyunSendQueue<std::vector<char>>::EDYCALLBACK  SENDCALLBACK;