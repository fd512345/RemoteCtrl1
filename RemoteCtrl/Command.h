#pragma once
#include "resource.h"  // 包含资源头文件
#include <map>
#include <atlimage.h>  // 包含ATL图像处理类
#include <direct.h>  // 包含目录操作相关函数
#include "Packet.h"
#include "EdoyunTool.h"
#include <stdio.h>  // 包含标准输入输出头文件
#include <io.h>  // 包含输入输出相关函数
#include <list>  // 包含列表容器
#include "LockInfoDialog.h"  // 包含锁定信息对话框头文件
#pragma warning(disable:4966) // fopen sprintf strcpy strstr  // 禁用特定警告 

class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
	{  // 定义静态函数RunCommand，接收void*类型的arg和int类型的status参数
		CCommand* thiz = (CCommand*)arg;  // 将arg强制转换为CCommand*类型并赋值给thiz
		if (status > 0) {  // 如果status大于0
			int ret = thiz->ExecuteCommand(status, lstPacket,inPacket);  // 调用thiz指向的CCommand对象的ExcuteCommand方法，传入status，返回值存入ret
			if (ret != 0) {  // 如果执行命令返回值ret不等于0
				TRACE("执行命令失败：%d ret=%d\r\n", status, ret);  // 输出执行命令失败的调试信息
			}
		}
		else {  // 如果status小于等于0
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);  // 弹出提示框，提示接入用户失败
		}
	}
protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); // 成员函数指针
	std::map<int, CMDFUNC> m_mapFunction; // 从命令号到功能的映射
	CLockInfoDialog dlg;  // 定义锁定信息对话框对象
	unsigned threadid;  // 存储线程ID
