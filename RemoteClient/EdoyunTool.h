#pragma once
#include <Windows.h>  // 包含Windows API头文件，以使用Windows特定的数据类型和函数
#include <string>  // 包含C++标准库中的string头文件，以使用std::string等字符串相关功能
#include <atlimage.h>  // 包含ATL图像处理头文件，以使用CImage类进行图像操作
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
	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();  // 获取数据包数据
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 分配全局内存
		if (hMem == NULL) {  // 内存分配失败
			TRACE("内存不足\r\n");  // 输出错误信息
			Sleep(1);  // 休眠1毫秒
			return -1;  // 继续下一次循环
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 创建流对象
		if (hRet == S_OK) {  // 流对象创建成功
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);  // 写入数据到流
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 定位到流开始位置
			if ((HBITMAP)image != NULL)  // 如果图像已存在
				image.Destroy();  // 销毁图像
			image.Load(pStream);  // 从流加载图像
		}
		return hRet;
	}
};

