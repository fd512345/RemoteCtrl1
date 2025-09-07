// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含框架头文件
#include "RemoteClient.h"  // 包含应用程序主头文件
#include "RemoteClientDlg.h"  // 包含当前对话框头文件
#include "afxdialogex.h"  // 包含扩展对话框头文件


#ifdef _DEBUG
#define new DEBUG_NEW  // 调试模式下使用DEBUG_NEW宏
#endif
#include "CWatchDialog.h"  // 包含监控对话框头文件


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx  // 关于对话框类定义
{
public:
	CAboutDlg();  // 构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };  // 设计时对话框ID
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()  // 声明消息映射
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)  // 关于对话框构造函数实现
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)  // 数据交换函数实现
{
	CDialogEx::DoDataExchange(pDX);  // 调用基类数据交换函数
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)  // 关于对话框消息映射开始
END_MESSAGE_MAP()  // 关于对话框消息映射结束


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)  // 主对话框构造函数
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)  // 初始化基类
	, m_server_address(0)  // 初始化服务器地址
	, m_nPort(_T(""))  // 初始化端口号
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);  // 加载应用程序图标
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)  // 数据交换函数
{
	CDialogEx::DoDataExchange(pDX);  // 调用基类数据交换函数
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);  // 绑定IP地址控件
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);  // 绑定端口编辑框
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);  // 绑定目录树控件
	DDX_Control(pDX, IDC_LIST_FILE, m_List);  // 绑定文件列表控件
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)  // 发送命令数据包函数
{
	UpdateData();  // 从控件更新数据到变量
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));  // 初始化套接字
	if (!ret) {  // 初始化失败
		AfxMessageBox("网络初始化失败!");  // 显示错误消息
		return -1;  // 返回错误码
	}
	CPacket pack(nCmd, pData, nLength);  // 创建数据包
	ret = pClient->Send(pack);  // 发送数据包
	TRACE("Send ret %d\r\n", ret);  // 输出发送结果
	int cmd = pClient->DealCommand();  // 处理命令响应
	TRACE("ack:%d\r\n", cmd);  // 输出响应命令
	if (bAutoClose)  // 如果需要自动关闭
		pClient->CloseSocket();  // 关闭套接字
	return cmd;  // 返回响应命令
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)  // 主对话框消息映射开始
	ON_WM_SYSCOMMAND()  // 系统命令消息
	ON_WM_PAINT()  // 绘制消息
	ON_WM_QUERYDRAGICON()  // 拖动图标查询消息
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)  // 测试按钮点击事件
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)  // 文件信息按钮点击事件
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)  // 目录树双击事件
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)  // 目录树单击事件
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)  // 文件列表右键点击事件
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)  // 下载文件命令
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)  // 删除文件命令
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)  // 运行文件命令
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket) // 注册自定义消息处理函数
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)  // 开始监控按钮点击事件
	ON_WM_TIMER()  // 定时器消息
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)  // 端口编辑框内容改变事件
END_MESSAGE_MAP()  // 主对话框消息映射结束


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()  // 对话框初始化函数
{
	CDialogEx::OnInitDialog();  // 调用基类初始化函数

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);  // 获取系统菜单
	if (pSysMenu != nullptr)  // 如果系统菜单存在
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);  // 加载关于菜单字符串
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())  // 如果字符串不为空
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);  // 添加分隔线
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);  // 添加关于菜单项
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();  // 从控件更新数据到变量
	m_server_address = 0x7F000001;//127.0.0.1  // 设置默认服务器地址为本地回环地址
	m_nPort = _T("9527");  // 设置默认端口号
	UpdateData(FALSE);  // 将变量数据更新到控件
	m_dlgStatus.Create(IDD_DLG_STATUS, this);  // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE);  // 隐藏状态对话框
	m_isFull = false;  // 初始化图像缓存状态
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)  // 系统命令处理函数
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)  // 如果是关于命令
	{
		CAboutDlg dlgAbout;  // 创建关于对话框
		dlgAbout.DoModal();  // 显示模态关于对话框
	}
	else  // 其他系统命令
	{
		CDialogEx::OnSysCommand(nID, lParam);  // 调用基类处理函数
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()  // 绘制函数
{
	if (IsIconic())  // 如果窗口处于最小化状态
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);  // 发送擦除背景消息

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);  // 获取图标宽度
		int cyIcon = GetSystemMetrics(SM_CYICON);  // 获取图标高度
		CRect rect;
		GetClientRect(&rect);  // 获取客户区矩形
		int x = (rect.Width() - cxIcon + 1) / 2;  // 计算图标X坐标
		int y = (rect.Height() - cyIcon + 1) / 2;  // 计算图标Y坐标

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);  // 绘制图标
	}
	else  // 非最小化状态
	{
		CDialogEx::OnPaint();  // 调用基类绘制函数
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()  // 查询拖动图标函数
{
	return static_cast<HCURSOR>(m_hIcon);  // 返回应用程序图标作为拖动图标
}



void CRemoteClientDlg::OnBnClickedBtnTest()  // 测试按钮点击事件处理函数
{
	SendCommandPacket(1981);  // 发送测试命令
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()  // 文件信息按钮点击事件处理函数
{
	int ret = SendCommandPacket(1);  // 发送获取文件信息命令
	if (ret == -1) {  // 命令处理失败
		AfxMessageBox(_T("命令处理失败!!!"));  // 显示错误消息
		return;  // 返回
	}
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
	std::string drivers = pClient->GetPacket().strData;  // 获取驱动信息
	std::string dr;
	m_Tree.DeleteAllItems();  // 清空目录树
	for (size_t i = 0; i < drivers.size(); i++)  // 遍历驱动信息
	{
		if (drivers[i] == ',') {  // 遇到分隔符
			dr += ":";  // 添加冒号
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);  // 插入驱动节点
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);  // 插入子节点占位
			dr.clear();  // 清空临时字符串
			continue;  // 继续下一次循环
		}
		dr += drivers[i];  // 拼接驱动字母
	}
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)  // 监控数据线程入口函数
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;  // 转换为当前对话框指针
	thiz->threadWatchData();  // 调用监控数据函数
	_endthread();  // 结束线程
}

