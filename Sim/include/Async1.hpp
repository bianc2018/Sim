/*
	异步接口 1
*/
#ifndef SIM_ASYNC_IO_HPP_
#define SIM_ASYNC_IO_HPP_
//SIM_ASYNC_IO_NEW
//#ifdef SIM_ASYNC_IO_NEW
#include <new>
//#endif

//平台相关
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
		#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
	#endif
	#include <stdio.h>
	#include <WinSock2.h>
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (SOCKET)(~0)
	#endif
	#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
	namespace sim
	{
		//初始化函数
		class AsyncInit
		{
		public:
			AsyncInit()
			{
				WSADATA wsaData;
				WSAStartup(MAKEWORD(2, 2), &wsaData);
			}
			~AsyncInit()
			{
				//终止 DLL 的使用
				WSACleanup();
			}
		};
	}
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif  
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	typedef int SOCKET;
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET -1
	#endif
#else
	#error "不支持的平台"
#endif

//引入日志
#ifdef USING_SIM_LOGGER
#include "Logger.hpp"
#else
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 
#define SIM_LOG_CONFIG(max_lv,handler,userdata)
#define SIM_LOG(lv,x) 
#define SIM_LDEBUG(x) 
#define SIM_LINFO(x) 
#define SIM_LWARN(x) 
#define SIM_LERROR(x) 
#endif // USING_SIM_LOGGER

//错误码
#define ASYNC_SUCCESS SOCK_SUCCESS
#define ASYNC_FAILURE SOCK_FAILURE
#define ASYNC_ERR_BASE (SOCK_ERR_BASE+100)
//服务空闲
#define ASYNC_IDLE			1
//服务为空
#define ASYNC_ERR_SERVICE			(-(ASYNC_ERR_BASE+1))
//操作超时
#define ASYNC_ERR_TIMEOUT			(-(ASYNC_ERR_BASE+2))
//目标套接字无效
#define ASYNC_ERR_SOCKET_INVAILD	(-(ASYNC_ERR_BASE+3))
//空的缓存
#define ASYNC_ERR_EMPTY_BUFF		(-(ASYNC_ERR_BASE+4))

//异步标识
//连接
#define ASYNC_FLAG_CONNECT			(1<<0)
//接收连接
#define ASYNC_FLAG_ACCEPT			(1<<1)
//接收数据
#define ASYNC_FLAG_RECV				(1<<2)
//发送
#define ASYNC_FLAG_SEND				(1<<3)
//断开链接
#define ASYNC_FLAG_DISCONNECT		(1<<4)
//错误发送
#define ASYNC_FLAG_ERROR		(1<<5)
//释放内存
#define ASYNC_FLAG_RELEASE		(1<<6)

namespace sim
{
	//网络地址
	struct NetAddr
	{
		//是否有效 false 为无效地址
		bool isusef;
		//0结尾
		char ipaddr[64];
		unsigned port;
		enum IpType
		{
			//目前只支持IPv4
			IPV4,
		};
		IpType type;
	};

	//异步传输
	struct AsyncBuffer
	{
		char*ptr;
		unsigned int size;
	};

	struct Event
	{
		//iocp 
#ifdef OS_WINDOWS
		OVERLAPPED  overlapped;
		WSABUF      wsa_buf;
#endif
		//关联sock
		SOCKET link_sock;

		//标记位
		unsigned int event_flag;

		//缓存
		AsyncBuffer cache;

		//传输的数据大小
		unsigned long long               bytes_transfered;

		//错误
		int error;

		//accept事件里面返回的客户端链接
		SOCKET accept_client;

		//用户缓存数据
		void* pdata;

		//关联地址 链接或者绑定 或者接收链接的时候有效
		NetAddr net_addr;
#ifdef USING_SIM_LOGGER
	public:
		//调试函数输出标识打印 仅仅用于日志输出
		std::string FormatFlag();
#endif
	};

	//事件回调
	typedef void(*AsyncEventHandler)(Event*e, void*pdata);

	//内存分配函数
	typedef void* (*AsyncEventMalloc)(size_t size);
	typedef void(*AsyncEventFree)(void*);

	//异步系统
	class AsyncEventService
	{
	public:
		AsyncEventService()
			:run_flag_(true) , malloc_(::malloc),free_(::free){};
		virtual ~AsyncEventService() {};
		
		//绑定一个sock
		virtual SockRet AddSocket(SOCKET socket)=0;

		//解除绑定
		virtual SockRet RemoveSocket(SOCKET socket)=0;

		//处理事件
		virtual SockRet ProcessEvent(unsigned int wait_ms)=0;
		
		//退出
		virtual SockRet Exit()=0;

		//设置内存申请函数
		virtual bool SetAlloc(AsyncEventMalloc m, AsyncEventFree f);

	protected:

	protected:
		virtual SockRet DoEvent(Event* e);

		virtual void* Malloc(size_t size);
		virtual void Free(void* p);
		//申请一个事件
		virtual Event* MallocEvent();
		virtual void FreeEvent(Event* pe);
	protected:
		bool run_flag_;
		AsyncEventMalloc malloc_;
		AsyncEventFree free_;
		//回调句柄
		AsyncEventHandler handler_; 
		void* pdata_;
	};

}