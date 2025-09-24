#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "EdoyunThread.h"

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
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent;//pop操作需要的
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体
public:
	CEdoyunQueue() {
		m_lock = false;
		//IOCP 的实现首先需要创建一个完成端口，这是整个 IOCP 机制的核心。
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CEdoyunQueue<T>::threadEntry,
				0, this
			);
		}
	}
	virtual ~CEdoyunQueue() {
		if (m_lock)return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("push back done %d %08p\r\n", ret, (void*)pParam);
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_lock)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("Clear %08p\r\n", (void*)pParam);
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case EQPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case EQPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case EQClear:
			m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	virtual void threadMain() {  // 线程主函数，声明为虚函数
		DWORD dwTransferred = 0;  // 用于存储传输的字节数，初始化为0
		PPARAM* pParam = NULL;  // 指向参数结构体指针的指针，初始化为空
		ULONG_PTR CompletionKey = 0;  // 完成键，用于标识完成端口上的特定操作，初始化为0
		OVERLAPPED* pOverlapped = NULL;  // 指向OVERLAPPED结构体的指针，用于异步I/O，初始化为空
		while (GetQueuedCompletionStatus(  // 循环获取完成端口上的完成状态
			m_hCompeletionPort,  // 完成端口句柄
			&dwTransferred,  // 接收传输的字节数
			&CompletionKey,  // 接收完成键
			&pOverlapped, INFINITE))  // 接收OVERLAPPED指针，等待时间为无限
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {  // 判断传输字节数为0或完成键为空的情况
				printf("thread is prepare to exit!\r\n");  // 打印线程准备退出的信息
				break;  // 跳出当前循环
			}

			pParam = (PPARAM*)CompletionKey;  // 将完成键转换为PPARAM*类型的指针
			DealParam(pParam);  // 处理参数的函数调用
		}
		while (GetQueuedCompletionStatus(  // 再次循环获取完成端口上的完成状态
			m_hCompeletionPort,  // 完成端口句柄
			&dwTransferred,  // 接收传输的字节数
			&CompletionKey,  // 接收完成键
			&pOverlapped, 0))  // 接收OVERLAPPED指针，等待时间为0（非阻塞）
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {  // 判断传输字节数为0或完成键为空的情况
				printf("thread is prepare to exit!\r\n");  // 打印线程准备退出的信息
				continue;  // 继续下一次循环
			}

			pParam = (PPARAM*)CompletionKey;  // 将完成键转换为PPARAM*类型的指针
			DealParam(pParam);  // 处理参数的函数调用
		}
		HANDLE hTemp = m_hCompeletionPort;  // 临时句柄变量，保存完成端口句柄
		m_hCompeletionPort = NULL;  // 将完成端口句柄置空
		CloseHandle(hTemp);  // 关闭临时保存的完成端口句柄
	}
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//队列正在析构
};


template<class T>
class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CEdoyunQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&EdoyunSendQueue<T>::threadTick));
	}
	virtual ~EdoyunSendQueue() {
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();

	}

protected:
	virtual bool PopFront(T& data) {
		return false;
	};
	bool PopFront() {
		typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T());
		if (CEdoyunQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CEdoyunQueue<T>::m_hCompeletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (WaitForSingleObject(CEdoyunQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)  // 检查线程m_hThread是否未处于超时状态（即线程已结束或可立即响应）
			return -1;                                          // 若线程未超时，直接返回0
		if (CEdoyunQueue<T>::m_lstData.size() > 0) {           // 检查队列CEdoyunQueue<T>的m_lstData成员元素数量是否大于0
			PopFront();                                        // 若队列有元素，调用PopFront方法弹出队首元素
		}
		return 0;                                              // 执行完毕，返回0
	}
	virtual void DealParam(typename CEdoyunQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case CEdoyunQueue<T>::EQPush:
			CEdoyunQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case CEdoyunQueue<T>::EQPop:
			if (CEdoyunQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CEdoyunQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0)
					CEdoyunQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CEdoyunQueue<T>::EQSize:
			pParam->nOperator = CEdoyunQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case CEdoyunQueue<T>::EQClear:
			CEdoyunQueue<T>::m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	EDYCALLBACK m_callback;
	EdoyunThread m_thread;
};

typedef EdoyunSendQueue<std::vector<char>>::EDYCALLBACK  SENDCALLBACK;