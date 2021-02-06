/*
	�첽����
	�ڶ����汾
*/
#ifndef SIM_ASYNC2_HPP_
#define SIM_ASYNC2_HPP_
//������־
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
#include "Memory.hpp"
#include "Mutex.hpp"
#include "Socket.hpp"
#include "RbTree.hpp"
#include "RefObject.hpp"
namespace sim
{
	enum AsyncEventType
	{
		ET_Error,
		ET_Connect,
		ET_Accept,
		ET_Send,
		ET_Recv,
		ET_DisConnect,
	};

	struct AsyncBuffer
	{
		char *ptr;
		unsigned int size;

		AsyncBuffer():ptr(NULL), size(0)
		{}
	};
	
	//����ǰ��
	struct AsyncContext;

	//�첽�¼�
	struct AsyncEvent
	{
		//�����ֽ�
		unsigned long bytes_transfered ;
		//����
		AsyncBuffer buffer;
		//���ܵ�������
		Socket accepted;
		//������
		int error;
		//����
		AsyncEventType type;

		//����������
		RefObject<AsyncContext> ctx;

		AsyncEvent():bytes_transfered(0), error(0), type(ET_Error)
		{

		}
	};

	//�¼��ص�
	typedef void(*AsyncSocketHandler)(Socket master,AsyncEvent *pevent,void*pdata);

	//�첽������
	struct AsyncContext
	{
		//����sock
		Socket master;

		AsyncSocketHandler handler;

		void*pdata;

		AsyncContext():handler(NULL), pdata(NULL)
		{

		}
	};

	void AsyncRefObjectDelete(void* ptr, void*pdata);

	//�첽����
	class Async:public Memory
	{
		friend void AsyncRefObjectDelete(void* ptr, void*pdata);
	public:
		~Async() {};
	public:
		//����socket
		virtual bool AddSocket(Socket sock, AsyncSocketHandler handler, void*pdata)=0;

		//�Ƴ�socket
		virtual bool DelSocket(Socket sock, bool bclose)=0;

		virtual bool RunOnce(unsigned wait_ms)=0;
	public:
		virtual bool Connect(Socket sock,const char* ipaddr, unsigned short port)=0;

		virtual bool Bind(Socket sock,const char* ipaddr, unsigned short port)=0;

		virtual bool Listen(Socket sock, int backlog)
		{
			return sock.Listen(backlog)==0;
		}

		virtual bool Send(Socket sock,const char* data, unsigned int data_len)=0;

		virtual bool Recv(Socket sock,char* data, unsigned int data_len)=0;

		//��������
		virtual bool Accept(Socket sock)=0;
	protected:
		virtual bool AddCtx(RefObject<AsyncContext> ctx)
		{
			AutoMutex lk(ctx_s_lock_);
			return ctxs_.Add(ctx->master.GetSocket(), ctx);
		}

		virtual RefObject<AsyncContext> GetCtx(Socket sock)
		{
			AutoMutex lk(ctx_s_lock_);
			RefObject<AsyncContext> temp;
			bool ret = ctxs_.Find(sock.GetSocket(),&temp);
			if (ret)
			{
				return temp;
			}
			return NULL;
		}

		virtual bool DelCtx(Socket sock)
		{
			AutoMutex lk(ctx_s_lock_);
			return ctxs_.Del(sock.GetSocket());
		}

		virtual void CallEvent(AsyncEvent*e)
		{
			RefObject<AsyncContext> p = e->ctx;
			if (p)
			{
				if (p->handler)
					p->handler(p->master, e, p->pdata);
			}
		}

		virtual void FreeAsyncContext(AsyncContext *ctx)
		{
			SIM_MEM_DEL(this, ctx);
		}
	private:
		//��
		Mutex ctx_s_lock_;
		//������ӳ��
		RbTree<RefObject<AsyncContext> > ctxs_;
		
	};

	void sim::AsyncRefObjectDelete(void * ptr, void * pdata)
	{
		if (pdata)
		{
			Async* p = (Async*)pdata;
			p->FreeAsyncContext((AsyncContext*)ptr);
		}
	}
}

#endif