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

		virtual BaseAsyncSocket* CreateRef(BaseAsyncSocket* psock);

		//摧毁 释放所有资源
		virtual SockRet Destroy(BaseAsyncSocket** psock);

		virtual SockRet RunOnce(unsigned int wait_ms);

		//退出
		virtual SockRet Exit();

		//加载拓展函数,加载失败返回空指针，然后终止运行
		static WsaExFunction LoadWsaExFunction();
		virtual Event *MallocEvent(BaseAsyncSocket* psock);
		virtual void FreeEvent(BaseAsyncSocket* psock, Event* pe);
	private:
		//新增
		virtual SockRet AddToIocp(BaseAsyncSocket*psock);
		
		//缓存释放
		virtual SockRet DeleteSock(BaseAsyncSocket*psock);
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
			:BaseAsyncSocket(service,sock), 
			bind_flag_(false), iocp_handler_(NULL)
			, refCount_(0)
		{}
		// SOCK_STREAM tcp SOCK_DGRAM udp
		IocpAsyncSocket(BaseAsyncEventService* service, SockType type)
			:BaseAsyncSocket(service, type), bind_flag_(false)
			, iocp_handler_(NULL), refCount_(0)
		{}

		IocpAsyncSocket(BaseAsyncEventService* service, int af, int type, int protocol)
			:BaseAsyncSocket(service,af, type, protocol), bind_flag_(false), 
			iocp_handler_(NULL)
			, refCount_(0)
		{}
	
	public:
		virtual SockRet Connect(const char* ipaddr, unsigned short port);

		virtual SockRet Bind(const char* ipaddr, unsigned short port);

		virtual SockRet Send(const char* data, unsigned int data_len);

		virtual SockRet Recv(char* data, unsigned int data_len);

		//关闭
		virtual SockRet Close();

		//接收链接
		virtual SockRet Accept(Socket* s);

		virtual SockRet Accept(Socket* s, char* remote_ip, unsigned int ip_len,
			unsigned short* remote_port);

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
		
		void SetIocpHandler(HANDLE iocp_handler)
		{
			iocp_handler_ = iocp_handler;
		}

		//断开链接
		virtual SockRet DisConnect();

		LONG addRef(void)
		{
			return ::InterlockedIncrement(&refCount_);
		}

		LONG decRef(void)
		{
			return ::InterlockedDecrement(&refCount_);
		}

		LONG getRefCount(void) const
		{
			return refCount_;
		}
	private:
		bool bind_flag_;
		HANDLE iocp_handler_;

		//引用对象
		LONG volatile refCount_;
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
		if (0 == AddToIocp(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		DeleteSock(pbase);
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
		if (0 == AddToIocp(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		DeleteSock(pbase);
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
		if (0 == AddToIocp(pbase))
		{
			//成功
			return pbase;
		}
		//失败释放
		DeleteSock(pbase);
		return NULL;
	}
	inline BaseAsyncSocket* IocpAsyncEventService::CreateRef(BaseAsyncSocket* psock)
	{
		SIM_FUNC_DEBUG();
		if (NULL == psock)
		{
			SIM_LERROR("cannt create ref by NULL");
			return NULL;
		}
		IocpAsyncSocket* temp = (IocpAsyncSocket*)psock;
		temp->addRef();
		return temp;
	}
	/*inline SockRet IocpAsyncEventService::Release(BaseAsyncSocket * psock)
	{
		if (NULL == psock)
			return SockRet(-1);

		IocpAsyncEvent *pe = (IocpAsyncEvent *)(MallocEvent());
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_RELEASE;
		return TRUE == CancelIoEx((HANDLE)psock->GetSocket(), lapped) ? 0 : 1;
		
	}*/
	
	inline SockRet IocpAsyncEventService::RunOnce(unsigned int wait_ms)
	{
		SIM_FUNC_DEBUG();

		DWORD               bytes_transfered = 0;
		IocpAsyncSocket     *socket= nullptr;
		OVERLAPPED          *over_lapped = nullptr;
		//取数据
		int res = GetQueuedCompletionStatus(iocp_handler_,
			&bytes_transfered, PULONG_PTR(&socket),
			&over_lapped, wait_ms);
		
		DWORD dw_err =  GetLastError();
		SIM_LDEBUG("GetQueuedCompletionStatus res=" << res << " GetLastError=" 
			<< dw_err);
		SockRet ret = ASYNC_FAILURE;
		if (res) 
		{
			//退出事件
			if ((PULONG_PTR)socket == (PULONG_PTR)EXIT_IOCP) 
			{
				run_flag_ = false;
				SIM_LWARN("Iocp is Exit");
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
						SIM_LWARN("Send or Recv bytes_transfered is 0  DISCONNECT");
						socket_event->event_flag |= ASYNC_FLAG_ERROR;
						socket_event->error = dw_err;
						//socket_event->event_flag |= ASYNC_FLAG_ERROR;
					}
					if (socket)
						socket->CallHandler((Event*)socket_event);
					ret = ASYNC_SUCCESS;
				}
				else
				{
					SIM_LWARN("over_lapped is Empty");
				}
			}

		}
		else 
		{
			
			// timer out event
			if (dw_err == WAIT_TIMEOUT) 
			{
				SIM_LDEBUG("Iocp " << iocp_handler_ << " wait timeout " << wait_ms << " ms");
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
					SIM_LDEBUG("Iocp " << iocp_handler_ << " get a event " << socket_event->FormatFlag());
					socket_event->bytes_transfered = bytes_transfered;
					if (socket)
						socket->CallHandler((Event*)socket_event);
					ret = ASYNC_SUCCESS;
				}
				else
				{
					SIM_LWARN("over_lapped is Empty");
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
					SIM_LERROR("Iocp " << iocp_handler_ << " get a error event " << socket_event->FormatFlag()<<" dw_err "<<dw_err);
					socket_event->event_flag |= ASYNC_FLAG_ERROR;
					socket_event->error= ret = dw_err;
					socket_event->bytes_transfered = bytes_transfered;
					if (socket)
						socket->CallHandler((Event*)socket_event);
					
				}
				else
				{
					SIM_LWARN("over_lapped is Empty");
				}
			}
			else 
			{
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event = CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					SIM_LERROR("socket_event " << socket_event->FormatFlag()<<" error "<<(void*)socket);
				}
				SIM_LERROR("GetQueuedCompletionStatus error "<< dw_err);
			}
		}

		if (over_lapped)
		{
			IocpAsyncEvent* socket_event = CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
			/*if (socket_event->event_flag&ASYNC_FLAG_DISCONNECT)
			{
				SIM_LDEBUG("Iocp " << iocp_handler_ << " DeleteSock " << (void*)socket);
				DeleteSock(socket);
			}*/
			//释放
			FreeEvent((BaseAsyncSocket*)socket,socket_event);

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
	inline Event * IocpAsyncEventService::MallocEvent(BaseAsyncSocket* psock)
	{
		if (NULL == psock)
		{
			return NULL;
		}
		
		SIM_FUNC_DEBUG();
		IocpAsyncEvent*pe = (IocpAsyncEvent*)Malloc(sizeof(IocpAsyncEvent));
		if (pe)
		{
			//调用构造
			pe = new(pe)IocpAsyncEvent();
			SIM_LDEBUG("MallocEvent " << (void*)pe);
			IocpAsyncSocket* sock = (IocpAsyncSocket*)psock;
			sock->addRef();
		}
		else
		{
			SIM_LERROR("MallocEvent Fail");
		}
		return pe;
	}
	inline void IocpAsyncEventService::FreeEvent(BaseAsyncSocket* psock, Event* pe)
	{
		SIM_FUNC_DEBUG();
		if (pe)
		{
			IocpAsyncEvent *p = (IocpAsyncEvent*)pe;
			p->~IocpAsyncEvent();
			Free(pe);
			SIM_LDEBUG("FreeEvent " << (void*)pe);

			//尝试关闭
			Destroy(&psock);
		}
	}
	inline SockRet IocpAsyncEventService::AddToIocp(BaseAsyncSocket * psock)
	{
		SIM_FUNC_DEBUG();
		if (psock == NULL)
		{
			return ASYNC_FAILURE;
		}

		HANDLE io_handle = CreateIoCompletionPort((HANDLE)psock->GetSocket(), iocp_handler_, (ULONG_PTR)psock, 0);
		if (io_handle == NULL)
		{
			return ASYNC_FAILURE;
		}
		IocpAsyncSocket*p = (IocpAsyncSocket*)psock;
		p->addRef();
		p->SetIocpHandler(iocp_handler_);
		return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncEventService::DeleteSock(BaseAsyncSocket * psock)
	{
		SIM_FUNC_DEBUG();
		//CancelIoEx((HANDLE)psock->GetSocket(),NULL);
		IocpAsyncSocket*piocp = (IocpAsyncSocket*)psock;
		psock->Close();

		//推送事件
		IocpAsyncEvent* pe = (IocpAsyncEvent*)Malloc(sizeof(IocpAsyncEvent));
		pe = new(pe)IocpAsyncEvent();
		pe->event_flag = ASYNC_FLAG_RELEASE;
		piocp->CallHandler((Event*)pe);
		pe->~IocpAsyncEvent();
		Free(pe);

		//析构
		piocp->~IocpAsyncSocket();
		Free(piocp);
		return SockRet(0);
	}
	inline SockRet IocpAsyncEventService::Destroy(BaseAsyncSocket** psock)
	{
		SIM_FUNC_DEBUG();
		if (NULL == psock)
		{
			SIM_LERROR("psock is NULL");
			return ASYNC_ERR_NULL_PTR;
		}
		BaseAsyncSocket* pbase = *psock;
		*psock = NULL;
		IocpAsyncSocket* piocp = (IocpAsyncSocket*)pbase;
		if (piocp->decRef() <= 0)
		{
			return DeleteSock(pbase);
		}
		return ASYNC_SUCCESS;
	}
	
	bool sim::IocpAsyncSocket::CallHandler(Event * e)
	{
		bool ret= BaseAsyncSocket::CallHandler(e);
		if (e->event_flag & ASYNC_FLAG_ACCEPT)
			pevent_->Free(e->cache.ptr);
		//异常关闭
		/*if (e->event_flag&ASYNC_FLAG_DISCONNECT )
			Close();*/
		return ret;
	}
	inline SockRet IocpAsyncSocket::DisConnect()
	{
		if (NULL == pevent_)
			return -1;

		IocpAsyncEventService* ps = (IocpAsyncEventService*)pevent_;

		DWORD dwBytesSent = 0;
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		IocpAsyncEvent* pe = (IocpAsyncEvent*)(ps->MallocEvent(this));
		OVERLAPPED* lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_DISCONNECT;
		WsaExFunction exfunc = ps->LoadWsaExFunction();
		int res = exfunc.__DisconnectionEx(GetSocket(), lapped, TF_REUSE_SOCKET, 0);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
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
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent(this));
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_CONNECT;
		WsaExFunction exfunc = ps->LoadWsaExFunction();
		int res = exfunc.__ConnectEx(GetSocket(),
			(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, &dwBytesSent, lapped);
	
		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			ps->FreeEvent(this, (Event*)pe);
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
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent(this));
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_SEND;
		pe->cache.ptr = (char*)data;
		pe->cache.size = data_len;
		pe->wsa_buf.buf = pe->cache.ptr;
		pe->wsa_buf.len = pe->cache.size;
		int res = WSASend(GetSocket(), &pe->wsa_buf, 1, nullptr, 0, lapped, nullptr);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			ps->FreeEvent(this, (Event*)pe);
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
		IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent(this));
		OVERLAPPED *lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_RECV;
		pe->cache.ptr = (char*)data;
		pe->cache.size = data_len;
		pe->wsa_buf.buf = pe->cache.ptr;
		pe->wsa_buf.len = pe->cache.size;
		int res = WSARecv(GetSocket(), &pe->wsa_buf, 1,  &dwBytes, &dwFlags, lapped, nullptr);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			ps->FreeEvent(this, (Event*)pe);
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncSocket::Close()
	{
		return Socket::Close();
		//
		//return DisConnect();

		//if (NULL == pevent_||NULL == iocp_handler_)
		//	return -1;

		//IocpAsyncEventService*ps = (IocpAsyncEventService*)pevent_;

		//DWORD dwBytesSent = 0;
		//DWORD dwFlags = 0;
		//DWORD dwBytes = 0;
		//IocpAsyncEvent *pe = (IocpAsyncEvent *)(ps->MallocEvent(this));
		//OVERLAPPED *lapped = &(pe->overlapped);
		//pe->event_flag = ASYNC_FLAG_RELEASE;
		////int res = CancelIoEx((HANDLE)GetSocket(), lapped);
		//int res =  PostQueuedCompletionStatus(iocp_handler_, dwBytes, (ULONG_PTR)this, lapped);
		//if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
		//	ps->FreeEvent(this, (Event*)pe);
		//	return ASYNC_FAILURE;
		//}
		//return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncSocket::Accept(Socket* s)
	{
		if (NULL == pevent_)
			return ASYNC_FAILURE;

		IocpAsyncEventService* ps = (IocpAsyncEventService*)pevent_;

		DWORD dwBytesRecv = 0;
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		IocpAsyncEvent* pe = (IocpAsyncEvent*)(ps->MallocEvent(this));
		OVERLAPPED* lapped = &(pe->overlapped);
		pe->event_flag = ASYNC_FLAG_ACCEPT;
		//通常可以创建一个套接字库 这个一般是tcp
		pe->accept_client = ps->CreateByType(TCP);
		pe->accept_client->SetHandler(handler_, pdata_);
		const int addr_size = sizeof(SOCKADDR_IN) + 16;
		pe->cache.size = 2 * addr_size;
		pe->cache.ptr = (char*)ps->Malloc(2 * addr_size);
		WsaExFunction exfunc = ps->LoadWsaExFunction();
		int res = exfunc.__AcceptEx(GetSocket(),pe->accept_client->GetSocket(),
			pe->cache.ptr, pe->cache.size-2* addr_size, addr_size, addr_size,
			&dwBytesRecv,lapped);

		if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
			ps->FreeEvent(this, (Event*)pe);
			ps->Destroy(&(pe->accept_client));
			return ASYNC_FAILURE;
		}
		return ASYNC_SUCCESS;
	}
	inline SockRet IocpAsyncSocket::Accept(Socket* s,
		char* remote_ip, unsigned int ip_len, unsigned short* remote_port)
	{
		return ASYNC_ERR_NOT_SUPPORT;
	}
}
#endif