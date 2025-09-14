// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含框架头文件
#include "RemoteClient.h"  // 包含应用程序主头文件
#include "RemoteClientDlg.h"  // 包含当前对话框头文件
#include "afxdialogex.h"  // 包含扩展对话框头文件
#include "ClientController.h"


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
public:
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
	, m_server_address(0x7F000001)  // 初始化服务器地址
	, m_nPort(_T("9527"))  // 初始化端口号
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
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)  // 开始监控按钮点击事件
	ON_WM_TIMER()  // 定时器消息
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)  // 端口编辑框内容改变事件
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)

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
	UpdateData();  // 从对话框控件更新数据到成员变量
	CClientController* pController = CClientController::getInstance();  // 获取CClientController类的单例对象指针pController
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));  // 调用pController的UpdateAddress方法，更新服务器地址和端口（m_nPort先转换为LPCTSTR再转成int）
	UpdateData(FALSE);  // 将变量数据更新到控件
	m_dlgStatus.Create(IDD_DLG_STATUS, this);  // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE);  // 隐藏状态对话框
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
	CClientController::getInstance()->SendCommandPacket(1981);  // 获取CClientController类的单例对象，并调用其SendCommandPacket方法，传入命令值1981
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()  // 文件信息按钮点击事件处理函数
{
	std::list<CPacket> lstPackets;                      // 定义一个存储 CPacket 类型对象的链表
	int ret = CClientController::getInstance()->SendCommandPacket(1, true, NULL, 0, &lstPackets);  // 调用单例类 CClientController 的 SendCommandPacket 方法发送命令包，将结果存入 ret
	if (ret == -1 || (lstPackets.size() <= 0)) {        // 判断命令是否处理失败（返回值为 -1 或者链表中没有数据包）
		AfxMessageBox(_T("命令处理失败!!!"));           // 弹出提示命令处理失败的消息框
		return;                                         // 函数返回
	}
	CPacket& head = lstPackets.front();                 // 获取链表的第一个元素（头元素）的引用
	std::string drivers = head.strData;                 // 从头部数据包中获取字符串数据存入 drivers	
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
	// 循环结束后再插入最后一个盘符
	if (!dr.empty()) {
		dr += ":";
		HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
	}
}

void CRemoteClientDlg::LoadFileCurrent()  // 加载当前目录文件
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hTree);  // 获取选中项路径
	m_List.DeleteAllItems();  // 清空文件列表
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());  // 发送获取目录信息命令
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取文件信息
	while (pInfo->HasNext) {  // 循环处理所有文件信息
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);  // 输出文件信息
		if (!pInfo->IsDirectory) {  // 如果是文件
			m_List.InsertItem(0, pInfo->szFileName);  // 添加到文件列表
		}
		int cmd = CClientController::getInstance()->DealCommand();  // 处理命令响应
		TRACE("ack:%d\r\n", cmd);  // 输出响应命令
		if (cmd < 0)break;  // 响应错误则跳出循环
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 获取下一个文件信息
	}
	//CClientController::getInstance()->CloseSocket();  // 关闭套接字
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
	std::list<CPacket> lstPackets;                      // 定义存储CPacket对象的链表
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), &lstPackets);  // 调用单例类的SendCommandPacket方法发送命令包，结果存nCmd
	if (lstPackets.size() > 0) {                        // 判断链表中是否有数据包
		TRACE("lstPackets.size = %d\r\n", lstPackets.size());  // 输出调试信息，显示lstPackets链表的元素个数
		std::list<CPacket>::iterator it = lstPackets.begin();  // 获取链表起始迭代器
		for (; it != lstPackets.end(); it++) {           // 遍历链表
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();  // 将数据包字符串数据转为PFILEINFO指针
			if (pInfo->HasNext == false)
				continue;
			if (pInfo->IsDirectory) {  // 如果是目录
				if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))  // 跳过当前目录和父目录
				{
					continue;  // 继续下一次循环
				}
				HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);  // 插入目录节点
				m_Tree.InsertItem("", hTemp, TVI_LAST);  // 插入子节点占位
			}
			else {  // 如果是文件
				m_List.InsertItem(0, pInfo->szFileName);  // 添加到文件列表
			}
		}
	}

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
	int nListSelected = m_List.GetSelectionMark();  // 获取文件列表选中项
	CString strFile = m_List.GetItemText(nListSelected, 0);  // 获取选中文件名
	HTREEITEM hSelected = m_Tree.GetSelectedItem();  // 获取目录树选中项
	strFile = GetPath(hSelected) + strFile;  // 拼接完整文件路径
	int ret = CClientController::getInstance()->DownFile(strFile);  // 获取CClientController类的单例对象，并调用其DownFile方法，传入strFile参数（用于指定要下载的文件相关信息）
	///////添加线程函数
	if (ret != 0) {  // 如果ret不等于0（表示下载操作返回非成功状态）
		MessageBox(_T("下载失败！"));  // 弹出“下载失败！”的消息框
		TRACE("下载失败 ret = %d\r\n", ret);  // 输出调试信息，显示“下载失败 ret = ”及ret的值
	}
}


void CRemoteClientDlg::OnDeleteFile()  // 删除文件命令处理函数
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hSelected);  // 获取选中项路径
	int nSelected = m_List.GetSelectionMark();  // 获取文件列表选中项
	CString strFile = m_List.GetItemText(nSelected, 0);  // 获取选中文件名
	strFile = strPath + strFile;  // 拼接完整文件路径
	int ret = CClientController::getInstance()->SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送删除文件命令
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
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送运行文件命令
	if (ret < 0) {  // 命令执行失败
		AfxMessageBox("打开文件命令执行失败！！！");  // 显示错误消息
	}
}




void CRemoteClientDlg::OnBnClickedBtnStartWatch()  // 开始监控按钮点击事件处理函数
{
	CClientController::getInstance()->StartWatchScreen();  // 启动屏幕监控  
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)  // 定时器事件处理函数
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);  // 调用基类定时器处理函数
}

void CRemoteClientDlg::OnEnChangeEditPort()  // 端口编辑框内容改变事件处理函数
{
	UpdateData();  // 从对话框控件更新数据到成员变量
	CClientController* pController = CClientController::getInstance();  // 获取CClientController类的单例对象指针pController
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));  // 调用pController的UpdateAddress方法，更新服务器地址和端口（m_nPort先转换为LPCTSTR再转成int）
}
void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();  // 从对话框控件更新数据到成员变量
	CClientController* pController = CClientController::getInstance();  // 获取CClientController类的单例对象指针pController
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));  // 调用pController的UpdateAddress方法，更新服务器地址和端口（m_nPort先转换为LPCTSTR再转成int）
}
