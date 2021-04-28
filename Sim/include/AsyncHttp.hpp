/*
	异步http协议簇
*/
#ifndef SIM_ASYNC_HTTP_HPP_
#define SIM_ASYNC_HTTP_HPP_
#include "Async.hpp"
#define SIM_PARSER_MULTI_THREAD
#include "HttpParser.hpp"
#include "WebSocketParser.hpp"
namespace sim
{
	class AsyncSession;
	class AsyncHttp;

	typedef void(*ASYNC_HTTP_REQUEST_HANDLE)(AsyncHandle handle, HttpRequestHead *Head,
		ContentLength_t content_lenght, ContentLength_t offset, const char*buff, ContentLength_t len, void *pdata);

	typedef void(*ASYNC_HTTP_RESPONSE_HANDLE)(AsyncHandle handle, HttpResponseHead *Head,
		ContentLength_t content_lenght, ContentLength_t offset,
		const char*buff, ContentLength_t len, void *pdata);

	typedef void(*ASYNC_WS_HANDLER)(AsyncHandle handle, WebSocketFrameHead* pFrame,
		PayLoadLength_t payload_offset,
		const char*payload_data, PayLoadLength_t data_len,
		void* pdata);

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
			, ws_handler_(NULL), ws_handler_data_(NULL)
			, close_flag_(false)
			, is_server(false)
			, websocket_handshake_(false)
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

