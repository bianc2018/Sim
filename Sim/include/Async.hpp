/*
	异步网络接口
*/
#ifndef SIM_ASYNC_HPP_
#define SIM_ASYNC_HPP_

//是否使用openssl
#ifdef SIM_USE_OPENSSL
#include "SSLCtx.hpp"
#endif //! SIM_USE_OPENSSL

//导入依赖
#include "Socket.hpp"
#include "RefObject.hpp"
#include "RbTree.hpp"
#include "Mutex.hpp"
#include "Queue.hpp"
#include "Timer.hpp"

//日志使能
#define SIM_ENABLE_LOGGER
#ifdef SIM_ENABLE_LOGGER
#include "Logger.hpp"
#else
//空的定义防止编译报错
#ifndef SIM_LOG
namespace sim
{
	//日志级别
	enum LogLevel
	{
		LNone,
		LError,
		LWarn,
		LInfo,
		LDebug,
	};
}		//格式
#define SIM_FORMAT_NUM(num,base,w,f)	
#define SIM_FORMAT_STR(str,w,f)			
#define SIM_HEX(num) SIM_FORMAT_NUM(num,16,8,'0')
#define SIM_FORMAT_STR0(str,w) SIM_FORMAT_STR(str,w,' ')

//防止重名
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 

//新增输出流
#define SIM_LOG_ADD(Stream,...) 
//配置输出句柄
#define SIM_LOG_HANDLER(max_lv,handler,userdata)
//配置控制台输出
#define SIM_LOG_CONSOLE(max_lv)\
				SIM_LOG_ADD(sim::LogConsoleStream,max_lv)

#define SIM_LOG(lv,x)
#define SIM_LDEBUG(x) SIM_LOG(sim::LDebug,x)
#define SIM_LINFO(x) SIM_LOG(sim::LInfo,x)
#define SIM_LWARN(x) SIM_LOG(sim::LWarn,x)
#define SIM_LERROR(x) SIM_LOG(sim::LError,x)
#endif
#endif

//windows平台
#ifdef OS_WINDOWS
	//windows平台使用iocp
	#ifndef ASYNC_IOCP
		#define ASYNC_IOCP
	#endif
	//防止 重复包含
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN  
	#endif
	//使用拓展接口
	#include <winsock2.h>
	#include <MSWSock.h>
#else
	#ifdef OS_LINUX
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
		#ifndef ASYNC_EPOLL
			#define ASYNC_EPOLL
		#endif
	#endif
#endif

#ifndef SIM_ASYNC_MAX_RECV_BUFF_SIZE 
#define SIM_ASYNC_MAX_RECV_BUFF_SIZE 1*1024*1024
#endif
#ifndef SIM_ASYNC_MIN_RECV_BUFF_SIZE 
#define SIM_ASYNC_MIN_RECV_BUFF_SIZE 4*1024
#endif
namespace sim
{
	//事件类型
	enum EType
	{
		//连接建立事件
		ETConnect,
		
		//接受连接事件
		ETAccept,
		
		//接收TCP数据事件
		ETRecv,
		
		//接收UDP数据事件（多了地址信息）
		ETRecvFrom,
		
		//数据发送事件
		ETSend,
		
		//连接关闭事件
		ETClose,
	};
	
	//连接句柄，一个句柄标识一个连接
	typedef SOCKET AsyncHandle;

	//接收连接回调
	typedef void(*AcceptHandler)(AsyncHandle handle, AsyncHandle client, void*data);

	//连接完成回调
	typedef void(*ConnectHandler)(AsyncHandle handle, void*data);
	
	//tcp数据接收回调
	typedef void(*RecvDataHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);

	//udp数据接收回调
	typedef void(*RecvDataFromHandler)(AsyncHandle handle, char *buff, unsigned int buff_len,char *from_ip,unsigned short port, void*data);

	//数据发送完成回调
	typedef void(*SendCompleteHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);

	//连接会话关闭原因枚举
	enum AsyncCloseReason
	{
		//异常关闭（发生了错误，平台错误码error）
		CloseError,
		//主动关闭 （本级调用close关闭）
		CloseActive,
		//会话正常接收关闭 （连接接收到eof）
		ClosePassive,
	};
	//连接关闭回调
	typedef void(*CloseHandler)(AsyncHandle handle, AsyncCloseReason reason, int error, void*data);

	//异步上下文对象基类
	class AsyncContext
	{
	public:
		//关联连接
		Socket sock;

		//连接类型 tcp or udp
		SockType type;
		
		//是否活跃 accept 或者 connect 或者 Listen
		bool is_active;

		AsyncCloseReason close_reason;
		//ssl
#ifdef SIM_USE_OPENSSL
		//ssl会话上下文
		RefObject<SSLCtx> ssl_ctx;
		
		//ssl会话
		SSLSession* ssl_session;
#endif
		//回调句柄
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
		RecvDataFromHandler recvdatafrom_handler;
		void*recvdatafrom_handler_data;

		//上下文data
		void*ctx_data;

		unsigned int min_recv_buff_size;
		unsigned int max_recv_buff_size;
	public:
		AsyncContext(SockType t)
			: accept_handler(NULL), accept_handler_data(NULL)
			, connect_handler(NULL), connect_handler_data(NULL)
			, recvdata_handler(NULL), recvdata_handler_data(NULL)
			, sendcomplete_handler(NULL), sendcomplete_handler_data(NULL)
			, close_handler(NULL), close_handler_data(NULL)
			,recvdatafrom_handler(NULL),recvdatafrom_handler_data(NULL)
			,type(t)
			, min_recv_buff_size(SIM_ASYNC_MIN_RECV_BUFF_SIZE)
			,max_recv_buff_size(SIM_ASYNC_MAX_RECV_BUFF_SIZE)
			, is_active(false)
			, close_reason(CloseError)
#ifdef SIM_USE_OPENSSL
			,ssl_session(NULL)
#endif
		{
			//memset(this, 0, sizeof(*this));
		}

