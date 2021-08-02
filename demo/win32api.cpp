//平台定义宏
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
#define OS_WINDOWS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  
#endif
#define UNICODE
#include <Windows.h>

#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif
#else
#error "不支持的平台"
#endif

#ifndef OS_WINDOWS
int main(int argc, char* argv[])
{
	return -1;
}
#else
int main(int argc, char* argv[])
{
	//Mess
	//MessageBox();
	return 0;
}
#endif