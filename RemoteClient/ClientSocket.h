#pragma once                                 // 防止头文件重复包含
#include "pch.h"                             // 包含预编译头文件
#include "framework.h"                       // 包含框架头文件
#include <string>                            // 包含字符串处理头文件
#include <vector>                            // 包含向量容器头文件
#include <list>								 // 包含列表容器头文件
#include <map>
#include <mutex>
#pragma pack(push)                           // 保存当前内存对齐方式
#pragma pack(1)                              // 设置内存对齐为1字节（紧凑对齐）
#define WM_SEND_PACK (WM_USER+1)  // 发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2)//发送包数据应答

class CPacket                                // 数据包类，用于封装和解析网络传输的数据
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // 默认构造函数，初始化成员变量
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {  // 带参数构造函数，用于创建发送的数据包
		sHead = 0xFEFF;                       // 设置固定包头标识
		nLength = nSize + 4;                  // 计算数据长度（包含命令和校验和的4字节）
		sCmd = nCmd;                          // 设置命令号
		if (nSize > 0) {                      // 如果有数据，则复制数据到内部缓冲区
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {                                // 无数据时清空缓冲区
			strData.clear();
		}
		sSum = 0;                             // 计算校验和（数据部分所有字节的和）
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) {            // 拷贝构造函数，复制数据包内容
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {  // 从字节流解析数据包的构造函数
		size_t i = 0;
		for (; i < nSize; i++) {              // 查找包头标识0xFEFF
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;                       // 跳过包头（2字节）
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {          // 检查剩余数据是否足够解析基本字段（长度4字节+命令2字节+校验和2字节）
			nSize = 0;                        // 数据不足，标记解析失败
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;  // 读取数据长度（4字节）并移动指针
		if (nLength + i > nSize) {            // 检查数据长度是否超过缓冲区总长度
			nSize = 0;                        // 数据不完整，标记解析失败
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;    // 读取命令号（2字节）并移动指针
		if (nLength > 4) {                    // 如果有实际数据（长度大于4字节，排除命令和校验和）
			strData.resize(nLength - 2 - 2);   // 调整数据缓冲区大小（总长度-命令2字节-校验和2字节）
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // 复制数据内容
			TRACE("%s\r\n", strData.c_str() + 12);  // 调试输出数据（跳过前12字节，可能是特定格式）
			i += nLength - 4;                  // 移动指针跳过数据部分
		}
		sSum = *(WORD*)(pData + i); i += 2;    // 读取校验和（2字节）并移动指针
		WORD sum = 0;                         // 重新计算校验和用于验证
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {                    // 校验和匹配，解析成功
			nSize = i;                        // 更新有效数据长度（已解析的字节数）
			return;
		}
		nSize = 0;                            // 校验和不匹配，解析失败
	}
	~CPacket() {}                             // 析构函数（空实现）
	CPacket& operator=(const CPacket& pack) { // 赋值运算符重载，用于数据包赋值
		if (this != &pack) {                  // 避免自赋值
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() {                              // 返回数据包总大小（包头+长度+命令+数据+校验和）
		return nLength + 6;                   // nLength包含命令+数据+校验和，加上包头2字节共+6
	}
	const char* Data(std::string& strOut) const {                      // 生成用于发送的二进制数据流
		strOut.resize(nLength + 6);           // 调整输出缓冲区大小
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;    // 写入包头（2字节）
		*(DWORD*)(pData) = nLength; pData += 4;  // 写入长度（4字节）
		*(WORD*)pData = sCmd; pData += 2;     // 写入命令（2字节）
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();  // 写入数据
		*(WORD*)pData = sSum;                 // 写入校验和（2字节）
		return strOut.c_str();                // 返回生成的数据流
	}

public:
	WORD sHead;                               // 包头标识（固定0xFEFF，2字节）
	DWORD nLength;                            // 数据长度（包含命令、数据、校验和，4字节）
	WORD sCmd;                                // 命令号（2字节）
	std::string strData;                      // 数据内容（可变长度）
	WORD sSum;                                // 校验和（数据部分的字节和，2字节）
	//std::string strOut;                       // 用于存储序列化后的输出数据
};
#pragma pack(pop)                            // 恢复之前的内存对齐方式

typedef struct MouseEvent {                  // 鼠标事件结构体，用于传输鼠标操作
	MouseEvent() {                           // 构造函数初始化
		nAction = 0;                         // 操作类型（移动、单击等）
		nButton = -1;                        // 按键（左键、右键等）
		ptXY.x = 0;                          // X坐标
		ptXY.y = 0;                          // Y坐标
	}
	WORD nAction;                             // 鼠标操作类型（2字节）
	WORD nButton;                             // 鼠标按键（2字节）
	POINT ptXY;                               // 坐标点（x和y各4字节，共8字节）
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {                   // 文件信息结构体，用于传输文件/目录信息
	file_info() {                            // 构造函数初始化
		IsInvalid = FALSE;                   // 是否无效
		IsDirectory = -1;                    // 是否为目录（0=文件，1=目录）
		HasNext = TRUE;                      // 是否有下一条信息
		memset(szFileName, 0, sizeof(szFileName));  // 文件名（初始化清空）
	}
	BOOL IsInvalid;                           // 有效性标识（4字节）
	BOOL IsDirectory;                         // 目录标识（4字节）
	BOOL HasNext;                             // 后续信息标识（4字节）
	char szFileName[256];                     // 文件名（256字节）
}FILEINFO, * PFILEINFO;


enum {
	CSM_AUTOCLOSE = 1,  // CSM = Client Socket Mode 自动关闭模式
};


typedef struct PacketData {
	std::string strData;  // 存储数据包的字符串数据
	UINT nMode;  // 数据包的模式
	WPARAM wParam;
	// 构造函数，从字节数据创建PacketData对象
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
		strData.resize(nLen);  // 调整strData大小以容纳nLen长度的数据
		memcpy((char*)strData.c_str(), pData, nLen);  // 将pData中的数据复制到strData
		nMode = mode;  // 设置模式
		wParam = nParam;
	}

	// 拷贝构造函数，用于复制PacketData对象
	PacketData(const PacketData& data) {
		strData = data.strData;  // 复制字符串数据
		nMode = data.nMode;  // 复制模式
		wParam = data.wParam;
	}

	// 赋值运算符重载，用于PacketData对象之间的赋值
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {  // 避免自赋值
			strData = data.strData;  // 复制字符串数据
			nMode = data.nMode;  // 复制模式
			wParam = data.wParam;
		}
		return *this;  // 返回自身引用以支持链式赋值
	}
}PACKET_DATA;

std::string GetErrInfo(int wsaErrCode);      // 声明获取WSA错误信息的函数
void Dump(BYTE* pData, size_t nSize);        // 声明二进制数据打印函数（调试用）
class CClientSocket                          // 客户端Socket类，用于网络通信
{
public:
	static CClientSocket* getInstance() {     // 单例模式，获取实例
		if (m_instance == NULL) {
			m_instance = new CClientSocket();
			TRACE("CClientSocket size is %d\r\n", sizeof(*m_instance));  // 输出调试信息，显示CClientSocket实例（通过m_instance解引用得到）的大小，格式为“CClientSocket size is [大小值]\r\n”
		}
		return m_instance;
	}
	bool InitSocket();


#define BUFFER_SIZE 4096000                  // 定义接收缓冲区大小（2MB）
	int DealCommand() {                      // 处理接收的命令（解析数据包）
		if (m_sock == -1)return -1;           // Socket无效返回-1
		char* buffer = m_buffer.data();      //TODO:多线程发送命令时可能会出现冲突 // 获取缓冲区指针
		static size_t index = 0;              // 缓冲区当前数据长度（静态变量，累计未解析数据）
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);  // 接收数据到缓冲区
			if (((int)len <= 0) && ((int)index <= 0)) {  // 接收失败且无未解析数据
				return -1;
			}
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			// 打印接收数据的长度（十进制和十六进制）以及索引值（十进制和十六进制）
			//Dump((BYTE*)buffer, index);      // 调试打印缓冲区数据（注释掉）
			index += len;                     // 更新缓冲区数据长度
			len = index;                      // 当前总数据长度
			m_packet = CPacket((BYTE*)buffer, len);  // 解析数据包
			TRACE("command %d\r\n", m_packet.sCmd);
			// 打印 m_packet 中的命令值（以十进制形式）
			if (len > 0) {                    // 解析到有效数据包
				memmove(buffer, buffer + len, index - len);  // 移动剩余数据到缓冲区头部
				index -= len;                 // 更新剩余数据长度
				return m_packet.sCmd;         // 返回解析到的命令号
			}
		}
		return -1;                            // 解析失败返回-1
	}

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);
	bool GetFilePath(std::string& strPath) {  // 获取数据包中的文件路径（针对特定命令）
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {  // 命令2-4包含文件路径
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {      // 获取数据包中的鼠标事件（命令5）
		if (m_packet.sCmd == 5) {             // 命令5为鼠标事件
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));  // 复制事件数据
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {                    // 获取当前解析的数据包
		return m_packet;
	}
	void CloseSocket() {                      // 关闭Socket连接
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;              // 标记为无效
	}
	void UpdateAddress(int nIP, int nPort) { // 定义 UpdateAddress 函数，用于更新 IP 和端口，返回 bool 类型
		if ((m_nIP != nIP) || (m_nPort != nPort)) { // 如果当前 IP 或端口与传入的 nIP、nPort 不同
			m_nIP = nIP; // 更新 IP 为 nIP
			m_nPort = nPort; // 更新端口为 nPort
		}
	}
private:
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);  // 定义指向CClientController类成员函数的指针类型MSGFUNC，该成员函数接收UINT、WPARAM、LPARAM类型参数，返回LRESULT
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend; // 定义一个存储 CPacket 类型对象的 std::list 容器 m_lstSend，用于管理待发送的数据包
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	// 定义一个 std::map 容器 m_mapAck，键为 int 类型，值为存储 CPacket 类型对象的 std::list 容器，用于按整数键关联 CPacket 对象的列表
	std::map<HANDLE, bool> m_mapAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;               // 接收缓冲区（向量容器）
	SOCKET m_sock;                            // Socket句柄
	CPacket m_packet;                         // 当前处理的数据包
	CClientSocket& operator=(const CClientSocket& ss) {}  // 禁用赋值运算符
	CClientSocket(const CClientSocket& ss);
	CClientSocket();
	~CClientSocket() {                        // 析构函数
		closesocket(m_sock);                  // 关闭Socket
		m_sock = INVALID_SOCKET;              // 标记为无效
		WSACleanup();                         // 清理Socket环境
	}

	static unsigned __stdcall threadEntry(void* arg); // 线程入口函数，静态成员函数，接收 void* 类型参数
	//void threadFunc(); // 线程执行的功能函数
	void threadFunc2();

	BOOL InitSockEnv() {                      // 初始化Winsock环境
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {  // 初始化Winsock 1.1版本
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {           // 释放单例实例
		TRACE("CClientSocket has been called!\r\n");  // 输出调试信息，提示CClientSocket被调用
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;  // 将m_instance指向的对象地址暂存到tmp
			m_instance = NULL;  // 把m_instance置为NULL，避免悬空指针
			delete tmp;  // 释放tmp指向的CClientSocket对象内存
			TRACE("CClientSocket has released!\r\n");  // 输出调试信息，提示CClientSocket已释放
		}
	}
	bool Send(const char* pData, int nSize) { // 发送原始数据
		if (m_sock == -1)return false;        // Socket无效返回false
		return send(m_sock, pData, nSize, 0) > 0;  // 发送数据，返回是否成功
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);  // 声明一个函数，返回值为LRESULT类型，函数名为SendPack，接收三个参数：无符号整型nMsg，WPARAM类型的wParam，LPARAM类型的lParam，用于发送数据包等相关操作
	static CClientSocket* m_instance;         // 单例实例指针
	class CHelper {                           // 辅助类，用于自动释放单例
	public:
		CHelper() {
			CClientSocket::getInstance();     // 构造时创建实例
		}
		~CHelper() {
			CClientSocket::releaseInstance(); // 析构时释放实例
		}
	};
	static CHelper m_helper;                  // 静态辅助对象，确保单例生命周期
};