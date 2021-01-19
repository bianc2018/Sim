/*
	�첽�ӿ�
*/
#ifndef SIM_ASYNC_IO_HPP_
#define SIM_ASYNC_IO_HPP_
#include "Socket.hpp"
namespace sim
{
#define ASYNC_FLAG_CONNECT			(1<<0)
#define ASYNC_FLAG_ACCEPT			(1<<1)
#define ASYNC_FLAG_RECV				(1<<2)
#define ASYNC_FLAG_SEND				(1<<3)
#define ASYNC_FLAG_DISCONNECT		(1<<4)

	class BaseAsyncSocket;

	struct AsyncBuffer
	{
		char*ptr;
		unsigned int size;
	};

	struct Event
	{
		//���λ
		unsigned int event_flag;
		//����
		AsyncBuffer cache;
		//����
		int error;
		//accept�¼����淵�صĿͻ�������
		SOCKET accept_client;
	};

	typedef void(*AsyncEventHandler)(BaseAsyncSocket*sock, Event*e, void*pdata);

	//�첽ϵͳ
	class BaseAsyncEventService
	{
	public:
		virtual ~BaseAsyncEventService() {};

		virtual SockRet AddSock(BaseAsyncSocket*psock) =0;
		virtual SockRet RemoveSock(BaseAsyncSocket*psock)=0;
		virtual SockRet Run()=0;
	private:

	};

	//�첽socket����
	class BaseAsyncSocket:public Socket
	{
	public:
		//���캯��
		BaseAsyncSocket();

		BaseAsyncSocket(BaseAsyncEventService* service, SOCKET sock);
		// SOCK_STREAM tcp SOCK_DGRAM udp
		BaseAsyncSocket(BaseAsyncEventService* service, SockType type);

		BaseAsyncSocket(BaseAsyncEventService* service, int af, int type, int protocol);

		virtual ~BaseAsyncSocket();
		//���þ��
		virtual bool SetHandler(AsyncEventHandler handler, void*pdata);
	public:
		
	protected:
		//�����ص�
		virtual bool CallHandler(Event *e);
	protected:
		AsyncEventHandler handler_;
		void*pdata_;
		BaseAsyncEventService *pevent_;
	};

	inline BaseAsyncSocket::BaseAsyncSocket()
		:Socket(), handler_(NULL), pdata_(NULL), pevent_(NULL) {}

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, SOCKET sock)
		:Socket(sock), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		if (pevent_)
		{
			pevent_->AddSock(this);
		}
	}

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, SockType type)
		:Socket(type), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		if (pevent_)
		{
			pevent_->AddSock(this);
		}
	}

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, int af, int type, int protocol)
		: Socket(af, type, protocol), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		if (pevent_)
		{
			pevent_->AddSock(this);
		}
	}

	inline BaseAsyncSocket::~BaseAsyncSocket()
	{
		if (pevent_)
		{
			pevent_->RemoveSock(this);
		}
	}

	inline bool BaseAsyncSocket::SetHandler(AsyncEventHandler handler, void * pdata)
	{
		if (handler&&pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
			return true;
		}
		return false;
	}
	
	inline bool BaseAsyncSocket::CallHandler(Event * e)
	{
		if (handler_)
		{
			handler_(this, e, pdata_);
			return true;
		}
		return false;
	}

}

#ifdef OS_WINDOWS
#include "async/iocp.hpp"
namespace sim {
	typedef IocpAsyncSocket  AsyncSocket;
	typedef IocpAsyncEventService  AsyncEventService;
}
#endif // OS_WINDOWS
#endif