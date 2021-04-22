/*
	异步http协议簇
*/
#ifndef SIM_ASYNC_HTTP_HPP_
#define SIM_ASYNC_HTTP_HPP_
#include "Async.hpp"
#define SIM_PARSER_MULTI_THREAD
#include "HttpParser2.hpp"
namespace sim
{
	class AsyncSession;
	class AsyncHttp;

	typedef void(*ASYNC_HTTP_REQUEST_HANDLE)(AsyncHandle handle, HttpRequestHead *Head,
		ContentLength_t content_lenght, ContentLength_t offset, const char*buff, ContentLength_t len, void *pdata);

	typedef void(*ASYNC_HTTP_RESPONSE_HANDLE)(AsyncHandle handle, HttpResponseHead *Head,
		ContentLength_t content_lenght, ContentLength_t offset,
		const char*buff, ContentLength_t len, void *pdata);

	enum AsyncSessionType
	{
		AS_EMPTY,//未初始化的一般是服务器
		//客户端
		AS_HTTP,
		AS_HTTPS,
		AS_WS,
		AS_WSS,
	};

	//一个会话
	class AsyncSession
	{
	public:
		AsyncSession(AsyncHttp& async)
			:type_(AS_EMPTY), 
			handle_(INVALID_SOCKET),
			request_handler_(NULL),
			request_handler_data_(NULL),
			response_handler_(NULL),
			response_handler_data_(NULL),
			accept_handler(NULL), accept_handler_data(NULL)
			, connect_handler(NULL), connect_handler_data(NULL)
			, close_handler(NULL), close_handler_data(NULL)
			, close_flag_(false)
			, is_server(false)
			,async_(async)
		{

		}
		void CopyHandler(const AsyncSession* pctx)
		{
			if (pctx)
			{
				accept_handler = pctx->accept_handler;
				accept_handler_data = pctx->accept_handler_data;
				connect_handler = pctx->connect_handler;
				connect_handler_data = pctx->connect_handler_data;
				close_handler = pctx->close_handler;
				close_handler_data = pctx->close_handler_data;

				request_handler_ = pctx->request_handler_;
				request_handler_data_ = pctx->request_handler_data_;

				response_handler_ = pctx->response_handler_;
				response_handler_data_ = pctx->response_handler_data_;
			}
		}
	public:
		void OnRequest(HttpRequestHead *Head,
			ContentLength_t content_lenght, ContentLength_t offset, const char*buff, ContentLength_t len)
		{
			if (request_handler_)
				request_handler_(handle_, Head, content_lenght, offset,
					buff, len, request_handler_data_);
		}
		void OnResponse(HttpResponseHead *Head,
			ContentLength_t content_lenght, ContentLength_t offset,
			const char*buff, ContentLength_t len)
		{
			if (response_handler_)
				response_handler_(handle_, Head, content_lenght, offset, buff, len, response_handler_data_);
			//close 或者已经关闭了 最后一个请求
			if (offset+len>= content_lenght&&(HttpParser::IsClose(Head->Head) || close_flag_))
			{
				Close();
			}
		}
		virtual void OnAccept(AsyncHandle client)
		{
			if (accept_handler)
				accept_handler(handle_, client, accept_handler_data);
		}
		virtual void OnConnect()
		{
			if (connect_handler)
				connect_handler(handle_, connect_handler_data);
		}

		virtual void OnClose(AsyncCloseReason reason, int error)
		{
			if (close_handler)
				close_handler(handle_, reason, error, close_handler_data);
		}
		virtual void OnSendComplete();
		virtual void Close();
	public:
		ASYNC_HTTP_REQUEST_HANDLE request_handler_;
		void *request_handler_data_;
		ASYNC_HTTP_RESPONSE_HANDLE response_handler_;
		void *response_handler_data_;

		AcceptHandler accept_handler;
		void*accept_handler_data;
		ConnectHandler connect_handler;
		void*connect_handler_data;
		CloseHandler close_handler;
		void*close_handler_data;
	public:
		AsyncSessionType type_;
		bool is_server;
		//句柄
		AsyncHandle handle_;

