/*
	�첽�ӿ�
*/
#ifndef SIM_ASYNC_HPP_
#define SIM_ASYNC_HPP_

#include "Socket.hpp"
#include "RefObject.hpp"
#include "RbTree.hpp"
#include "Mutex.hpp"
#include "Queue.hpp"
#include "Timer.hpp"

#define SIM_ENABLE_LOGGER
#ifdef SIM_ENABLE_LOGGER
#include "Logger.hpp"
#else
#ifndef SIM_LOG
namespace sim
{
	//��־����
	enum LogLevel
	{
		LNone,
		LError,
		LWarn,
		LInfo,
		LDebug,
	};
}		//��ʽ
#define SIM_FORMAT_NUM(num,base,w,f)	
#define SIM_FORMAT_STR(str,w,f)			
#define SIM_HEX(num) SIM_FORMAT_NUM(num,16,8,'0')
#define SIM_FORMAT_STR0(str,w) SIM_FORMAT_STR(str,w,' ')

//��ֹ����
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 

//���������
#define SIM_LOG_ADD(Stream,...) 
//����������
#define SIM_LOG_HANDLER(max_lv,handler,userdata)
//���ÿ���̨���
#define SIM_LOG_CONSOLE(max_lv)\
				SIM_LOG_ADD(sim::LogConsoleStream,max_lv)

