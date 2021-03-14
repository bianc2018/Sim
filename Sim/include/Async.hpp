/*
	异步接口
*/
#ifndef SIM_ASYNC_HPP_
#define SIM_ASYNC_HPP_
#include "Socket.hpp"
#include "RefObject.hpp"
#include "RbTree.hpp"
#include "Mutex.hpp"
#include "Queue.hpp"

#ifdef OS_WINDOWS
	#ifndef ASYNC_IOCP
		#define ASYNC_IOCP
	#endif
	#include <winsock2.h>
	#include <MSWSock.h>
#else
	#ifdef OS_LINUX
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
		#ifndef ASYNC_EPOLL
			#define ASYNC_EPOLL
		#endif
	#endif
#endif
namespace sim
{
	enum EType
	{
		ETConnect,
		ETAccept,
		ETRecv,
		ETSend,
		ETClose,
	};

	typedef SOCKET AsyncHandle ;
	typedef void(*AcceptHandler)(AsyncHandle handle, AsyncHandle client,void*data);
	typedef void(*ConnectHandler)(AsyncHandle handle, void*data);
	typedef void(*RecvDataHandler)(AsyncHandle handle, char *buff,unsigned int buff_len, void*data);
	typedef void(*ErrorHandler)(AsyncHandle handle,int error, void*data);

	class AsyncContext
	{
	public:
		Socket sock;

		AcceptHandler accept_handler;
		void*accept_handler_data;
		ConnectHandler connect_handler;
		void*connect_handler_data;
		RecvDataHandler recvdata_handler;
		void*recvdata_handler_data;
		ErrorHandler error_handler;
		void*error_handler_data;
	public:
		AsyncContext()
			: accept_handler(NULL), accept_handler_data(NULL)
			, connect_handler(NULL), connect_handler_data(NULL)
			, recvdata_handler(NULL), recvdata_handler_data(NULL)
			, error_handler(NULL), error_handler_data(NULL)
		{
			//memset(this, 0, sizeof(*this));
		}
		void CopyHandler(const AsyncContext* pctx)
		{
			if (pctx)
			{
				accept_handler = pctx->accept_handler;
				accept_handler_data = pctx->accept_handler_data;
				connect_handler = pctx->connect_handler;
				connect_handler_data = pctx->connect_handler_data;
				recvdata_handler = pctx->recvdata_handler;
				recvdata_handler_data = pctx->recvdata_handler_data;
				error_handler = pctx->error_handler;
				error_handler_data = pctx->error_handler_data;
			}
		}
		virtual ~AsyncContext()
		{
			sock.Close();
		}
	public:
		virtual void OnAccept(AsyncHandle client)
		{
			if (accept_handler)
				accept_handler(sock.GetSocket(), client, accept_handler_data);
		}
		virtual void OnConnect()
		{
			if (connect_handler)
				connect_handler(sock.GetSocket(), connect_handler_data);
		}
		virtual void OnRecvData(char *buff, unsigned int buff_len)
		{
			if (recvdata_handler)
				recvdata_handler(sock.GetSocket(), buff, buff_len, recvdata_handler_data);
		}
		virtual void OnError(int error)
		{
			if (error_handler)
				error_handler(sock.GetSocket(), error, error_handler_data);
		}
	};

	class Async
	{
	public:
		virtual int Poll(unsigned int wait_ms) = 0;

		virtual AsyncHandle CreateTcpHandle() = 0;
		virtual AsyncHandle CreateUdpHandle() = 0;

		virtual int AddTcpServer(AsyncHandle handle,const char* bind_ipaddr, unsigned short bind_port) = 0;
		virtual int AddTcpConnect(AsyncHandle handle,const char* remote_ipaddr, unsigned short remote_port) = 0;
		virtual int AddUdpConnect(AsyncHandle handle) = 0;
		virtual int AddUdpConnect(AsyncHandle handle,const char* bind_ipaddr, unsigned short bind_port) = 0;

		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len) = 0;
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port) = 0;

		virtual int Close(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->sock.Close();
				ReleaseCtx(handle);
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}

	public:
		virtual void SetAcceptHandler(AsyncHandle handle, AcceptHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->accept_handler = handler;
				ref->accept_handler_data= pdata;
			}
		}
		virtual void SetConnectHandler(AsyncHandle handle, ConnectHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->connect_handler = handler;
				ref->connect_handler_data= pdata;
			}
		}
		virtual void SetRecvDataHandler(AsyncHandle handle, RecvDataHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->recvdata_handler = handler;
				ref->recvdata_handler_data= pdata;
			}
		}
		virtual void SetErrorHandler(AsyncHandle handle, ErrorHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->error_handler = handler;
				ref->error_handler_data = pdata;
			}
		}
	protected:
		virtual void AddCtx(RefObject<AsyncContext> ctx)
		{
			if (ctx)
			{
				AutoMutex lk(ctx_s_lock_);
				ctx_s_.Add(ctx->sock.GetSocket(), ctx);
			}
		}
		virtual RefObject<AsyncContext> GetCtx(AsyncHandle handle)
		{
			AutoMutex lk(ctx_s_lock_);
			RefObject<AsyncContext> ref;
			ctx_s_.Find(handle, &ref);
			return ref;
		}
		virtual void ReleaseCtx(AsyncHandle handle)
		{
			AutoMutex lk(ctx_s_lock_);
			ctx_s_.Del(handle);
		}

	private:
		Mutex ctx_s_lock_;
		RbTree<RefObject<AsyncContext> > ctx_s_;
	};

