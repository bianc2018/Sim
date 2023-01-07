/*
* 全局宏定义，主要订一些全局的状态宏
*/
#ifndef SIM_MACRO_DEF_HPP_
#define SIM_MACRO_DEF_HPP_
#include <stdio.h>

#ifdef __APPLE__
// 在MacOS系统上执行的代码
#elif __linux__
  // 在Linux系统上执行的代码
#elif _WIN32
    // 在Windows系统上执行的代码
    #ifndef SIM_OS_WINDOWS
        #define SIM_OS_WINDOWS
    #endif
    #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
        #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
    #endif
#endif

#ifdef __GNUC__
  // 在GCC编译器下执行的代码
#elif _MSC_VER
  // 在VC++编译器下执行的代码
#endif

#if __cplusplus == 199711L
  // C++98/03代码
#elif __cplusplus == 201103L
  // C++11代码
#elif __cplusplus == 201402L
  // C++14代码
#elif __cplusplus == 201703L
  // C++17代码
#elif __cplusplus > 201703L
  // 更高版本的C++代码
#endif

#if _MSC_VER == 1500
  // VS2008代码
#elif _MSC_VER == 1900
  // VS2015代码
#elif _MSC_VER == 1910
  // VS2017代码
#elif _MSC_VER == 1920
  // VS2019代码
#endif

#endif // !SIM_MACRO_DEF_HPP_
