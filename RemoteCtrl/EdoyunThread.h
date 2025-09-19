#pragma once
#include "pch.h"
#include <Windows.h>
#include <atomic> 
#include <vector>
#include <mutex>

class ThreadFuncBase {}; // 定义基础线程函数类，作为基类
typedef int (ThreadFuncBase::* FUNCTYPE)(); // 定义成员函数指针类型 FUNCTYPE，指向 ThreadFuncBase 类中返回 int 且无参数的成员函数

class ThreadWorker { // 线程工作类，用于封装线程执行的成员函数及对象
public:
	ThreadWorker() : thiz(NULL), func(NULL) {} // 默认构造函数，初始化对象指针和函数指针为 NULL

	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) : thiz(obj), func(f) {} // 带参构造函数，初始化对象指针和函数指针


	ThreadWorker(const ThreadWorker& worker)
	{
		thiz = worker.thiz;  // 将 worker 对象的 thiz 属性值赋给变量 thiz
		func = worker.func;  // 将 worker 对象的 func 属性值赋给变量 func
	}

	ThreadWorker& operator=(const ThreadWorker& worker)
	{
		if (this != &worker)
		{
			thiz = worker.thiz;  // 将 worker 对象的 thiz 属性值赋给变量 thiz
			func = worker.func;  // 将 worker 对象的 func 属性值赋给变量 func
		}
		return *this;
	}
	//	ThreadWorker& operator=(ThreadWorker&& worker) = delete;

	int operator()() { // 函数调用运算符重载，使对象可像函数一样调用
		if (IsValid()) { // 判断对象和函数指针是否有效
			return (thiz->*func)(); // 调用指定对象的成员函数
		}
		return -1; // 无效时返回 -1
	}
	bool IsValid() const { // 判断对象和函数指针是否有效
		return (thiz != NULL) && (func != NULL); // 对象指针和函数指针都不为 NULL 时有效
	}
private:
	ThreadFuncBase* thiz; // 指向 ThreadFuncBase 派生类对象的指针
	FUNCTYPE func; // 成员函数指针
};





class EdoyunThread
{
public:
	EdoyunThread()
	{ // EdoyunThread 类的构造函数
		m_hThread = NULL; // 初始化线程句柄为 NULL
		m_bStatus = false;
	}

	~EdoyunThread()
	{
		Stop();
	}

	bool Start()// true 表示成功 false 表示失败
	{ // 启动线程的方法
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this); // 调用 _beginthread 创建线程，线程入口函数为 ThreadEntry，传入当前对象指针 this
		if (!IsValid()) { // 调用 IsValid 方法判断（通常是判断线程等是否有效），若无效
			m_bStatus = false; // 将状态标记 m_bStatus 设为 false
		}
		return m_bStatus;
	}

	bool IsValid()
	{ // 判断线程是否有效 true有效 false表示线程异常or已经终止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false; // 若线程句柄为 NULL 或无效句柄值，返回 false
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT; // 等待线程，超时时间为 0，若返回 WAIT_TIMEOUT 表示线程正在运行，返回 true
	}

	bool Stop()
	{
		if (m_bStatus == false) return true; // 如果状态 m_bStatus 为 false，直接返回 true
		m_bStatus = false; // 将状态 m_bStatus 设为 false

		DWORD ret = WaitForSingleObject(m_hThread, 1000) == WAIT_OBJECT_0; // 等待线程 m_hThread 结束，若等待成功（返回 WAIT_OBJECT_0）则返回 true，否则返回 false
		if (ret == WAIT_TIMEOUT) {  // 判断等待线程返回结果是否为超时
			TerminateThread(m_hThread, -1);  // 若超时，强制终止线程m_hThread，退出码为-1
		}
		UpdateWorker();
		return WAIT_OBJECT_0;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if (m_worker.load() != NULL && (m_worker.load() != &worker)) {  // 检查 m_worker 中存储的指针是否非空
			::ThreadWorker* pWorker = m_worker.load();  // 加载 m_worker 中的指针到 pWorker
			m_worker.store(NULL);  // 将 m_worker 中存储的指针置为 NULL
			delete pWorker;  // 释放 pWorker 指向的 ThreadWorker 对象内存
		}
		if (m_worker.load() == &worker) return;  // 检查原子操作加载的m_worker是否等于worker的地址，若是则直接返回，避免重复操作
		if (!worker.IsValid()) {  // 检查 worker 是否有效
			m_worker.store(NULL);  // 将 m_worker 中存储的指针置为 NULL
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));  // 新建 ThreadWorker 对象并存储其指针到 m_worker
	}

	//true表示空闲 false表示已经分配了工作，注释：说明 IsIdle 方法返回 true 时线程空闲，返回 false 时已分配工作
	bool IsIdle() { // 定义 IsIdle 方法，判断线程是否空闲
		if (m_worker.load() == NULL)
		{
			return true;
		}
		return !m_worker.load()->IsValid(); // 加载 m_worker 并判断其是否有效，取反后返回，即若 m_worker 无效则返回 true（表示空闲），有效则返回 false（表示已分配工作）
	}