		void CopySSLCtx(const AsyncContext* pctx)
		{
			if (pctx)
			{
#ifdef SIM_USE_OPENSSL
				ssl_ctx = pctx->ssl_ctx;
#endif
			}
		}
		//复制句柄
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
				recvdatafrom_handler = pctx->recvdatafrom_handler;
				recvdatafrom_handler_data = pctx->recvdatafrom_handler_data;
#ifdef SIM_USE_OPENSSL
				ssl_ctx = pctx->ssl_ctx;
#endif
			}
		}
		
		//析构
		virtual ~AsyncContext()
		{
			//释放ssl上下文
			ReleaseSSLCtx();
			
#ifdef OS_WINDOWS
			//回调
			OnClose(close_reason, GetLastError());
#else
			OnClose(close_reason, errno);
#endif
			SIM_LDEBUG("handle " << sock.GetSocket() << " closed ");
			//关闭连接
			sock.Close();
			
		}
		
		void ReleaseSSLCtx()
		{
#ifdef SIM_USE_OPENSSL
			//会话存在 关闭会话
			if (ssl_session)
			{
				ssl_ctx->DelSession(ssl_session);
				ssl_session = NULL;
			}
			//重置ssl环境
			ssl_ctx.reset();
#endif
		}

		//加密数据，输入未加密数据 返回已经加密的数据 非ssl连接返回原始数据
		virtual RefBuff Encrypt(RefBuff input)
		{
#ifdef SIM_USE_OPENSSL
			if (ssl_session)
			{
				//输入数据
				ssl_session->InEncrypt(input.get(), input.size());
				
				//读取已经加密了的数据
				RefBuff output;
				const int buff_size = 4 * 1024;
				char buff[buff_size] = {};
				while (true)
				{
					int len = ssl_session->OutEncrypt(buff, buff_size);
					if (len <= 0)
						break;
					output = output + RefBuff(buff, len);
				}
				return output;
			}
			else
			{
				return input;
			}
#else
			return input;
#endif
		}
		//解密 如上
		virtual RefBuff Decrypt(RefBuff input)
		{
#ifdef SIM_USE_OPENSSL
			if (ssl_session)
			{
				RefBuff output;
				ssl_session->InDecrypt(input.get(), input.size());

				const int buff_size = 4 * 1024;
				char buff[buff_size] = {};
				while (true)
				{
					int len = ssl_session->OutDecrypt(buff, buff_size);
					if (len <= 0)
						break;
					output = output + RefBuff(buff, len);
				}
				return output;
			}
			else
			{
				return input;
			}
#else
			return input;
#endif
		}

		//新建SSL session
		virtual bool NewSSLSession()
		{
#ifdef SIM_USE_OPENSSL
			if (ssl_session)
			{
				//进行握手，这里可能会堵塞，后续优化
				if (false == ssl_session->HandShake())
				{
					//握手失败
					SIM_LERROR("sock.GetSocket()" << sock.GetSocket()
						<< " HandShake fail");
					return false;
				}
				//已存在
				return true;
			}
			else
			{
				if (ssl_ctx)
				{
					//新建连接
					ssl_session =ssl_ctx->NewSession(sock.GetSocket());
					if (NULL == ssl_session)
					{
						SIM_LERROR("sock.GetSocket()"<< sock.GetSocket()
							<< " NewSession return NULL");
						return false;
					}
					//进行握手，这里可能会堵塞，后续优化
					if (false == ssl_session->HandShake())
					{
						//握手失败
						SIM_LERROR("sock.GetSocket()" << sock.GetSocket()
							<< " HandShake fail");
						return false;
					}
				}
				return true;
			}
#else
			return false;
#endif
		}
	public:
		//回调触发
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
#ifdef SIM_USE_OPENSSL
			//解密数据
			RefBuff data = Decrypt(RefBuff(buff, buff_len));

			if (recvdata_handler&&sock.IsVaild()&& data.size()>0)
				recvdata_handler(sock.GetSocket(), data.get(), data.size(), recvdata_handler_data);
#else
			if (recvdata_handler&&sock.IsVaild())
				recvdata_handler(sock.GetSocket(), buff, buff_len, recvdata_handler_data);
#endif
		}
		virtual void OnRecvDataFrom(char *buff, unsigned int buff_len,char *from_ip, unsigned short port)
		{
#ifdef SIM_USE_OPENSSL
			//将数据解密
			RefBuff data = Decrypt(RefBuff(buff, buff_len));

			if (recvdatafrom_handler&&sock.IsVaild() && data.size() > 0)
				recvdatafrom_handler(sock.GetSocket(), data.get(), data.size(), from_ip, port, recvdata_handler_data);
#else
			if (recvdatafrom_handler&&sock.IsVaild())
				recvdatafrom_handler(sock.GetSocket(), buff, buff_len, from_ip, port, recvdata_handler_data);
#endif
			
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

	//异步服务对象基类
	class Async
	{
		//不允许复制拷贝
		Async(const Async &other) {};
		Async& operator=(const Async &other) {};
	public:
		Async() {};

		//运行接口，取事件和分发
		virtual int Poll(unsigned int wait_ms) = 0;

		//创建空句柄
		virtual AsyncHandle CreateTcpHandle() = 0;
		virtual AsyncHandle CreateUdpHandle() = 0;
		//type 句柄类型
		virtual AsyncHandle CreateHandle(SockType type) = 0;

		//启动TCP服务
		//handle 句柄
		//bind_ipaddr \0 结尾字符串，绑定地址，可以是ip也可以是NULL,暂时只支持ipv4
		//bind_port 绑定端口
		//acctept_num 接收连接数
		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10) = 0;
		
		//启动tcp连接
		//handle 句柄
		//remote_ipaddr \0 结尾字符串，远程主机地址，ip地址,暂时只支持ipv4
		//remote_port 远程端口
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port) = 0;

		//启动udp连接
		//handle 句柄
		//bind_ipaddr \0 结尾字符串，绑定地址，可以是ip也可以是NULL,暂时只支持ipv4
		//bind_port 绑定端口
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port) = 0;

		//ssl协议簇
#ifdef SIM_USE_OPENSSL
		//将重置连接为ssl，如果原来连接是ssl，则将会重置为 meth;
		virtual int ConvertToSSL(AsyncHandle handle, const SSL_METHOD *meth)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
				return SOCK_FAILURE;
			return ConvertToSSL(ref, meth);
		}