#define SIM_LOG(lv,x)
#define SIM_LDEBUG(x) SIM_LOG(sim::LDebug,x)
#define SIM_LINFO(x) SIM_LOG(sim::LInfo,x)
#define SIM_LWARN(x) SIM_LOG(sim::LWarn,x)
#define SIM_LERROR(x) SIM_LOG(sim::LError,x)
#endif
#endif
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

	typedef SOCKET AsyncHandle;
	typedef void(*AcceptHandler)(AsyncHandle handle, AsyncHandle client, void*data);
	typedef void(*ConnectHandler)(AsyncHandle handle, void*data);
	typedef void(*RecvDataHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);
	typedef void(*SendCompleteHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);
	//typedef void(*ErrorHandler)(AsyncHandle handle,int error, void*data);
	enum AsyncCloseReason
	{
		//�����ر�
		CloseActive,
		//�Ự�������չر�
		ClosePassive,
		//�쳣�ر�
		CloseError=-1,
	};
	typedef void(*CloseHandler)(AsyncHandle handle, AsyncCloseReason reason, int error, void*data);

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
		SendCompleteHandler sendcomplete_handler;
		void*sendcomplete_handler_data;
		CloseHandler close_handler;
		void*close_handler_data;
	public:
		AsyncContext()
			: accept_handler(NULL), accept_handler_data(NULL)
			, connect_handler(NULL), connect_handler_data(NULL)
			, recvdata_handler(NULL), recvdata_handler_data(NULL)
			, sendcomplete_handler(NULL), sendcomplete_handler_data(NULL)
			, close_handler(NULL), close_handler_data(NULL)
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
				sendcomplete_handler = pctx->sendcomplete_handler;
				sendcomplete_handler_data = pctx->sendcomplete_handler_data;
				close_handler = pctx->close_handler;
				close_handler_data = pctx->close_handler_data;
			}
		}
		virtual ~AsyncContext()
		{
			SIM_LDEBUG("close sock " << sock.GetSocket());
			sock.Close();
		}
	public:
		virtual void OnAccept(AsyncHandle client)
		{
			if (accept_handler&&sock.IsVaild())
				accept_handler(sock.GetSocket(), client, accept_handler_data);
		}
		virtual void OnConnect()
		{
			if (connect_handler&&sock.IsVaild())
				connect_handler(sock.GetSocket(), connect_handler_data);
		}
		virtual void OnRecvData(char *buff, unsigned int buff_len)
		{
			if (recvdata_handler&&sock.IsVaild())
				recvdata_handler(sock.GetSocket(), buff, buff_len, recvdata_handler_data);
		}
		virtual void OnSendComplete(char *buff, unsigned int buff_len)
		{
			if (sendcomplete_handler&&sock.IsVaild())
				sendcomplete_handler(sock.GetSocket(), buff, buff_len, sendcomplete_handler_data);
		}
		/*virtual void OnError(int error)
		{
			if (close_handler&&sock.IsVaild())
				close_handler(sock.GetSocket(), AsyncError, error, close_handler_data);
		}
		virtual void OnClose()
		{
			if (close_handler&&sock.IsVaild())
				close_handler(sock.GetSocket(), AsyncEnd, 0, close_handler_data);
		}*/
		virtual void OnClose(AsyncCloseReason reason, int error)
		{
			if (close_handler&&sock.IsVaild())
				close_handler(sock.GetSocket(), reason, error, close_handler_data);
		}
	};

	class Async
	{
		//�������ƿ���
		Async(const Async &other) {};
		Async& operator=(const Async &other) {};
	public:
		Async() {}
		virtual int Poll(unsigned int wait_ms) = 0;

		virtual AsyncHandle CreateTcpHandle() = 0;
		virtual AsyncHandle CreateUdpHandle() = 0;

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10) = 0;
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port) = 0;
		virtual int AddUdpConnect(AsyncHandle handle) = 0;
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port) = 0;

		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len) = 0;
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port) = 0;

		virtual int Close(AsyncHandle handle)
		{
			SIM_FUNC_DEBUG();
			return Close(handle, CloseActive);
			//RefObject<AsyncContext> ref = GetCtx(handle);
			//if (ref)
			//{
			//	//ref->sock.Close();
			//	ReleaseCtx(handle);
			//	SIM_LDEBUG("handle " << handle << " closed ");
			//	return SOCK_SUCCESS;
			//}
			//return SOCK_FAILURE;
		}

	public:
		virtual void SetAcceptHandler(AsyncHandle handle, AcceptHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->accept_handler = handler;
				ref->accept_handler_data = pdata;
			}
		}
		virtual void SetConnectHandler(AsyncHandle handle, ConnectHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->connect_handler = handler;
				ref->connect_handler_data = pdata;
			}
		}
		virtual void SetRecvDataHandler(AsyncHandle handle, RecvDataHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->recvdata_handler = handler;
				ref->recvdata_handler_data = pdata;
			}
		}
		/*virtual void SetErrorHandler(AsyncHandle handle, ErrorHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->error_handler = handler;
				ref->error_handler_data = pdata;
			}
		}*/
		virtual void SetCloseHandler(AsyncHandle handle, CloseHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->close_handler = handler;
				ref->close_handler_data = pdata;
			}
		}

		virtual void SetSendCompleteHandler(AsyncHandle handle, SendCompleteHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->sendcomplete_handler = handler;
				ref->sendcomplete_handler_data = pdata;
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
		virtual int Close(AsyncHandle handle, AsyncCloseReason reason)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ReleaseCtx(handle);
				SIM_LDEBUG("handle " << handle << " closed ");
#ifdef OS_WINDOWS
				ref->OnClose(reason, GetLastError());
#else
				ref->OnClose(reason, errno);
#endif
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}
	private:
		Mutex ctx_s_lock_;
		RbTree<RefObject<AsyncContext> > ctx_s_;
	};

