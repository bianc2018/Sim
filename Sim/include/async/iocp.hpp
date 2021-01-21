/*
	异步socket 由iocp封装
*/
#ifndef SIM_IOCP_HPP_
#define SIM_IOCP_HPP_
#include "Async.hpp"

#include <winsock2.h>
#include <MSWSock.h>
#include <new>
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif
namespace sim
{
	//声明
	class IocpAsyncSocket;

	enum STATE_CODE {
		EXIT_IOCP = 0xFFFFFFFF,
		WEAK_UP_IOCP = 0xAAAAFFFF,
	};

	//wsa拓展函数
	class WsaExFunction
	{
	public:
		WsaExFunction();

		static void* _GetExFunctnion(const SOCKET &socket, const GUID& which);
	public:
		LPFN_ACCEPTEX                __AcceptEx;
		LPFN_CONNECTEX               __ConnectEx;
		LPFN_GETACCEPTEXSOCKADDRS    __AcceptExScokAddrs;
		LPFN_DISCONNECTEX            __DisconnectionEx;
	};

	//CONTAINING_RECORD
	struct IocpAsyncEvent :public Event
	{
		OVERLAPPED  overlapped;
		WSABUF      wsa_buf;
		IocpAsyncEvent()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	class IocpAsyncEventService :public BaseAsyncEventService
	{
	public:
		IocpAsyncEventService(unsigned int thread_num = 0);
		virtual ~IocpAsyncEventService();

		//新增一个套接字
		virtual BaseAsyncSocket* CreateBySocket(SOCKET socket);
		virtual BaseAsyncSocket* CreateByType(SockType type);
		virtual BaseAsyncSocket* Create(int af, int type, int protocol);

		//释放
		virtual SockRet Release(BaseAsyncSocket*psock);

		//运行
		//virtual SockRet Run(unsigned int wait_ms);
		//运行 运行一次
		virtual SockRet RunOnce(unsigned int wait_ms);

		//退出
		virtual SockRet Exit();

		//加载拓展函数,加载失败返回空指针，然后终止运行
		static WsaExFunction LoadWsaExFunction();

	//protected:
		virtual Event *MallocEvent()
		{
			IocpAsyncEvent*pe = (IocpAsyncEvent*)Malloc(sizeof(IocpAsyncEvent));
			if (pe)
			{
				//调用构造
				pe = new(pe)IocpAsyncEvent();
			}
			return pe;
		}
		virtual void FreeEvent(Event *pe)
		{
			if (pe)
			{
				IocpAsyncEvent *p = (IocpAsyncEvent*)pe;
				p->~IocpAsyncEvent();
				Free(pe);
			}
		}
	private:
		//新增
		virtual SockRet Add(BaseAsyncSocket*psock);
		
		//移除
		virtual SockRet Remove(BaseAsyncSocket*psock);
		
	private:
		HANDLE                iocp_handler_;
	
	};

	//class BaseAsyncSocket;
	class IocpAsyncSocket:public BaseAsyncSocket
	{
		friend class IocpAsyncEventService;
	protected:
		//BaseAsyncEventService
		IocpAsyncSocket(BaseAsyncEventService* service, SOCKET sock)
			:BaseAsyncSocket(service,sock), bind_flag_(false)
		{}
		// SOCK_STREAM tcp SOCK_DGRAM udp
		IocpAsyncSocket(BaseAsyncEventService* service, SockType type)
			:BaseAsyncSocket(service, type), bind_flag_(false)
		{}

		IocpAsyncSocket(BaseAsyncEventService* service, int af, int type, int protocol)
			:BaseAsyncSocket(service,af, type, protocol), bind_flag_(false)
		{}
	
	public:
		virtual SockRet Connect(const char* ipaddr, unsigned short port);

		virtual SockRet Bind(const char* ipaddr, unsigned short port);

		virtual SockRet Send(const char* data, unsigned int data_len);

		virtual SockRet Recv(char* data, unsigned int data_len);

		/*virtual Socket Accept();

		virtual Socket Accept(char* remote_ip, unsigned int ip_len,
			unsigned short* remote_port);

		virtual SockRet Send(const char* data, unsigned int data_len);

		virtual SockRet SendTo(const char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port);

		virtual SockRet Recv(char* data, unsigned int data_len);

		virtual SockRet Recvfrom(char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port);

		virtual SockRet Close();*/
	protected:
		//触发回调
		virtual bool CallHandler(Event *e);
	private:
		bool bind_flag_;
	};

	sim::WsaExFunction::WsaExFunction()
	{
		Socket::Init();
		SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		__AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
		__ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
		__AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
		__DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);
		closesocket(socket);
	}