#endif
		//return SOCK_FAILURE; is_server 是否为协议服务端 is_add 是否已经添加
		virtual int ConvertToSSL(AsyncHandle handle, bool is_server, bool is_add)
		{
#ifndef SIM_USE_OPENSSL
			return SOCK_FAILURE;
#else
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
				return SOCK_FAILURE;

			if (!is_add)
			{
				//释放
				ref->ReleaseSSLCtx();
				return SOCK_SUCCESS;
			}
			else
			{
				//根据连接类型选择不同的method
				if (is_server)
				{
					if (ref->type == TCP)
						return ConvertToSSL(ref, SSLv23_server_method());
					else if (ref->type == UDP)
						return ConvertToSSL(ref, DTLSv1_2_server_method());
				}
				else
				{
					if (ref->type == TCP)
						return ConvertToSSL(ref, SSLv23_client_method());
						//return ConvertToSSL(ref, TLS_client_method());
					else if (ref->type == UDP)
						return ConvertToSSL(ref, DTLSv1_2_client_method());
				}
			}
			return SOCK_FAILURE;
#endif
		}
		
		//设置 ssl证书 0无错误
		//handle 句柄
		//pub_key_file 公钥证书
		//pri_key_file 私钥证书
		virtual int SetSSLKeyFile(AsyncHandle handle, const char *pub_key_file, const char*pri_key_file)
		{
#ifndef SIM_USE_OPENSSL
			//不使用opensll直接保存
			return SOCK_FAILURE;
#else
			SIM_FUNC_DEBUG();
			//都不可为NULL
			if (NULL == pub_key_file || NULL == pri_key_file)
			{
				SIM_LERROR("SetSSLKeyFile Fail,some file is NULL");
				return SOCK_FAILURE;
			}
			//获取连接上下文
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				SIM_LDEBUG("handle " << handle << " SetSSLKeyFile  pub:"<< pub_key_file<<",pri:"<< pri_key_file);
				//获取ssl上下文
				if (ref->ssl_ctx)
					if(ref->ssl_ctx->SetKeyFile(pub_key_file, pri_key_file))//设置
						return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
#endif
		}
		
		//设置ssl主机名称
		virtual int SetSSLHostName(AsyncHandle handle, const char* hostname)
		{
#ifndef SIM_USE_OPENSSL
			//不使用opensll直接保存
			return SOCK_FAILURE;
#else
			SIM_FUNC_DEBUG();
			//都不可为NULL
			if (NULL == hostname )
			{
				SIM_LERROR("SetSSLHostName Fail,hostname is NULL");
				return SOCK_FAILURE;
			}
			//获取连接上下文
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				SIM_LDEBUG("handle " << handle << " SetSSLHostName  hostname:" << hostname);
				//获取ssl上下文
				if (ref->ssl_ctx)
				{
					if (NULL == ref->ssl_session)
					{
						ref->ssl_session = ref->ssl_ctx->NewSession(ref->sock.GetSocket());
						if (NULL == ref->ssl_session)
							return SOCK_FAILURE;
					}
					if (ref->ssl_session->SetHostName(hostname))//设置
						return SOCK_SUCCESS;
				}
			}
			return SOCK_FAILURE;
#endif
		}

		/*
		char* hostname = "www.reuters.com";
		SSL_CTX* ctx = SSL_CTX_new(TLS_method());
		SSL* ssl = SSL_new(ctx);
		BIO rbio = BIO_new(BIO_s_mem());
		BIO wbio = BIO_new(BIO_s_mem());
		SSL_set_bio(ssl, rbio, wbio);
		SSL_set_connect_state(ssl);
		SSL_set_tlsext_host_name(ssl, hostname);
		*/


		//TCP数据发送接口
		//handle 句柄
		//buff 发送缓存
		//buff_len 发送数据长度
		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len) = 0;
		
		//UDP数据发送接口
		//handle 句柄
		//buff 发送缓存
		//buff_len 发送数据长度
		//remote_ipaddr \0 结尾字符串，远程主机地址，ip地址,暂时只支持ipv4
		//remote_port 远程端
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* remote_ipaddr, unsigned short remote_port) = 0;

		//主动关闭连接
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

		//是否存在
		virtual bool IsHas(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				return true;
			}
			return false;
		}
		virtual bool IsActive(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				return ref->is_active;
			}
			return false;
		}
	public:
		//句柄设置
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
		virtual void SetRecvDataFromHandler(AsyncHandle handle, RecvDataFromHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->recvdatafrom_handler = handler;
				ref->recvdatafrom_handler_data = pdata;
			}
		}
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

		//设置外部上下文，仅作记录
		virtual void SetCtxData(AsyncHandle handle, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->ctx_data = pdata;
			}
		}
		//获取ctx数据
		virtual void* GetCtxData(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				return ref->ctx_data ;
			}
			return NULL;
		}
	protected:
		//将上下文与句柄绑定
		virtual void AddCtx(RefObject<AsyncContext> ctx)
		{
			if (ctx)
			{
				AutoMutex lk(ctx_s_lock_);
				ctx_s_.Add(ctx->sock.GetSocket(), ctx);
			}
		}
		
		//根据句柄获取上下文
		virtual RefObject<AsyncContext> GetCtx(AsyncHandle handle)
		{
			AutoMutex lk(ctx_s_lock_);
			RefObject<AsyncContext> ref;
			ctx_s_.Find(handle, &ref);
			return ref;
		}
		
		//释放上下文
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
				ref->is_active = false;
				ref->close_reason = reason;
				//先释放上下文
				ReleaseCtx(handle);
//				SIM_LDEBUG("handle " << handle << " closed ");
//#ifdef OS_WINDOWS
//				//回调
//				ref->OnClose(reason, GetLastError());
//#else
//				ref->OnClose(reason, errno);
//#endif
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}

#ifdef SIM_USE_OPENSSL
		virtual int ConvertToSSL(RefObject<AsyncContext> ref, const SSL_METHOD *meth)
		{
			//已存在的先删除掉
			if (ref->ssl_session)
			{
				ref->ssl_ctx->DelSession(ref->ssl_session);
				ref->ssl_session = NULL;
			}
			//重置
			ref->ssl_ctx = RefObject<SSLCtx>(new SSLCtx(meth));
			return SOCK_SUCCESS;
		}
#endif
	private:
		//锁
		Mutex ctx_s_lock_;
		//句柄与上下文映射集
		RbTree<RefObject<AsyncContext> > ctx_s_;
	};

	//iocp实现
