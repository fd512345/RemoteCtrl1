#include "pch.h"
#include "ClientController.h"

CClientController* CClientController::getInstance() {
	if (m_instance == NULL) {  // 检查单例实例是否为空
		m_instance = new CClientController();  // 若为空，创建CClientController实例
		// 定义结构体数组，存储消息ID与对应的消息处理成员函数指针
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			{WM_SEND_PACK, &CClientController::OnSendPack},
			{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus},
			{WM_SHOW_WATCH, &CClientController::OnShowWatcher},
			{-1, NULL}  // 数组结束标记，func为NULL
		};
		// 遍历结构体数组，将消息与处理函数的映射插入到m_mapFunc中
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return nullptr;  // 返回nullptr（此处存在问题，单例模式应返回创建的m_instance）
}
int CClientController::Invoke(CWnd*& m_pMainWnd)
{
	m_pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}
LRESULT CClientController::SendMessage(MSG msg)
{
	UUID uuid;  // 定义UUID类型变量uuid，用于唯一标识消息
	UuidCreate(&uuid);  // 生成UUID并存储到uuid中
	// 向m_mapMessage中插入键值对（uuid和msg），auto用于自动推导pr的类型
	auto pr = m_mapMessage.insert(std::pair<UUID, MSG>(uuid, msg));
	// 向指定线程（线程ID为m_nThreadID）发送自定义消息WM_SEND_MESSAGE，附带pr.second和pr.first的地址作为参数
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&pr.second, (LPARAM)&pr.first);
	return LRESULT();  // 返回LRESULT类型的默认构造值
}
int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(  // 调用_beginthreadex创建线程，返回线程句柄并赋值给m_hThread
		NULL, 0,  // 安全属性和栈大小，使用默认值
		&CClientController::threadEntry,  // 线程入口函数为CClientController类的threadEntry静态成员函数
		this, 0, &m_nThreadID);  // 传入当前对象指针this作为线程参数，创建标志为0，传出线程ID到m_nThreadID
	return 0;
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;  // 将void*类型的arg强制转换为CClientController*类型并赋值给thiz
	thiz->threadFunc();  // 调用thiz指向的CClientController对象的threadFunc方法
	_endthreadex(0);  // 结束当前线程，参数0表示线程退出码
	return 0;  // 这里的return 0实际可能因_endthreadex的调用而不会执行到，主要是为了符合函数返回值要求等情况
	return 0;
}
void CClientController::threadFunc() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {  // 从消息队列获取消息，当获取到消息（返回非0）时继续循环，获取失败（返回0）或出错（返回-1）时退出
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {  // 判断消息是否为自定义的WM_SEND_MESSAGE消息
			MSG* pmsg = (MSG*)msg.wParam;  // 将消息的wParam参数转换为MSG*类型指针
			UUID* puuid = (UUID*)msg.lParam;  // 将消息的lParam参数转换为UUID*类型指针
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->message);  // 在m_mapFunc中查找pmsg->message对应的处理函数迭代器
			if (it != m_mapFunc.end()) {  // 如果找到对应的处理函数
				(this->*(it->second))(pmsg->message, pmsg->wParam, pmsg->lParam);  // 调用对应的成员函数处理消息
				std::map<UUID, MSG>::iterator it = m_mapMessage.find(*puuid);  // 在m_mapMessage中查找*puuid对应的消息迭代器
				if (it != m_mapMessage.end()) {  // 如果找到对应的消息
					m_mapMessage.erase(*puuid);  // 从m_mapMessage中删除该消息
				}
			}
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);  // 在m_mapFunc中查找当前消息对应的处理函数迭代器
			if (it != m_mapFunc.end()) {  // 如果找到对应的处理函数
				(this->*(it->second))(msg.message, msg.wParam, msg.lParam);  // 调用对应的成员函数处理消息
			}
		}
	}
}