void CRemoteClientDlg::threadWatchData()  // 监控数据线程函数
{//可能存在异步问题，导致程序崩溃
	Sleep(50);  // 休眠50毫秒
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
	} while (pClient == NULL);  // 等待实例初始化完成
	while (!m_isClosed) {//等价于while(true)  // 循环直到关闭标志为真
		if (m_isFull == false) {//更新数据到缓存  // 如果缓存未满
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);  // 发送获取屏幕数据命令
			if (ret == 6) {  // 成功获取数据
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();  // 获取数据包数据
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 分配全局内存
				if (hMem == NULL) {  // 内存分配失败
					TRACE("内存不足了！");  // 输出错误信息
					Sleep(1);  // 休眠1毫秒
					continue;  // 继续下一次循环
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 创建流对象
				if (hRet == S_OK) {  // 流对象创建成功
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);  // 写入数据到流
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 定位到流开始位置
					if ((HBITMAP)m_image != NULL)  // 如果图像已存在
						m_image.Destroy();  // 销毁图像
					m_image.Load(pStream);  // 从流加载图像
					m_isFull = true;  // 设置缓存满标志
				}
			}
			else {  // 获取数据失败
				Sleep(1);  // 休眠1毫秒
			}
		}
		else Sleep(1);  // 缓存已满，休眠1毫秒
	}
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)  // 下载文件线程入口函数
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;  // 转换为当前对话框指针
	thiz->threadDownFile();  // 调用下载文件函数
	_endthread();  // 结束线程
}