		//解析器
		RefObject<BaseParser> parser_;

		//关闭标记 ==true 发送数据完毕之后关闭连接
		bool close_flag_;

		//http 公共报文头
		KvMap http_common_head_;

		AsyncHttp& async_;
	};
	
	class AsyncHttp:protected SimAsync
	{
	public:
		virtual int Poll(unsigned int wait_ms)
		{
			return SimAsync::Poll(wait_ms);
		}
		//生成一个空的会话
		virtual AsyncHandle CreateSession()
		{
			//SOCK_FAILURE
			AsyncHandle handle = CreateHandle(TCP);
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref)
			{
				AsyncSession*ss = new AsyncSession((*this));
				ref->ctx_data = ss;
				ref->close_handler = AsyncHttp::SessionCloseHandler;
				ref->close_handler_data = ref->ctx_data;
				ref->accept_handler = AsyncHttp::SessionAcceptHandler;
				ref->accept_handler_data = ref->ctx_data;
				ref->connect_handler = AsyncHttp::SessionConnectHandler;
				ref->connect_handler_data = ref->ctx_data;
				ref->recvdata_handler = AsyncHttp::SessionRecvDataHandler;
				ref->recvdata_handler_data = ref->ctx_data;
				ref->sendcomplete_handler = AsyncHttp::SessionSendCompleteHandler;
				ref->sendcomplete_handler_data = ref->ctx_data;
				ss->handle_ = handle;
			}
			return handle;
		}
		//设置回调
		virtual void SetAcceptHandler(AsyncHandle handle, AcceptHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->accept_handler = handler;
				((AsyncSession*)ref->ctx_data)->accept_handler_data = pdata;
			}
		}
		virtual void SetConnectHandler(AsyncHandle handle, ConnectHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->connect_handler = handler;
				((AsyncSession*)ref->ctx_data)->connect_handler_data = pdata;
			}
		}
		virtual void SetCloseHandler(AsyncHandle handle, CloseHandler handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->close_handler = handler;
				((AsyncSession*)ref->ctx_data)->close_handler_data = pdata;
			}
		}
		virtual AsyncSession* GetSession(AsyncHandle handle)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref && ref->ctx_data)
			{
				return ((AsyncSession*)ref->ctx_data);
			}
			return NULL;
		}
		virtual void SetHttpRequestHandler(AsyncHandle handle, ASYNC_HTTP_REQUEST_HANDLE handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->request_handler_ = handler;
				((AsyncSession*)ref->ctx_data)->request_handler_data_ = pdata;
			}
		}
		virtual void SetHttpResponseHandler(AsyncHandle handle, ASYNC_HTTP_RESPONSE_HANDLE handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->response_handler_ = handler;
				((AsyncSession*)ref->ctx_data)->response_handler_data_ = pdata;
			}
		}

		//设置https相关参数
		virtual int SetSSLKeyFile(AsyncHandle handle, const char *pub_key_file, const char*pri_key_file)
		{
			return SimAsync::SetSSLKeyFile(handle, pub_key_file, pri_key_file);
		}
		virtual int EnableSSL(AsyncHandle handle, bool is_server, bool is_add)
		{
			return SimAsync::ConvertToSSL(handle, is_server, is_add);
		}

		//连接 或者监听连接 \0 结尾
		virtual bool Connect(AsyncHandle handle, const char*host)
		{
			if (NULL == host)
				return false;

			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				AsyncSession *ss = (AsyncSession*)ref->ctx_data;

				HttpUrl out;
				if (!HttpParser::ParserUrl(host, out))
					return false;
				//解析域名
				sim::Socket::Init();
				Str ip = "";
				if (0 != sim::Socket::GetHostByName(out.host.c_str(), AsyncHttp::HTTP_CLI_GetHostByNameCallBack, &ip))
					return false;
				//添加通用头
				ss->http_common_head_.Append("Connection", "keep-alive");
				ss->http_common_head_.Append("User-Agent", "Sim.HttpApi");
				ss->http_common_head_.Append("Host", out.host);
				ss->http_common_head_.Append("Accept", "*/*");
				ss->is_server = false;
				if (out.scheme == "http")
				{
					ss->type_ = AS_HTTP;
				}
				else if (out.scheme == "ws")
				{
					ss->type_ = AS_WS;
				}
				else if (out.scheme == "https")
				{
					EnableSSL(handle, false, true);
					ss->type_ = AS_HTTPS;
				}
				else if (out.scheme == "wss")
				{
					EnableSSL(handle, false, true);
					ss->type_ = AS_WSS;
				}
				else
				{
					//不支持
					return false;
				}
				return AddTcpConnect(handle, ip.c_str(), out.port) == SOCK_SUCCESS;
			}
			return false;
		}
		
		virtual bool Listen(AsyncHandle handle, const char*host, const char *pub_key_file=NULL, const char*pri_key_file=NULL)
		{
			if (NULL == host)
				return false;

			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				AsyncSession *ss = (AsyncSession*)ref->ctx_data;

				HttpUrl out;
				if (!HttpParser::ParserUrl(host, out))
					return false;
				//解析域名
				sim::Socket::Init();
				Str ip = "";
				if(out.host.size()!=0)
					if (0 != sim::Socket::GetHostByName(out.host.c_str(), AsyncHttp::HTTP_CLI_GetHostByNameCallBack, &ip))
						return false;
				//添加通用头
				ss->http_common_head_.Append("Connection", "keep-alive");
				ss->http_common_head_.Append("Server", "Sim.HttpApi");
				ss->is_server = true;

				if (out.scheme == "http")
				{
					ss->type_ = AS_HTTP;
				}
				else if (out.scheme == "ws")
				{
					ss->type_ = AS_WS;
				}
				else if (out.scheme == "https")
				{
					EnableSSL(handle, true, true);
					SetSSLKeyFile(handle, pub_key_file, pri_key_file);
					ss->type_ = AS_HTTPS;
				}
				else if (out.scheme == "wss")
				{
					EnableSSL(handle, true, true);
					SetSSLKeyFile(handle,pub_key_file, pri_key_file);
					ss->type_ = AS_WSS;
				}
				else
				{
					//不支持
					return false;
				}
				return AddTcpServer(handle, ip.size()==0?NULL:ip.c_str(), out.port) == SOCK_SUCCESS;
			}
			return false;
		}
		//关闭
		virtual int Close(AsyncHandle handle)
		{
			return SimAsync::Close(handle);
		}
	public:
		//发送数据
		virtual bool Send(AsyncHandle handle, HttpRequestHead& Head,
			ContentLength_t content_lenght, ContentLength_t& offset,
			const char* buff, ContentLength_t len)
		{
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return false;
			}
			Head.Head.AppendMap(ss->http_common_head_);
			Str data = HttpParser::PrintRequest(Head, content_lenght, offset, buff, len);
			if (offset == content_lenght)
			{
				if (HttpParser::IsClose(Head.Head))
				{
					ss->close_flag_ = true;
				}
			}
			return Send(handle, data.c_str(), data.size());
		}
		virtual bool Send(AsyncHandle handle, HttpResponseHead& Head,
			ContentLength_t content_lenght, ContentLength_t& offset,
			const char* buff, ContentLength_t len)
		{
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return false;
			}
			Head.Head.AppendMap(ss->http_common_head_);
			Str data = HttpParser::PrintResponse(Head, content_lenght, offset, buff, len);
			if (offset == content_lenght)
			{
				if (HttpParser::IsClose(Head.Head))
				{
					ss->close_flag_ = true;
				}
			}
			return Send(handle, data.c_str(), data.size());
		}
		virtual int Send(AsyncHandle handle, const char *buff, unsigned int buff_len)
		{
			return SimAsync::Send(handle, buff,buff_len);
		}
	protected:
		//发送ws升级请求
		///Upgrade: websocket 升级为websocket
		virtual bool SendWebSocketUpgrade(AsyncSession* ss)
		{
			return false;
		}
		//协议升级为WebSocket
		virtual bool UpgradeWebSocket(AsyncSession* ss)
		{
			return false;
		}
	protected:
		//回调
		static void Session_HTTP_REQUEST_HANDLER(HttpParser* ctx, HttpRequestHead* Head,
			ContentLength_t content_lenght, ContentLength_t offset, const char* buff, ContentLength_t len, void* pdata)
		{
			if (pdata)
			{
				AsyncSession* ss = (AsyncSession*)pdata;
				ss->OnRequest(Head, content_lenght, offset, buff, len);
			}
		}

		static void Session_HTTP_RESPONSE_HANDLER(HttpParser* ctx, HttpResponseHead* Head,
			ContentLength_t content_lenght, ContentLength_t offset,
			const char* buff, ContentLength_t len, void* pdata)
		{
			if (pdata)
			{
				AsyncSession* ss = (AsyncSession*)pdata;
				ss->OnResponse(Head, content_lenght, offset, buff, len);
			}
		}

		static void SessionCloseHandler(AsyncHandle handle, AsyncCloseReason reason, int error, void*data)
		{
			if (data)
			{
				AsyncSession*ss = (AsyncSession*)data;
				ss->OnClose(reason, error);
				delete ss;
			}
		}
		
		static void SessionAcceptHandler(AsyncHandle handle, AsyncHandle client, void*data)
		{
			if (data)
			{
				AsyncSession* ss = (AsyncSession*)data;
				//布置上下文
				RefObject<AsyncContext> ref = ss->async_.GetCtx(client);
				if (ref)
				{
					AsyncSession* client_session = new AsyncSession(ss->async_);
					ref->ctx_data = client_session;
					ref->close_handler = AsyncHttp::SessionCloseHandler;
					ref->close_handler_data = ref->ctx_data;
					ref->accept_handler = AsyncHttp::SessionAcceptHandler;
					ref->accept_handler_data = ref->ctx_data;
					ref->connect_handler = AsyncHttp::SessionConnectHandler;
					ref->connect_handler_data = ref->ctx_data;
					ref->recvdata_handler = AsyncHttp::SessionRecvDataHandler;
					ref->recvdata_handler_data = ref->ctx_data;
					ref->sendcomplete_handler = AsyncHttp::SessionSendCompleteHandler;
					ref->sendcomplete_handler_data = ref->ctx_data;
					client_session->CopyHandler(ss);
					client_session->type_ = ss->type_;
					client_session->handle_ = client;
					//都是http解析器
					client_session->parser_ = RefObject<BaseParser>(new HttpRequestParser(AsyncHttp::Session_HTTP_REQUEST_HANDLER, client_session));
					client_session->http_common_head_ = ss->http_common_head_;
					//回调
					ss->OnAccept(client);
				}
			}
		}
		
		static void SessionConnectHandler(AsyncHandle handle, void*data)
		{
			if (data)
			{
				AsyncSession* ss = (AsyncSession*)data;
				ss->parser_ = 
					RefObject<BaseParser>(new HttpResponseParser(AsyncHttp::Session_HTTP_RESPONSE_HANDLER,(void*)ss));
				if (ss->type_ == AS_WS || ss->type_ == AS_WSS)
				{
					if (!ss->async_.SendWebSocketUpgrade(ss))
					{
						//请求失败
						ss->Close();
						return;
					}
				}
				ss->OnConnect();
			}
		}
		
		//tcp
		static void SessionRecvDataHandler(AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			if (data)
			{
				AsyncSession* ss = (AsyncSession*)data;
				if (ss->parser_)
					ss->parser_->Parser(buff, buff_len);
			}
		}
		
		static void SessionSendCompleteHandler(AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			if (data)
			{
				AsyncSession* ss = (AsyncSession*)data;
				ss->OnSendComplete();
			}
		}
		
		static bool HTTP_CLI_GetHostByNameCallBack(const char* ip, void* pdata)
		{
			if (pdata)
			{
				Str*s = (Str*)pdata;
				(*s) = ip;
			}
			return false;
		}
	private:
	};

	void sim::AsyncSession::OnSendComplete()
	{
		if (is_server&&close_flag_)
		{
			async_.Close(handle_);
		}
	}
	void sim::AsyncSession::Close()
	{
		async_.Close(handle_);
	}
}
#endif