/*
	�첽����ӿ�
*/
#ifndef SIM_ASYNC_HPP_
#define SIM_ASYNC_HPP_

//�Ƿ�ʹ��openssl
#ifdef SIM_USE_OPENSSL
#include "SSLCtx.hpp"
#endif //! SIM_USE_OPENSSL

//��������
#include "Socket.hpp"
#include "RefObject.hpp"
#include "RbTree.hpp"
#include "Mutex.hpp"
#include "Queue.hpp"
#include "Timer.hpp"

//��־ʹ��
#define SIM_ENABLE_LOGGER
#ifdef SIM_ENABLE_LOGGER
#include "Logger.hpp"
#else
//�յĶ����ֹ���뱨��
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

//windowsƽ̨
#ifdef OS_WINDOWS
	//windowsƽ̨ʹ��iocp
	#ifndef ASYNC_IOCP
		#define ASYNC_IOCP
	#endif
	//��ֹ �ظ�����
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN  
	#endif
	//ʹ����չ�ӿ�
	#include <winsock2.h>
	#include <MSWSock.h>
#else
	#ifdef OS_LINUX
		//����linuxƽ̨�ӿ�
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
		//linuxʹ��epoll
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
	//�¼�����
	enum EType
	{
		//���ӽ����¼�
		ETConnect,
		
		//���������¼�
		ETAccept,
		
		//����TCP�����¼�
		ETRecv,
		
		//����UDP�����¼������˵�ַ��Ϣ��
		ETRecvFrom,
		
		//���ݷ����¼�
		ETSend,
		
		//���ӹر��¼�
		ETClose,
	};
	
	//���Ӿ����һ�������ʶһ������
	typedef SOCKET AsyncHandle;

	//�������ӻص�
	typedef void(*AcceptHandler)(AsyncHandle handle, AsyncHandle client, void*data);

	//������ɻص�
	typedef void(*ConnectHandler)(AsyncHandle handle, void*data);
	
	//tcp���ݽ��ջص�
	typedef void(*RecvDataHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);

	//udp���ݽ��ջص�
	typedef void(*RecvDataFromHandler)(AsyncHandle handle, char *buff, unsigned int buff_len,char *from_ip,unsigned short port, void*data);

	//���ݷ�����ɻص�
	typedef void(*SendCompleteHandler)(AsyncHandle handle, char *buff, unsigned int buff_len, void*data);

	//���ӻỰ�ر�ԭ��ö��
	enum AsyncCloseReason
	{
		//�쳣�رգ������˴���ƽ̨������error��
		CloseError,
		//�����ر� ����������close�رգ�
		CloseActive,
		//�Ự�������չر� �����ӽ��յ�eof��
		ClosePassive,
	};
	//���ӹرջص�
	typedef void(*CloseHandler)(AsyncHandle handle, AsyncCloseReason reason, int error, void*data);

	//�첽�����Ķ������
	class AsyncContext
	{
	public:
		//��������
		Socket sock;

		//�������� tcp or udp
		SockType type;
		
		//�Ƿ��Ծ accept ���� connect ���� Listen
		bool is_active;

		AsyncCloseReason close_reason;
		//ssl
#ifdef SIM_USE_OPENSSL
		//ssl�Ự������
		RefObject<SSLCtx> ssl_ctx;
		
		//ssl�Ự
		SSLSession* ssl_session;
#endif
		//�ص����
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

		//������data
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
		//���ƾ��
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
		
		//����
		virtual ~AsyncContext()
		{
			//�ͷ�ssl������
			ReleaseSSLCtx();
			
#ifdef OS_WINDOWS
			//�ص�
			OnClose(close_reason, GetLastError());
#else
			OnClose(close_reason, errno);
#endif
			SIM_LDEBUG("handle " << sock.GetSocket() << " closed ");
			//�ر�����
			sock.Close();
			
		}
		
		void ReleaseSSLCtx()
		{
#ifdef SIM_USE_OPENSSL
			//�Ự���� �رջỰ
			if (ssl_session)
			{
				ssl_ctx->DelSession(ssl_session);
				ssl_session = NULL;
			}
			//����ssl����
			ssl_ctx.reset();
#endif
		}

		//�������ݣ�����δ�������� �����Ѿ����ܵ����� ��ssl���ӷ���ԭʼ����
		virtual RefBuff Encrypt(RefBuff input)
		{
#ifdef SIM_USE_OPENSSL
			if (ssl_session)
			{
				//��������
				ssl_session->InEncrypt(input.get(), input.size());
				
				//��ȡ�Ѿ������˵�����
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
		//���� ����
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

		//�½�SSL session
		virtual bool NewSSLSession()
		{
#ifdef SIM_USE_OPENSSL
			if (ssl_session)
			{
				//�������֣�������ܻ�����������Ż�
				if (false == ssl_session->HandShake())
				{
					//����ʧ��
					SIM_LERROR("sock.GetSocket()" << sock.GetSocket()
						<< " HandShake fail");
					return false;
				}
				//�Ѵ���
				return true;
			}
			else
			{
				if (ssl_ctx)
				{
					//�½�����
					ssl_session =ssl_ctx->NewSession(sock.GetSocket());
					if (NULL == ssl_session)
					{
						SIM_LERROR("sock.GetSocket()"<< sock.GetSocket()
							<< " NewSession return NULL");
						return false;
					}
					//�������֣�������ܻ�����������Ż�
					if (false == ssl_session->HandShake())
					{
						//����ʧ��
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
		//�ص�����
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
			//��������
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
			//�����ݽ���
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

	//�첽����������
	class Async
	{
		//�������ƿ���
		Async(const Async &other) {};
		Async& operator=(const Async &other) {};
	public:
		Async() {};

		//���нӿڣ�ȡ�¼��ͷַ�
		virtual int Poll(unsigned int wait_ms) = 0;

		//�����վ��
		virtual AsyncHandle CreateTcpHandle() = 0;
		virtual AsyncHandle CreateUdpHandle() = 0;
		//type �������
		virtual AsyncHandle CreateHandle(SockType type) = 0;

		//����TCP����
		//handle ���
		//bind_ipaddr \0 ��β�ַ������󶨵�ַ��������ipҲ������NULL,��ʱֻ֧��ipv4
		//bind_port �󶨶˿�
		//acctept_num ����������
		virtual int AddTcpServer(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port, unsigned int acctept_num = 10) = 0;
		
		//����tcp����
		//handle ���
		//remote_ipaddr \0 ��β�ַ�����Զ��������ַ��ip��ַ,��ʱֻ֧��ipv4
		//remote_port Զ�̶˿�
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port) = 0;

		//����udp����
		//handle ���
		//bind_ipaddr \0 ��β�ַ������󶨵�ַ��������ipҲ������NULL,��ʱֻ֧��ipv4
		//bind_port �󶨶˿�
		virtual int AddUdpConnect(AsyncHandle handle, const char* bind_ipaddr, unsigned short bind_port) = 0;

		//sslЭ���
#ifdef SIM_USE_OPENSSL
		//����������Ϊssl�����ԭ��������ssl���򽫻�����Ϊ meth;
		virtual int ConvertToSSL(AsyncHandle handle, const SSL_METHOD *meth)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
				return SOCK_FAILURE;
			return ConvertToSSL(ref, meth);
		}
#endif
		//return SOCK_FAILURE; is_server �Ƿ�ΪЭ������ is_add �Ƿ��Ѿ����
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
				//�ͷ�
				ref->ReleaseSSLCtx();
				return SOCK_SUCCESS;
			}
			else
			{
				//������������ѡ��ͬ��method
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
		
		//���� ssl֤�� 0�޴���
		//handle ���
		//pub_key_file ��Կ֤��
		//pri_key_file ˽Կ֤��
		virtual int SetSSLKeyFile(AsyncHandle handle, const char *pub_key_file, const char*pri_key_file)
		{
#ifndef SIM_USE_OPENSSL
			//��ʹ��opensllֱ�ӱ���
			return SOCK_FAILURE;
#else
			SIM_FUNC_DEBUG();
			//������ΪNULL
			if (NULL == pub_key_file || NULL == pri_key_file)
			{
				SIM_LERROR("SetSSLKeyFile Fail,some file is NULL");
				return SOCK_FAILURE;
			}
			//��ȡ����������
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				SIM_LDEBUG("handle " << handle << " SetSSLKeyFile  pub:"<< pub_key_file<<",pri:"<< pri_key_file);
				//��ȡssl������
				if (ref->ssl_ctx)
					if(ref->ssl_ctx->SetKeyFile(pub_key_file, pri_key_file))//����
						return SOCK_SUCCESS;
			}
			return SOCK_FAILURE;
#endif
		}
		
		//����ssl��������
		virtual int SetSSLHostName(AsyncHandle handle, const char* hostname)
		{
#ifndef SIM_USE_OPENSSL
			//��ʹ��opensllֱ�ӱ���
			return SOCK_FAILURE;
#else
			SIM_FUNC_DEBUG();
			//������ΪNULL
			if (NULL == hostname )
			{
				SIM_LERROR("SetSSLHostName Fail,hostname is NULL");
				return SOCK_FAILURE;
			}
			//��ȡ����������
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				SIM_LDEBUG("handle " << handle << " SetSSLHostName  hostname:" << hostname);
				//��ȡssl������
				if (ref->ssl_ctx)
				{
					if (NULL == ref->ssl_session)
					{
						ref->ssl_session = ref->ssl_ctx->NewSession(ref->sock.GetSocket());
						if (NULL == ref->ssl_session)
							return SOCK_FAILURE;
					}
					if (ref->ssl_session->SetHostName(hostname))//����
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


		//TCP���ݷ��ͽӿ�
		//handle ���
		//buff ���ͻ���
		//buff_len �������ݳ���
		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len) = 0;
		
		//UDP���ݷ��ͽӿ�
		//handle ���
		//buff ���ͻ���
		//buff_len �������ݳ���
		//remote_ipaddr \0 ��β�ַ�����Զ��������ַ��ip��ַ,��ʱֻ֧��ipv4
		//remote_port Զ�̶�
		virtual int SendTo(AsyncHandle handle, const char *buff, unsigned int buff_len,
			const char* remote_ipaddr, unsigned short remote_port) = 0;

		//�����ر�����
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

		//�Ƿ����
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
		//�������
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

		//�����ⲿ�����ģ�������¼
		virtual void SetCtxData(AsyncHandle handle, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				ref->ctx_data = pdata;
			}
		}
		//��ȡctx����
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
		//��������������
		virtual void AddCtx(RefObject<AsyncContext> ctx)
		{
			if (ctx)
			{
				AutoMutex lk(ctx_s_lock_);
				ctx_s_.Add(ctx->sock.GetSocket(), ctx);
			}
		}
		
		//���ݾ����ȡ������
		virtual RefObject<AsyncContext> GetCtx(AsyncHandle handle)
		{
			AutoMutex lk(ctx_s_lock_);
			RefObject<AsyncContext> ref;
			ctx_s_.Find(handle, &ref);
			return ref;
		}
		
		//�ͷ�������
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
				//���ͷ�������
				ReleaseCtx(handle);
//				SIM_LDEBUG("handle " << handle << " closed ");
//#ifdef OS_WINDOWS
//				//�ص�
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
			//�Ѵ��ڵ���ɾ����
			if (ref->ssl_session)
			{
				ref->ssl_ctx->DelSession(ref->ssl_session);
				ref->ssl_session = NULL;
			}
			//����
			ref->ssl_ctx = RefObject<SSLCtx>(new SSLCtx(meth));
			return SOCK_SUCCESS;
		}
#endif
	private:
		//��
		Mutex ctx_s_lock_;
		//�����������ӳ�伯
		RbTree<RefObject<AsyncContext> > ctx_s_;
	};

	//iocpʵ��
#ifdef ASYNC_IOCP
	//�첽�¼�
	class IocpAsyncEvent
	{
	public:
		//IO�ص�����
		OVERLAPPED  overlapped;
		
		//ʱ������
		EType type;

		//�����Ӵ��� acceptexʱ������
		Socket accepted;

		//����
		RefBuff buff;
		//buff����������
		WSABUF wsa_buf;
		
		//������ֽ���Ŀ
		DWORD bytes_transfered;
		//RefObject<AsyncContext> ref;

		//��ַ ����SendTo or Recvfrom
		struct sockaddr_in temp_addr;
		int temp_addr_len;

		//��ʼ��
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

	//�첽������
	class IocpAsyncContext :public AsyncContext
	{
	public:
		//ʹ�� Connect ֮ǰ��Ҫ��һ�����ض˿ڣ����=true���ʶ�Ѿ��󶨹��ˣ�=false�������һ���˿�
		bool bind_flag;
		IocpAsyncContext(SockType type) :bind_flag(false), AsyncContext(type)
		{
		}
		~IocpAsyncContext()
		{
			
		}
	};

	//wsa��չ�������ض���
	class WsaExFunction
	{
	public:
		WsaExFunction()
		{
			//��ʼ��socket
			Socket::Init();
			
			//����һ���յ�socket
			SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			
			//������չ����
			__AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
			__ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
			__AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
			__DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);
			
			//�ر�����
			closesocket(socket);
		}
	private:
		//������չ����
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

	//iocpʵ�ֵ��첽�������
	class IocpAsync :public Async
	{
	public:
		//thread_num �󶨵��߳���
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
		//�����վ��
		virtual AsyncHandle CreateHandle(SockType type)
		{
			SIM_FUNC_DEBUG();
			//������Ӧ�����Ķ���
			RefObject<AsyncContext> ref(new IocpAsyncContext(type));
			//������Ӧ��socket
			ref->sock = Socket(type);
			//����Ƿ񴴽��ɹ�
			if (!ref->sock.IsVaild())
			{
				SIM_LERROR("sock Create error");
				return SOCK_FAILURE;
			}
			//����ӳ��
			AddCtx(ref);
			SIM_LDEBUG((sim::TCP == type ? "TCP" : "UDP") << ".handle " << ref->sock.GetSocket() << " is cteated");
			//���ش����ľ�������ʵ���Ƕ�Ӧ���׽���
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

		//���һ��TCP������
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

			//�󶨵�ַ
			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				SIM_LERROR("handle " << handle << " Bind ipaddr:" << (bind_ipaddr == NULL ? "NULL" : bind_ipaddr) << ":" << bind_port << " fail,ret=" << ret);
				ReleaseCtx(handle);
				return ret;
			}

			//����
			const int listen_size = 1024 * 4;
			ret = ref->sock.Listen(listen_size);
			if (ret != SOCK_SUCCESS)
			{
				SIM_LERROR("handle " << handle << " Listen listen_size:" << listen_size << " fail,ret=" << ret);
				//bind error
				ReleaseCtx(handle);
				return ret;
			}
			//���÷Ƕ���
			ref->sock.SetNonBlock(true);

			//����ɶ˿�
			HANDLE iocp_handler = CreateIoCompletionPort((HANDLE)ref->sock.GetSocket(), iocp_handler_,
				(ULONG_PTR)(ref->sock.GetSocket()), 0);
			if (NULL == iocp_handler)
			{
				//ReleaseCtx(ref->sock.GetSocket());
				SIM_LERROR("handle " << handle << " CreateIoCompletionPort fail" << "  WSAGetLastError()=" << WSAGetLastError());
				ReleaseCtx(handle);
				return SOCK_FAILURE;
			}

			//����acctept_num����accept ��������
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

		//���һ��TCP����
		virtual int AddTcpConnect(AsyncHandle handle, const char* remote_ipaddr, unsigned short remote_port)
		{
			SIM_FUNC_DEBUG();
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (!ref)
			{
				SIM_LERROR("handle " << handle << " not find");
				return SOCK_FAILURE;
			}
			//����Ϊ�Ƕ���
			ref->sock.SetNonBlock(true);

			//����ɶ˿�
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

			//������������
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

			//�󶨱��ض˿�
			int ret = ref->sock.Bind(bind_ipaddr, bind_port);
			if (ret != SOCK_SUCCESS)
			{
				//bind error
				SIM_LERROR("handle " << handle << " Bind ipaddr:" << bind_ipaddr << ":" << bind_port << " fail,ret=" << ret);
				ReleaseCtx(handle);
				return ret;
			}

			//����Ϊ�Ƕ���
			ref->sock.SetNonBlock(true);

			//����ɶ˿�
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
			//��ʼ��������
			return RecvFrom(ref) ? SOCK_SUCCESS : SOCK_FAILURE;
		}

		//�¼�ѭ�� wait_ms���ȴ�ʱ��
		virtual int Poll(unsigned wait_ms)
		{
			SIM_FUNC_DEBUG();
			//�������ݳ���
			DWORD               bytes_transfered = 0;
			//iocp ctx
			SOCKET socket = 0;
			//IO�ص�����ָ��
			OVERLAPPED          *over_lapped = NULL;

			//ȡ�¼�����
			BOOL res = GetQueuedCompletionStatus(iocp_handler_,
				&bytes_transfered, PULONG_PTR(&socket),
				&over_lapped, wait_ms);

			//���Դ�ӡ
			SIM_LDEBUG("GetQueuedCompletionStatus res=" << res
				<< " socket " << socket << " bytes_transfered " << bytes_transfered
				<< " over_lapped=0x" << SIM_HEX(over_lapped))

				if (over_lapped)
				{
					//ת��Ϊ�¼�����
					IocpAsyncEvent* socket_event = \
						CONTAINING_RECORD(over_lapped, IocpAsyncEvent, overlapped);

					//��ȡ������
					RefObject<AsyncContext> ref = GetCtx(socket);
					if (ref)
					{
						//���ô����ֽ���
						socket_event->bytes_transfered = bytes_transfered;
						//��ȡ������
						DWORD dw_err = GetLastError();
						if (FALSE == res)
						{
							//�������󣬹ر�����
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
							//���յ�����
							if (socket_event->type == ETAccept)
							{
								//�ȷ�����һ���������󣬷�ֹ������������һ������
								if (false == Accept(ref))
								{
									SIM_LERROR("Accept fail " << " sock= " << ref->sock.GetSocket());
									delete socket_event;
									return SOCK_FAILURE;
								}

								//�����µ�������
								RefObject<AsyncContext> accepted(new IocpAsyncContext(sim::TCP));
								
								//��������
								accepted->sock = socket_event->accepted;

								//����ssl �Ự
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

								//��Ҫ�ӵ�iocp��������
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

								//���������ӵĻص�
								accepted->CopyHandler(ref.get());
								//���ӳ��
								AddCtx(accepted);
								accepted->is_active = true;
								//�ص�
								ref->OnAccept(accepted->sock.GetSocket());

								//��������
								if (false == Recv(accepted))
								{
									SIM_LERROR("Recv fail  sock= " << accepted->sock.GetSocket());
									Close(accepted->sock.GetSocket(), CloseError);
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}

							}
							//���ӳɹ�
							else if (socket_event->type == ETConnect)
							{
								//��ʼ��ssl�Ự
#ifdef SIM_USE_OPENSSL
								if (!ref->NewSSLSession())
								{
									SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), CloseError);//����ʧ�ܹر�����
									delete socket_event;
									return SOCK_FAILURE;
								}
#endif
								ref->is_active = true;
								//�ص�
								ref->OnConnect();

								//Recv(ref);//��������
								if (false == Recv(ref))//��������
								{
									SIM_LERROR("Recv fail  sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), CloseError);
									//printf("delete event %p at %d\n", socket_event, __LINE__);
									delete socket_event;
									return SOCK_FAILURE;
								}
							}
							//udp�����������
							else if (socket_event->type == ETRecvFrom)
							{
								//��������Ϊ0���ǶԶ��Ѿ��ر�������
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("recvfrom socket_event->bytes_transfered=0 ,socket is end");
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//������ַ
									const int ip_len = 32;
									char ip[ip_len] = { 0 };
									unsigned short port = 0;
									ref->sock.AddressToIpV4(&socket_event->temp_addr, ip, ip_len, &port);

									//�ص�
									ref->OnRecvDataFrom(socket_event->buff.get(), socket_event->bytes_transfered, ip, port);

									//�����buff���������չһ��
									if (socket_event->bytes_transfered >= socket_event->buff.size()
										&& socket_event->buff.size()<ref->max_recv_buff_size)
									{
										unsigned int now = socket_event->bytes_transfered*1.5 + 1;
										socket_event->buff = RefBuff(now> ref->max_recv_buff_size? ref->max_recv_buff_size :now);
									}

									if (false == RecvFrom(ref, socket_event->buff))//��������
									{
										SIM_LERROR("recvfrom fail " << " sock= " << ref->sock.GetSocket());
										Close(ref->sock.GetSocket(), CloseError);
										//printf("delete event %p at %d\n", socket_event, __LINE__);
										delete socket_event;
										return SOCK_FAILURE;
									}
								}
							}
							//tcp �����������
							else if (socket_event->type == ETRecv)
							{
								//���ս�����
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("recv socket_event->bytes_transfered=0 ,socket is end.sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//���ջص�
									ref->OnRecvData(socket_event->buff.get(), socket_event->bytes_transfered);

									//�����buff���������չһ��
									if (socket_event->bytes_transfered >= socket_event->buff.size()
										&& socket_event->buff.size() < ref->max_recv_buff_size)
									{
										unsigned int now = socket_event->bytes_transfered*1.5 + 1;
										socket_event->buff = RefBuff(now > ref->max_recv_buff_size ? ref->max_recv_buff_size : now);
									}
									if (ref->is_active)
									{
										//�������ͽ�����������
										if (false == Recv(ref, socket_event->buff))//��������
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
							//�����������
							else if (socket_event->type == ETSend)
							{
								//���ݴ���0 ���Ҳ�Ǵ���
								if (socket_event->bytes_transfered == 0)
								{
									SIM_LERROR("Send socket_event->bytes_transfered=0 ,socket is end.sock= " << ref->sock.GetSocket());
									Close(ref->sock.GetSocket(), ClosePassive);
								}
								else
								{
									//������ɻص�
									ref->OnSendComplete(socket_event->buff.get(), socket_event->bytes_transfered);
								}
							}
							//δ֪������
							else
							{
								//���ﲻ���κ�����
								SIM_LWARN("event->type =" << socket_event->type << " not do something");
							}
						}
					}
					else
					{
						//�ò��������ģ����ӿ����Ѿ����ر��ˣ�ֻ��ɾ���¼����󣬲�����������
						SIM_LERROR("socket " << socket << " not found ref");
						//printf("delete event %p at %d\n", socket_event, __LINE__);
						delete socket_event;
						return SOCK_FAILURE;
					}

					//�޴���
					//printf("delete event %p at %d\n", socket_event, __LINE__);
					delete socket_event;
					return SOCK_SUCCESS;
				}
				else
				{
					//over_lapped==NULL �����쳣
					return SOCK_FAILURE;
				}

		}
		
		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			SIM_FUNC_DEBUG();
			//����ǿյĻ���ֱ�ӷ��سɹ���
			if (buff == NULL || 0 == buff_len)
			{
				SIM_LWARN("Send Empty Buff!");
				return SOCK_SUCCESS;
			}
			//��ȡ���
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				//ʹ���²�ӿڷ�������
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
		//����
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
			//ʹ����չ����������������
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
			//����
			e->buff = buff;
			e->wsa_buf.buf = buff.get();
			e->wsa_buf.len = buff.size();
			DWORD* bytes_transfered = &e->bytes_transfered;

			DWORD dwFlags = 0;

			//��������
			int res = WSASend(ref->sock.GetSocket(), &e->wsa_buf, 1,
				bytes_transfered, dwFlags, pol, nullptr);

			if ((SOCKET_ERROR == res) && (WSA_IO_PENDING != WSAGetLastError())) {
				delete e;//ʧ��ɾ���¼�
				SIM_LERROR("WSASend error res=" << res << "  WSAGetLastError()=" << WSAGetLastError());
				return false;
			}
			return true;
		}

		virtual bool SendTo(RefObject<AsyncContext> ref, RefBuff buff, const char* ipaddr, unsigned short port)
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

			//ת��Ϊ�ṹ��
			ref->sock.IpToAddressV4(ipaddr, port, &e->temp_addr);

			DWORD dwFlags = 0;

			//����ʹ��WSASendTo�ӿ�
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
			//Ĭ�ϻ���Ϊ4K
			//const unsigned int buff_size = SIM_ASYNC_MIN_RECV_BUFF_SIZE;
			return Recv(ref, RefBuff(ref->min_recv_buff_size));
		}
		
		virtual bool RecvFrom(RefObject<AsyncContext> ref)
		{
			SIM_FUNC_DEBUG();
			const unsigned int buff_size = 4 *1024;
			return RecvFrom(ref, RefBuff(buff_size));
		}
		
		//���û���
		virtual bool Recv(RefObject<AsyncContext> ref, RefBuff buff)
		{
			SIM_FUNC_DEBUG();
			//����Ϊ�շ��ر���
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
			e->buff = buff;
			e->buff.set('\0');//����
			e->wsa_buf.buf = e->buff.get();
			e->wsa_buf.len = e->buff.size();

			DWORD dwFlags = 0;

			//������������
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

			//�½��¼�
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

			//��Ҫ�����ڴ��ŵ�ַ
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

			//ʹ��AcceptEx
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
			//����wsa��չ��������ֻ֤����һ��
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
	//���ͻ���
	struct SendBuff
	{
		//����
		RefBuff buff;
		
		//��ǰƫ����
		unsigned int offset;

		//���͵ص� ��Ҫ��udp�ӿ� sendtoʹ��
		struct sockaddr_in to_addr;
		SendBuff():offset(0)
		{
			memset(&to_addr, 0, sizeof(to_addr));
		}
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

		//���ݽ��ջ���
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
		//max_ef ����׽�����Ŀ
		EpollAsync(unsigned int max_ef = 4096)
		{
			//��ʼ��
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
			//�ر�
			close(epollfd_);
		}
	public:
		//�¼�ѭ��
		virtual int Poll(unsigned int wait_ms)
		{
			//һ�����ȡ�¼���Ŀ
			const unsigned int MAXEVENTS = 100;
			struct epoll_event events[MAXEVENTS];

			//ȡ�¼�
			int n = epoll_wait(epollfd_, events, MAXEVENTS, wait_ms);
			if (-1 == n)
			{
				SIM_LERROR("Failed to wait."<<strerror(errno));
				return SOCK_FAILURE;
			}
			SIM_LDEBUG("epoll_wait." << n);
			for (int i = 0; i < n; i++)
			{
				//��ȡ������
				RefObject<AsyncContext> ref = GetCtx(events[i].data.fd);
				if (!ref)
				{
					SIM_LERROR("not found ref " << events[i].data.fd);
					continue;
				}

				//�¼�
				uint32_t ee = events[i].events;

				//���ӹ������һ��
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

				//�Ƿ���Խ�������
				if (ep_ref->accept_flag)
				{
					//accept
					Socket accepted_socket;

					//��������
					int ret = ep_ref->sock.Accept(accepted_socket, 10);
					
					//�ٴ���λaccept����
					Accept(ref);

					//�ж��Ƿ���ճɹ�
					if (ret != SOCK_SUCCESS)
					{
						//����ʧ��
						SIM_LERROR(ref->sock.GetSocket()<<" Accept Failed.ret="<< ret<<"." << strerror(errno)<<" flag="<<SIM_HEX(ee));
						continue;
					}

					//����������������
					SIM_LDEBUG("accept")
					RefObject<AsyncContext> accepted(new EpollAsyncContext(sim::TCP));

					accepted->sock = accepted_socket;
#ifdef SIM_USE_OPENSSL
					accepted->CopySSLCtx(ep_ref);
					//�½�SSL�Ự
					if (!accepted->NewSSLSession())
					{
						SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
						continue;
					}
#endif
					//���÷Ƕ���
					accepted->sock.SetNonBlock(true);
					accepted->CopyHandler(ep_ref);//�������
					//ӳ��
					AddCtx(accepted);
					//�����¼�
					AddEpoll(accepted);
					//�ص�
					accepted->is_active = true;
					ref->OnAccept(accepted->sock.GetSocket());
					
				}
				else if (ep_ref->connect_flag)
				{
					//�����Ѿ�������
					ep_ref->connect_flag = false;
					//�½��Ự
#ifdef SIM_USE_OPENSSL
					if (!ep_ref->NewSSLSession())
					{
						SIM_LERROR("NewSSLSession fail " << ref->sock.GetSocket());
						Close(ref->sock.GetSocket(), CloseError);
						continue;
					}
#endif
					//ȥ�����
					ModifyEpollEvent(ref, EPOLLOUT, false);

					//���ӻص�
					ref->is_active = true;
					ref->OnConnect();

					//�����ɶ�
					ModifyEpollEvent(ref, EPOLLIN, true);
				}
				//��д
				else
				{
					if (EPOLLIN & ee)//�ɶ�
					{
						
						const int ip_len = 64;
						char ip_buff[ip_len] = { 0 };
						unsigned short port = 0;

						//���뻺��
						RefBuff buff(ep_ref->recv_buff_size);

						//����Э������ѡ��ͬ�����ݽ��սӿ�
						int ret = -1;
						if (ep_ref->type == TCP)
						{
							ret = ep_ref->sock.Recv(buff.get(), buff.size());
						}
						else if (ep_ref->type == UDP)
						{
							ret = ep_ref->sock.Recvfrom(buff.get(), buff.size(), ip_buff, ip_len, &port);
						}

						//�������
						if (ret < 0)
						{
							//����
							SIM_LERROR(ref->sock.GetSocket() << " Recv Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						else if (ret == 0)
						{
							//�ر���
							SIM_LINFO(ref->sock.GetSocket() << " Recv 0 socket close.");
							Close(ref->sock.GetSocket(), ClosePassive);
						}
						else
						{
							//���ճɹ�,Ȼ��ص�
							if (ep_ref->type == TCP)
								ep_ref->OnRecvData(buff.get(), ret);
							else if (ep_ref->type == UDP)
								ep_ref->OnRecvDataFrom(buff.get(), ret,ip_buff,port);

							//�Ƿ��������
							if (ret >= ep_ref->recv_buff_size&&ep_ref->recv_buff_size < ep_ref->max_recv_buff_size)
							{
								ep_ref->recv_buff_size = ep_ref->recv_buff_size*1.5 + 1;
								if (ep_ref->recv_buff_size > ep_ref->max_recv_buff_size)
									ep_ref->recv_buff_size = ep_ref->max_recv_buff_size;
							}

							//������������
							ModifyEpollEvent(ref, EPOLLIN, true);
						}

					}
					if (EPOLLOUT &ee)
					{
						//�������� ��Ҫ���϶�����
						AutoMutex lk(ep_ref->send_queue_lock);
						//ȡ��һ���ڵ�
						QueueNode<SendBuff>*pHead = ep_ref->send_queue_buff.Next(NULL);
						if (NULL == pHead)
						{
							//�ڵ�Ϊ�գ��򲻴����ر������ؼ���
							SIM_LERROR(ref->sock.GetSocket() << " send cache is empty.");
							ModifyEpollEvent(ref, EPOLLOUT, false);
							continue;
						}
						//����Э��ѡ��ͬ�Ľӿڷ���
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
						//����ʧ��
						if (ret < 0)
						{
							SIM_LERROR(ref->sock.GetSocket() << " Send Failed." << strerror(errno));
							Close(ref->sock.GetSocket(), CloseError);
						}
						//���ͳɹ�����ƫ����
						pHead->data.offset += ret;
						if (pHead->data.offset >= pHead->data.buff.size())
						{
							//�������
							ep_ref->OnSendComplete(pHead->data.buff.get(), pHead->data.buff.size());
							//ȥ��
							ep_ref->send_queue_buff.PopFront(NULL);
						}
						//û��Ҫд����
						if (ep_ref->send_queue_buff.isEmpty())
						{
							ModifyEpollEvent(ref, EPOLLOUT, false);
						}
						else
						{
							//����
							ModifyEpollEvent(ref, EPOLLOUT, true);
						}
					}
				}
			}

			return SOCK_SUCCESS;
		}

		virtual AsyncHandle CreateHandle(SockType type)
		{
			//������ʵ��
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

			//���ö˿ڸ���
			ref->sock.SetReusePort(true);

			//��
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

			//���÷Ƕ���
			ref->sock.SetNonBlock(true);

			//�ӵ�epoll��
			AddEpoll(ref);
			ref->is_active = true;
			//��ӽ����¼�
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
			//���÷�����
			ref->sock.SetNonBlock(true);
			//��ӵ�epoll��
			AddEpoll(ref);
			//����
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
			//�������ö˿�
			ref->sock.SetReusePort(true);

			//��
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
			//�������ӽӿ�
			int ret = ep_ref->sock.Connect(ipaddr, port);
			//����ɹ���ֱ�ӻص�
			if (ret == 0)
			{
				ep_ref->OnConnect();
				return true;
			}
			//��������������������������������Ѿ������ˡ�
			ep_ref->connect_flag = true;
			return ModifyEpollEvent(ref, EPOLLOUT, true);
		}

		virtual bool Send(RefObject<AsyncContext> ref, RefBuff buff)
		{
			/*if (NULL == ref)
				return false;*/
			//�������ݵ�ʱ���Ƚ����ݼӵ���������Ȼ��������
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
			//sendto��Ҫ��ת����ַ
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
		//��������
		virtual bool Accept(RefObject<AsyncContext> ref)
		{
			//accept_flag ����ΪtrueȻ���������
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();
			ep_ref->accept_flag = true;
			return ModifyEpollEvent(ref, EPOLLIN, true);
		}

		//epoll���ƽӿ�
		virtual bool EpollCtrl(RefObject<AsyncContext> ref, int opt,uint32_t flag)
		{
			EpollAsyncContext* ep_ref = (EpollAsyncContext*)ref.get();

			//flag����
			ep_ref->ep_event.events = ep_ref->eflag=flag;
			ep_ref->ep_event.data.ptr = NULL;
			//�׽�������
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
			//��ӵ�epoll ��Ĭ������EPOLLIN ���� EPOLLHUP | EPOLLERR ��������쳣 ����
			return EpollCtrl(ref, EPOLL_CTL_ADD, EPOLLIN | EPOLLHUP | EPOLLERR);
		}
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
			//�Ƴ�����
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
				//���Ƴ�����
				RemoveEpoll(ref);
				//�ͷ�������
				ReleaseCtx(handle);
				////��ӡ
				//SIM_LDEBUG("handle " << handle << " closed ");
				////�ص�
				//ref->OnClose(reason, errno);
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