void CRemoteClientDlg::threadDownFile()  // 下载文件线程函数
{
	int nListSelected = m_List.GetSelectionMark();  // 获取文件列表选中项
	CString strFile = m_List.GetItemText(nListSelected, 0);  // 获取选中文件名
	CFileDialog dlg(FALSE, NULL,  // 创建文件保存对话框
		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, this);
	if (dlg.DoModal() == IDOK) {  // 如果用户确认保存
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");  // 打开本地文件
		if (pFile == NULL) {  // 文件打开失败
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));  // 显示错误消息
			m_dlgStatus.ShowWindow(SW_HIDE);  // 隐藏状态对话框
			EndWaitCursor();  // 结束等待光标
			return;  // 返回
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();  // 获取目录树选中项
		strFile = GetPath(hSelected) + strFile;  // 拼接完整文件路径
		TRACE("%s\r\n", LPCSTR(strFile));  // 输出文件路径
		CClientSocket* pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);  // 发送下载命令
			if (ret < 0) {  // 命令执行失败
				AfxMessageBox("执行下载命令失败！！");  // 显示错误消息
				TRACE("执行下载失败：ret = %d\r\n", ret);  // 输出错误信息
				break;  // 跳出循环
			}
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
		} while (false);
		fclose(pFile);  // 关闭文件
		pClient->CloseSocket();  // 关闭套接字
	}
	m_dlgStatus.ShowWindow(SW_HIDE);  // 隐藏状态对话框
	EndWaitCursor();  // 结束等待光标
	MessageBox(_T("下载完成！！"), _T("完成"));  // 显示下载完成消息
}

void CRemoteClientDlg::LoadFileCurrent()  // 加载当前目录文件
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hTree);  // 获取选中项路径
	m_List.DeleteAllItems();  // 清空文件列表
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());  // 发送获取目录信息命令
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取文件信息
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
	while (pInfo->HasNext) {  // 循环处理所有文件信息
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);  // 输出文件信息
		if (!pInfo->IsDirectory) {  // 如果是文件
			m_List.InsertItem(0, pInfo->szFileName);  // 添加到文件列表
		}
		int cmd = pClient->DealCommand();  // 处理命令响应
		TRACE("ack:%d\r\n", cmd);  // 输出响应命令
		if (cmd < 0)break;  // 响应错误则跳出循环
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取下一个文件信息
	}
	pClient->CloseSocket();  // 关闭套接字
}

void CRemoteClientDlg::LoadFileInfo()  // 加载文件信息
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);  // 获取鼠标位置
	m_Tree.ScreenToClient(&ptMouse);  // 转换为客户区坐标
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);  // 获取鼠标指向的目录树项
	if (hTreeSelected == NULL)  // 如果没有指向任何项
		return;  // 返回
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)  // 如果没有子项
		return;  // 返回
	DeleteTreeChildrenItem(hTreeSelected);  // 删除子项
	m_List.DeleteAllItems();  // 清空文件列表
	CString strPath = GetPath(hTreeSelected);  // 获取选中项路径
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());  // 发送获取目录信息命令
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取文件信息
	CClientSocket* pClient = CClientSocket::getInstance();  // 获取客户端套接字实例
	int Count = 0;
	while (pInfo->HasNext) {  // 循环处理所有文件信息
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);  // 输出文件信息
		if (pInfo->IsDirectory) {  // 如果是目录
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))  // 跳过当前目录和父目录
			{
				int cmd = pClient->DealCommand();  // 处理命令响应
				TRACE("ack:%d\r\n", cmd);  // 输出响应命令
				if (cmd < 0)break;  // 响应错误则跳出循环
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取下一个文件信息
				continue;  // 继续下一次循环
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);  // 插入目录节点
			m_Tree.InsertItem("", hTemp, TVI_LAST);  // 插入子节点占位
		}
		else {  // 如果是文件
			m_List.InsertItem(0, pInfo->szFileName);  // 添加到文件列表
		}
		Count++;  // 计数加一
		int cmd = pClient->DealCommand();  // 处理命令响应
		//TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;  // 响应错误则跳出循环
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取下一个文件信息
	}
	pClient->CloseSocket();  // 关闭套接字
	TRACE("Count = %d\r\n", Count);  // 输出文件数量
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)  // 获取目录树项的路径
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);  // 获取当前项文本
		strRet = strTmp + '\\' + strRet;  // 拼接路径
		hTree = m_Tree.GetParentItem(hTree);  // 获取父项
	} while (hTree != NULL);  // 循环直到根节点
	return strRet;  // 返回完整路径
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)  // 删除目录树子项
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);  // 获取子项
		if (hSub != NULL)m_Tree.DeleteItem(hSub);  // 如果子项存在则删除
	} while (hSub != NULL);  // 循环删除所有子项
}



