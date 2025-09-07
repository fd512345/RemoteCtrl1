#pragma once  // 防止头文件重复包含
#include "pch.h"  // 包含预编译头文件
#include "framework.h"  // 包含MFC框架头文件
void Dump(BYTE* pData, size_t nSize);  // 声明Dump函数，用于十六进制数据打印
#pragma pack(push)  // 保存当前内存对齐方式
#pragma pack(1)  // 设置内存对齐为1字节（紧凑对齐）

class CPacket  // 数据包类，用于网络通信的数据封装与解析
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // 默认构造函数，初始化成员变量
	// 构造函数：根据命令和数据创建数据包
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;  // 设置包头标识（固定值）
		nLength = nSize + 4;  // 计算数据长度（包含命令和校验和的4字节）
		sCmd = nCmd;  // 设置命令类型
		if (nSize > 0) {  // 如果有数据，复制数据到strData
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {  // 无数据则清空strData
			strData.clear();
		}
		sSum = 0;  // 计算校验和（数据部分所有字节的和）
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) {  // 拷贝构造函数
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	// 构造函数：从字节流解析数据包
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		// 查找包头标识0xFEFF
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);//将包头赋给sHead
				i += 2;  // 跳过包头（2字节）
				break;
			}
		}
		// 检查数据是否足够解析包头（包头+长度+命令+校验和的最小长度）
		if (i + 4 + 2 + 2 > nSize) {  // 4字节长度 + 2字节命令 + 2字节校验和
			nSize = 0;  // 数据不完整，解析失败
			return;
		}
		TRACE("包头 is from here!\r\n");
		nLength = *(DWORD*)(pData + i);  // 获取数据长度（4字节）
		i += 4;  // 跳过长度字段
		// 检查数据总长度是否足够
		if (nLength + i > nSize) {
			nSize = 0;  // 数据不完整，解析失败
			return;
		}
		sCmd = *(WORD*)(pData + i);  // 获取命令类型（2字节）
		i += 2;  // 跳过命令字段
		if (nLength > 4) {  // 数据部分长度 = nLength - 4（命令2字节+校验和2字节）
			strData.resize(nLength - 2 - 2);  // 调整数据缓冲区大小
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // 复制数据
			i += nLength - 4;  // 跳过数据部分
		}
		sSum = *(WORD*)(pData + i);  // 获取校验和（2字节）
		i += 2;  // 跳过校验和
		// 验证校验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {  // 校验和匹配，解析成功
			nSize = i;  // 返回已解析的字节数
			return;
		}
		nSize = 0;  // 校验和不匹配，解析失败
	}
	~CPacket() {}  // 析构函数
	CPacket& operator=(const CPacket& pack) {  // 赋值运算符重载
		if (this != &pack) {  // 防止自赋值
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() {  // 返回数据包总大小（包头+长度+命令+数据+校验和）
		return nLength + 6;  // 6 = 包头2字节 + 长度4字节
	}
	const char* Data() {  // 将数据包转换为字节流，用于发送
		strOut.resize(nLength + 6);  // 分配缓冲区
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;  // 写入包头
		*(DWORD*)(pData) = nLength; pData += 4;  // 写入长度
		*(WORD*)pData = sCmd; pData += 2;  // 写入命令
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();  // 写入数据
		*(WORD*)pData = sSum;  // 写入校验和
		return strOut.c_str();  // 返回字节流
	}

public:
	WORD sHead;  // 包头标识（固定0xFEFF，2字节）
	DWORD nLength;  // 数据长度（包含命令和校验和，4字节）
	WORD sCmd;  // 命令类型（2字节）
	std::string strData;  // 实际数据内容
	WORD sSum;  // 校验和（数据部分的字节和，2字节）
	std::string strOut;  // 用于存储转换后的字节流
};
#pragma pack(pop)  // 恢复之前的内存对齐方式

typedef struct MouseEvent {  // 鼠标事件结构体
	MouseEvent() {  // 构造函数初始化
		nAction = 0;  // 鼠标动作（单击/双击/按下/放开等）
		nButton = -1;  // 鼠标按键（左键/右键/中键）
		ptXY.x = 0;  // 鼠标X坐标
		ptXY.y = 0;  // 鼠标Y坐标
	}
	WORD nAction;  // 鼠标动作（0=单击，1=双击，2=按下，3=放开）
	WORD nButton;  // 鼠标按键（0=左键，1=右键，2=中键，4=无按键）
	POINT ptXY;  // 鼠标坐标点
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {  // 文件信息结构体
	file_info() {  // 构造函数初始化
		IsInvalid = FALSE;  // 是否无效
		IsDirectory = -1;  // 是否为目录（0=文件，1=目录）
		HasNext = TRUE;  // 是否有下一个文件（0=无，1=有）
		memset(szFileName, 0, sizeof(szFileName));  // 文件名
	}
	BOOL IsInvalid;  // 文件信息是否有效
	BOOL IsDirectory;  // 是否为目录
	BOOL HasNext;  // 是否有后续文件
	char szFileName[256];  // 文件名（最大255字节）
}FILEINFO, * PFILEINFO;

class CServerSocket  // 服务器Socket类（单例模式）
{
public:
	// 获取单例实例
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  // 懒汉式初始化
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	// 初始化Socket（绑定端口并监听）
	bool InitSocket() {
		if (m_sock == -1)return false;  // 检查Socket是否有效
		sockaddr_in serv_adr;  // 服务器地址结构体
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;  // IPv4协议
		serv_adr.sin_addr.s_addr = INADDR_ANY;  // 绑定所有本地IP
		serv_adr.sin_port = htons(9527);  // 绑定端口9527（网络字节序）
		// 绑定Socket到端口
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;  // 绑定失败
		}
		// 开始监听连接（最大1个等待连接）
		if (listen(m_sock, 1) == -1) {
			return false;  // 监听失败
		}
		return true;  // 初始化成功
	}

	// 接受客户端连接
	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");  // 调试输出
		sockaddr_in client_adr;  // 客户端地址结构体
		int cli_sz = sizeof(client_adr);
		// 接受客户端连接，获取客户端Socket
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);  // 调试输出客户端Socket
		if (m_client == -1)return false;  // 接受连接失败
		return true;  // 接受连接成功
	}