#ifdef ASYNC_IOCP
	//异步事件
	class IocpAsyncEvent
	{
	public:
		//IO重叠对象
		OVERLAPPED  overlapped;
		
		//时间类型
		EType type;

		//子连接存在 acceptex时候启用
		Socket accepted;

		//缓存
		RefBuff buff;
		//buff的数据引用
		WSABUF wsa_buf;
		
		//传输的字节数目
		DWORD bytes_transfered;
		//RefObject<AsyncContext> ref;

		//地址 用于SendTo or Recvfrom
		struct sockaddr_in temp_addr;
		int temp_addr_len;

		//初始化
		IocpAsyncEvent() :bytes_transfered(0)
		{
			//memset(&wsa_buf, 0, sizeof(wsa_buf));
			memset(&overlapped, 0, sizeof(overlapped));
			memset(&wsa_buf, 0, sizeof(wsa_buf));
			memset(&temp_addr, 0, sizeof(temp_addr));
			temp_addr_len = sizeof(temp_addr);
			//printf("new iocp event %p\n", this);
		}
		~IocpAsyncEvent()
		{
			//printf("delete iocp event %p\n", this);
		}

	};

	//异步上下文
	class IocpAsyncContext :public AsyncContext
	{
	public:
		//使用 Connect 之前需要绑定一个本地端口，如果=true则标识已经绑定过了，=false则随机绑定一个端口
		bool bind_flag;
		IocpAsyncContext(SockType type) :bind_flag(false), AsyncContext(type)
		{
		}
		~IocpAsyncContext()
		{
			
		}
	};

	//wsa拓展函数加载对象
	class WsaExFunction
	{
	public:
		WsaExFunction()
		{
			//初始化socket
			Socket::Init();
			
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

	//iocp实现的异步网络服务
	class IocpAsync :public Async
	{
	public:
		//thread_num 绑定的线程数
		IocpAsync(unsigned int thread_num = 0)
		{
			SIM_FUNC_DEBUG();
			//初始化
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
		//创建空句柄
		virtual AsyncHandle CreateHandle(SockType type)
		{
			SIM_FUNC_DEBUG();
			//创建对应上下文对象
			RefObject<AsyncContext> ref(new IocpAsyncContext(type));
			//创建对应空socket
			ref->sock = Socket(type);
			//检查是否创建成功
			if (!ref->sock.IsVaild())
			{
				SIM_LERROR("sock Create error");
				return SOCK_FAILURE;
			}
			//建立映射
			AddCtx(ref);
			SIM_LDEBUG((sim::TCP == type ? "TCP" : "UDP") << ".handle " << ref->sock.GetSocket() << " is cteated");
			//返回创建的句柄，句柄实际是对应的套接字
			return ref->sock.GetSocket();
		}
		virtual AsyncHandle CreateTcpHandle()
		{
			SIM_FUNC_DEBUG();
			return CreateHandle(TCP);
		}
		virtual AsyncHandle CreateUdpHandle()
		{
			SIM_FUNC_DEBUG();
			return CreateHandle(UDP);
		}

		//添加一个TCP服务器
		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}
			ref->sock.SetReusePort(true);

			//绑定地址
			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				SIM_LERROR("handle " << handle << " Bind ipaddr:" << (bind_ipaddr == NULL ? "NULL" : bind_ipaddr) << ":" << bind_port << " fail,ret=" << ret);
				ReleaseCtx(handle);
				return ret;
			}

			//监听
			const int listen_size = 1024 * 4;
			ret = ref->sock.Listen(listen_size);
			if (ret != SOCK_SUCCESS)
			{
				SIM_LERROR("handle " << handle << " Listen listen_size:" << listen_size << " fail,ret=" << ret);
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			//设置非堵塞
			ref->sock.SetNonBlock(true);

			//绑定完成端口
			HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_handler)
			{
				//ReleaseCtx(ref->sock.GetSocket());
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}

			//根据acctept_num发起accept 接收连接
			const int accept_size = 32;
			if (acctept_num <= 0)
				acctept_num = accept_size;

			ref->is_active = true;
			SIM_LERROR("handle " << handle << " accept_size=" << acctept_num);
			//accept
			for (int i = 0; i < acctept_num; ++i)
			{
				Accept(ref);
			}
			return SOCK_SUCCESS;
		}

		//添加一个TCP连接
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}
			//设置为非堵塞
			ref->sock.SetNonBlock(true);

			//绑定完成端口
			HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_handler)
				/*if (NULL == CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
					(ULONG_PTR)(ref->sock.GetSocket()), 0))*/
			{
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}

			//发起连接请求
			SIM_LDEBUG("handle " << handle << " add to iocp");
			if (!Connect(ref, remote_ipaddr, remote_port))
			{
				SIM_LERROR("handle " << handle << " Connect fail");
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			return SOCK_SUCCESS;
		}
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}

			//绑定本地端口
			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				SIM_LERROR("handle " << handle << " Bind ipaddr:" << bind_ipaddr << ":" << bind_port << " fail,ret=" << ret);
				ReleaseCtx(handle);
				return ret;
			}

			//设置为非堵塞
			ref->sock.SetNonBlock(true);

			//绑定完成端口
			HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_handler)
			{
				//ReleaseCtx(ref->sock.GetSocket());
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			ref->is_active = true;
			//开始接收数据
			return RecvFrom(ref) ? SOCK_SUCCESS : SOCK_FAILURE;
		}

		//事件循环 wait_ms最大等待时间
		virtual int Poll(unsigned wait_ms)
		{
			SIM_FUNC_DEBUG();
			//传输数据长度
			DWORD               bytes_transfered = 0;
			//iocp ctx
			SOCKET socket = 0;
			//IO重叠对象指针
			OVERLAPPED          *over_lapped = NULL;

			//取事件数据
			BOOL res = GetQueuedCompletionStatus(iocp_handler_,
				&bytes_transfered, PULONG_PTR(&socket),
				&over_lapped, wait_ms);

			//调试打印
			SIM_LDEBUG("GetQueuedCompletionStatus res=" << res
				<< " socket " << socket << " bytes_transfered " << bytes_transfered
				<< " over_lapped=0x" << SIM_HEX(over_lapped))

				if (over_lapped)
				{
					//转换为事件对象
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);

					//获取上下文
					RefObject<AsyncContext> ref = GetCtx(socket);
					if (ref)
					{
						//设置传输字节数
						socket_event->bytes_transfered = bytes_transfered;
						//获取错误码
						DWORD dw_err = GetLastError();
						if (FALSE == res)
						{
							//发生错误，关闭连接
							//超时
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
							//接收到连接
							if (socket_event->type == ETAccept)
							{
								//先发送下一个接收请求，防止来不及接收下一个连接
								if (false == Accept(ref))
								{
									SIM_LERROR("Accept fail " << " sock= " << ref->sock.GetSocket());
									delete socket_event;
									return SOCK_FAILURE;
								}

								//创建新的上下文
								RefObject<AsyncContext> accepted(new IocpAsyncContext(sim::TCP));
								
								//设置连接
								accepted->sock = socket_event->accepted;

								//创建ssl 会话
#ifdef SIM_USE_OPENSSL
								accepted->CopySSLCtx(ref.get());
								if (!accepted->NewSSLSession())
								{
									SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket() << " sock= " << accepted->sock.GetSocket());
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}
#endif

								//需要加到iocp队列里面
								HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)accepted->sock.GetSocket(), iocp_handler_,
									(ULONG_PTR)(accepted->sock.GetSocket()), 0);
								if (NULL == iocp_handler)
								{
									SIM_LERROR("CreateIoCompletionPort fail iocp_handler_=" << SIM_HEX(iocp_handler)
										<< " sock= " << accepted->sock.GetSocket() << "  WSAGetLastError()=" << WSAGetLastError());
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}

								//拷贝父连接的回调
								accepted->CopyHandler(ref.get());
								//添加映射
								AddCtx(accepted);
								accepted->is_active = true;
								//回调
								ref->OnAccept(accepted->sock.GetSocket());

								//接收数据
								if (false == Recv(accepted))
								{
									SIM_LERROR("Recv fail  sock= " << accepted->sock.GetSocket());
									Close(accepted->sock.GetSocket(), CloseError);
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}

							}
							//连接成功
							else if (socket_event->type == ETConnect)
							{
								//初始化ssl会话
#ifdef SIM_USE_OPENSSL
								if (!ref->NewSSLSession())
								{
									SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), CloseError);//握手失败关闭连接
									delete socket_event;
									return SOCK_FAILURE;
								}
