/*
	字符编码转换
	https://github.com/fhy1013/code_convert/blob/master/code_convert.cpp
*/
#ifndef SIM_CODE_CONVERT_HPP_
#define SIM_CODE_CONVERT_HPP_

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
	#define OS_WINDOWS
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

#include <stdio.h>
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN  
#endif
#include <windows.h>
namespace sim
{

	std::string ToUtf8(const std::string& src_str)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, src_str.c_str(), -1, NULL, 0);
		wchar_t* wstr = new wchar_t[len + 1];
		memset(wstr, 0, len + 1);
		MultiByteToWideChar(CP_ACP, 0, src_str.c_str(), -1, wstr, len);
		len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* str = new char[len + 1];
		memset(str, 0, len + 1);
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
		std::string strTemp = str;
		if (wstr) delete[] wstr;
		if (str) delete[] str;
		return strTemp;
	}

	std::string FromUtf8(const std::string& src_str) 
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, src_str.c_str(), -1, NULL, 0);
		wchar_t* wszGBK = new wchar_t[len + 1];
		memset(wszGBK, 0, len * 2 + 2);
		MultiByteToWideChar(CP_UTF8, 0, src_str.c_str(), -1, wszGBK, len);
		len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
		char* szGBK = new char[len + 1];
		memset(szGBK, 0, len + 1);
		WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
		std::string strTemp(szGBK);
		if (wszGBK) delete[] wszGBK;
		if (szGBK) delete[] szGBK;
		return strTemp;
	}
}
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <iconv.h>
namespace sim
{
	std::string Convert(const std::string& src_str, const std::string& charset_src, const std::string& charset_dst)
	{
		//return src_str;

		printf("Convert:%s\n", src_str.c_str());
		iconv_t stCvt;
		stCvt = iconv_open(charset_src.c_str(), charset_dst.c_str());
		if (stCvt == 0)
			return "";

		// copy the string to a buffer as iconv function requires a non-const char
	// pointer.
		std::vector<char> in_buf(src_str.begin(), src_str.end());
		char* src_ptr = &in_buf[0];
		size_t src_size = src_str.size();

		std::vector<char> buf(1024);
		std::string dst;
		while (0 < src_size) {
			char* dst_ptr = &buf[0];
			size_t dst_size = buf.size();
			size_t res = ::iconv(stCvt, &src_ptr, &src_size, &dst_ptr, &dst_size);
			if (res == (size_t)-1) {
				if (errno == E2BIG) {
					// ignore this error
					printf("E2BIG\n");
				}
				else if (1) {
					// skip character
					++src_ptr;
					--src_size;
					printf("skip character %d\n", errno);
				}
				else 
				{
					iconv_close(stCvt);
					return "";
				}
			}
			dst.append(&buf[0], buf.size() - dst_size);
		}
		iconv_close(stCvt);
		return dst;
	}

	//转换为Utf-8
	std::string ToUtf8(const std::string& src_str)
	{
		//return Convert(src_str, "gbk", "utf-8");
		return src_str;
	}

	//utf8转换为本地编码
	std::string FromUtf8(const std::string& src_str) 
	{
		//return Convert(src_str, "utf-8", "gbk");
		return src_str;
	}
}
#endif
#endif