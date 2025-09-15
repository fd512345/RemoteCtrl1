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
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)  // MFC的消息映射宏，将自定义消息WM_SEND_PACK_ACK与CWatchDialog类的OnSendPacket函数关联，当收到WM_SEND_PACK_ACK消息时，调用OnSendPacket处理

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

	InitUIData(); // 函数声明：用于初始化界面（UI）相关的数据	
	// TODO: 在此添加额外的初始化代码
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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);  // 获取CClientController类的单例对象，并调用其SendCommandPacket方法，传入命令值1981
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()  // 文件信息按钮点击事件处理函数
{
	std::list<CPacket> lstPackets;                      // 定义一个存储 CPacket 类型对象的链表
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);  // 调用单例类 CClientController 的 SendCommandPacket 方法发送命令包，将结果存入 ret
	if (ret == 0) {        // 判断命令是否处理失败（返回值为 -1 或者链表中没有数据包）
		AfxMessageBox(_T("命令处理失败!!!"));           // 弹出提示命令处理失败的消息框
		return;                                         // 函数返回
	}
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	UpdateData();  // 从控件更新数据到变量
	m_server_address = 0x7F000001;//127.0.0.1  // 设置默认服务器地址为本地回环地址
	m_nPort = _T("9527");  // 设置默认端口号
	UpdateData();  // 从对话框控件更新数据到成员变量
	CClientController* pController = CClientController::getInstance();  // 获取CClientController类的单例对象指针pController
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));  // 调用pController的UpdateAddress方法，更新服务器地址和端口（m_nPort先转换为LPCTSTR再转成int）
	UpdateData(FALSE);  // 将变量数据更新到控件
	m_dlgStatus.Create(IDD_DLG_STATUS, this);  // 创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE);  // 隐藏状态对话框
}

void CRemoteClientDlg::LoadFileCurrent()  // 加载当前目录文件
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();  // 获取目录树选中项
	CString strPath = GetPath(hTree);  // 获取选中项路径
	m_List.DeleteAllItems();  // 清空文件列表
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());  // 发送获取目录信息命令
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

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();  // 清空目录树
	for (size_t i = 0; i < drivers.size(); i++)  // 遍历驱动信息
	{
		if (drivers[i] == ',') {  // 遇到分隔符
			dr += ":";  // 添加冒号
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);  // 插入驱动节点
			tree.InsertItem(NULL, hTemp, TVI_LAST);  // 插入子节点占位
			dr.clear();  // 清空临时字符串
			continue;  // 继续下一次循环
		}
		dr += drivers[i];  // 拼接驱动字母
	}
	// 循环结束后再插入最后一个盘符
	if (!dr.empty()) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		tree.InsertItem(NULL, hTemp, TVI_LAST);
	}

}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	TRACE("hasnext %d isdirectory %d %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);  // 输出调试信息，显示pInfo对象的HasNext（是否有下一个）、IsDirectory（是否为目录）以及szFileName（文件名）
	if (finfo.HasNext == false)
		return;
	if (finfo.IsDirectory) {  // 如果是目录
		if (CString(finfo.szFileName) == "." || (CString(finfo.szFileName) == ".."))  // 跳过当前目录和父目录
		{
			return;
		}
		TRACE("hselected %08X\r\n", hParent, m_Tree.GetSelectedItem());  // 以8位十六进制格式输出lParam的值，用于调试查看选中相关的句柄等信息
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, (HTREEITEM)hParent, TVI_LAST);  // 插入目录节点
		m_Tree.InsertItem("", hTemp, TVI_LAST);  // 插入子节点占位
		m_Tree.Expand(hParent, TVE_EXPAND);  // 对树控件m_Tree中由lParam转换为HTREEITEM类型的项执行展开操作，TVE_EXPAND表示展开该树项
	}
	else {  // 如果是文件
		m_List.InsertItem(0, finfo.szFileName);  // 添加到文件列表
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;  // 定义静态变量length和index，用于记录数据长度和索引，静态变量生命周期为整个程序运行期间，且只初始化一次
	TRACE("length %d index %d\r\n", length, index);  // 输出调试信息，显示length（长度）和index（索引）的值
	if (length == 0) {  // 判断length是否为0，为0则从head.strData中获取数据长度并赋值给length
		length = *(long long*)strData.c_str();  // 将head.strData的C字符串首地址强制转换为long long*类型指针，解引用获取数据长度并赋值给length
		if (length == 0) {  // 判断length是否为0，若为0表示文件长度异常
			AfxMessageBox("文件长度为零或者无法读取文件！！！");  // 弹出提示文件长度异常或无法读取的消息框
			CClientController::getInstance()->DownloadEnd();  // 调用客户端控制器单例的DownloadEnd方法，结束下载操作
		}

	}
	else if (length > 0 && (index >= length)) {  // 当length大于0且index大于等于length时，说明文件写入完成
		fclose(pFile);  // 关闭通过lParam传递的文件指针指向的文件
		length = 0;  // 将length重置为0
		index = 0;   // 将index重置为0
		CClientController::getInstance()->DownloadEnd();  // 调用客户端控制器单例的DownloadEnd方法，结束下载操作
	}
	else {  // 否则，继续写入文件数据
		fwrite(strData.c_str(), 1, strData.size(), pFile);  // 将head.strData中的数据写入到pFile指向的文件中，每次写1个字节，共写head.strData.size()个字节
		index += strData.size();  // 累加已写入的数据长度到index中
		TRACE("index = %d\r\n", index);  // 输出调试信息，显示index（索引）的值
		if (index >= length) {  // 判断索引index是否大于等于长度length
			fclose(pFile);  // 关闭由lParam转换为FILE*类型的文件指针
			length = 0;  // 将length（长度）置为0
			index = 0;  // 将index（索引）置为0
			CClientController::getInstance()->DownloadEnd();  // 调用CClientController单例对象的DownloadEnd方法，标记下载结束
		}
	}
}

