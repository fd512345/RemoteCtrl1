#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "resource.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "EdoyunTool.h"

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
	//更新网络服务器地址
	void UpdateAddress(int nIP, int nPort) {  // 更新地址的函数，参数为IP和端口
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);  // 获取CClientSocket单例对象并调用其UpdateAddress方法
	}

	int DealCommand() {  // 处理命令的函数
		return CClientSocket::getInstance()->DealCommand();  // 获取CClientSocket单例对象并调用其DealCommand方法，返回结果
	}

	void CloseSocket() {  // 关闭套接字的函数
		CClientSocket::getInstance()->CloseSocket();  // 获取CClientSocket单例对象并调用其CloseSocket方法
	}

	bool SendPacket(const CPacket& pack) {  // 发送数据包的函数，参数为CPacket常量引用
		CClientSocket* pClient = CClientSocket::getInstance();  // 获取CClientSocket单例对象指针pClient
		if (pClient->InitSocket() == false) return false;  // 调用pClient的InitSocket方法，若失败则返回false
		pClient->Send(pack);  // 调用pClient的Send方法发送pack
	}

	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0) {
		CClientSocket* pClient = CClientSocket::getInstance();  // 获取CClientSocket类的单例对象指针pClient
		if (pClient->InitSocket() == false) return false;  // 调用pClient的InitSocket方法，若失败则返回false
		pClient->Send(CPacket(nCmd, pData, nLength));  // 调用pClient的Send方法，发送构造的CPacket对象
		int cmd = DealCommand();  // 调用DealCommand方法处理命令，获取返回值cmd
		TRACE("ack:%d\r\n", cmd);  // 输出调试信息，显示ack值为cmd
		if (bAutoClose)  // 如果bAutoClose为真
			CloseSocket();  // 调用CloseSocket方法关闭套接字
		return cmd;  // 返回cmd
	}

	int GetImage(CImage& image) {  // 获取图像的函数，参数为CImage引用
		CClientSocket* pClient = CClientSocket::getInstance();  // 获取CClientSocket类的单例对象指针pClient
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);  // 调用CEdoYunTool的Bytes2Image方法，将pClient获取的数据包中的strData转换为图像image并返回结果
	}

	int DownFile(CString strPath) {
		CFileDialog dlg(
			FALSE, NULL,
			strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK) {  // 若文件对话框确认（用户选择了本地保存路径等）
			m_strRemote = strPath;  // 记录远程文件路径
			m_strLocal = dlg.GetPathName();  // 获取用户选择的本地保存路径
			// 创建下载线程，传入线程入口函数和this指针
			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {  // 检查线程创建后状态，若不是超时（表示线程可能已结束等异常）
				return -1;  // 返回错误标识
			}
			m_remoteDlg.BeginWaitCursor();  // 开始显示等待光标
			m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));  // 设置状态对话框文本为“命令正在执行中！”
			m_statusDlg.ShowWindow(SW_SHOW);  // 显示状态对话框
			m_statusDlg.CenterWindow(&m_remoteDlg);  // 将状态对话框在m_remoteDlg中心显示
			m_statusDlg.SetActiveWindow();  // 将状态对话框设为活动窗口
	
		}
		return 0;  // 返回成功标识
	}

	void StartWatchScreen();  // 声明StartWatchScreen函数，用于启动屏幕监视
protected:	
	void threadWatchScreen();  // 声明普通函数threadWatchScreen，用于实现屏幕监视相关逻辑
	static void threadWatchScreen(void* arg);  // 声明静态函数threadWatchScreen（作为线程入口等场景使用），参数为void*类型的arg 
	void threadDownloadFile();  // 声明线程函数threadDownloadFile，用于执行文件下载逻辑
	static void threadDownloadEntry(void* arg);  // 声明静态线程入口函数threadDownloadEntry，符合__stdcall调用约定，参数为void*类型的arg
	CClientController() :  // CClientController类的构造函数，使用初始化列表
		m_statusDlg(&m_remoteDlg),  // 初始化m_statusDlg，传入m_remoteDlg的地址
		m_watchDlg(&m_remoteDlg)  // 初始化m_watchDlg，传入m_remoteDlg的地址
	{
		m_hThreadWatch = INVALID_HANDLE_VALUE;  // 将m_hThreadWatch设为无效句柄值
		m_hThreadDownload = INVALID_HANDLE_VALUE;  // 将m_hThreadDownload设为无效句柄值
		m_hThread = INVALID_HANDLE_VALUE;  // 将m_hThread设为无效句柄值
		m_nThreadID = -1;  // 将m_nThreadID设为-1
		m_isClosed = true;
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
	typedef struct MsgInfo {  // 定义结构体类型MsgInfo
		MSG msg;  // 定义MSG类型的成员msg
		LRESULT result;  // 定义LRESULT类型的成员result
		MsgInfo(MSG m) {  // 带MSG参数的构造函数
			result = 0;  // 将result初始化为0
			memcpy(&msg, &m, sizeof(MSG));  // 把m的内容复制到msg
		}
		MsgInfo(const MsgInfo& m) {  // 拷贝构造函数
			result = m.result;  // 复制result的值
			memcpy(&msg, &m.msg, sizeof(MSG));  // 复制msg的内容
		}
		MsgInfo& operator=(const MsgInfo& m) {  // 重载赋值运算符
			if (this != &m) {  // 防止自赋值
				result = m.result;  // 复制result的值
				memcpy(&msg, &m.msg, sizeof(MSG));  // 复制msg的内容
			}
			return *this;  // 返回当前对象的引用，支持链式赋值
		}
	} MSGINFO;  // 定义结构体别名MSGINFO

	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 定义指向CClientController类成员函数的指针类型MSGFUNC，该成员函数接收UINT、WPARAM、LPARAM类型参数，返回LRESULT
	static std::map<UINT, MSGFUNC> m_mapFunc;  // 定义静态的std::map，键为UINT类型，值为MSGFUNC类型，用于存储消息与对应处理函数的映射关系
	CWatchDialog m_watchDlg;  // 定义 CWatchDialog 类型的变量 m_watchDlg
	CRemoteClientDlg m_remoteDlg;  // 定义 CRemoteClientDlg 类型的变量 m_remoteDlg
	CStatusDlg m_statusDlg;  // 定义 CStatusDlg 类型的变量 m_statusDlg
	HANDLE m_hThread;  // 定义 HANDLE 类型的变量 m_hThread，用于线程句柄
	HANDLE m_hThreadDownload;  // 定义 HANDLE 类型的变量 m_hThreadDownload，用于下载线程句柄
	HANDLE m_hThreadWatch;  // 定义 HANDLE 类型的变量 m_hThreadWatch，用于线程句柄
	bool m_isClosed;  // 定义 bool 类型的变量 m_isClosed，用于标识是否关闭

	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
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