void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)  // 目录树双击事件处理函数
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;  // 设置结果
	LoadFileInfo();  // 加载文件信息
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)  // 目录树单击事件处理函数
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;  // 设置结果
	LoadFileInfo();  // 加载文件信息
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)  // 文件列表右键点击事件处理函数
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);  // 转换为项激活结构
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;  // 设置结果
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);  // 获取鼠标位置
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);  // 转换为客户区坐标
	int ListSelected = m_List.HitTest(ptList);  // 获取鼠标指向的列表项
	if (ListSelected < 0)return;  // 如果没有指向任何项则返回
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);  // 加载右键菜单
	CMenu* pPupup = menu.GetSubMenu(0);  // 获取弹出菜单
	if (pPupup != NULL) {  // 如果弹出菜单存在
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);  // 显示弹出菜单
	}
}


void CRemoteClientDlg::OnDownloadFile()  // 下载文件命令处理函数
{
	///////添加线程函数
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);  // 启动下载文件线程
	BeginWaitCursor();  // 开始等待光标
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));  // 设置状态文本
	m_dlgStatus.ShowWindow(SW_SHOW);  // 显示状态对话框
	m_dlgStatus.CenterWindow(this);  // 居中显示状态对话框
	m_dlgStatus.SetActiveWindow();  // 激活状态对话框
}


void CRemoteClientDlg::OnDeleteFile()  // 删除文件命令处理函数
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hSelected);  // 获取选中项路径
	int nSelected = m_List.GetSelectionMark();  // 获取文件列表选中项
	CString strFile = m_List.GetItemText(nSelected, 0);  // 获取选中文件名
	strFile = strPath + strFile;  // 拼接完整文件路径
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送删除文件命令
	if (ret < 0) {  // 命令执行失败
		AfxMessageBox("删除文件命令执行失败！！！");  // 显示错误消息
	}
	LoadFileCurrent();  // 重新加载当前目录文件
}


void CRemoteClientDlg::OnRunFile()  // 运行文件命令处理函数
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hSelected);  // 获取选中项路径
	int nSelected = m_List.GetSelectionMark();  // 获取文件列表选中项
	CString strFile = m_List.GetItemText(nSelected, 0);  // 获取选中文件名
	strFile = strPath + strFile;  // 拼接完整文件路径
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送运行文件命令
	if (ret < 0) {  // 命令执行失败
		AfxMessageBox("打开文件命令执行失败！！！");  // 显示错误消息
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)  // 自定义消息处理函数
{//实现消息响应函数④
	int ret = 0;
	int cmd = wParam >> 1;  // 获取命令
	switch (cmd) {
	case 4: {  // 下载文件命令
		CString strFile = (LPCSTR)lParam;  // 获取文件路径
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送命令
	}
		  break;
	case 5: {//鼠标操作命令
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));  // 发送命令
	}
		  break;
	case 6:  // 获取屏幕数据命令
	case 7:  // 锁定机器命令
	case 8: {  // 解锁机器命令
		ret = SendCommandPacket(cmd, wParam & 1);  // 发送命令
	}
		  break;
	default:  // 其他命令
		ret = -1;  // 返回错误码
	}

	return ret;  // 返回结果
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()  // 开始监控按钮点击事件处理函数
{
	m_isClosed = false;  // 重置关闭标志
	CWatchDialog dlg(this);  // 创建监控对话框
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);  // 启动监控线程
	dlg.DoModal();  // 显示模态监控对话框
	m_isClosed = true;  // 设置关闭标志
	WaitForSingleObject(hThread, 500);  // 等待线程结束
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)  // 定时器事件处理函数
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);  // 调用基类定时器处理函数
}

void CRemoteClientDlg::OnEnChangeEditPort()  // 端口编辑框内容改变事件处理函数
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}