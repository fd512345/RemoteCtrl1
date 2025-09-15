#include "pch.h"
#include "ClientSocket.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC>  // 定义一个std::map类型，键为UINT，值为CClientController类的MSGFUNC类型
CClientController::m_mapFunc;  // CClientController类的静态成员变量m_mapFunc，用于存储消息与对应处理函数的映射
CClientController* CClientController::m_instance = NULL;  // CClientController类的静态成员变量m_instance，初始化为NULL，用于实现单例模式
CClientController::CHelper CClientController::m_helper;  // 定义CClientController类的静态成员变量m_helper，其类型为CClientController类中的CHelper类

CClientController* CClientController::getInstance() {
	if (m_instance == NULL) {  // 检查单例实例是否为空
		m_instance = new CClientController();  // 若为空，创建CClientController实例
		TRACE("CClientController size is %d\r\n", sizeof(*m_instance));  // 输出CClientController实例（通过m_instance解引用得到）的大小，格式为“CClientController size is [大小值]\r\n”
		// 定义结构体数组，存储消息ID与对应的消息处理成员函数指针
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			//{WM_SEND_PACK, &CClientController::OnSendPack},
			//{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus},
			{WM_SHOW_WATCH, &CClientController::OnShowWatcher},
			{(UINT)-1, NULL}  // 数组结束标记，func为NULL
		};
		// 遍历结构体数组，将消息与处理函数的映射插入到m_mapFunc中
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;  // 返回nullptr（此处存在问题，单例模式应返回创建的m_instance）
}

int CClientController::Invoke(CWnd*& m_pMainWnd)
{
	m_pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{  // 发送消息的成员函数
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // 创建事件对象hEvent
	if (hEvent == NULL) return -2;  // 如果创建失败，返回-2
	MSGINFO info(msg);  // 用msg构造MSGINFO对象info
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);  // 向指定线程发送消息，传递info和hEvent
	WaitForSingleObject(hEvent, INFINITE);  // 等待事件对象hEvent，直到有信号
	CloseHandle(hEvent);
	return info.result;  // 返回info的result值
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(  // 调用_beginthreadex创建线程，返回线程句柄并赋值给m_hThread
		NULL, 0,  // 安全属性和栈大小，使用默认值
		&CClientController::threadEntry,  // 线程入口函数地址，指向CClientController类的threadEntry成员函数
		this, 0, &m_nThreadID); // CreateThread  // 调用CreateThread函数，传入this指针（线程函数的参数）、0（栈大小）、&m_nThreadID（用于存储线程ID）
	m_statusDlg.Create(IDD_DLG_STATUS,  // 调用m_statusDlg的Create方法，创建对话框，对话框资源ID为IDD_DLG_STATUS
		&m_remoteDlg);  // 传入m_remoteDlg的地址作为父窗口相关参数	
	return 0;
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;  // 将void*类型的arg强制转换为CClientController*类型并赋值给thiz
	thiz->threadFunc();  // 调用thiz指向的CClientController对象的threadFunc方法
	_endthreadex(0);  // 结束当前线程，参数0表示线程退出码
	return 0;  // 这里的return 0实际可能因_endthreadex的调用而不会执行到，主要是为了符合函数返回值要求等情况
}


LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	TRACE("cmd: %d %s start %lld \r\n", nCmd, __FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取CClientSocket类的单例对象指针pClient
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);  // 调用pClient对象的SendPacket方法，发送一个CPacket数据包，参数包括窗口句柄hWnd、构造的CPacket对象（包含命令nCmd、数据指针pData、数据长度nLength）以及自动关闭标志bAutoClose	TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
	return ret;  // 返回发送数据包的结果ret
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);  // 隐藏状态对话框m_statusDlg
	m_remoteDlg.EndWaitCursor();  // 结束m_remoteDlg的等待光标显示，恢复正常光标
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));  // 在m_remoteDlg上弹出消息框，显示“下载完成！！”，标题为“完成”
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(
		FALSE, NULL,
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {  // 若文件对话框确认（用户选择了本地保存路径等）
		m_strRemote = strPath;  // 记录远程文件路径
		m_strLocal = dlg.GetPathName();  // 获取用户选择的本地保存路径
		FILE* pFile = fopen(m_strLocal, "wb+");  // 以二进制读写方式打开本地文件（路径为m_strLocal）
		if (pFile == NULL) {  // 如果文件打开失败
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));  // 弹出提示消息框
			return -1;  // 直接返回，终止函数执行
		}

		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);  // 调用SendCommandPacket函数发送命令数据包，参数依次为：目标对话框m_remoteDlg、命令标识4、是否为某种特殊状态（此处为false）、转换为BYTE*类型的m_strRemote字符串数据、m_strRemote的长度、转换为WPARAM类型的文件指针pFile
		// 创建下载线程，传入线程入口函数和this指针
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {  // 检查线程创建后状态，若不是超时（表示线程可能已结束等异常）
		//	return -1;  // 返回错误标识
		//}
		m_remoteDlg.BeginWaitCursor();  // 开始显示等待光标
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));  // 设置状态对话框文本为“命令正在执行中！”
		m_statusDlg.ShowWindow(SW_SHOW);  // 显示状态对话框
		m_statusDlg.CenterWindow(&m_remoteDlg);  // 将状态对话框在m_remoteDlg中心显示
		m_statusDlg.SetActiveWindow();  // 将状态对话框设为活动窗口

	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;  // 设置标记m_isClosed为false，表示未关闭
	//m_watchDlg.SetParent(&m_remoteDlg); // 将 m_watchDlg 的父窗口设置为 m_remoteDlg
	// 创建屏幕监视线程，线程入口函数为CClientController的threadWatchScreen，栈大小为0，传入this指针作为参数
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal();  // 以模态方式显示对话框dlg
	m_isClosed = true;  // 对话框关闭后，设置m_isClosed为true
	// 等待屏幕监视线程结束，超时时间为500毫秒
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed)
	{
		if (m_watchDlg.isFull() == false) {  // 如果远程对话框未处于“满”的状态
			std::list<CPacket> lstPacks; // 定义存储 CPacket 类型对象的列表 lstPacks
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);  // 调用SendCommandPacket函数，向m_watchDlg对应的窗口发送命令数据包，参数分别为窗口句柄、命令标识6、自动相关标志true、数据指针NULL、数据长度0，返回值存入ret
			//TODO:添加消息响应函数WM_SEND_PACK_ACK  // 待办：添加对WM_SEND_PACK_ACK消息的响应函数
			//TODO:控制发送频率  // 待办：实现发送频率的控制逻辑
			if (ret == 6) { // 如果返回值 ret 为 6

				if (CEdoyunTool::Bytes2Image(m_watchDlg.GetImage(), lstPacks.front().strData) == 0) { // 调用 Bytes2Image 函数将数据转为图像，若成功（返回 0）
					m_watchDlg.SetImageStatus(true); // 设置图像状态为 true
					TRACE("成功设置图片 %08X\r\n", (HBITMAP)m_watchDlg.GetImage());
					TRACE("和校验：%04X\r\n", lstPacks.front().sSum);
					// 打印调试信息，输出“和校验：”以及 lstPacks 列表中第一个元素的 sSum 成员（以4位十六进制形式显示，不足补0）
				}
				else { // 若 Bytes2Image 函数执行失败
					TRACE("获取图片失败！ret = %d\r\n", ret); // 打印获取图片失败的调试信息
				}
			}
		}
		Sleep(1);
	}
	TRACE("thread end %d\r\n", m_isClosed);  // 输出调试信息，显示线程结束时m_isClosed变量的值，用于调试跟踪线程关闭状态相关情况
}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;  // 将传入的void*类型参数arg转换为CClientController*类型指针thiz
	thiz->threadWatchScreen();  // 调用thiz指向的CClientController对象的threadDownloadFile方法
	_endthread();  // 结束当前线程

}

