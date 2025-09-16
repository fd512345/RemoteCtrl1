#pragma once
template<class T>
class CEdoyunQueue
{//线程安全的队列（利用IOCP实现）
public:
	CEdoyunQueue yunQueue(); // 类的构造函数
	~CEdoyunQueue yunQueue(); // 类的析构函数
	bool PushBack(const T& data); // 向队列尾部添加数据的方法，返回是否成功
	bool PopFront(T& data); // 从队列头部弹出数据到 data，返回是否成功
	size_t Size(); // 获取队列大小的方法
	void Clear(); // 清空队列的方法
private:
	static void threadEntry(void* arg); // 静态线程入口函数
	void threadMain(); // 线程主函数
private:
	std::list<T> m_lstData; // 存储数据的列表
	HANDLE m_hCompeletionPort; // I/O 完成端口句柄
	HANDLE m_hThread; // 线程句柄
public:
	typedef struct IocpParam {
		int nOperator; // 操作标识，用于区分不同的操作类型
		T strData; // 存储相关的数据
		HANDLE hEvent;//pop操作需要的
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) { // IocpParam 结构体的构造函数，接收操作标识、数据以及可选的回调函数指针
			nOperator = op; // 将传入的操作标识赋值给结构体成员 nOperator
			strData = sData; // 将传入的数据赋值给结构体成员 strData
		}
		IocpParam()
		{
			nOperator = -1;
		}
	} PPARAM;//Post Parameter 用于投递信息的结构体
	enum {
		EQPush,
		EQPop, 
		EQSize,  
		EQClear
	};

};