#define BUFFER_SIZE 4096  // 接收缓冲区大小
	// 处理客户端命令（接收并解析数据包）
	int DealCommand() {
		if (m_client == -1)return -1;  // 客户端Socket无效
		char* buffer = new char[BUFFER_SIZE];  // 分配接收缓冲区
		if (buffer == NULL) {
			TRACE("内存溢出!\r\n");  // 内存分配失败
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);  // 清空缓冲区
		size_t index = 0;  // 缓冲区当前数据长度
		while (true) {
			// 接收客户端数据到缓冲区
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {  // 接收失败或连接关闭
				delete[]buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);  // 调试输出接收长度
			index += len;  // 更新缓冲区数据长度
			len = index;//保留 index 记录的总长度，同时用 len 接收解析后的有效数据包长度，确保缓冲区中剩余数据的处理逻辑正确。
			// 解析数据包
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {  // 解析成功
				// 移动未解析的数据到缓冲区头部
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;  // 更新剩余数据长度
				delete[]buffer;
				return m_packet.sCmd;  // 返回命令类型
			}
		}
		delete[]buffer;  // 释放缓冲区
		return -1;  // 处理失败
	}

	// 发送原始数据
	bool Send(const char* pData, int nSize) {
		if (m_client == -1)return false;  // 客户端Socket无效
		return send(m_client, pData, nSize, 0) > 0;  // 发送数据并返回结果
	}
	// 发送数据包
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;  // 客户端Socket无效
		//Dump((BYTE*)pack.Data(), pack.Size());  // 调试时打印数据包
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;  // 发送数据包
	}
	// 获取文件路径（从数据包中）
	bool GetFilePath(std::string& strPath) {
		// 命令2-4（文件操作）或9（删除文件）包含文件路径
		if (((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) || (m_packet.sCmd == 9)) {
			strPath = m_packet.strData;  // 提取路径
			return true;
		}
		return false;  // 不包含路径
	}
	// 获取鼠标事件（从数据包中）
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {  // 命令5为鼠标事件
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));  // 复制事件数据
			return true;
		}
		return false;  // 不是鼠标事件命令
	}
	CPacket& GetPacket() {  // 获取当前解析的数据包
		return m_packet;
	}
	void CloseClient() {  // 关闭客户端连接
		closesocket(m_client);
		m_client = INVALID_SOCKET;  // 标记为无效
	}
private:
	SOCKET m_client;  // 客户端Socket
	SOCKET m_sock;  // 服务器监听Socket
	CPacket m_packet;  // 当前处理的数据包
	CServerSocket& operator=(const CServerSocket& ss) {}  // 禁用赋值运算符
	CServerSocket(const CServerSocket& ss) {  // 禁用拷贝构造函数
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {  // 私有构造函数（单例模式）
		m_client = INVALID_SOCKET;  // 初始化客户端Socket为无效
		if (InitSockEnv() == FALSE) {  // 初始化Socket环境
			MessageBox(NULL, _T("无法初始化网络环境,程序即将退出!"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);  // 初始化失败则退出
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  // 创建TCP Socket
	}
	~CServerSocket() {  // 析构函数
		closesocket(m_sock);  // 关闭监听Socket
		WSACleanup();  // 清理Socket环境
	}
	// 初始化Socket环境（加载Winsock库）
	BOOL InitSockEnv() {
		WSADATA data;
		// 加载Winsock 1.1版本
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;  // 加载失败
		}
		return TRUE;  // 加载成功
	}
	// 释放单例实例
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;  // 释放实例
		}
	}
	static CServerSocket* m_instance;  // 单例实例指针
	class CHelper {  // 辅助类，用于自动释放单例
	public:
		CHelper() {  // 构造时创建单例
			CServerSocket::getInstance();
		}
		~CHelper() {  // 析构时释放单例
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;  // 辅助类实例（自动管理单例生命周期）
};