void CRemoteClientDlg::LoadFileInfo()  // 加载文件信息
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);  // 获取鼠标位置
	m_Tree.ScreenToClient(&ptMouse);  // 转换为客户区坐标
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);  // 获取鼠标指向的目录树项
	if (hTreeSelected == NULL)  // 如果没有指向任何项
		return;  // 返回
	DeleteTreeChildrenItem(hTreeSelected);  // 删除子项
	m_List.DeleteAllItems();  // 清空文件列表
	CString strPath = GetPath(hTreeSelected);  // 获取选中项路径
	TRACE("hTreeSelected %08X\r\n", hTreeSelected);  // 以8位十六进制格式输出hTreeSelected的值，用于调试查看树控件中选中项相关的句柄等信息
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);  // 调用单例类的SendCommandPacket方法发送命令包，结果存nCmd
}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {  // 根据数据包对象的sCmd成员（命令标识）进行分支判断
	case 1:  // 获取驱动信息
		Str2Tree(strData, m_Tree); // 调用Str2Tree函数，将head.strData中的数据转换并构建到m_Tree树控件中
		break;
	case 2://获取文件信息

		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		// 调用UpdateFileInfo函数，把head.strData的C字符串强制转为PFILEINFO指针后解引用，得到FILEINFO对象作为第一个参数，
		// 再将lParam强制转为HTREEITEM类型的树项句柄作为第二个参数，以此更新对应文件信息		
		break;
	case 3:  // 若sCmd为3，执行此处逻辑
		MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION); // 弹出消息框，显示“删除文件完成！”，标题为“操作完成”，图标为信息图标
		break;
	case 4:  // 若sCmd为4，执行此处逻辑（目前暂未编写具体逻辑）

		UpdateDownloadFile(strData, (FILE*)lParam);
		// 调用UpdateDownloadFile函数，第一个参数为head.strData（可能是与下载文件相关的数据字符串），
		// 第二个参数是将lParam强制转换为FILE*类型的文件指针，用于更新下载文件相关信息
		break;
	case 9:  // 当命令标识为9时
		MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION); // 弹出消息框，显示“删除文件完成！”，标题为“操作完成”，图标为信息图标		
		break;  // 跳出switch结构
	case 1981:  // 当命令标识为1981时
		MessageBox("连接测试成功！", "连接成功", MB_ICONINFORMATION); // 弹出消息框，显示“连接测试成功！”，标题为“连接成功”，图标为信息图标		
		break;  // 跳出switch结构
	default:  // 命令标识不匹配以上case时的默认分支
		TRACE("unknow data received! %d\r\n", nCmd);  // 输出“unknow data received!”以及未知的命令标识值head.sCmd
		break;  // 跳出switch结构
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送删除文件命令
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());  // 发送运行文件命令
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

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2))
	{
		TRACE("socket is error %d\r\n", lParam); // 当lParam为-1或-2时，输出套接字错误及lParam的值
	}
	else if (lParam == 1)
	{
		//对方关闭了套接字
		TRACE("socket is closed!\r\n"); // 当lParam为1时，输出套接字已关闭的信息
	}
	else
	{  // 判断lParam参数是否为0，若为0则执行后续相关逻辑（此处暂未编写具体逻辑）
		if (wParam != NULL) {  // 检查pPacket是否不为空，不为空则进行后续数据包命令处理
			CPacket head = *(CPacket*)wParam;  // 将wParam强制转换为CPacket*类型的指针pPacket，用于操作数据包对象
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam); // 调用DealCommand函数，处理head.strData中的命令，lParam为相关参数
		}
	}
	return 0;
}