// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "CWatchDialog.h"
#include "afxdialogex.h"
#include "ClientController.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;  // 初始化图像缓存状态
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{//800 450
	CRect clientRect;
	if (isScreen)ScreenToClient(&point);//全局坐标到客户区域坐标
	TRACE("x=%d y=%d\r\n", point.x, point.y);
	//本地坐标，到远程坐标
	m_picture.GetWindowRect(clientRect);
	TRACE("x=%d y=%d\r\n", clientRect.Width(), clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;  // 初始化图像缓存状态
	SetTimer(0, 45, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CClientController* pParent = CClientController::getInstance();
		if (m_isFull) {

			CRect rect;
			m_picture.GetWindowRect(rect); // 获取图片控件的窗口矩形
			m_nObjWidth = m_image.GetWidth(); // 若对象宽度未设置，获取图像宽度并赋值
			m_nObjHeight = m_image.GetHeight(); // 若对象高度未设置，获取图像高度并赋值
			m_image.StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY); // 将图像拉伸绘制到图片控件的设备上下文
			m_picture.InvalidateRect(NULL); // 使图片控件整个客户区无效，触发重绘
			m_image.Destroy(); // 销毁图像对象，释放资源
			m_isFull = false; // 标记不再处于“满”状态
			TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
			// 打印调试信息，输出“更新图片完成”、图片宽度 m_nObjWidth、图片高度 m_nObjHeight，
			// 以及将 m_image 转换为 HBITMAP 类型后的十六进制值（8位宽度，不足补0）		
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//双击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));// 调用 CClientController 单例对象的 SendCommandPacket 方法，发送命令包，参数分别为命令标识 5、是否为某种特定类型（true）、事件数据的指针（转换为 BYTE* 类型）、事件数据的大小	
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		TRACE("x=%d y=%d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x=%d y=%d\r\n", point.x, point.y);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//左键
		event.nAction = 1;//双击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//左键
		event.nAction = 2;//按下 //TODO:服务端要做对应的修改
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//左键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;//没有按键
		event.nAction = 0;//移动
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point);
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(8);
}
