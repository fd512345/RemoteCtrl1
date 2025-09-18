#pragma once  // 防止头文件被多次包含
#include <Windows.h>  // 包含Windows API头文件
#include <string>  // 包含字符串处理头文件
#include <atlimage.h>  // 包含ATL图像处理类头文件


class CEdoyunTool  // 自定义工具类，提供数据转换和调试输出功能
{
public:
	// 以十六进制形式输出字节数据到调试窗口
	static void Dump(BYTE* pData, size_t nSize)
	{
		std::string strOut;  // 存储十六进制字符串
		for (size_t i = 0; i < nSize; i++)  // 遍历每个字节
		{
			char buf[8] = "";  // 临时缓冲区，存储单个字节的十六进制表示
			if (i > 0 && (i % 16 == 0))strOut += "\n";  // 每16个字节换行
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);  // 转换为两位十六进制字符串
			strOut += buf;  // 拼接至输出字符串
		}
		strOut += "\n";  // 末尾添加换行
		OutputDebugStringA(strOut.c_str());  // 输出到调试窗口
	}

	// 将字节缓冲区转换为CImage图像对象
	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();  // 获取字符串缓冲区的字节指针
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 分配可移动的全局内存块
		if (hMem == NULL) {  // 内存分配失败
			TRACE("内存不足了！\r\n");  // 输出调试信息
			Sleep(1);  // 短暂休眠
			return -1;  // 返回错误码
		}
		IStream* pStream = NULL;  // 定义流对象指针
		// 在全局内存上创建流对象
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {  // 流对象创建成功
			ULONG length = 0;  // 记录实际写入的字节数
			// 将字节数据写入流
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 };  // 用于设置流的位置
			// 将流指针移动到起始位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL)  // 如果图像对象已有位图
				image.Destroy();  // 销毁现有位图
			image.Load(pStream);  // 从流加载图像
		}
		return hRet;  // 返回操作结果（HRESULT）
	}
};