void CClientController::threadDownloadFile() {
	FILE* pFile = fopen(m_strLocal, "wb+");  // 以二进制读写方式打开本地文件（路径为m_strLocal）
	if (pFile == NULL) {  // 如果文件打开失败
		AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));  // 弹出提示消息框
		m_statusDlg.ShowWindow(SW_HIDE);  // 隐藏状态对话框
		m_remoteDlg.EndWaitCursor();  // 结束等待光标显示
		return;  // 直接返回，终止函数执行
	}
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取CClientSocket类的单例对象指针pClient
	do {
		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);  // 调用SendCommandPacket方法发送命令（命令码为4）、指定不自动关闭（false）、将m_strRemote转换为BYTE*类型的远程路径数据、数据长度为m_strRemote的长度，获取返回值ret
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();  // 获取文件长度
		if (nLength == 0) {  // 文件长度为零
			AfxMessageBox("文件长度为零或者无法读取文件！！！");  // 显示错误消息
			break;  // 跳出循环
		}
		long long nCount = 0;
		while (nCount < nLength) {  // 循环接收文件数据
			ret = pClient->DealCommand();  // 处理命令响应
			if (ret < 0) {  // 传输失败
				AfxMessageBox("传输失败！！");  // 显示错误消息
				TRACE("传输失败：ret = %d\r\n", ret);  // 输出错误信息
				break;  // 跳出循环
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);  // 写入文件数据
			nCount += pClient->GetPacket().strData.size();  // 更新已接收数据长度
		}
	} while (false);  // do-while循环，由于条件为false，只执行一次循环体
	fclose(pFile);  // 关闭之前打开的文件指针pFile
	pClient->CloseSocket();  // 调用pClient的CloseSocket方法关闭套接字
	m_statusDlg.ShowWindow(SW_HIDE);  // 隐藏状态对话框
	m_remoteDlg.EndWaitCursor();  // 结束m_remoteDlg的等待光标显示
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));  // 显示下载完成消息
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;  // 将传入的void*类型参数arg转换为CClientController*类型指针thiz
	thiz->threadDownloadFile();  // 调用thiz指向的CClientController对象的threadDownloadFile方法
	_endthread();  // 结束当前线程
}

void CClientController::threadFunc() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {  // 从消息队列获取消息，当获取到消息（返回非0）时继续循环，获取失败（返回0）或出错（返回-1）时退出
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {  // 判断消息是否为WM_SEND_MESSAGE
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;  // 将msg.wParam转换为MSGINFO*类型的pmsg
			HANDLE hEvent = (HANDLE)msg.lParam;  // 将msg.lParam转换为HANDLE类型的hEvent
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);  // 在m_mapFunc中查找msg.message对应的迭代器
			if (it != m_mapFunc.end()) {  // 如果找到对应的项
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);  // 调用对应的成员函数并将结果赋值给pmsg->result
			}
			else {  // 如果没找到
				pmsg->result = -1;  // 将pmsg->result设为-1
			}
			SetEvent(hEvent);  // 设置事件hEvent为有信号状态
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);  // 在m_mapFunc中查找当前消息对应的处理函数迭代器
			if (it != m_mapFunc.end()) {  // 如果找到对应的处理函数
				(this->*(it->second))(msg.message, msg.wParam, msg.lParam);  // 调用对应的成员函数处理消息
			}
		}
	}
}