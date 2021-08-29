/*基础网络模块*/
#ifndef SIM_NET_HPP_
#define SIM_NET_HPP_

#include <stdio.h>
#include "RefObject.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
    #define OS_WINDOWS
#endif
//windows平台使用iocp
#ifndef ASYNC_IOCP
    #define ASYNC_IOCP
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
    #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif


#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN  
#endif
#include <WinSock2.h>
//使用拓展接口
#include <winsock2.h>
#include <MSWSock.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
namespace sim
{
    //初始化函数
    class NetWsInit
    {
    public:
        NetWsInit()
        {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }
        ~NetWsInit()
        {
            //终止 DLL 的使用
            WSACleanup();
            }
        };
}

typedef SOCKET NetSock;

#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif  
//引入linux平台接口
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
//linux使用epoll
#ifndef ASYNC_EPOLL
   #define ASYNC_EPOLL
#endif
typedef int NetSock;
#ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
#endif
#else
#error "不支持的平台"
#endif

namespace sim
{
	//缓存数据
	typedef sim::RefBuff NetBuffer;

	//网络地址
	class NetAddress
	{
		NetBuffer ip;
		unsigned port;
	};

	//会话基类
	class NetSession
	{
	public:
		NetSession();
		~NetSession();
    public:
        //回调
		//接收连接回调
		virtual void OnAccept(NetSock client)=0;

		//连接完成回调
		virtual void OnConnect()=0;

		//tcp数据接收回调
		virtual void OnRecvData(const NetBuffer&buff)=0;

		//udp数据接收回调
		virtual void OnRecvDataFrom(const NetBuffer& buff,
			const NetAddress&address)=0;

		//数据发送完成回调
		virtual void OnSendComplete(const NetBuffer& buff);
    public:
        //接口
		virtual bool Connect(const NetAddress&address);

		virtual bool Bind(const NetAddress& address);

		virtual bool Listen(int backlog);

		virtual bool StartAccept();

		virtual bool Send(const NetBuffer& buff);

		virtual bool SendTo(const NetBuffer& buff, const NetAddress& address);

		virtual bool Close()=0;
	private:

	};

	//网络管理基类
	class NetManager
	{
	public:
		NetManager();
		~NetManager();

	private:

	};
}

#endif // SIM_NET_HPP_