private:
	void ThreadWorker() { // 虚函数，线程工作逻辑，子类可重写
		while (m_bStatus) { // 当状态 m_bStatus 为真时循环
			if (m_worker.load() == NULL) {  // 检查工作线程对象指针是否为空
				Sleep(1);                    // 若为空，线程休眠1毫秒
				continue;                    // 休眠后继续循环，再次检查工作线程对象指针状态
			}
			::ThreadWorker worker = *m_worker.load(); // 加载 ThreadWorker 对象到 worker
			if (worker.IsValid()) { // 判断 worker 是否有效
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {  // 检查线程m_hThread是否处于超时状态（即线程仍在运行，没有立即结束）
					int ret = worker(); // 调用 worker 的函数调用运算符，执行相关操作并获取返回值
					if (ret != 0) { // 如果返回值不等于 0
						CString str; // 定义 CString 类型变量 str
						str.Format(_T("thread found warning code %d\r\n"), ret); // 格式化字符串，包含警告代码 ret
						OutputDebugString(str); // 输出调试字符串
					}
					if (ret < 0) { // 如果返回值小于 0
						m_worker.store(NULL);
					}
				}
			}
			else { // 如果 worker 无效
				Sleep(1); // 线程休眠 1 毫秒
			}
		}
	}

	static void ThreadEntry(void* arg) { // 静态线程入口函数
		EdoyunThread* thiz = (EdoyunThread*)arg; // 将传入的参数转换为 EdoyunThread 指针
		if (thiz) { // 若指针有效
			thiz->ThreadWorker(); // 调用线程工作逻辑方法
		}
		_endthread(); // 结束线程
	}

private:
	HANDLE m_hThread; // 线程句柄，用于标识线程
	bool m_bStatus;//false表示线程将要关闭 true表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class EdoyunThreadPool { // 定义线程池类 EdoyunThreadPool
public:
	EdoyunThreadPool(size_t size) { // 带参构造函数，参数为线程池大小
		m_threads.resize(size);  // 调整 m_threads 容器大小为 size
		for (size_t i = 0; i < size; i++)  // 循环创建多个 EdoyunThread 对象
			m_threads[i] = new EdoyunThread();  // 为 m_threads 每个元素分配新的 EdoyunThread 对象
	}
	EdoyunThreadPool() {} // 默认构造函数
	~EdoyunThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++) {  // 遍历线程指针数组m_threads
			delete m_threads[i];                         // 释放数组中第i个线程指针指向的动态分配的线程对象内存
			m_threads[i] = NULL;                         // 将该位置的指针置为NULL，防止野指针
		}
		m_threads.clear();
	} // 析构函数
	bool Invoke() { // 调用函数，用于启动线程池中的线程
		bool ret = true; // 初始化返回值为 true
		for (size_t i = 0; i < m_threads.size(); i++) { // 遍历线程容器
			if (m_threads[i]->Start() == false) { // 如果当前线程启动失败
				ret = false; // 将返回值设为 false
				break; // 跳出循环
			}
		}
		if (ret == false) { // 如果启动有失败的情况
			for (size_t i = 0; i < m_threads.size(); i++) // 遍历线程容器
				m_threads[i]->Stop(); // 停止当前线程（这里原代码可能存在问题，i 是循环变量，循环外使用可能越界或逻辑错误，需结合实际类逻辑修正，此处按代码原文注释）
		}
		return ret; // 返回最终的启动结果
	}
	void Stop()
	{
		for (size_t i = 0; i < m_threads.size(); i++) // 遍历线程容器
			m_threads[i]->Stop(); // 停止当前线程（这里原代码可能存在问题，i 是循环变量，循环外使用可能越界或逻辑错误，需结合实际类逻辑修正，此处按代码原文注释）

	}
	//返回-1表示分配失败,所有线程都在忙大于等于0,表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) { // 定义 DispatchWorker 函数，参数为 const 引用的 ThreadWorker 对象
		int index = -1; // 初始化索引为 -1，用于记录找到的空闲线程索引
		m_lock.lock(); // 加锁，保证线程安全，防止多线程同时操作共享数据
		for (size_t i = 0; i < m_threads.size(); i++) { // 遍历线程容器
			if (m_threads[i]->IsIdle()) { // 如果当前线程处于空闲状态
				m_threads[i]->UpdateWorker(worker); // 给该空闲线程分配工作
				index = i; // 记录该线程的索引
				break; // 找到一个空闲线程后就跳出循环
			}
		}
		m_lock.unlock(); // 解锁
		return index; // 返回找到的空闲线程索引，若没找到则返回初始的 -1	
	}

	bool CheckThreadValid(size_t index) { // 定义 CheckThreadValid 函数，参数为线程索引 index
		if (index < m_threads.size()) { // 如果索引在 m_threads 容器的有效范围内
			return m_threads[index]->IsValid(); // 返回该索引对应线程的 IsValid 方法结果（判断线程是否有效）
		}
		return false; // 索引无效时返回 false
	}
private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads; // 存储 EdoyunThread 类型线程的向量容器
};