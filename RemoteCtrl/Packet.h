#pragma once
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