#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>

#define WM_SEND_PACK (WM_USER+1)  // 发送包数据
#define WM_SEND_DATA (WM_USER+2)  // 发送数据
#define WM_SHOW_STATUS (WM_USER+3)  // 展示状态
#define WM_SHOW_WATCH (WM_USER+4)  // 远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)  // 自定义消息处理，定义WM_SEND_MESSAGE为WM_USER加上0x1000的消息标识

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitController();
	//启动
	int Invoke(CWnd*& m_pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);  // 声明SendMessage函数，用于发送消息，接收消息标识、wParam和lParam参数，返回LRESULT类型结果
protected:
	CClientController() {  // CClientController类的构造函数

	}
	~CClientController() {  // CClientController类的析构函数
		WaitForSingleObject(m_hThread, 100);  // 等待线程m_hThread结束，超时时间100毫秒
	}
	void threadFunc();  // 声明成员函数threadFunc
	static unsigned __stdcall threadEntry(void* arg);  // 声明静态的线程入口函数threadEntry
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 声明处理发送包消息的函数
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 声明处理发送数据消息的函数
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 声明处理展示状态消息的函数
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 声明处理展示监控消息的函数
private:
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 定义指向CClientController类成员函数的指针类型MSGFUNC，该成员函数接收UINT、WPARAM、LPARAM类型参数，返回LRESULT
	static std::map<UINT, MSGFUNC> m_mapFunc;  // 定义静态的std::map，键为UINT类型，值为MSGFUNC类型，用于存储消息与对应处理函数的映射关系
	std::map<UUID, MSG> m_mapMessage;  // 定义std::map，键为UUID类型，值为MSG类型，用于存储消息队列
	CWatchDialog m_watchDlg;  // 定义 CWatchDialog 类型的变量 m_watchDlg
	CRemoteClientDlg m_remoteDlg;  // 定义 CRemoteClientDlg 类型的变量 m_remoteDlg
	CStatusDlg m_statusDlg;  // 定义 CStatusDlg 类型的变量 m_statusDlg
	HANDLE m_hThread;  // 定义 HANDLE 类型的变量 m_hThread，用于线程句柄
	unsigned m_nThreadID;  // 定义 DWORD 类型的变量 m_nThreadID，用于线程 ID
	static CClientController* m_instance;
	class CHelper {                           // 辅助类，用于自动释放单例
	public:
		CHelper() {
			CClientController::getInstance();     // 构造时创建实例
		}
		~CHelper() {
			CClientController::releaseInstance(); // 析构时释放实例
		}
	};
	static CHelper m_helper;                  // 静态辅助对象，确保单例生命周期

};

