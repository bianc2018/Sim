/*
	�첽�ӿ�
*/
#ifndef SIM_ASYNC_IO_HPP_
#define SIM_ASYNC_IO_HPP_
//SIM_ASYNC_IO_NEW
//#ifdef SIM_ASYNC_IO_NEW
#include <new>
#include "Socket.hpp"
//#endif

//������
#define ASYNC_SUCCESS SOCK_SUCCESS
#define ASYNC_FAILURE SOCK_FAILURE
#define ASYNC_ERR_BASE (SOCK_ERR_BASE+100)
//�������
#define ASYNC_IDLE			1
//����Ϊ��
#define ASYNC_ERR_SERVICE			(-(ASYNC_ERR_BASE+1))
//������ʱ
#define ASYNC_ERR_TIMEOUT			(-(ASYNC_ERR_BASE+2))
//Ŀ���׽�����Ч
#define ASYNC_ERR_SOCKET_INVAILD	(-(ASYNC_ERR_BASE+3))
//�յĻ���
#define ASYNC_ERR_EMPTY_BUFF		(-(ASYNC_ERR_BASE+4))

//�첽��ʶ
//����
#define ASYNC_FLAG_CONNECT			(1<<0)
//��������
#define ASYNC_FLAG_ACCEPT			(1<<1)
//��������
#define ASYNC_FLAG_RECV				(1<<2)
//����
#define ASYNC_FLAG_SEND				(1<<3)
//�Ͽ�����
#define ASYNC_FLAG_DISCONNECT		(1<<4)
//������
#define ASYNC_FLAG_ERROR		(1<<5)
//�ͷ��ڴ�
#define ASYNC_FLAG_RELEASE		(1<<6)

namespace sim
{
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
		//��������ݴ�С
		unsigned long long               bytes_transfered;
		//����
		int error;
		//accept�¼����淵�صĿͻ�������
		SOCKET accept_client;
	};

	typedef void(*AsyncEventHandler)(BaseAsyncSocket*sock, Event*e, void*pdata);

	//�ڴ���亯��
	typedef void* (*AsyncEventMalloc)(size_t size);
	typedef void(*AsyncEventFree)(void*);
	//�첽ϵͳ
	class BaseAsyncEventService
	{
	public:
		BaseAsyncEventService() 
			:run_flag_(true) , malloc_(::malloc),free_(::free){};
		virtual ~BaseAsyncEventService() {};
		//����һ���׽���
		virtual BaseAsyncSocket* CreateBySocket(SOCKET socket) = 0;
		virtual BaseAsyncSocket* CreateByType(SockType type) = 0;
		virtual BaseAsyncSocket* Create(int af, int type, int protocol) = 0;

		//�ͷ�
		virtual SockRet Release(BaseAsyncSocket*psock) = 0;

		////����
		//virtual SockRet Add(BaseAsyncSocket*psock) = 0;
		////�Ƴ�
		//virtual SockRet Remove(BaseAsyncSocket*psock)=0;

		//����
		virtual SockRet Run(unsigned int wait_ms)
		{
			run_flag_ = true;
			while (run_flag_)
				RunOnce(wait_ms);
			return 0;
		}
		//���� ����һ��
		virtual SockRet RunOnce(unsigned int wait_ms)=0;

		//�˳�
		virtual SockRet Exit()
		{
			run_flag_ = false;
			return SockRet(0);
		}

		//�����ڴ�����
		virtual bool SetAlloc(AsyncEventMalloc m, AsyncEventFree f)
		{
			if (m && f)
			{
				malloc_ = m;
				free_ = f;
			}
			return false;
		}
		virtual void* Malloc(size_t size)
		{
			if (malloc_)
				return malloc_(size);
			return NULL;
		}
		virtual void Free(void*p)
		{
			if (free_)
				free_(p);
		}
	//protected:
		//����һ���¼�
		virtual Event *MallocEvent()
		{
			Event*pe = (Event*)Malloc(sizeof(Event));
			if (pe)
			{
				//��ʼ��
				memset(pe, 0, sizeof(*pe));
			}
			return pe;
		}
		virtual void FreeEvent(Event *pe)
		{
			if (pe)
			{
				Free(pe);
			}
		}
	protected:
		bool run_flag_;
		AsyncEventMalloc malloc_;
		AsyncEventFree free_;
	};

	//�첽socket����
	class BaseAsyncSocket:public Socket
	{
	protected:
		//���캯��
		//BaseAsyncSocket();

		BaseAsyncSocket(BaseAsyncEventService* service, SOCKET sock);
		// SOCK_STREAM tcp SOCK_DGRAM udp
		BaseAsyncSocket(BaseAsyncEventService* service, SockType type);

		BaseAsyncSocket(BaseAsyncEventService* service, int af, int type, int protocol);

		virtual ~BaseAsyncSocket();
		
	public:
		//���þ��
		virtual bool SetHandler(AsyncEventHandler handler, void* pdata);
	protected:
		//�����ص�
		virtual bool CallHandler(Event *e);
	protected:
		AsyncEventHandler handler_;
		void*pdata_;
		BaseAsyncEventService *pevent_;
	};

	/*inline BaseAsyncSocket::BaseAsyncSocket()
		:Socket(), handler_(NULL), pdata_(NULL), pevent_(NULL) {}*/

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, SOCKET sock)
		:Socket(sock), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		SetNonBlock(true);
		/*if (pevent_)
		{
			pevent_->AddSock(this);
		}*/
	}

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, SockType type)
		:Socket(type), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		SetNonBlock(true);
		/*if (pevent_)
		{
			pevent_->AddSock(this);
		}*/
	}

	inline BaseAsyncSocket::BaseAsyncSocket(BaseAsyncEventService * service, int af, int type, int protocol)
		: Socket(af, type, protocol), handler_(NULL), pdata_(NULL), pevent_(service)
	{
		SetNonBlock(true);
		/*if (pevent_)
		{
			pevent_->AddSock(this);
		}*/
	}

	inline BaseAsyncSocket::~BaseAsyncSocket()
	{
		/*if (pevent_)
		{
			pevent_->RemoveSock(this);
		}*/
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
	typedef BaseAsyncSocket  AsyncSocket;
	typedef IocpAsyncEventService  AsyncEventService;
}
#endif // OS_WINDOWS
#endif