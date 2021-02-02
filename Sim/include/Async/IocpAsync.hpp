/*
* IOCP 封装
*/
#ifndef SIM_IOCP_ASYNC_HPP_
#define SIM_IOCP_ASYNC_HPP_
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#include <MSWSock.h>
#endif

#include "Async2.hpp"

#ifdef OS_WINDOWS

namespace sim
{
	//异步事件
	struct IocpAsyncEvent:public AsyncEvent
	{
		OVERLAPPED  overlapped;
		IocpAsyncEvent()
		{
			//memset(&wsa_buf, 0, sizeof(wsa_buf));
			memset(&overlapped, 0, sizeof(overlapped));
		}
	};

	//异步上下文
	struct IocpAsyncContext :public AsyncContext
	{
		bool bind_flag;
		IocpAsyncContext() :bind_flag(false)
		{
			//memset(this, 0, sizeof(*this));
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
		IocpAsync(unsigned int thread_num=0)
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
		//新增socket
		virtual bool AddSocket(Socket sock, AsyncSocketHandler handler, void*pdata)
		{
			if (NULL == handler)
			{
				SIM_LERROR("handler is NULL");
				return false;
			}
			if (!sock.IsVaild())
			{
				SIM_LERROR("sock=" << sock.GetSocket() << " is not vaild");
				return false;
			}
			sock.SetNonBlock(true);

			if (NULL == CreateIoCompletionPort((HANDLE)sock.GetSocket(), iocp_handler_, (ULONG_PTR)(sock.GetSocket()), 0))
			{
				SIM_LERROR("CreateIoCompletionPort return NULL");
				return false;
			}

			IocpAsyncContext*pctx = SIM_MEM_NEW(IocpAsyncContext, this);
			if (NULL == pctx)
			{
				SIM_LERROR("SIM_MEM_NEW return NULL");
				return false;
			}
			pctx->handler = handler;
			pctx->bind_flag = false;
			pctx->master = sock;
			pctx->pdata = pdata;
			return AddCtx(RefObject<AsyncContext>(pctx, AsyncRefObjectDelete, this));
		}

		//移除socket
		virtual bool DelSocket(Socket sock, bool bclose)
		{
			if (bclose)
			{
				sock.Close();
			}
			return DelCtx(sock);
		}

		virtual bool RunOnce(unsigned wait_ms)
		{
			SIM_FUNC_DEBUG();

			DWORD               bytes_transfered = 0;
			SOCKET socket = 0;
			OVERLAPPED          *over_lapped = nullptr;
			//取数据
			BOOL res = GetQueuedCompletionStatus(iocp_handler_,
				&bytes_transfered, PULONG_PTR(&socket),
				&over_lapped, wait_ms);
			DWORD dw_err = GetLastError();
			SIM_LDEBUG("GetQueuedCompletionStatus res=" << res << " GetLastError="
				<< dw_err);
			if (FALSE == res)
			{
				if (dw_err == WAIT_TIMEOUT)
				{
					SIM_LDEBUG("Iocp " << iocp_handler_ << " wait timeout " << wait_ms << " ms");
				}

				if (over_lapped)
				{
					//出现错误了
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					socket_event->error = dw_err;
					CallEvent((AsyncEvent*)socket_event);
					
					SIM_MEM_DEL(this, socket_event);
					return false;
				}
				else
				{
					//其他不处理
					SIM_LERROR("GetQueuedCompletionStatus  GetLastError="
						<< dw_err);
					return false;
				}
				
			}
			else
			{
				IocpAsyncEvent* socket_event = \
					CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
				socket_event->bytes_transfered = bytes_transfered;
				CallEvent((AsyncEvent*)socket_event);
				SIM_MEM_DEL(this, socket_event);
				return true;
			}
		}
	public:
		virtual bool Connect(Socket sock, const char* ipaddr, unsigned short port)
		{
			//获取上下文
			RefObject<AsyncContext> ctx = GetCtx(sock);
			if (false == ctx)
			{
				SIM_LERROR("sock="<< sock.GetSocket()<<" not find");
				return false;
			}
			
			//创建sockaddr_in结构体变量
			struct sockaddr_in addr;
			if (!Socket::IpToAddressV4(ipaddr, port, &addr))
			{
				SIM_LERROR("IpToAddressV4 fail,ipaddr="<<ipaddr<<" port="<<port);
				return false;
			}

			//connectex 需要先绑定
			IocpAsyncContext *pioc = (IocpAsyncContext *)ctx.get();
			if (!pioc->bind_flag)
			{
				pioc->master.Bind(NULL, 0);//随机绑定一个
				pioc->bind_flag = true;
			}

			//新建事件
			IocpAsyncEvent *e = SIM_MEM_NEW(IocpAsyncEvent, this);
			if (NULL == e)
			{
				SIM_LERROR("SIM_MEM_NEW IocpAsyncEvent return NULL");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->ctx = ctx;
			e->type = ET_Connect;

			//发送数据
			DWORD dwBytesSent = 0;
			DWORD dwFlags = 0;
			DWORD dwBytes = 0;
			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__ConnectEx(sock.GetSocket(),
				(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, (DWORD*)&(e->bytes_transfered), pol);

			//检查返回
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				SIM_LERROR("Connect to " << ipaddr << " :" << port << " error,last error " << WSAGetLastError());
				SIM_MEM_DEL(this, e);
				return false;
			}
			return true;
		}

		virtual bool Bind(Socket sock, const char* ipaddr, unsigned short port)
		{
			RefObject<AsyncContext> ctx = GetCtx(sock);
			if (false == ctx)
			{
				SIM_LERROR("sock=" << sock.GetSocket() << " not find");
				return false;
			}
			IocpAsyncContext *pioc = (IocpAsyncContext *)ctx.get();
			if (!pioc->bind_flag)
			{
				
				SockRet ret = sock.Bind(ipaddr, port);
				if (ret == 0)
				{
					pioc->bind_flag = true;
					return true;
				}
				return false;
			}
			return true;
		}

		virtual bool Send(Socket sock, const char* data, unsigned int data_len)
		{
			//获取上下文
			RefObject<AsyncContext> ctx = GetCtx(sock);
			if (false == ctx)
			{
				SIM_LERROR("sock=" << sock.GetSocket() << " not find");
				return false;
			}
			//新建事件
			IocpAsyncEvent *e = SIM_MEM_NEW(IocpAsyncEvent, this);
			if (NULL == e)
			{
				SIM_LERROR("SIM_MEM_NEW IocpAsyncEvent return NULL");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->ctx = ctx;
			e->type = ET_Send;

			e->buffer.ptr = (char*)data;
			e->buffer.size = data_len;
			WSABUF wsa_buf;
			wsa_buf.buf = e->buffer.ptr;
			wsa_buf.len = e->buffer.size;

			DWORD dwFlags = 0;

			int res = WSASend(sock.GetSocket(), &wsa_buf, 1, (DWORD*)&e->bytes_transfered, dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				SIM_LERROR("WSASend  error,last error " << WSAGetLastError());
				SIM_MEM_DEL(this, e);
				return false;
			}
			return true;
		}

		virtual bool Recv(Socket sock, char* data, unsigned int data_len)
		{
			//获取上下文
			RefObject<AsyncContext> ctx = GetCtx(sock);
			if (false == ctx)
			{
				SIM_LERROR("sock=" << sock.GetSocket() << " not find");
				return false;
			}
			//新建事件
			IocpAsyncEvent *e = SIM_MEM_NEW(IocpAsyncEvent, this);
			if (NULL == e)
			{
				SIM_LERROR("SIM_MEM_NEW IocpAsyncEvent return NULL");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->ctx = ctx;
			e->type = ET_Recv;

			e->buffer.ptr = (char*)data;
			e->buffer.size = data_len;
			WSABUF wsa_buf;
			wsa_buf.buf = e->buffer.ptr;
			wsa_buf.len = e->buffer.size;
			DWORD dwFlags = 0;

			int res = WSARecv(sock.GetSocket(), &wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				SIM_LERROR("WSARecv  error,last error " << WSAGetLastError());
				SIM_MEM_DEL(this, e);
				return false;
			}
			return true;
		}

		//接收链接
		virtual bool Accept(Socket sock)
		{
			//获取上下文
			RefObject<AsyncContext> ctx = GetCtx(sock);
			if (false == ctx)
			{
				SIM_LERROR("sock=" << sock.GetSocket() << " not find");
				return false;
			}
			//新建事件
			IocpAsyncEvent *e = SIM_MEM_NEW(IocpAsyncEvent, this);
			if (NULL == e)
			{
				SIM_LERROR("SIM_MEM_NEW IocpAsyncEvent return NULL");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->ctx = ctx;
			e->type = ET_Accept;

			const int addr_size = sizeof(SOCKADDR_IN) + 16;
			e->buffer.size = 2 * addr_size;
			e->buffer.ptr = (char*)Malloc(2 * addr_size);
			//创建子连接
			e->accepted = sim::Socket(sim::TCP);

			DWORD dwFlags = 0;

			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__AcceptEx(sock.GetSocket(), e->accepted.GetSocket(),
				e->buffer.ptr, e->buffer.size - 2 * addr_size, addr_size, addr_size,
				(DWORD*)&e->bytes_transfered, pol);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				SIM_LERROR("WSARecv  error,last error " << WSAGetLastError());
				Free(e->buffer.ptr);
				SIM_MEM_DEL(this, e);
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

		virtual void CallEvent(AsyncEvent*e)
		{
			if (e->type == ET_Accept)
			{
				//回收缓存
				Free(e->buffer.ptr);
				e->buffer.ptr = NULL;
				e->buffer.size = 0;
				e->bytes_transfered = 0;
				//加到iocp中？
				//
			}
			Async::CallEvent(e);
		}
	private:
		HANDLE iocp_handler_;
	};
}
#endif

#endif