#ifdef ASYNC_IOCP
	//异步事件
	class IocpAsyncEvent 
	{
	public:
		EType type;
		//子连接存在
		Socket accepted;
		RefBuff buff;
		OVERLAPPED  overlapped;
		DWORD bytes_transfered;
		IocpAsyncEvent():bytes_transfered(0)
		{
			//memset(&wsa_buf, 0, sizeof(wsa_buf));
			memset(&overlapped, 0, sizeof(overlapped));
		}
	};

	//异步上下文
	class IocpAsyncContext:public AsyncContext
	{
	public:
		bool bind_flag;
		IocpAsyncContext() :bind_flag(false)
		{
		}
	};

	//wsa拓展函数
	class WsaExFunction
	{
	public:
		WsaExFunction()
		{
			Socket::Init();
			SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			__AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
			__ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
			__AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
			__DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);
			closesocket(socket);
		}
	private:
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

	class IocpAsync :public Async
	{
	public:
		IocpAsync(unsigned int thread_num = 0)
		{
			//初始化
			Socket::Init();
			iocp_handler_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, thread_num);
		}
		virtual ~IocpAsync()
		{
			CloseHandle(iocp_handler_);
			iocp_handler_ = INVALID_HANDLE_VALUE;
		}
	public:
		virtual AsyncHandle CreateTcpHandle()
		{
			RefObject<AsyncContext> ref(new IocpAsyncContext());
			ref->sock = Socket(sim::TCP);
			if (!ref->sock.IsVaild())
			{
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			return ref->sock.GetSocket();
		}
		virtual AsyncHandle CreateUdpHandle()
		{
			RefObject<AsyncContext> ref(new IocpAsyncContext());
			ref->sock = Socket(sim::UDP);
			if (!ref->sock.IsVaild())
			{
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			return ref->sock.GetSocket();
		}

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}

			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			const int listen_size = 1024;
			ret = ref->sock.Listen(listen_size);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				ReleaseCtx(handle);
				return ret;
			}


			ref->sock.SetNonBlock(true);
			if (NULL == CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0))
			{
				//ReleaseCtx(ref->sock.GetSocket());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}

			const int accept_size = 8;
			//accept
			for (int i = 0; i < accept_size; ++i)
			{
				Accept(ref);
			}
			return ref->sock.GetSocket();
		}
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}

			ref->sock.SetNonBlock(true);
			if (NULL == CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0))
			{
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}

			if (!Connect(ref, remote_ipaddr, remote_port))
			{
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			return ref->sock.GetSocket();
		}
		virtual int AddUdpConnect(AsyncHandle handle)
		{
			return SOCK_FAILURE;
		}
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			return SOCK_FAILURE;
		}
		
		virtual int Poll(unsigned wait_ms)
		{

			DWORD               bytes_transfered = 0;
			SOCKET socket = 0;
			OVERLAPPED          *over_lapped = NULL;
			//取数据
			BOOL res = GetQueuedCompletionStatus(iocp_handler_,
				&bytes_transfered, PULONG_PTR(&socket),
				&over_lapped, wait_ms);

			if (over_lapped)
			{
				printf("res =%d\n",res);
				IocpAsyncEvent* socket_event = \
					CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
				RefObject<AsyncContext> ref = GetCtx(socket);
				if (ref)
				{
					socket_event->bytes_transfered = bytes_transfered;
					DWORD dw_err = GetLastError();
					if (FALSE == res)
					{
						//超时
						if (dw_err == WAIT_TIMEOUT)
						{
							//SIM_LDEBUG("Iocp " << iocp_handler_ << " wait timeout " << wait_ms << " ms");
						}
						ref->OnError(dw_err);
						ReleaseCtx(socket);
					}
					else
					{
						if (socket_event->type == ETAccept)
						{
							RefObject<AsyncContext> accepted(new IocpAsyncContext());
							accepted->CopyHandler(ref.get());
							accepted->sock = socket_event->accepted;
							//需要加到iocp队列里面
							if (NULL == CreateIoCompletionPort((HANDLE)accepted->sock.GetSocket(), iocp_handler_,
								(ULONG_PTR)(accepted->sock.GetSocket()), 0))
							{
								delete socket_event;
								return SOCK_FAILURE;
							}
							//accepted->sock.SetNonBlock(true);
							AddCtx(accepted);
							ref->OnAccept(accepted->sock.GetSocket());
							Recv(accepted);//接收数据
							Accept(ref);
						}
						else if (socket_event->type == ETConnect)
						{
							ref->OnConnect();
							Recv(ref);//接收数据
						}
						else if (socket_event->type == ETRecv)
						{
							ref->OnRecvData(socket_event->buff.get(), socket_event->bytes_transfered);
							if (socket_event->bytes_transfered == 0)
							{
								ReleaseCtx(socket);
							}
							else
							{
								Recv(ref, socket_event->buff);//接收数据
							}
						}
					}
				}
				else
				{
					//notfind
					delete socket_event;
					return SOCK_FAILURE;
				}
				delete socket_event;
				return SOCK_SUCCESS;
			}
			else
			{
				return SOCK_FAILURE;
			}

		}

		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			if (buff == NULL || 0 == buff_len)
			{
				return SOCK_SUCCESS;
			}

			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				return Send(ref, RefBuff(buff, buff_len))? SOCK_SUCCESS: SOCK_FAILURE;
			}
			return SOCK_FAILURE;
		}
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port)
		{
			return SOCK_FAILURE;
		}

		virtual int Close(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->sock.Close();
				ReleaseCtx(handle);
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}
	protected:
		virtual bool Connect(RefObject<AsyncContext> ref, const char* ipaddr, unsigned short port)
		{
			//创建sockaddr_in结构体变量
			struct sockaddr_in addr;
			if (!Socket::IpToAddressV4(ipaddr, port, &addr))
			{
				return false;
			}

			//connectex 需要先绑定
			IocpAsyncContext *pioc = (IocpAsyncContext *)ref.get();
			if (!pioc->bind_flag)
			{
				pioc->sock.Bind(NULL, 0);//随机绑定一个
				pioc->bind_flag = true;
			}

			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent;
			if (NULL == e)
			{
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETConnect;

			//发送数据
			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__ConnectEx(ref->sock.GetSocket(),
				(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, (DWORD*)&(e->bytes_transfered), pol);

			//检查返回
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				return false;
			}
			return true;
		}

		virtual bool Send(RefObject<AsyncContext> ref,RefBuff buff)
		{
			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETSend;

			e->buff = buff;
			WSABUF wsa_buf;
			wsa_buf.buf = buff.get();
			wsa_buf.len = buff.size();

			DWORD dwFlags = 0;

			int res = WSASend(ref->sock.GetSocket(), &wsa_buf, 1,
				(DWORD*)&e->bytes_transfered, dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				return false;
			}
			return true;
		}

		virtual bool Recv(RefObject<AsyncContext> ref)
		{
			const unsigned int buff_size = 4*1024;
			return Recv(ref, RefBuff(buff_size));
		}

		//复用缓存
		virtual bool Recv(RefObject<AsyncContext> ref,RefBuff buff)
		{
			if (buff.size() <= 0)
			{
				//return Recv(ref);
				return false;
			}

			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETRecv;

			e->buff = buff;
			WSABUF wsa_buf;
			wsa_buf.buf = e->buff.get();
			wsa_buf.len = e->buff.size();

			DWORD dwFlags = 0;

			int res = WSARecv(ref->sock.GetSocket(), &wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, pol, nullptr);
			auto err = WSAGetLastError();
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				return false;
			}
			return true;
		}
		//接收链接
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;  
			e->type = ETAccept;

			const int addr_size = sizeof(SOCKADDR_IN) + 16;
			e->buff=RefBuff(2 * addr_size);
			
			//创建子连接
			e->accepted = sim::Socket(sim::TCP);
			printf("try accept %d\n", e->accepted.GetSocket());
			if (!e->accepted.IsVaild())
			{
				return false;
			}
			DWORD dwFlags = 0;

			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__AcceptEx(ref->sock.GetSocket(), e->accepted.GetSocket(),
				e->buff.get(), e->buff.size() - 2 * addr_size, addr_size, addr_size,
				(DWORD*)&e->bytes_transfered, pol);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				return false;
			}
			return true;
		}
	protected:
		//加载拓展函数
		WsaExFunction &LoadWsaExFunction()
		{
			static WsaExFunction ex_func;
			return ex_func;
		}

	private:
		HANDLE iocp_handler_;
	};
	//iocp 实现
	typedef IocpAsync SimAsync;
