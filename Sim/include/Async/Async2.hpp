/*
	异步连接
	第二个版本
*/
#ifndef SIM_ASYNC2_HPP_
#define SIM_ASYNC2_HPP_
//引入日志
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
	
	//声明前置
	struct AsyncContext;

	//异步事件
	struct AsyncEvent
	{
		//传输字节
		unsigned long bytes_transfered ;
		//缓存
		AsyncBuffer buffer;
		//接受的新连接
		Socket accepted;
		//错误码
		int error;
		//类型
		AsyncEventType type;

		//引用上下文
		RefObject<AsyncContext> ctx;

		AsyncEvent():bytes_transfered(0), error(0), type(ET_Error)
		{

		}
	};

	//事件回调
	typedef void(*AsyncSocketHandler)(Socket master,AsyncEvent *pevent,void*pdata);

	//异步上下文
	struct AsyncContext
	{
		//关联sock
		Socket master;

		AsyncSocketHandler handler;

		void*pdata;

		AsyncContext():handler(NULL), pdata(NULL)
		{

		}
	};

	void AsyncRefObjectDelete(void* ptr, void*pdata);

	//异步服务
	class Async:public Memory
	{
		friend void AsyncRefObjectDelete(void* ptr, void*pdata);
	public:
		~Async() {};
	public:
		//新增socket
		virtual bool AddSocket(Socket sock, AsyncSocketHandler handler, void*pdata)=0;

		//移除socket
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

		//接收链接
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
		//锁
		Mutex ctx_s_lock_;
		//上下文映射
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