/*
* ȫ�ֺ궨�壬��Ҫ��һЩȫ�ֵ�״̬��
*/
#ifndef SIM_MACRO_DEF_HPP_
#define SIM_MACRO_DEF_HPP_
#include <stdio.h>

#ifdef __APPLE__
// ��MacOSϵͳ��ִ�еĴ���
#elif __linux__
  // ��Linuxϵͳ��ִ�еĴ���
#elif _WIN32
    // ��Windowsϵͳ��ִ�еĴ���
    #ifndef SIM_OS_WINDOWS
        #define SIM_OS_WINDOWS
    #endif
    #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
        #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
    #endif
#endif

#ifdef __GNUC__
  // ��GCC��������ִ�еĴ���
#elif _MSC_VER
  // ��VC++��������ִ�еĴ���
#endif

#if __cplusplus == 199711L
  // C++98/03����
#elif __cplusplus == 201103L
  // C++11����
#elif __cplusplus == 201402L
  // C++14����
#elif __cplusplus == 201703L
  // C++17����
#elif __cplusplus > 201703L
  // ���߰汾��C++����
#endif

#if _MSC_VER == 1500
  // VS2008����
#elif _MSC_VER == 1900
  // VS2015����
#elif _MSC_VER == 1910
  // VS2017����
#elif _MSC_VER == 1920
  // VS2019����
#endif

#endif // !SIM_MACRO_DEF_HPP_