#endif // ASYNC_IOCP

#ifdef ASYNC_EPOLL
	struct SendBuff
	{
		RefBuff buff;
		unsigned int offset;

	};
	//异步上下文
	class EpollAsyncContext :public AsyncContext
	{
	public:
		//事件记录
		struct epoll_event ep_event;

		//发送队列及锁
		Mutex send_queue_lock;
		Queue<SendBuff> send_queue_buff;
		//接收符号，true时接收连接
		bool accept_flag;
		//连接符号，true 时连接句柄
		bool connect_flag;
		//事件句柄
		uint32_t eflag;
		EpollAsyncContext():accept_flag(false), connect_flag(false), eflag(EPOLLIN | EPOLLHUP | EPOLLERR| EPOLLET)
		{
			ep_event.data.ptr = (void*)this;
			ep_event.events = eflag;
		}
	};

	class EpollAsync :public Async
	{
	public:
		EpollAsync(unsigned int thread_num = 8)
		{
			//初始化
			Socket::Init();
			epollfd_ = epoll_create(thread_num);
			if (-1 == epollfd_)
			{
				printf("Failed to create epoll context.%s", strerror(errno));
				exit(1);
			}
		}
		virtual ~EpollAsync()
		{
			
		}
	public:
		virtual int Poll(unsigned int wait_ms)
		{
			unsigned int MAXEVENTS = 100;
			struct epoll_event events[MAXEVENTS];
			int n = epoll_wait(epollfd_, events, MAXEVENTS, wait_ms);
			if (-1 == n)
			{
				printf("Failed to wait.%s\n", strerror(errno));
				return SOCK_FAILURE;
			}
			for (int i = 0; i < n; i++)
			{
				printf("epoll_wait.%d\n", n);
				RefObject<AsyncContext> ref = GetCtx(events[i].data.fd);
				if (!ref)
					continue;
				
				uint32_t ee = events[i].events;

				if (ee & EPOLLHUP || ee & EPOLLERR)
				{
					/*
					* EPOLLHUP and EPOLLERR are always monitored.
					*/
					printf("EPOLLHUP and EPOLLERR are always monitored.\n");
					ref->OnError(errno);
					ReleaseCtx(ref);
					continue;
				}
				
				EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

				if (ep_ref->accept_flag)
				{
					//accept
					Socket accepted_socket;
					int ret = ep_ref->sock.Accept(accepted_socket,1000);
					if (ret != SOCK_SUCCESS)
					{
						ref->OnError(errno);
						ReleaseCtx(ref);
						continue;
					}
					RefObject<AsyncContext> accepted(new EpollAsyncContext());
					accepted->CopyHandler(ep_ref);
					accepted->sock = accepted_socket;
					AddCtx(accepted);
					ref->OnAccept(accepted->sock.GetSocket());
					AddEpoll(accepted);
				}
				else if (ep_ref->connect_flag)
				{
					//连接已经建立了
					ep_ref->connect_flag = false;
					ref->OnConnect();
				}
				//读写
				else
				{
					if (EPOLLIN & ee)//可读
					{
						RefBuff buff(1024*4);
						int ret = ep_ref->sock.Recv(buff.get(), buff.size());
						if (ret < 0)
						{
							ep_ref->OnError(ret);
							ReleaseCtx(ref);
						}
						else if (ret == 0)
						{
							ReleaseCtx(ref);
						}
						else
						{
							ep_ref->OnRecvData(buff.get(), ret);
							/*ep_ref->eflag = ep_ref->eflag | EPOLLIN;
							ModifyEpoll(ref);*/
						}
					}
					if (EPOLLOUT &ee)
					{
						AutoMutex lk(ep_ref->send_queue_lock);
						QueueNode<SendBuff>*pHead = ep_ref->send_queue_buff.Next(NULL);
						if (NULL == pHead)
						{
							ep_ref->eflag = ep_ref->eflag & (~EPOLLOUT);
							ModifyEpoll(ref);
							continue;
						}
						int ret = ep_ref->sock.Send(pHead->data.buff.get() + pHead->data.offset, pHead->data.buff.size() - pHead->data.offset);
						if (ret < 0)
						{
							ep_ref->OnError(ret);
							ReleaseCtx(ref);
						}
						pHead->data.offset += ret;
						if (pHead->data.offset >= pHead->data.buff.size())
						{
							//去除
							ep_ref->send_queue_buff.PopFront(NULL);
						}
						//去除写事件
						if (ep_ref->send_queue_buff.isEmpty())
						{
							ep_ref->eflag = ep_ref->eflag&(~EPOLLOUT);
							ModifyEpoll(ref);
						}
						/*else
						{
							ep_ref->eflag = ep_ref->eflag|EPOLLOUT;
							ModifyEpoll(ref);
						}*/
					}
				}
			}

			return SOCK_SUCCESS;
		}

		virtual AsyncHandle CreateTcpHandle()
		{
			RefObject<AsyncContext> ref(new EpollAsyncContext());
			ref->sock = Socket(sim::TCP);
			if (!ref->sock.IsVaild())
			{
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			return ref->sock.GetSocket();
		}
		virtual AsyncHandle CreateUdpHandle()
		{
			RefObject<AsyncContext> ref(new EpollAsyncContext());
			ref->sock = Socket(sim::UDP);
			if (!ref->sock.IsVaild())
			{
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			return ref->sock.GetSocket();
		}

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}
			ref->sock.SetReusePort(true);

			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			const int listen_size = 1024;
			ret = ref->sock.Listen(listen_size);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			ref->sock.SetNonBlock(true);
			AddEpoll(ref);
			Accept(ref);
			return ref->sock.GetSocket();
		}
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}

			ref->sock.SetNonBlock(true);
			AddEpoll(ref);
			if (!Connect(ref, remote_ipaddr, remote_port))
			{
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			return ref->sock.GetSocket();
		}
		virtual int AddUdpConnect(AsyncHandle handle)
		{
			return SOCK_FAILURE;
		}
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			return SOCK_FAILURE;
		}

		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			if (buff == NULL || 0 == buff_len)
			{
				return SOCK_SUCCESS;
			}

			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				return Send(ref, RefBuff(buff, buff_len)) ? SOCK_SUCCESS : SOCK_FAILURE;
			}
			return SOCK_FAILURE;
		}
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port)
		{
			return SOCK_FAILURE;
		}

		virtual int Close(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				RemoveEpoll(ref);
				ref->sock.Close();
				ReleaseCtx(handle);
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}
	protected:
		virtual bool Connect(RefObject<AsyncContext> ref, const char* ipaddr, unsigned short port)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			int ret = ep_ref->sock.Connect(ipaddr, port);
			if (ret == 0)
				return true;
			ep_ref->connect_flag = true;
			if (!(ep_ref->eflag&EPOLLOUT))
			{
				return ModifyEpoll(ref);
			}
			return true;
		}

		virtual bool Send(RefObject<AsyncContext> ref, RefBuff buff)
		{
			/*if (NULL == ref)
				return false;*/

			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			AutoMutex lk(ep_ref->send_queue_lock);
			SendBuff send;
			send.buff = buff;
			send.offset = 0;
			ep_ref->send_queue_buff.PushBack(send);
			if (!(ep_ref->eflag&EPOLLOUT))
			{
				ep_ref->eflag |= EPOLLOUT;
				return ModifyEpoll(ref);
			}
			return true;
		}

		//接收链接
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			ep_ref->accept_flag = true;
			if (!(ep_ref->eflag&EPOLLIN))
			{
				ep_ref->eflag |= EPOLLIN;
				ModifyEpoll(ref);
			}
		}

		//Add
		virtual bool AddEpoll(RefObject<AsyncContext> ref)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_ADD, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				printf("Failed to modify an event for socket %d Error:%s", ep_ref->sock.GetSocket(), strerror(errno));
				return false;
			}
			return true;
		}
		virtual bool ModifyEpoll(RefObject<AsyncContext> ref)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_MOD, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				printf("Failed to modify an event for socket %d Error:%s", ep_ref->sock.GetSocket(), strerror(errno));
				return false;
			}
			return true;

		}
		virtual bool RemoveEpoll(RefObject<AsyncContext> ref)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_DEL, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				printf("Failed to modify an event for socket %d Error:%s", ep_ref->sock.GetSocket(), strerror(errno));
				return false;
			}
			return true;
		}
	private:
		int epollfd_;
	};
	//epoll实现
	typedef EpollAsync SimAsync;
#endif //ASYNC_EPOLL
}
#endif