#endif
								ref->is_active = true;
								//回调
								ref->OnConnect();

								//Recv(ref);//接收数据
								if (false == Recv(ref))//接收数据
								{
									SIM_LERROR("Recv fail  sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), CloseError);
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}
							}
							//udp接收数据完成
							else if (socket_event->type == ETRecvFrom)
							{
								//接收数据为0则是对端已经关闭连接了
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("recvfrom socket_event->bytes_transfered=0 ,socket is end");
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//解析地址
									const int ip_len = 32;
									char ip[ip_len] = { 0 };
									unsigned short port = 0;
									ref->sock.AddressToIpV4(&socket_event->temp_addr, ip, ip_len, &port);

									//回调
									ref->OnRecvDataFrom(socket_event->buff.get(), socket_event->bytes_transfered, ip, port);

									//如果是buff不足可以拓展一下
									if (socket_event->bytes_transfered >= socket_event->buff.size()
										&& socket_event->buff.size()<ref->max_recv_buff_size)
									{
										unsigned int now = socket_event->bytes_transfered*1.5 + 1;
										socket_event->buff = RefBuff(now> ref->max_recv_buff_size? ref->max_recv_buff_size :now);
									}

									if (false == RecvFrom(ref, socket_event->buff))//接收数据
									{
										SIM_LERROR("recvfrom fail " << " sock= " << ref->sock.GetSocket());
										Close(ref->sock.GetSocket(), CloseError);
										//printf("delete event %p at %d\n", socket_event, __LINE__);
										delete socket_event;
										return SOCK_FAILURE;
									}
								}
							}
							//tcp 接收数据完成
							else if (socket_event->type == ETRecv)
							{
								//接收结束了
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("recv socket_event->bytes_transfered=0 ,socket is end.sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//接收回调
									ref->OnRecvData(socket_event->buff.get(), socket_event->bytes_transfered);

									//如果是buff不足可以拓展一下
									if (socket_event->bytes_transfered >= socket_event->buff.size()
										&& socket_event->buff.size() < ref->max_recv_buff_size)
									{
										unsigned int now = socket_event->bytes_transfered*1.5 + 1;
										socket_event->buff = RefBuff(now > ref->max_recv_buff_size ? ref->max_recv_buff_size : now);
									}
									if (ref->is_active)
									{
										//继续发送接收数据请求
										if (false == Recv(ref, socket_event->buff))//接收数据
										{
											SIM_LERROR("Recv fail " << " sock= " << ref->sock.GetSocket());
											Close(ref->sock.GetSocket(), CloseError);
											//printf("delete event %p at %d\n", socket_event, __LINE__);
											delete socket_event;
											return SOCK_FAILURE;
										}
									}
								}
							}
							//发送数据完成
							else if (socket_event->type == ETSend)
							{
								//数据传输0 这个也是错误
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("Send socket_event->bytes_transfered=0 ,socket is end.sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//发送完成回调
									ref->OnSendComplete(socket_event->buff.get(), socket_event->bytes_transfered);
								}
							}
							//未知的类型
							else
							{
								//这里不做任何事情
								SIM_LWARN("event->type =" << socket_event->type << " not do something");
							}
						}
					}
					else
					{
						//拿不到上下文，连接可能已经被关闭了，只是删除事件对象，不做其他操作
						SIM_LERROR("socket " << socket << " not found ref");
						//printf("delete event %p at %d\n", socket_event, __LINE__);
						delete socket_event;
						return SOCK_FAILURE;
					}

					//无错误
					//printf("delete event %p at %d\n", socket_event, __LINE__);
					delete socket_event;
					return SOCK_SUCCESS;
				}
				else
				{
					//over_lapped==NULL 出现异常
					return SOCK_FAILURE;
				}

		}
		
		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			SIM_FUNC_DEBUG();
			//如果是空的话就直接返回成功吧
			if (buff == NULL || 0 == buff_len)
			{
				SIM_LWARN("Send Empty Buff!");
				return SOCK_SUCCESS;
			}
			//获取句柄
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				//使用下层接口发送数据
#ifdef SIM_USE_OPENSSL
				return Send(ref, ref->Encrypt(RefBuff(buff, buff_len))) ? SOCK_SUCCESS : SOCK_FAILURE;
#else
				return Send(ref, RefBuff(buff, buff_len)) ? SOCK_SUCCESS : SOCK_FAILURE;
#endif
			}
			return SOCK_FAILURE;
		}
		
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port)
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
#ifdef SIM_USE_OPENSSL
				return Send(ref, ref->Encrypt(RefBuff(buff, buff_len))) ? SOCK_SUCCESS : SOCK_FAILURE;
#else
				return SendTo(ref, RefBuff(buff, buff_len), ipaddr, port) ? SOCK_SUCCESS : SOCK_FAILURE;