protected:
	static unsigned __stdcall threadLockDlg(void* arg)  // 定义锁定对话框线程函数
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);  // 结束线程
		return 0;  // 返回
	}
	void threadLockDlgMain()
	{
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());  // 输出线程信息
		dlg.Create(IDD_DIALOG_INFO, NULL);  // 创建对话框
		dlg.ShowWindow(SW_SHOW);  // 显示对话框
		//遮蔽后台窗口
		CRect rect;  // 定义矩形对象
		rect.left = 0;  // 设置矩形左边界
		rect.top = 0;  // 设置矩形上边界
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//w1  // 设置矩形右边界为屏幕宽度
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);  // 设置矩形下边界为屏幕高度
		rect.bottom = LONG(rect.bottom * 1.10);  // 适当增加下边界
		TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);  // 输出矩形信息
		dlg.MoveWindow(rect);  // 移动对话框到指定位置和大小
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);  // 获取静态文本控件
		if (pText) {  // 如果获取成功
			CRect rtText;  // 定义矩形对象
			pText->GetWindowRect(rtText);  // 获取静态文本控件位置和大小
			int nWidth = rtText.Width();//w0  // 获取控件宽度
			int x = (rect.right - nWidth) / 2;  // 计算控件x坐标（居中）
			int nHeight = rtText.Height();  // 获取控件高度
			int y = (rect.bottom - nHeight) / 2;  // 计算控件y坐标（居中）
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());  // 移动控件到居中位置
		}

		//窗口置顶
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);  // 设置对话框置顶
		//限制鼠标功能
		ShowCursor(false);  // 隐藏鼠标光标
		//隐藏任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  // 隐藏任务栏
		//限制鼠标活动范围
		dlg.GetWindowRect(rect);  // 获取对话框位置和大小
		rect.left = 0;  // 设置限制范围左边界
		rect.top = 0;  // 设置限制范围上边界
		rect.right = 1;  // 设置限制范围右边界
		rect.bottom = 1;  // 设置限制范围下边界
		ClipCursor(rect);  // 限制鼠标活动范围
		MSG msg;  // 定义消息结构体
		while (GetMessage(&msg, NULL, 0, 0)) {  // 消息循环
			TranslateMessage(&msg);  // 转换消息
			DispatchMessage(&msg);  // 分发消息
			if (msg.message == WM_KEYDOWN) {  // 处理键盘按下消息
				TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);  // 输出消息信息
				if (msg.wParam == 0x41) {//按下a键 退出  ESC（1B)  // 如果按下A键
					break;  // 退出消息循环
				}
			}
		}
		ClipCursor(NULL);  // 解除鼠标活动范围限制
		//恢复鼠标
		ShowCursor(true);  // 显示鼠标光标
		//恢复任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);  // 显示任务栏
		dlg.DestroyWindow();  // 销毁对话框
	}
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {//1==>A 2==>B 3==>C ... 26==>Z  // 定义获取磁盘驱动器信息的函数
		std::string result;  // 存储驱动器信息结果
		for (int i = 1; i <= 26; i++) {  // 遍历A-Z驱动器
			if (_chdrive(i) == 0) {  // 检查驱动器是否存在
				if (result.size() > 0)  // 如果已有内容，添加分隔符
					result += ',';
				result += 'A' + i - 1;  // 添加驱动器字母
			}
		}

		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));  // 将构造的CPacket对象添加到lstPacket容器中，其中CPacket的构造参数分别为1、result的C风格字符串指针（转换为BYTE*）、result的大小		return 0;  // 返回成功
		return 0;
	}

	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {  // 定义获取目录信息的函数
		std::string strPath = inPacket.strData;  // 存储目录路径
		//std::list<FILEINFO> lstFileInfos;  // 注释：存储文件信息的列表

		if (_chdir(strPath.c_str()) != 0) {  // 切换到目标目录失败
			FILEINFO finfo;  // 定义文件信息结构体
			finfo.HasNext = FALSE;  // 设置没有后续文件
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			OutputDebugString(_T("没有权限访问目录！！"));  // 输出调试信息
			return -2;  // 返回错误
		}
		_finddata_t fdata;  // 定义文件查找结构体
		int hfind = 0;  // 查找句柄
		if ((hfind = _findfirst("*", &fdata)) == -1) {  // 查找第一个文件失败
			OutputDebugString(_T("没有找到任何文件！！"));  // 输出调试信息
			FILEINFO finfo;  // 定义文件信息结构体
			finfo.HasNext = FALSE;  // 设置没有后续文件
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			return -3;  // 返回错误
		}
		int count = 0;  // 记录文件数量
		do {  // 遍历查找文件
			FILEINFO finfo;  // 定义文件信息结构体
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;  // 判断是否为目录
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));  // 复制文件名
			TRACE("%s\r\n", finfo.szFileName);  // 输出文件名
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			count++;  // 增加文件计数
		} while (!_findnext(hfind, &fdata));  // 查找下一个文件
		TRACE("server: count = %d\r\n", count);  // 输出文件总数
		//发送信息到控制端
		FILEINFO finfo;  // 定义文件信息结构体
		finfo.HasNext = FALSE;  // 设置没有后续文件
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {  // 定义运行文件的函数
		std::string strPath = inPacket.strData;  // 存储目录路径
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);  // 执行文件
		lstPacket.push_back(CPacket(3, NULL, 0));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {  // 定义下载文件的函数
		std::string strPath = inPacket.strData;  // 存储目录路径
		long long data = 0;  // 存储文件大小
		FILE* pFile = NULL;  // 文件指针
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");  // 打开文件
		if (err != 0) {  // 打开文件失败
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			return -1;  // 返回错误
		}
		if (pFile != NULL) {  // 文件打开成功
			fseek(pFile, 0, SEEK_END);  // 移动到文件末尾
			data = _ftelli64(pFile);  // 获取文件大小
			lstPacket.push_back(CPacket(4, NULL, 8));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			fseek(pFile, 0, SEEK_SET);  // 移动到文件开头
			char buffer[1024] = "";  // 存储文件数据的缓冲区
			size_t rlen = 0;  // 读取的字节数
			do {  // 循环读取文件内容
				rlen = fread(buffer, 1, 1024, pFile);  // 读取数据
				lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			} while (rlen >= 1024);  // 直到读取的字节数小于缓冲区大小
			fclose(pFile);  // 关闭文件
		}
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义处理鼠标事件的函数
	{
		MOUSEEV mouse;  // 定义鼠标事件结构体
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));  // 复制事件数据
			DWORD nFlags = 0;  // 鼠标事件标志
			switch (mouse.nButton) {  // 根据鼠标按钮设置标志
			case 0://左键
				nFlags = 1;
				break;
			case 1://右键
				nFlags = 2;
				break;
			case 2://中键
				nFlags = 4;
				break;
			case 4://没有按键
				nFlags = 8;
				break;
			}
			if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);  // 如果有按键，设置鼠标位置
			switch (mouse.nAction)  // 根据鼠标动作设置标志
			{
			case 0://单击
				nFlags |= 0x10;
				break;
			case 1://双击
				nFlags |= 0x20;
				break;
			case 2://按下
				nFlags |= 0x40;
				break;
			case 3://放开
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);  // 输出鼠标事件信息
			switch (nFlags)  // 根据标志执行相应的鼠标事件
			{
			case 0x21://左键双击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x11://左键单击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://左键按下
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://左键放开
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x22://右键双击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://右键单击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://右键按下
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://右键放开
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://中键双击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://中键单击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://中键按下
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://中键放开
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://单纯的鼠标移动
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			}
			lstPacket.push_back(CPacket(5, NULL, 0));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		
		return 0;  // 返回成功
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义发送屏幕截图的函数
	{
		CImage screen;//GDI  // 定义图像对象，用于存储屏幕截图
		HDC hScreen = ::GetDC(NULL);  // 获取屏幕设备上下文
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//24   ARGB8888 32bit RGB888 24bit RGB565  RGB444  // 获取屏幕每像素位数
		int nWidth = GetDeviceCaps(hScreen, HORZRES);  // 获取屏幕宽度
		int nHeight = GetDeviceCaps(hScreen, VERTRES);  // 获取屏幕高度
		screen.Create(nWidth, nHeight, nBitPerPixel);  // 创建图像
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  // 将屏幕内容复制到图像
		ReleaseDC(NULL, hScreen);  // 释放屏幕设备上下文
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 分配全局内存
		if (hMem == NULL)return -1;  // 内存分配失败，返回错误
		IStream* pStream = NULL;  // 定义流对象
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 创建流
		if (ret == S_OK) {  // 流创建成功
			screen.Save(pStream, Gdiplus::ImageFormatPNG);  // 将图像保存到流
			LARGE_INTEGER bg = { 0 };  // 用于定位流的起始位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 将流指针移到起始位置
			PBYTE pData = (PBYTE)GlobalLock(hMem);  // 锁定全局内存并获取指针
			SIZE_T nSize = GlobalSize(hMem);  // 获取全局内存大小
			lstPacket.push_back(CPacket(6, pData, nSize));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
			GlobalUnlock(hMem);  // 解锁全局内存
		}
		pStream->Release();  // 释放流对象
		GlobalFree(hMem);  // 释放全局内存
		screen.ReleaseDC();  // 释放图像的设备上下文
		return 0;  // 返回成功
	}
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义锁定机器的函数
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {  // 如果对话框未创建
			//_beginthread(threadLockDlg, 0, NULL);  // 注释：创建线程的另一种方式
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);  // 创建线程
			TRACE("threadid=%d\r\n", threadid);  // 输出线程ID
		}
		lstPacket.push_back(CPacket(7, NULL, 0));  // 创建响应数据包
		return 0;  // 返回成功
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义解锁机器的函数
	{
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);  // 注释：发送按键消息的另一种方式
		//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);  // 注释：发送按键消息的另一种方式
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);  // 向锁定线程发送A键按下消息
		lstPacket.push_back(CPacket(8, NULL, 0));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义测试连接的函数
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}

	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 定义删除本地文件的函数
	{
		std::string strPath = inPacket.strData;  // 存储目录路径

		// 将TCHAR改为wchar_t，与LPWSTR类型匹配
		wchar_t sPath[MAX_PATH] = L"";  // 存储宽字符文件路径，使用宽字符初始化

		// 多字节转宽字符（修复类型不兼容问题）
		MultiByteToWideChar(
			CP_ACP, 0,
			strPath.c_str(),
			-1,  // 使用-1自动处理字符串长度（包括终止符）
			sPath,  // 现在是wchar_t*类型，与LPWSTR兼容
			MAX_PATH  // 直接使用缓冲区大小，更清晰
		);

		// 如果需要使用宽字符版本的删除文件函数（更适合中文路径）
		// DeleteFileW(sPath);

		// 保留ANSI版本删除文件（如果确实需要）
		DeleteFileA(strPath.c_str());  // 删除文件
		lstPacket.push_back(CPacket(9, NULL, 0));  // 将构造的CPacket对象添加到lstPacket容器中，构造参数为命令2、finfo的地址（转换为BYTE*）、finfo的大小			
		return 0;  // 返回成功
	}
};