	inline void * WsaExFunction::_GetExFunctnion(const SOCKET & socket, const GUID & which) 
	{
		void* func = nullptr;
		DWORD bytes = 0;
		WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&which,
			sizeof(which), &func, sizeof(func), &bytes, NULL, NULL);

		return func;
	}

	sim::IocpAsyncEventService::IocpAsyncEventService(unsigned int thread_num)
		:iocp_handler_(INVALID_HANDLE_VALUE)
	{
		//初始化
		Socket::Init();
		iocp_handler_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, thread_num);
	}
	inline IocpAsyncEventService::~IocpAsyncEventService()
	{
		//反初始化
		Exit();
		CloseHandle(iocp_handler_);
		iocp_handler_ = INVALID_HANDLE_VALUE;
	}
	inline BaseAsyncSocket * IocpAsyncEventService::CreateBySocket(SOCKET socket)
	{
		//申请缓存
		void*ps = Malloc(sizeof(IocpAsyncSocket));
		if (NULL == ps)
			return NULL;//申请失败
		//调用构造
		BaseAsyncSocket*pbase = (BaseAsyncSocket*)(new(ps)IocpAsyncSocket(this,socket));
		if (0 == Add(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		Release(pbase);
		return NULL;
	}
	inline BaseAsyncSocket * IocpAsyncEventService::CreateByType(SockType type)
	{
		//申请缓存
		void*ps = Malloc(sizeof(IocpAsyncSocket));
		if (NULL == ps)
			return NULL;//申请失败
		//调用构造
		BaseAsyncSocket*pbase = (BaseAsyncSocket*)(new(ps)IocpAsyncSocket(this,type));
		if (0 == Add(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		Release(pbase);
		return NULL;
	}
	inline BaseAsyncSocket * IocpAsyncEventService::Create(int af, int type, int protocol)
	{
		//申请缓存
		void*ps = Malloc(sizeof(IocpAsyncSocket));
		if (NULL == ps)
			return NULL;//申请失败
		//调用构造
		BaseAsyncSocket*pbase = (BaseAsyncSocket*)(new(ps)IocpAsyncSocket(this,af, type, protocol));
		if (0 == Add(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		Release(pbase);
		return NULL;
	}
	inline SockRet IocpAsyncEventService::Release(BaseAsyncSocket * psock)
	{
		if (NULL == psock)
			return SockRet(-1);

		IocpAsyncEvent *pe = (IocpAsyncEvent *)(MallocEvent());
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_RELEASE;
		return TRUE == CancelIoEx((HANDLE)psock->GetSocket(), lapped) ? 0 : 1;
		
	}
	
	//0 成功 -1 已退出 -2 超时
	inline SockRet IocpAsyncEventService::RunOnce(unsigned int wait_ms)
	{
		DWORD               bytes_transfered = 0;
		IocpAsyncSocket     *socket= nullptr;
		OVERLAPPED          *over_lapped = nullptr;
		//取数据
		int res = GetQueuedCompletionStatus(iocp_handler_,
			&bytes_transfered, PULONG_PTR(&socket),
			&over_lapped, wait_ms);

		DWORD dw_err = 0;
		SockRet ret = ASYNC_FAILURE;
		if (res) 
		{
			//退出事件
			if ((PULONG_PTR)socket == (PULONG_PTR)EXIT_IOCP) 
			{
				run_flag_ = false;
				return ASYNC_SUCCESS;
			}
			else
			{
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					socket_event->bytes_transfered = bytes_transfered;
					//传输的数据为空，连接断开了
					if (0 == bytes_transfered
						&& (socket_event->event_flag&ASYNC_FLAG_SEND
							|| socket_event->event_flag&ASYNC_FLAG_RECV))
					{
						socket_event->event_flag |= ASYNC_FLAG_DISCONNECT;
						//socket_event->event_flag |= ASYNC_FLAG_ERROR;
					}
					if (socket)
						socket->CallHandler((Event*)socket_event);
					ret = ASYNC_SUCCESS;
				}
			}

		}
		else 
		{
			dw_err = GetLastError();
			// timer out event
			if (dw_err == WAIT_TIMEOUT) 
			{
				ret = ASYNC_IDLE;
			}
			else if (ERROR_NETNAME_DELETED == dw_err 
				|| NO_ERROR == dw_err 
				|| ERROR_IO_PENDING == dw_err) 
			{
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event =\
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					socket_event->bytes_transfered = bytes_transfered;
					if (socket)
						socket->CallHandler((Event*)socket_event);
					ret = ASYNC_SUCCESS;
				}
			}
			else if (ERROR_CONNECTION_REFUSED == dw_err 
				|| ERROR_SEM_TIMEOUT == dw_err || 
				WSAENOTCONN == dw_err 
				|| ERROR_OPERATION_ABORTED == dw_err) 
			{
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					socket_event->event_flag |= ASYNC_FLAG_ERROR;
					socket_event->error= ret = dw_err;
					socket_event->bytes_transfered = bytes_transfered;
					if (socket)
						socket->CallHandler((Event*)socket_event);
					
				}
			}
			else 
			{
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					socket_event->event_flag |= ASYNC_FLAG_ERROR|ASYNC_FLAG_DISCONNECT;
					socket_event->error = ret = dw_err;
					socket_event->bytes_transfered = bytes_transfered;
					if (socket)
						socket->CallHandler((Event*)socket_event);

				}
				printf("GetQueuedCompletionStatus error %d\n", dw_err);
			}
		}

		if (over_lapped)
		{
			IocpAsyncEvent* socket_event = CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
			if (socket_event->event_flag&ASYNC_FLAG_RELEASE)
				Remove(socket);
			//释放
			FreeEvent(socket_event);

		}
		return ret;
	}
	inline SockRet IocpAsyncEventService::Exit()
	{
		PostQueuedCompletionStatus(iocp_handler_, 0, EXIT_IOCP, nullptr);
		return BaseAsyncEventService::Exit();
	}
	inline WsaExFunction IocpAsyncEventService::LoadWsaExFunction()
	{
		static WsaExFunction exfunc;
		return exfunc;
	}
	inline SockRet IocpAsyncEventService::Add(BaseAsyncSocket * psock)
	{
		if (CreateIoCompletionPort((HANDLE)psock->GetSocket(), iocp_handler_, (ULONG_PTR)psock, 0) == NULL)
		{
			return -1;
		}
		return 0;
	}
	inline SockRet IocpAsyncEventService::Remove(BaseAsyncSocket * psock)
	{
		IocpAsyncSocket*piocp = (IocpAsyncSocket*)psock;
		//析构
		piocp->~IocpAsyncSocket();
		Free(piocp);
		return SockRet(0);
	}
	
	bool sim::IocpAsyncSocket::CallHandler(Event * e)
	{
		bool ret= BaseAsyncSocket::CallHandler(e);
		//异常关闭
		/*if (e->event_flag&ASYNC_FLAG_DISCONNECT )
			Close();*/
		return ret;
	}
	SockRet sim::IocpAsyncSocket::Connect(const char * ipaddr, unsigned short port)
	{
		if (NULL == pevent_)
			return -1;

		IocpAsyncEventService*ps = (IocpAsyncEventService*)pevent_;

		//创建sockaddr_in结构体变量
		struct sockaddr_in addr;
		if (!IpToAddressV4(ipaddr, port, &addr))
			return -1;

		if (!bind_flag_)
			Bind(NULL, 0);//随机绑定一个

		DWORD dwBytesSent=0;
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent());
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_CONNECT;
		WsaExFunction exfunc = ps->LoadWsaExFunction();
		int res = exfunc.__ConnectEx(GetSocket(),
			(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, &dwBytesSent, lapped);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncSocket::Bind(const char* ipaddr, unsigned short port)
	{
		SockRet ret = BaseAsyncSocket::Bind(ipaddr, port);
		if(ret == ASYNC_SUCCESS)
			bind_flag_ = true;
		return ret;
	}
	inline SockRet IocpAsyncSocket::Send(const char * data, unsigned int data_len)
	{
		if (NULL == pevent_)
			return -1;

		IocpAsyncEventService*ps = (IocpAsyncEventService*)pevent_;

		DWORD dwBytesSent = 0;
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent());
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_SEND;
		pe->cache.ptr = (char*)data;
		pe->cache.size = data_len;
		pe->wsa_buf.buf = pe->cache.ptr;
		pe->wsa_buf.len = pe->cache.size;
		int res = WSASend(GetSocket(), &pe->wsa_buf, 1, nullptr, 0, lapped, nullptr);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncSocket::Recv(char * data, unsigned int data_len)
	{
		if (NULL == pevent_)
			return -1;

		IocpAsyncEventService*ps = (IocpAsyncEventService*)pevent_;

		DWORD dwBytesSent = 0;
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent());
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_RECV;
		pe->cache.ptr = (char*)data;
		pe->cache.size = data_len;
		pe->wsa_buf.buf = pe->cache.ptr;
		pe->wsa_buf.len = pe->cache.size;
		int res = WSARecv(GetSocket(), &pe->wsa_buf, 1,  &dwBytes, &dwFlags, lapped, nullptr);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
	}
}
#endif