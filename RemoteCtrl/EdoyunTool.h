#pragma once
class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize)  // 定义数据打印函数，用于调试
	{
		std::string strOut;  // 存储要输出的字符串
		for (size_t i = 0; i < nSize; i++)  // 遍历数据
		{
			char buf[8] = "";  // 存储单个字节的十六进制表示
			if (i > 0 && (i % 16 == 0))strOut += "\n";  // 每16个字节换行
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);  // 将字节转换为十六进制字符串
			strOut += buf;  // 拼接字符串
		}
		strOut += "\n";  // 最后加一个换行
		OutputDebugStringA(strOut.c_str());  // 输出调试信息 
	}
	static bool Init()
	{
		HMODULE hModule = ::GetModuleHandle(nullptr);  // 获取当前模块的句柄，若为 nullptr 表示获取失败
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");  // 输出获取模块句柄失败的错误信息
			return false;  // 返回 false 表示初始化失败
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))  // 初始化 MFC 库，若返回 false 表示初始化失败
		{
			wprintf(L"错误: MFC 初始化失败\n");  // 输出 MFC 初始化失败的错误信息
			return false;  // 返回 false 表示初始化失败
		}
		return true;
	}

};