				ws_handler_ = pctx->ws_handler_;
				ws_handler_data_ = pctx->ws_handler_data_;
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
			if (offset+len>= content_lenght&&(HttpParser::IsClose(Head->Head)))
			{
				Close();
			}
		}

		void OnWsFrame(WebSocketFrameHead* pFrame,
			PayLoadLength_t payload_offset,
			const char*payload_data, PayLoadLength_t data_len)
		{
			if (ws_handler_)
				ws_handler_(handle_, pFrame, payload_offset, payload_data, data_len, ws_handler_data_);
			//close 或者已经关闭了 最后一个请求
			if (false == is_server)
			{
				if (payload_offset + data_len >= payload_offset && (pFrame->opcode == SIM_WS_OPCODE_CLOSE ))
				{
					Close();
				}
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
		ASYNC_WS_HANDLER ws_handler_;
		void *ws_handler_data_;

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

		HttpUrl url_;

		bool websocket_handshake_;
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
		virtual void SetWsFrameHandler(AsyncHandle handle, ASYNC_WS_HANDLER handler, void *pdata)
		{
			RefObject<AsyncContext> ref = GetCtx(handle);
			if (ref&&ref->ctx_data)
			{
				((AsyncSession*)ref->ctx_data)->ws_handler_ = handler;
				((AsyncSession*)ref->ctx_data)->ws_handler_data_ = pdata;
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

				if (!HttpParser::ParserUrl(host, ss->url_))
					return false;
				//解析域名
				sim::Socket::Init();
				Str ip = "";
				if (0 != sim::Socket::GetHostByName(ss->url_.host.c_str(), AsyncHttp::HTTP_CLI_GetHostByNameCallBack, &ip))
					return false;
				//添加通用头
				ss->http_common_head_.Append("Connection", "keep-alive");
				ss->http_common_head_.Append("User-Agent", "Sim.HttpApi");
				ss->http_common_head_.Append("Host", ss->url_.host);
				ss->http_common_head_.Append("Accept", "*/*");
				ss->is_server = false;
				if (ss->url_.scheme == "http")
				{
					ss->type_ = AS_HTTP;
				}
				else if (ss->url_.scheme == "ws")
				{
					ss->type_ = AS_WS;
				}
				else if (ss->url_.scheme == "https")
				{
					EnableSSL(handle, false, true);
					ss->type_ = AS_HTTPS;
				}
				else if (ss->url_.scheme == "wss")
				{
					EnableSSL(handle, false, true);
					ss->type_ = AS_WSS;
				}
				else
				{
					//不支持
					return false;
				}
				return AddTcpConnect(handle, ip.c_str(), ss->url_.port) == SOCK_SUCCESS;
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

				if (!HttpParser::ParserUrl(host, ss->url_))
					return false;
				//解析域名
				sim::Socket::Init();
				Str ip = "";
				if(ss->url_.host.size()!=0)
					if (0 != sim::Socket::GetHostByName(ss->url_.host.c_str(), AsyncHttp::HTTP_CLI_GetHostByNameCallBack, &ip))
						return false;
				//添加通用头
				ss->http_common_head_.Append("Connection", "keep-alive");
				ss->http_common_head_.Append("Server", "Sim.HttpApi");
				ss->is_server = true;

				if (ss->url_.scheme == "http")
				{
					ss->type_ = AS_HTTP;
				}
				else if (ss->url_.scheme == "ws")
				{
					ss->type_ = AS_WS;
				}
				else if (ss->url_.scheme == "https")
				{
					EnableSSL(handle, true, true);
					SetSSLKeyFile(handle, pub_key_file, pri_key_file);
					ss->type_ = AS_HTTPS;
				}
				else if (ss->url_.scheme == "wss")
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
				return AddTcpServer(handle, ip.size()==0?NULL:ip.c_str(), ss->url_.port) == SOCK_SUCCESS;
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
		virtual int Send(AsyncHandle handle, HttpRequestHead& Head,
			ContentLength_t content_lenght, ContentLength_t& offset,
			const char* buff, ContentLength_t len)
		{
			//printf("send http request\n");
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return -1;
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
		
		virtual int Send(AsyncHandle handle, HttpRequestHead& Head,
			const char* body, ContentLength_t size)
		{
			ContentLength_t offset = 0;
			return Send(handle, Head, size, offset, body, size);
		}

		virtual int Send(AsyncHandle handle, HttpResponseHead& Head,
			ContentLength_t content_lenght, ContentLength_t& offset,
			const char* buff, ContentLength_t len)
		{
			//printf("send http response\n");
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return -1;
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
		
		virtual int Send(AsyncHandle handle, HttpResponseHead& Head,
			const char* body, ContentLength_t size)
		{
			ContentLength_t offset = 0;
			return Send(handle, Head, size, offset, body, size);
		}

		virtual int  SendWebSocketFrame(AsyncHandle handle, const char* payload_data, 
			PayLoadLength_t data_len,
			unsigned char opcode = SIM_WS_OPCODE_TEXT)
		{
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return -1;
			}
			PayLoadLength_t payload_offset=0;
			WebSocketFrameHead FrameHead;
			FrameHead.fin = true;
			FrameHead.opcode = opcode;
			if (!ss->is_server)
				FrameHead.mask = true;
			FrameHead.payload_length = data_len;
			return Send(handle, FrameHead, payload_offset, payload_data, data_len);
		}
		virtual int  Send(AsyncHandle handle, WebSocketFrameHead& FrameHead, 
			PayLoadLength_t &payload_offset
			, const char*payload_data, PayLoadLength_t data_len)
		{
			//printf("send http  websocket frame\n");
			AsyncSession* ss = GetSession(handle);
			if (NULL == ss)
			{
				return -1;
			}

			Str data = WebSocketParser::PrintFrame(FrameHead, payload_offset, payload_data, data_len);
			if (FrameHead.payload_length == payload_offset)
			{
				if (SIM_WS_OPCODE_CLOSE == FrameHead.opcode)
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

		//http 主动回复websocket握手
		virtual bool ResponseWebSocketHandShake(AsyncHandle handle, const Str &SecWebSocketKey)
		{
			AsyncSession* ss = GetSession(handle);
			//会话存在 是服务端 而且 握手未成功
			if (ss&&(ss->is_server)&&(ss->websocket_handshake_==false))
			{
				//握手信息
				if (ResponseWebSocketUpgrade(ss->handle_, SecWebSocketKey))
				{
					if (UpgradeWebSocket(handle))
					{
						return true;
					}
				}
			}
			return false;
		}
		
		//主动升级为websocket
		virtual bool StartWebSocketHandShake(AsyncHandle handle, const Str&path)
		{
			AsyncSession* ss = GetSession(handle);
			//会话存在 是客户端 而且 握手未成功
			if (ss && (!ss->is_server) && (ss->websocket_handshake_ == false))
			{
				if (ss->type_ = AS_HTTP)
					ss->type_ = AS_WS;
				else if (ss->type_ = AS_HTTPS)
					ss->type_ = AS_WSS;
				else
					return true;

				if (SendWebSocketUpgrade(handle, path))
				{
					return true;
				}
			}
			return false;
		}
		virtual bool IsHas(AsyncHandle handle)
		{
			return SimAsync::IsHas(handle);
		}
		virtual bool IsActive(AsyncHandle handle)
		{
			return SimAsync::IsActive(handle);
		}
	protected:
		//发送ws升级请求
		///Upgrade: websocket 升级为websocket
		virtual bool SendWebSocketUpgrade(AsyncHandle handle,const Str&path)
		{
			/*
				GET / HTTP/1.1
				Host: localhost:8080
				Origin: http://127.0.0.1:3000
				Connection: Upgrade
				Upgrade: websocket
				Sec-WebSocket-Version: 13
				Sec-WebSocket-Key: w4v7O6xFTi36lq3RNcgctw==
			*/
			AsyncSession* ss = GetSession(handle);
			if (ss)
			{
				//ss->url_
				ss->http_common_head_.Append("Upgrade", "websocket", HM_COVER);
				ss->http_common_head_.Append("Connection", "Upgrade", HM_COVER);
				ss->http_common_head_.Append("Sec-WebSocket-Key", WebSocketParser::GenerateSecWebSocketKey(), HM_COVER);
				ss->http_common_head_.Append("Sec-WebSocket-Version", "13", HM_COVER);
				HttpRequestHead rhead;
				rhead.Method = "GET";
				rhead.Url = path;
				rhead.Version = "HTTP/1.1";
				ContentLength_t offset=0;
				return Send(handle, rhead, 0, offset, NULL, 0)!=-1;
			}
			return false;
		}
		
		//回复web升级请求
		virtual bool ResponseWebSocketUpgrade(AsyncHandle handle, const Str &SecWebSocketKey)
		{
			/*
				HTTP/1.1 101 Switching Protocols
				Connection:Upgrade
				Upgrade: websocket
				Sec-WebSocket-Accept: Oy4NRAQ13jhfONC7bP8dTKb4PTU=
			*/
			AsyncSession* ss = GetSession(handle);
			if (ss)
			{
				HttpResponseHead rhead;
				rhead.Status = "101";
				rhead.Reason = "Switching Protocols";
				rhead.Version = "HTTP/1.1";
				ContentLength_t offset = 0;
				rhead.Head.Append("Upgrade", "websocket", HM_COVER);
				rhead.Head.Append("Connection", "Upgrade", HM_COVER);
				rhead.Head.Append("Sec-WebSocket-Accept",
					WebSocketParser::GenerateSecWebSocketAccept(SecWebSocketKey),
					HM_COVER);
				return Send(handle, rhead, 0, offset, NULL, 0) != -1;
			}
			return false;
		}
		
		//协议升级为WebSocket
		virtual bool UpgradeWebSocket(AsyncHandle handle)
		{
			AsyncSession* ss = GetSession(handle);
			if (ss)
			{
				ss->parser_ = RefObject<BaseParser>(new WebSocketParser(AsyncHttp::Session_WEBSOCKET_HANDLER, (void*)ss));
				if (ss->type_ == AS_HTTP)
					ss->type_ = AS_WS;
				else if (ss->type_ == AS_HTTPS)
					ss->type_ = AS_WSS;
				ss->websocket_handshake_ = true;
				return true;
			}
			return false;
		}

		//检查
		virtual bool CheckWebSocketUpgradeResponse(AsyncHandle handle, HttpResponseHead *response)
		{
			AsyncSession* ss = GetSession(handle);
			if (ss&&response)
			{
				Str response_sec_accept = response->Head.GetCase("Sec-WebSocket-Accept", "");
				if (response_sec_accept.size() == 0)
				{
					return false;
				}
				Str request_sec_accept = WebSocketParser::GenerateSecWebSocketAccept(ss->http_common_head_.GetCase("Sec-WebSocket-Key", ""));
				if (request_sec_accept == response_sec_accept)
				{
					return true;
				}
			}
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
				if (ss->type_ == AS_WS || ss->type_ == AS_WSS)
				{
					if (ss->websocket_handshake_ == true)
						return;//已经握手成功了。

					Str SecKey = Head->Head.GetCase("Sec-WebSocket-Key", "");
					if (SecKey.size()!=0)
					{
						if (ss->async_.ResponseWebSocketHandShake(ss->handle_, SecKey))
						{
							ss->OnRequest(Head, content_lenght, offset, buff, len);
							return;
						}
					}
					
					//握手失败
					ss->Close();
					return;
				}
				else
				{
					ss->OnRequest(Head, content_lenght, offset, buff, len);
				}
			}
		}

		static void Session_HTTP_RESPONSE_HANDLER(HttpParser* ctx, HttpResponseHead* Head,
			ContentLength_t content_lenght, ContentLength_t offset,
			const char* buff, ContentLength_t len, void* pdata)
		{
			if (pdata)
			{
				AsyncSession* ss = (AsyncSession*)pdata;

				if (ss->type_ == AS_WS || ss->type_ == AS_WSS)
				{
					//如果是ws，而且收到101 "Upgrade", "websocket"
					if (Head->Status == "101"
						&& (BaseParser::ToLower("websocket") == BaseParser::ToLower(Head->Head.GetCase("Upgrade", ""))))
					{
						if (ss->async_.CheckWebSocketUpgradeResponse(ss->handle_, Head))
						{
							//握手成功
							if (ss->async_.UpgradeWebSocket(ss->handle_))
							{
								ss->OnResponse(Head, content_lenght, offset, buff, len);
								return;
							}
						}
					}
					//握手失败
					ss->Close();
				}
				else
				{
					ss->OnResponse(Head, content_lenght, offset, buff, len);
				}
			}
		}
		
		static void Session_WEBSOCKET_HANDLER(WebSocketParser* parser,
			WebSocketFrameHead* pFrame, PayLoadLength_t payload_offset,
			const char*payload_data, PayLoadLength_t data_len,
			void* pdata)
		{
			if (pdata)
			{
				AsyncSession* ss = (AsyncSession*)pdata;

				//收到未知的操作码关闭链接
				if (pFrame->opcode > SIM_WS_OPCODE_0F)
				{
					ss->Close();
					return;
				}
				ss->OnWsFrame(pFrame, payload_offset, payload_data, data_len);
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
					client_session->is_server = true;
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
					if (!ss->async_.SendWebSocketUpgrade(handle,ss->url_.path))
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
				{
					//防止回调中释放解析器
					RefObject<BaseParser> parser = ss->parser_;
					if (false == ss->parser_->Parser(buff, buff_len))
						ss->Close();
				}
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
		//服务端发送完毕就关闭连接，客户端接收完毕才关闭
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