#endif
				/*return SendTo(ref, RefBuff(buff, buff_len), ipaddr,port) ? SOCK_SUCCESS : SOCK_FAILURE;*/
			}
			return SOCK_FAILURE;
		}

	protected:
		//连接
		virtual bool Connect(RefObject<AsyncContext> ref, const char* ipaddr, unsigned short port)
		{
			SIM_FUNC_DEBUG();
			//创建sockaddr_in结构体变量
			struct sockaddr_in addr;
			if (!Socket::IpToAddressV4(ipaddr, port, &addr))
			{
				SIM_LERROR("IpToAddressV4 error ip " << ipaddr << ":" << port);
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
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETConnect;
			//使用拓展函数发送连接请求
			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__ConnectEx(ref->sock.GetSocket(),
				(sockaddr*)&addr, sizeof(addr), (PVOID)NULL, 0, (DWORD*)&(e->bytes_transfered), pol);

			//检查返回
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
			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETSend;
			//缓存
			e->buff = buff;
			e->wsa_buf.buf = buff.get();
			e->wsa_buf.len = buff.size();
			DWORD* bytes_transfered = &e->bytes_transfered;

			DWORD dwFlags = 0;

			//发送请求
			int res = WSASend(ref->sock.GetSocket(), &e->wsa_buf, 1,
				bytes_transfered, dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;//失败删除事件
				SIM_LERROR("WSASend error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

		virtual bool SendTo(RefObject<AsyncContext> ref, RefBuff buff, const char* ipaddr, unsigned short port)
		{
			SIM_FUNC_DEBUG();
			//新建事件
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

			//转换为结构体
			ref->sock.IpToAddressV4(ipaddr, port, &e->temp_addr);

			DWORD dwFlags = 0;

			//这里使用WSASendTo接口
			int res = WSASendTo(ref->sock.GetSocket(), &e->wsa_buf, 1,
				bytes_transfered, dwFlags, (struct sockaddr*)&e->temp_addr, e->temp_addr_len, pol, nullptr);

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
			//默认缓存为4K
			//const unsigned int buff_size = SIM_ASYNC_MIN_RECV_BUFF_SIZE;
			return Recv(ref, RefBuff(ref->min_recv_buff_size));
		}
		
		virtual bool RecvFrom(RefObject<AsyncContext> ref)
		{
			SIM_FUNC_DEBUG();
			const unsigned int buff_size = 4 *1024;
			return RecvFrom(ref, RefBuff(buff_size));
		}
		
		//复用缓存
		virtual bool Recv(RefObject<AsyncContext> ref, RefBuff buff)
		{
			SIM_FUNC_DEBUG();
			//缓存为空返回报错
			if (buff.size() <= 0)
			{
				//return Recv(ref);
				SIM_LERROR("recv buff is empty");
				return false;
			}

			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent;
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETRecv;
			e->buff = buff;
			e->buff.set('\0');//清零
			e->wsa_buf.buf = e->buff.get();
			e->wsa_buf.len = e->buff.size();

			DWORD dwFlags = 0;

			//接收数据请求
			//printf("use event %p\n", e);
			int res = WSARecv(ref->sock.GetSocket(), &e->wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, pol, nullptr);
			//printf("not use event %p\n", e);
			int err = WSAGetLastError();
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				//printf("delete event %p at %d\n", e, __LINE__);
				SIM_LERROR("WSARecv error res=" << res << " WSAGetLastError = " << err);
				return false;
			}
			return true;
		}

		virtual bool RecvFrom(RefObject<AsyncContext> ref, RefBuff buff)
		{
			SIM_FUNC_DEBUG();
			if (buff.size() <= 0)
			{
				//return Recv(ref);
				SIM_LERROR("recv buff is empty");
				return false;
			}

			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent;
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETRecvFrom;
			//e->ref = ref;
			e->buff = buff;
			e->buff.set('\0');
			e->wsa_buf.buf = e->buff.get();
			e->wsa_buf.len = e->buff.size();

			DWORD dwFlags = 0;

			int res = WSARecvFrom(ref->sock.GetSocket(), &e->wsa_buf, 1, (DWORD*)&e->bytes_transfered, &dwFlags, 
				(struct sockaddr*)&e->temp_addr,&e->temp_addr_len, pol, nullptr);
			int err = WSAGetLastError();
			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;
				SIM_LERROR("WSARecvFrom error res=" << res << " WSAGetLastError = " << err);
				return false;
			}
			return true;
		}

		//接收链接
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			SIM_FUNC_DEBUG();
			//新建事件
			IocpAsyncEvent *e = new IocpAsyncEvent();
			if (NULL == e)
			{
				SIM_LERROR("create IocpAsyncEvent error ");
				return false;
			}
			OVERLAPPED  *pol = &e->overlapped;
			e->type = ETAccept;

			//需要申请内存存放地址
			const int addr_size = sizeof(SOCKADDR_IN) + 16;
			e->buff = RefBuff(2 * addr_size);

			//创建子连接
			e->accepted = sim::Socket(sim::TCP);
			if (!e->accepted.IsVaild())
			{
				SIM_LERROR("create socket error ");
				return false;
			}
			DWORD dwFlags = 0;

			//使用AcceptEx
			WsaExFunction exfunc = LoadWsaExFunction();
			int res = exfunc.__AcceptEx(ref->sock.GetSocket(), e->accepted.GetSocket(),
				e->buff.get(), e->buff.size() - 2 * addr_size, addr_size, addr_size,
				(DWORD*)&e->bytes_transfered, pol);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				e->accepted.Close();//需要关闭连接
				delete e;
				SIM_LERROR("exfunc.__AcceptEx error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

	protected:
		//加载拓展函数
		WsaExFunction &LoadWsaExFunction()
		{
			SIM_FUNC_DEBUG();
			//加载wsa拓展函数，保证只加载一次
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
	//发送缓存
	struct SendBuff
	{
		//数据
		RefBuff buff;
		
		//当前偏移量
		unsigned int offset;

		//发送地点 主要由udp接口 sendto使用
		struct sockaddr_in to_addr;
		SendBuff():offset(0)
		{
			memset(&to_addr, 0, sizeof(to_addr));
		}
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

		//数据接收缓存
		unsigned int recv_buff_size;

		EpollAsyncContext(SockType t) :AsyncContext(t),
			accept_flag(false), connect_flag(false), eflag(EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLET)
			,recv_buff_size(SIM_ASYNC_MIN_RECV_BUFF_SIZE)
		{
			ep_event.data.ptr = (void*)this;
			ep_event.events = eflag;
		}
	};

	class EpollAsync :public Async
	{
	public:
		//max_ef 最大套接字数目
		EpollAsync(unsigned int max_ef = 4096)
		{
			//初始化
			Socket::Init();
			epollfd_ = epoll_create(max_ef);
			if (-1 == epollfd_)
			{
				SIM_LERROR("Failed to create epoll context."<<strerror(errno));
				exit(1);
			}
		}
		virtual ~EpollAsync()
		{
			//关闭
			close(epollfd_);
		}
	public:
		//事件循环
		virtual int Poll(unsigned int wait_ms)
		{
			//一次最大取事件数目
			const unsigned int MAXEVENTS = 100;
			struct epoll_event events[MAXEVENTS];

			//取事件
			int n = epoll_wait(epollfd_, events, MAXEVENTS, wait_ms);
			if (-1 == n)
			{
				SIM_LERROR("Failed to wait."<<strerror(errno));
				return SOCK_FAILURE;
			}
			SIM_LDEBUG("epoll_wait." << n);
			for (int i = 0; i < n; i++)
			{
				//获取上下文
				RefObject<AsyncContext> ref = GetCtx(events[i].data.fd);
				if (!ref)
				{
					SIM_LERROR("not found ref " << events[i].data.fd);
					continue;
				}

				//事件
				uint32_t ee = events[i].events;

				//连接挂起或者一次
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

				//是否可以接收连接
				if (ep_ref->accept_flag)
				{
					//accept
					Socket accepted_socket;

					//接收请求
					int ret = ep_ref->sock.Accept(accepted_socket, 10);
					
					//再次置位accept请求
					Accept(ref);

					//判断是否接收成功
					if (ret != SOCK_SUCCESS)
					{
						//接收失败
						SIM_LERROR(ref->sock.GetSocket()<<" Accept Failed.ret="<< ret<<"." << strerror(errno)<<" flag="<<SIM_HEX(ee));
						continue;
					}

					//创建子连接上下文
					SIM_LDEBUG("accept")
					RefObject<AsyncContext> accepted(new EpollAsyncContext(sim::TCP));

					accepted->sock = accepted_socket;
#ifdef SIM_USE_OPENSSL
					accepted->CopySSLCtx(ep_ref);
					//新建SSL会话
					if (!accepted->NewSSLSession())
					{
						SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
						continue;
					}
#endif
					//设置非堵塞
					accepted->sock.SetNonBlock(true);
					accepted->CopyHandler(ep_ref);//拷贝句柄
					//映射
					AddCtx(accepted);
					//监听事件
					AddEpoll(accepted);
					//回调
					accepted->is_active = true;
					ref->OnAccept(accepted->sock.GetSocket());
					
				}
				else if (ep_ref->connect_flag)
				{
					//连接已经建立了
					ep_ref->connect_flag = false;
					//新建会话
#ifdef SIM_USE_OPENSSL
					if (!ep_ref->NewSSLSession())
					{
						SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
						Close(ref->sock.GetSocket(), CloseError);
						continue;
					}
#endif
					//去除输出
					ModifyEpollEvent(ref, EPOLLOUT, false);

					//连接回调
					ref->is_active = true;
					ref->OnConnect();

					//监听可读
					ModifyEpollEvent(ref, EPOLLIN, true);
				}
				//读写
				else
				{
					if (EPOLLIN & ee)//可读
					{
						
						const int ip_len = 64;
						char ip_buff[ip_len] = { 0 };
						unsigned short port = 0;

						//申请缓存
						RefBuff buff(ep_ref->recv_buff_size);

						//根据协议类型选择不同的数据接收接口
						int ret = -1;
						if (ep_ref->type == TCP)
						{
							ret = ep_ref->sock.Recv(buff.get(), buff.size());
						}
						else if (ep_ref->type == UDP)
						{
							ret = ep_ref->sock.Recvfrom(buff.get(), buff.size(), ip_buff, ip_len, &port);
						}

						//接收情况
						if (ret < 0)
						{
							//错误
							SIM_LERROR(ref->sock.GetSocket() << " Recv Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						else if (ret == 0)
						{
							//关闭了
							SIM_LINFO(ref->sock.GetSocket() << " Recv 0 socket close.");
							Close(ref->sock.GetSocket(), ClosePassive);
						}
						else
						{
							//接收成功,然后回调
							if (ep_ref->type == TCP)
								ep_ref->OnRecvData(buff.get(), ret);
							else if (ep_ref->type == UDP)
								ep_ref->OnRecvDataFrom(buff.get(), ret,ip_buff,port);

							//是否调整缓存
							if (ret >= ep_ref->recv_buff_size&&ep_ref->recv_buff_size < ep_ref->max_recv_buff_size)
							{
								ep_ref->recv_buff_size = ep_ref->recv_buff_size*1.5 + 1;
								if (ep_ref->recv_buff_size > ep_ref->max_recv_buff_size)
									ep_ref->recv_buff_size = ep_ref->max_recv_buff_size;
							}

							//继续接收数据
							ModifyEpollEvent(ref, EPOLLIN, true);
						}

					}
					if (EPOLLOUT &ee)
					{
						//发送数据 需要加上队列锁
						AutoMutex lk(ep_ref->send_queue_lock);
						//取第一个节点
						QueueNode<SendBuff>*pHead = ep_ref->send_queue_buff.Next(NULL);
						if (NULL == pHead)
						{
							//节点为空，则不处理，关闭输出监控即可
							SIM_LERROR(ref->sock.GetSocket() << " send cache is empty.");
							ModifyEpollEvent(ref, EPOLLOUT, false);
							continue;
						}
						//根据协议选择不同的接口发送
						int ret = -1;
						if (ep_ref->type == TCP)
						{
							ret = ep_ref->sock.Send(pHead->data.buff.get() + pHead->data.offset,
								pHead->data.buff.size() - pHead->data.offset);
						}
						else
						{
							ret = ::sendto(ep_ref->sock.GetSocket(),
								pHead->data.buff.get() + pHead->data.offset,
								pHead->data.buff.size() - pHead->data.offset
								, 0,(struct sockaddr*)&pHead->data.to_addr, sizeof(pHead->data.to_addr));
						}
						//发送失败
						if (ret < 0)
						{
							SIM_LERROR(ref->sock.GetSocket() << " Send Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						//发送成功计算偏移量
						pHead->data.offset += ret;
						if (pHead->data.offset >= pHead->data.buff.size())
						{
							//发送完毕
							ep_ref->OnSendComplete(pHead->data.buff.get(), pHead->data.buff.size());
							//去除
							ep_ref->send_queue_buff.PopFront(NULL);
						}
						//没有要写的了
						if (ep_ref->send_queue_buff.isEmpty())
						{
							ModifyEpollEvent(ref, EPOLLOUT, false);
						}
						else
						{
							//新增
							ModifyEpollEvent(ref, EPOLLOUT, true);
						}
					}
				}
			}

			return SOCK_SUCCESS;
		}

		virtual AsyncHandle CreateHandle(SockType type)
		{
			//创建空实例
			RefObject<AsyncContext> ref(new EpollAsyncContext(type));
			ref->sock = Socket(type);
			if (!ref->sock.IsVaild())
			{
				return SOCK_FAILURE;
			}
			AddCtx(ref);
			return ref->sock.GetSocket();
		}

		virtual AsyncHandle CreateTcpHandle()
		{
			return CreateHandle(TCP);
		}
		virtual AsyncHandle CreateUdpHandle()
		{
			return CreateHandle(UDP);
		}

		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}

			//设置端口复用
			ref->sock.SetReusePort(true);

			//绑定
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

			//设置非堵塞
			ref->sock.SetNonBlock(true);

			//加到epoll中
			AddEpoll(ref);
			ref->is_active = true;
			//添加接收事件
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
			//设置非阻塞
			ref->sock.SetNonBlock(true);
			//添加到epoll中
			AddEpoll(ref);
			//链接
			if (!Connect(ref, remote_ipaddr, remote_port))
			{
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}
			return SOCK_SUCCESS;
		}
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				return SOCK_FAILURE;
			}
			//设置重用端口
			ref->sock.SetReusePort(true);

			//绑定
			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			ref->sock.SetNonBlock(true);
			AddEpoll(ref);
			ref->is_active = true;
			return SOCK_SUCCESS;
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
				if (ref->type != TCP)
				{
					SIM_LERROR("Not Tcp");
					return SOCK_FAILURE;
				}
#ifdef SIM_USE_OPENSSL
				return Send(ref, ref->Encrypt(RefBuff(buff, buff_len))) ? SOCK_SUCCESS : SOCK_FAILURE;
#else
				return Send(ref, RefBuff(buff, buff_len)) ? SOCK_SUCCESS : SOCK_FAILURE;
#endif
				//return Send(ref, RefBuff(buff, buff_len)) ? SOCK_SUCCESS : SOCK_FAILURE;
			}
			return SOCK_FAILURE;
		}
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* ipaddr, unsigned short port)
		{
			if (buff == NULL || 0 == buff_len)
			{
				return SOCK_SUCCESS;
			}

			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				if (ref->type != UDP)
				{
					SIM_LERROR("Not Tcp");
					return SOCK_FAILURE;
				}
#ifdef SIM_USE_OPENSSL
				//return Send(ref, ref->Encrypt(RefBuff(buff, buff_len))) ? SOCK_SUCCESS : SOCK_FAILURE;
				return SendTo(ref, ref->Encrypt(RefBuff(buff, buff_len)), ipaddr, port) ? SOCK_SUCCESS : SOCK_FAILURE;
#else
				return SendTo(ref, RefBuff(buff, buff_len), ipaddr, port) ? SOCK_SUCCESS : SOCK_FAILURE;
#endif
				//return SendTo(ref, RefBuff(buff, buff_len), ipaddr,port) ? SOCK_SUCCESS : SOCK_FAILURE;
			}
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
			//调用连接接口
			int ret = ep_ref->sock.Connect(ipaddr, port);
			//如果成功则直接回调
			if (ret == 0)
			{
				ep_ref->OnConnect();
				return true;
			}
			//否则监听输出，如果可以输出则表明连接已经建立了。
			ep_ref->connect_flag = true;
			return ModifyEpollEvent(ref, EPOLLOUT, true);
		}

		virtual bool Send(RefObject<AsyncContext> ref, RefBuff buff)
		{
			/*if (NULL == ref)
				return false;*/
			//发送数据的时候，先将数据加到队列里面然后监听输出
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			AutoMutex lk(ep_ref->send_queue_lock);
			SendBuff send;
			send.buff = buff;
			send.offset = 0;
			ep_ref->send_queue_buff.PushBack(send);
			return ModifyEpollEvent(ref, EPOLLOUT, true);
		}
		virtual bool SendTo(RefObject<AsyncContext> ref, RefBuff buff,const char* ipaddr, unsigned short port)
		{
			/*if (NULL == ref)
				return false;*/
			//sendto需要先转换地址
			SendBuff send;
			if (!ref->sock.IpToAddressV4(ipaddr, port, &send.to_addr))
			{
				SIM_LERROR("SendTo failue,IpToAddressV4 " << ipaddr << ":" << port << " bad!");
				return false;
			}
			send.buff = buff;
			send.offset = 0;

			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			AutoMutex lk(ep_ref->send_queue_lock);
			ep_ref->send_queue_buff.PushBack(send);
			return ModifyEpollEvent(ref, EPOLLOUT, true);
		}
		//接收链接
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			//accept_flag 设置为true然后监听输入
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			ep_ref->accept_flag = true;
			return ModifyEpollEvent(ref, EPOLLIN, true);
		}

		//epoll控制接口
		virtual bool EpollCtrl(RefObject<AsyncContext> ref, int opt,uint32_t flag)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			//flag设置
			ep_ref->ep_event.events = ep_ref->eflag=flag;
			ep_ref->ep_event.data.ptr = NULL;
			//套接字设置
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
			//添加到epoll 中默认启用EPOLLIN 输入 EPOLLHUP | EPOLLERR 挂起或者异常 监听
			return EpollCtrl(ref, EPOLL_CTL_ADD, EPOLLIN | EPOLLHUP | EPOLLERR);
		}
		//修改事件is_add =true 添加 否则 删除
		virtual bool ModifyEpollEvent(RefObject<AsyncContext> ref, uint32_t _event, bool is_add)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			//添加
			if (is_add)
			{
				if (ep_ref->eflag&_event)
				{
					//已经存在了，不修改了，减少一次调用
					return true;
				}
				return EpollCtrl(ref, EPOLL_CTL_MOD, ep_ref->eflag | _event);
			}
			else
			{
				if (ep_ref->eflag&_event)
				{
					return EpollCtrl(ref, EPOLL_CTL_MOD, ep_ref->eflag &( ~_event));//取反&去除
				}
				//不存在了，不修改了，减少一次调用
				return true;
				
			}
		}

		virtual bool RemoveEpoll(RefObject<AsyncContext> ref)
		{
			//移除引用
			return EpollCtrl(ref, EPOLL_CTL_DEL,0);

		}

		virtual int Close(AsyncHandle handle, AsyncCloseReason reason)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->is_active = false;
				ref->close_reason = reason;
				//先移除监听
				RemoveEpoll(ref);
				//释放上下文
				ReleaseCtx(handle);
				////打印
				//SIM_LDEBUG("handle " << handle << " closed ");
				////回调
				//ref->OnClose(reason, errno);
				return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
		}
	private:
		int epollfd_;
	};
	//epoll实现
	typedef EpollAsync SimAsync;
#endif //ASYNC_EPOLL
}
#endif