#ifdef ASYNC_IOCP
	//�첽�¼�
	class IocpAsyncEvent
	{
	public:
		OVERLAPPED  overlapped;
		EType type;
		//�����Ӵ���
		Socket accepted;
		RefBuff buff;
		WSABUF wsa_buf;
		
		DWORD bytes_transfered;
		//RefObject<AsyncContext> ref;

		IocpAsyncEvent() :bytes_transfered(0)
		{
			//memset(&wsa_buf, 0, sizeof(wsa_buf));
			memset(&overlapped, 0, sizeof(overlapped));
			memset(&wsa_buf, 0, sizeof(wsa_buf));
		}

	};

	//�첽������
	class IocpAsyncContext :public AsyncContext
	{
	public:
		bool bind_flag;
		HANDLE iocp_handler;
		IocpAsyncContext() :bind_flag(false), iocp_handler(NULL)
		{
		}
		~IocpAsyncContext()
		{
			if (iocp_handler)
			{
				SIM_LDEBUG("close iocp_handler 0x" << SIM_HEX(iocp_handler) << " sock " << sock.GetSocket());
				//CloseHandle(iocp_handler);
			}
		}
	};

	//wsa��չ����
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
			SIM_FUNC_DEBUG();
			//��ʼ��
			Socket::Init();
			iocp_handler_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, thread_num);
			SIM_LDEBUG("IocpAsync init 0x" << SIM_HEX(iocp_handler_) << " thread_num " << thread_num);
		}
		virtual ~IocpAsync()
		{
			SIM_FUNC_DEBUG();
			SIM_LDEBUG("IocpAsync CloseHandle 0x" << SIM_HEX(iocp_handler_));
			CloseHandle(iocp_handler_);
			iocp_handler_ = INVALID_HANDLE_VALUE;
		}
	public:
		virtual AsyncHandle CreateTcpHandle()
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref(new IocpAsyncContext());
			ref->sock = Socket(sim::TCP);
			if (!ref->sock.IsVaild())
			{
				SIM_LERROR("sock Create error");
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			SIM_LDEBUG("TCP.handle " << ref->sock.GetSocket() << " is cteated");
			return ref->sock.GetSocket();
		}
		virtual AsyncHandle CreateUdpHandle()
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref(new IocpAsyncContext());
			ref->sock = Socket(sim::UDP);
			if (!ref->sock.IsVaild())
			{
				SIM_LERROR("sock Create error");
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			SIM_LDEBUG("UDP.handle " << ref->sock.GetSocket() << " is cteated");
			return ref->sock.GetSocket();
		}

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}

			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				SIM_LERROR("handle " << handle << " Bind ipaddr:" << bind_ipaddr << ":" << bind_port << " fail,ret=" << ret);
				ReleaseCtx(handle);
				return ret;
			}
			const int listen_size = 1024*4;
			ret = ref->sock.Listen(listen_size);
			if (ret != SOCK_SUCCESS)
			{
				SIM_LERROR("handle " << handle << " Listen listen_size:" << listen_size << " fail,ret=" << ret);
				//bind error
				ReleaseCtx(handle);
				return ret;
			}

			ref->sock.SetNonBlock(true);
			IocpAsyncContext* iocp_ctx = (IocpAsyncContext*)ref.get();
			iocp_ctx->iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_ctx->iocp_handler)
			{
				//ReleaseCtx(ref->sock.GetSocket());
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			const int accept_size = 32;
			if (acctept_num <= 0)
				acctept_num = accept_size;

			SIM_LERROR("handle " << handle << " accept_size=" << acctept_num);
			//accept
			for (int i = 0; i < acctept_num; ++i)
			{
				Accept(ref);
			}
			return SOCK_SUCCESS;
		}
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}

			ref->sock.SetNonBlock(true);
			IocpAsyncContext* iocp_ctx = (IocpAsyncContext*)ref.get();
			iocp_ctx->iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_ctx->iocp_handler)
				/*if (NULL == CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
					(ULONG_PTR)(ref->sock.GetSocket()), 0))*/
			{
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			SIM_LDEBUG("handle " << handle << " add to iocp");
			if (!Connect(ref, remote_ipaddr, remote_port))
			{
				SIM_LERROR("handle " << handle << " Connect fail");
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			return SOCK_SUCCESS;
		}
		virtual int AddUdpConnect(AsyncHandle handle)
		{
			SIM_FUNC_DEBUG();
			return SOCK_FAILURE;
		}
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			SIM_FUNC_DEBUG();
			return SOCK_FAILURE;
		}

		virtual int Poll(unsigned wait_ms)
		{
			SIM_FUNC_DEBUG();
			DWORD               bytes_transfered = 0;
			SOCKET socket = 0;
			OVERLAPPED          *over_lapped = NULL;
			//ȡ����
			BOOL res = GetQueuedCompletionStatus(iocp_handler_,
				&bytes_transfered, PULONG_PTR(&socket),
				&over_lapped, wait_ms);
			SIM_LDEBUG("GetQueuedCompletionStatus res=" << res
				<< " socket " << socket << " bytes_transfered " << bytes_transfered 
				<< " over_lapped=0x" << SIM_HEX(over_lapped))
				if (over_lapped)
				{
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);
					RefObject<AsyncContext> ref = GetCtx(socket);
					if (ref)
					{
						socket_event->bytes_transfered = bytes_transfered;
						DWORD dw_err = GetLastError();
						if (FALSE == res)
						{
							//��ʱ
							if (dw_err == WAIT_TIMEOUT)
							{
								SIM_LDEBUG("Iocp " << iocp_handler_ << " wait timeout " << wait_ms << " ms");
							}
							SIM_LERROR("OnError dw_err=" << dw_err << " by socket " << ref->sock.GetSocket()
								<< " type " << socket_event->type);
							Close(socket, CloseError);
						}
						else
						{
							if (socket_event->type == ETAccept)
							{
								if (false == Accept(ref))
								{
									SIM_LERROR("Accept fail " << " sock= " << ref->sock.GetSocket());
									Close(socket, CloseError);
									delete socket_event;
									return SOCK_FAILURE;
								}

								RefObject<AsyncContext> accepted(new IocpAsyncContext());
								accepted->CopyHandler(ref.get());
								accepted->sock = socket_event->accepted;
								//��Ҫ�ӵ�iocp��������
								IocpAsyncContext* iocp_ctx = (IocpAsyncContext*)accepted.get();
								iocp_ctx->iocp_handler = CreateIoCompletionPort((HANDLE)accepted->sock.GetSocket(), iocp_handler_,
									(ULONG_PTR)(accepted->sock.GetSocket()), 0);
								if (NULL == iocp_ctx->iocp_handler)
									/*if (NULL == CreateIoCompletionPort((HANDLE)accepted->sock.GetSocket(), iocp_handler_,
										(ULONG_PTR)(accepted->sock.GetSocket()), 0))*/
								{
									SIM_LERROR("CreateIoCompletionPort fail iocp_handler_=" << SIM_HEX(iocp_ctx->iocp_handler)
										<< " sock= " << accepted->sock.GetSocket() << "  WSAGetLastError()=" << WSAGetLastError());
									delete socket_event;
									return SOCK_FAILURE;
								}
								//accepted->sock.SetNonBlock(true);
								AddCtx(accepted);

								ref->OnAccept(accepted->sock.GetSocket());

								if (false == Recv(accepted))//��������
								{
									SIM_LERROR("Recv fail  sock= " << accepted->sock.GetSocket());
									Close(accepted->sock.GetSocket(), CloseError);
									delete socket_event;
									return SOCK_FAILURE;
								}
								
							}
							else if (socket_event->type == ETConnect)
							{
								ref->OnConnect();
								//Recv(ref);//��������
								if (false == Recv(ref))//��������
								{
									SIM_LERROR("Recv fail  sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), CloseError);
									delete socket_event;
									return SOCK_FAILURE;
								}
							}
							else if (socket_event->type == ETRecv)
							{
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("recv socket_event->bytes_transfered=0 ,socket is end");
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									ref->OnRecvData(socket_event->buff.get(), socket_event->bytes_transfered);
									//Recv(ref, socket_event->buff);//��������
									if (false == Recv(ref, socket_event->buff))//��������
									{
										SIM_LERROR("Recv fail " << " sock= " << ref->sock.GetSocket());
										Close(ref->sock.GetSocket(), CloseError);
										delete socket_event;
										return SOCK_FAILURE;
									}
								}
							}
							else if (socket_event->type == ETSend)
							{
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("Send socket_event->bytes_transfered=0 ,socket is end");
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									ref->OnSendComplete(socket_event->buff.get(), socket_event->bytes_transfered);
								}
							}
							else
							{
								SIM_LWARN("event->type =" << socket_event->type << " not do something");
							}
						}
					}
					else
					{
						//notfind
						SIM_LERROR("socket " << socket << " not found ref");
						delete socket_event;
						return SOCK_FAILURE;
					}

					//�޴���
					delete socket_event;
					return SOCK_SUCCESS;
				}
				else
				{
					//SIM_LERROR("GetQueuedCompletionStatus fail over_lapped is NULL GetLastError=" << GetLastError());
					return SOCK_FAILURE;
				}

		}

		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			SIM_FUNC_DEBUG();
			if (buff == NULL || 0 == buff_len)
			{
				SIM_LWARN("Send Empty Buff!");
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
			SIM_FUNC_DEBUG();
			return SOCK_FAILURE;
		}

	protected:
		virtual bool Connect(RefObject<AsyncContext> ref, const char* ipaddr, unsigned short port)
		{
			SIM_FUNC_DEBUG();
			//����sockaddr_in�ṹ�����
			struct sockaddr_in addr;
			if (!Socket::IpToAddressV4(ipaddr, port, &addr))
			{
				SIM_LERROR("IpToAddressV4 error ip " << ipaddr << ":" << port);
				return false;
			}

			//connectex ��Ҫ�Ȱ�
			IocpAsyncContext *pioc = (IocpAsyncContext *)ref.get();
			if (!pioc->bind_flag)
			{
				pioc->sock.Bind(NULL, 0);//�����һ��
				pioc->bind_flag = true;
			}

			//�½��¼�
			IocpAsyncEvent *e = new IocpAsyncEvent;
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETConnect;
			//e->ref = ref;
			//��������
			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__ConnectEx(ref->sock.GetSocket(),
				(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, (DWORD*)&(e->bytes_transfered), pol);

			//��鷵��
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				SIM_LERROR("exfunc.__ConnectEx error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

		virtual bool Send(RefObject<AsyncContext> ref, RefBuff buff)
		{
			SIM_FUNC_DEBUG();
			//�½��¼�
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETSend;
			//e->ref = ref;
			e->buff = buff;
			e->wsa_buf.buf = buff.get();
			e->wsa_buf.len = buff.size();
			DWORD* bytes_transfered = &e->bytes_transfered;

			//����
		/*	static char sbuff[100] = "hello world";
			wsa_buf.buf = sbuff;
			wsa_buf.len = ::strlen(sbuff);
			bytes_transfered = new DWORD();*/
			//pol = new OVERLAPPED();

			DWORD dwFlags = 0;

			int res = WSASend(ref->sock.GetSocket(), &e->wsa_buf, 1,
				bytes_transfered, dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				SIM_LERROR("WSASend error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

		virtual bool Recv(RefObject<AsyncContext> ref)
		{
			SIM_FUNC_DEBUG();
			const unsigned int buff_size = 4 * 1024;
			return Recv(ref, RefBuff(buff_size));
		}

		//���û���
		virtual bool Recv(RefObject<AsyncContext> ref, RefBuff buff)
		{
			SIM_FUNC_DEBUG();
			if (buff.size() <= 0)
			{
				//return Recv(ref);
				SIM_LERROR("recv buff is empty");
				return false;
			}

			//�½��¼�
			IocpAsyncEvent *e = new IocpAsyncEvent;
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETRecv;
			//e->ref = ref;
			e->buff = buff;
			e->buff.set('\0');
			e->wsa_buf.buf = e->buff.get();
			e->wsa_buf.len = e->buff.size();

			DWORD dwFlags = 0;

			int res = WSARecv(ref->sock.GetSocket(), &e->wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, pol, nullptr);
			int err = WSAGetLastError();
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				SIM_LERROR("WSARecv error res=" << res << " WSAGetLastError = " << err);
				return false;
			}
			return true;
		}
		//��������
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			SIM_FUNC_DEBUG();
			//�½��¼�
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETAccept;
			//e->ref = ref;
			const int addr_size = sizeof(SOCKADDR_IN) + 16;
			e->buff = RefBuff(2 * addr_size);

			//����������
			e->accepted = sim::Socket(sim::TCP);
			if (!e->accepted.IsVaild())
			{
				SIM_LERROR("create socket error ");
				return false;
			}
			DWORD dwFlags = 0;

			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__AcceptEx(ref->sock.GetSocket(), e->accepted.GetSocket(),
				e->buff.get(), e->buff.size() - 2 * addr_size, addr_size, addr_size,
				(DWORD*)&e->bytes_transfered, pol);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				e->accepted.Close();//��Ҫ�ر�����
				delete e;
				SIM_LERROR("exfunc.__AcceptEx error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

	protected:
		//������չ����
		WsaExFunction &LoadWsaExFunction()
		{
			SIM_FUNC_DEBUG();
			static WsaExFunction ex_func;
			return ex_func;
		}

	private:
		HANDLE iocp_handler_;
	};
	//iocp ʵ��
	typedef IocpAsync SimAsync;
#endif // ASYNC_IOCP

#ifdef ASYNC_EPOLL
	struct SendBuff
	{
		RefBuff buff;
		unsigned int offset;

	};
	//�첽������
	class EpollAsyncContext :public AsyncContext
	{
	public:
		//�¼���¼
		struct epoll_event ep_event;

		//���Ͷ��м���
		Mutex send_queue_lock;
		Queue<SendBuff> send_queue_buff;
		//���շ��ţ�trueʱ��������
		bool accept_flag;
		//���ӷ��ţ�true ʱ���Ӿ��
		bool connect_flag;
		//�¼����
		uint32_t eflag;
		EpollAsyncContext() :accept_flag(false), connect_flag(false), eflag(EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLET)
		{
			ep_event.data.ptr = (void*)this;
			ep_event.events = eflag;
		}
	};

	class EpollAsync :public Async
	{
	public:
		EpollAsync(unsigned int thread_num = 4096)
		{
			//��ʼ��
			Socket::Init();
			epollfd_ = epoll_create(thread_num);
			if (-1 == epollfd_)
			{
				SIM_LERROR("Failed to create epoll context."<<strerror(errno));
				exit(1);
			}
		}
		virtual ~EpollAsync()
		{
			close(epollfd_);
		}
	public:
		virtual int Poll(unsigned int wait_ms)
		{
			const unsigned int MAXEVENTS = 100;
			struct epoll_event events[MAXEVENTS];
			TimeSpan ts;
			int n = epoll_wait(epollfd_, events, MAXEVENTS, wait_ms);
			SIM_LDEBUG("epoll_wait use " << ts.Get() << " ms");
			if (-1 == n)
			{
				SIM_LERROR("Failed to wait."<<strerror(errno));
				return SOCK_FAILURE;
			}
			for (int i = 0; i < n; i++)
			{
				SIM_LDEBUG("epoll_wait."<< n);
				RefObject<AsyncContext> ref = GetCtx(events[i].data.fd);
				if (!ref)
				{
					SIM_LERROR("not found ref " << events[i].data.fd);
					continue;
				}

				uint32_t ee = events[i].events;

				if (ee & EPOLLHUP || ee & EPOLLERR)
				{
					/*
					* EPOLLHUP and EPOLLERR are always monitored.
					*/
					SIM_LERROR("EPOLLHUP and EPOLLERR are always monitored.fd "<< events[i].data.fd);
					Close(ref->sock.GetSocket(), CloseError);
					continue;
				}

				EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

				if (ep_ref->accept_flag)
				{
					ts.ReSet();

					//accept
					Socket accepted_socket;
					int ret = ep_ref->sock.Accept(accepted_socket, 10);
					if (ret != SOCK_SUCCESS)
					{
						SIM_LERROR(ref->sock.GetSocket()<<" Accept Failed.ret="<< ret<<"." << strerror(errno)<<" flag="<<SIM_HEX(ee));
						SIM_LDEBUG("accept use " << ts.Get() << " ms");
						//Close(ref->sock.GetSocket(), CloseError);
						continue;
					}

					Accept(ref);

					SIM_LDEBUG("accept")
					RefObject<AsyncContext> accepted(new EpollAsyncContext());
					accepted->CopyHandler(ep_ref);
					accepted->sock = accepted_socket;
					accepted->sock.SetNonBlock(true);
					AddCtx(accepted);
					ref->OnAccept(accepted->sock.GetSocket());
					AddEpoll(accepted);

					SIM_LDEBUG("accept use " << ts.Get() << " ms");
				}
				else if (ep_ref->connect_flag)
				{
					ts.ReSet();

					//�����Ѿ�������
					ep_ref->connect_flag = false;
					ref->OnConnect();
					//�ɶ�
					ModifyEpollEvent(ref, EPOLLIN, true);
					SIM_LDEBUG("connect use " << ts.Get() << " ms");
				}
				//��д
				else
				{
					if (EPOLLIN & ee)//�ɶ�
					{
						ts.ReSet();

						RefBuff buff(1024 * 4);
						int ret = ep_ref->sock.Recv(buff.get(), buff.size());
						if (ret < 0)
						{
							SIM_LERROR(ref->sock.GetSocket() << " Recv Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						else if (ret == 0)
						{
							SIM_LINFO(ref->sock.GetSocket() << " Recv 0 socket close.");
							Close(ref->sock.GetSocket(), ClosePassive);
						}
						else
						{
							ep_ref->OnRecvData(buff.get(), ret);
							ModifyEpollEvent(ref, EPOLLIN, true);
							/*ep_ref->eflag = ep_ref->eflag | EPOLLIN;
							ModifyEpoll(ref);*/
						}

						SIM_LDEBUG("recv use " << ts.Get() << " ms");
					}
					if (EPOLLOUT &ee)
					{
						ts.ReSet();

						AutoMutex lk(ep_ref->send_queue_lock);
						QueueNode<SendBuff>*pHead = ep_ref->send_queue_buff.Next(NULL);
						if (NULL == pHead)
						{
							SIM_LERROR(ref->sock.GetSocket() << " send cache is empty.");
							ModifyEpollEvent(ref, EPOLLOUT, false);
							continue;
						}
						int ret = ep_ref->sock.Send(pHead->data.buff.get() + pHead->data.offset, pHead->data.buff.size() - pHead->data.offset);
						if (ret < 0)
						{
							SIM_LERROR(ref->sock.GetSocket() << " Send Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						pHead->data.offset += ret;
						if (pHead->data.offset >= pHead->data.buff.size())
						{
							ep_ref->OnSendComplete(pHead->data.buff.get(), pHead->data.buff.size());
							//ȥ��
							ep_ref->send_queue_buff.PopFront(NULL);
						}
						//ȥ��д�¼�
						if (ep_ref->send_queue_buff.isEmpty())
						{
							ModifyEpollEvent(ref, EPOLLOUT, false);
						}
						else
						{
							//����
							ModifyEpollEvent(ref, EPOLLOUT, true);
						}

						SIM_LDEBUG("send use " << ts.Get() << " ms");
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

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10)
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
			return SOCK_SUCCESS;
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
			return SOCK_SUCCESS;
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
			return Async::Close(handle);
		}
	protected:
		virtual bool Connect(RefObject<AsyncContext> ref, const char* ipaddr, unsigned short port)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			int ret = ep_ref->sock.Connect(ipaddr, port);
			if (ret == 0)
			{
				ep_ref->OnConnect();
				return true;
			}
			ep_ref->connect_flag = true;
			return ModifyEpollEvent(ref, EPOLLOUT, true);
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
			return ModifyEpollEvent(ref, EPOLLOUT, true);
		}

		//��������
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			ep_ref->accept_flag = true;
			return ModifyEpollEvent(ref, EPOLLIN, true);
		}

		virtual bool EpollCtrl(RefObject<AsyncContext> ref, int opt,uint32_t flag)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag=flag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, opt, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				SIM_LERROR(ref->sock.GetSocket() << " epoll_ctl opt "<< opt <<" flag "<<SIM_HEX(flag)<<" Failed." << strerror(errno));
				return false;
			}
			return true;
		}
		//Add
		virtual bool AddEpoll(RefObject<AsyncContext> ref)
		{
			return EpollCtrl(ref, EPOLL_CTL_ADD, EPOLLIN | EPOLLHUP | EPOLLERR);

			/*EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_ADD, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				SIM_LERROR(ref->sock.GetSocket() << " epoll_ctl Failed." << strerror(errno));
				return false;
			}
			return true;*/
		}
		//virtual bool ModifyEpoll(RefObject<AsyncContext> ref)
		//{
		//	/*EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
		//	ep_ref->ep_event.events = ep_ref->eflag;
		//	ep_ref->ep_event.data.ptr = NULL;
		//	ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
		//	if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_MOD, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
		//	{
		//		SIM_LERROR(ref->sock.GetSocket() << " epoll_ctl Failed." << strerror(errno));
		//		return false;
		//	}
		//	return true;*/
		//}
		
		//�޸��¼�is_add =true ��� ���� ɾ��
		virtual bool ModifyEpollEvent(RefObject<AsyncContext> ref, uint32_t _event, bool is_add)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			//���
			if (is_add)
			{
				if (ep_ref->eflag&_event)
				{
					//�Ѿ������ˣ����޸��ˣ�����һ�ε���
					return true;
				}
				return EpollCtrl(ref, EPOLL_CTL_MOD, ep_ref->eflag | _event);
			}
			else
			{
				if (ep_ref->eflag&_event)
				{
					return EpollCtrl(ref, EPOLL_CTL_MOD, ep_ref->eflag &( ~_event));//ȡ��&ȥ��
				}
				//�������ˣ����޸��ˣ�����һ�ε���
				return true;
				
			}
		}

		virtual bool RemoveEpoll(RefObject<AsyncContext> ref)
		{
			return EpollCtrl(ref, EPOLL_CTL_DEL,0);

			/*EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			ep_ref->ep_event.events = ep_ref->eflag;
			ep_ref->ep_event.data.ptr = NULL;
			ep_ref->ep_event.data.fd = ep_ref->sock.GetSocket();
			if (-1 == epoll_ctl(epollfd_, EPOLL_CTL_DEL, ep_ref->sock.GetSocket(), &ep_ref->ep_event))
			{
				SIM_LERROR(ref->sock.GetSocket() << " epoll_ctl Failed." << strerror(errno));
				return false;
			}
			return true;*/
		}

		virtual int Close(AsyncHandle handle, AsyncCloseReason reason)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				RemoveEpoll(ref);
				ReleaseCtx(handle);
				SIM_LDEBUG("handle " << handle << " closed ");
				ref->OnClose(reason, errno);
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}
	private:
		int epollfd_;
	};
	//epollʵ��
	typedef EpollAsync SimAsync;
#endif //ASYNC_EPOLL
}
#endif