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
#ifndef NET_IOCP
    #define NET_IOCP
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
#ifndef NET_EPOLL
   #define NET_EPOLL
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
	//前置声明
	class NetSession;
	class NetManager;

	//缓存数据
	typedef sim::RefBuff NetBuffer;
	typedef sim::RefBuff NetString;
	typedef RefObject<NetSession> NetSessionPtr;
	typedef RefObject<NetManager> NetManagerPtr;

	//事件类型
	enum NetType
	{
		//连接建立事件
		NetTypeConnect,

		//接受连接事件
		NetTypeAccept,

		//接收TCP数据事件
		NetTypeRecv,

		//接收UDP数据事件（多了地址信息）
		NetTypeRecvFrom,

		//数据发送事件
		NetTypeSend,

		//连接关闭事件
		NetTypeClose,
	};

	//网络地址
	class NetAddress
	{
		NetString ip;
		unsigned port;
	};

	//会话基类
	class NetSession :public sim::EnableRefFormThis<NetSession>
	{
		friend class NetManager;

		NetSession(WeakObject<NetManager> net_manager);
	public:
		virtual NetSessionPtr NewSession(WeakObject<NetManager> net_manager) = 0;
		virtual ~NetSession() {};
	public:
		//回调
		//接收连接回调
		virtual void OnAccept(NetSock client) = 0;

		//连接完成回调
		virtual void OnConnect() = 0;

		//tcp数据接收回调
		virtual void OnRecvData(const NetBuffer&buff) = 0;

		//udp数据接收回调
		virtual void OnRecvDataFrom(const NetBuffer& buff,
			const NetAddress&address) = 0;

		//数据发送完成回调
		virtual void OnSendComplete(const NetBuffer& buff) = 0;

		//连接关闭
		virtual void OnClose() = 0;
	public:
		//接口
		virtual bool StartConnect(const NetAddress&address) = 0;

		virtual bool StartAccept(const NetAddress& address, int backlog = 1024) = 0;

		virtual bool StopAccept() = 0;

		virtual bool StartRecv(NetBuffer buff = NULL) = 0;

		virtual bool StartRecvForm(NetBuffer buff = NULL) = 0;

		virtual bool StopRecv() = 0;

		virtual bool Send(const NetBuffer& buff) = 0;

		virtual bool SendTo(const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close() = 0;

	private:

	private:
		NetSock sock_;
		bool bind_flag_, connect_flag_, accept_flag_, recv_flag_;
		WeakObject<NetManager> net_manager_;
	};

	//网络管理基类
	class NetManager :public sim::EnableRefFormThis<NetManager>
	{
		friend class NetSession;

		NetManager();
	public:
		virtual ~NetManager() {};

		virtual NetManagerPtr NewManager() = 0;

		//事件循环 wait_ms最大等待时间
		virtual int Poll(unsigned wait_ms) = 0;

	protected:
		//具体功能实现
		virtual bool StartConnect(NetSessionPtr ss, const NetAddress&address) = 0;

		virtual bool Accept(NetSessionPtr ss, const NetAddress& address, int backlog = 1024) = 0;

		virtual bool Recv(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool RecvForm(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool Send(NetSessionPtr ss, const NetBuffer& buff) = 0;

		virtual bool SendTo(NetSessionPtr ss, const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close(NetSessionPtr ss) = 0;
	private:

	};

#ifdef NET_IOCP

	//wsa拓展函数加载对象
	class WsaExFunction
	{
	public:
		WsaExFunction()
		{
			static NetWsInit init;

			//创建一个空的socket
			SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

			//加载拓展函数
			__AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
			__ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
			__AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
			__DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);

			//关闭连接
			closesocket(socket);
		}
	private:
		//加载拓展函数
		static void* _GetExFunctnion(const SOCKET &socket, const GUID& which)
		{
			void* func = nullptr;
			DWORD bytes = 0;
			WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&which,
				sizeof(which), &func, sizeof(func), &bytes, NULL, NULL);

			return func;
		}
	public:
		LPFN_ACCEPTEX                __AcceptEx;
		LPFN_CONNECTEX               __ConnectEx;
		LPFN_GETACCEPTEXSOCKADDRS    __AcceptExScokAddrs;
		LPFN_DISCONNECTEX            __DisconnectionEx;
	};

	//重叠对象
	struct IocpOverlapped
	{
		//IO重叠对象
		OVERLAPPED  overlapped;

		NetType type;

		//子连接存在 acceptex时候启用
		NetSock accepted;

		//缓存
		NetBuffer buff;
		//buff的数据引用
		WSABUF wsa_buf;

		//传输的字节数目
		DWORD bytes_transfered;

		//地址 用于SendTo or Recvfrom
		struct sockaddr_in temp_addr;
		int temp_addr_len;

		//会话引用
		NetSessionPtr ss;
	};

	typedef NetSession IocpNetSession;

	class IocpNetManager :public NetManager
	{
		IocpNetManager();
	public:
		virtual ~IocpNetManager();

		virtual NetManagerPtr NewManager();

		//事件循环 wait_ms最大等待时间
		virtual int Poll(unsigned wait_ms);

	protected:
		//具体功能实现
		virtual bool StartConnect(NetSessionPtr ss, const NetAddress&address);

		virtual bool Accept(NetSessionPtr ss, const NetAddress& address, int backlog = 1024);

		virtual bool Recv(NetSessionPtr ss, NetBuffer buff = NULL);

		virtual bool RecvForm(NetSessionPtr ss, NetBuffer buff = NULL);

		virtual bool Send(NetSessionPtr ss, const NetBuffer& buff);

		virtual bool SendTo(NetSessionPtr ss, const NetBuffer& buff, const NetAddress& address);

		virtual bool Close(NetSessionPtr ss);
	protected:
		//加载拓展函数
		WsaExFunction &LoadWsaExFunction()
		{
			//加载wsa拓展函数，保证只加载一次
			static WsaExFunction ex_func;
			return ex_func;
		}

	private:
		HANDLE iocp_handler_;
	};

	typedef IocpNetSession SimNetSession;
	typedef IocpNetManager SimNetManager;
#endif
}
#endif // SIM_NET_HPP_
