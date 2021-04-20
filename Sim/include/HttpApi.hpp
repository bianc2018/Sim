/*
	http api
*/
#ifndef SIM_HTTP_API_HTTP_
#define SIM_HTTP_API_HTTP_
//#include "Timer.hpp"
#include <stdio.h>
#include "HttpParser.hpp"
#include "Async.hpp"
#include "TaskPool.hpp"

//全局环境句柄
#define SIM_HTTP_GLOBAL_CTX_THREAD_NUM 1
#define SIM_HTTP_GLOBAL_CTX_POLL_WAIT_MS 10000
namespace sim
{
	//获取文件大小
	size_t GetFileSize(const Str& file_name) {
		FILE* fp = fopen(file_name.c_str(), "r");
		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fclose(fp);
		return size; //单位是：byte
	}

	class HttpGlobalCtx
	{
		HttpGlobalCtx(int thread_num= SIM_HTTP_GLOBAL_CTX_THREAD_NUM)
			:pool(thread_num), exit_flag_(false)
		{
			for (int i = 0; i < thread_num; ++i)
				pool.Post(APoll, this, NULL);
		}

	public:
		~HttpGlobalCtx()
		{
			exit_flag_ = true;
			pool.WaitAllDone(SIM_HTTP_GLOBAL_CTX_POLL_WAIT_MS+1000);
		}
		static SimAsync &Get()
		{
			static HttpGlobalCtx ctx;
			return ctx.async_;
		}
	private:
		static void* APoll(void* lpParam)
		{
			HttpGlobalCtx* pc = (HttpGlobalCtx*)lpParam;
			while (!pc->exit_flag_)
			{
				pc->async_.Poll(SIM_HTTP_GLOBAL_CTX_POLL_WAIT_MS);
			}
			return NULL;
		}
	private:
		bool exit_flag_;
		sim::TaskPool pool;//线程池
		SimAsync async_;
	};
	enum HTTP_S_STATUS
	{
		//http 连接建立
		HttpConnect,
		//接收到一个完整的报文
		HttpGetResponseComplete,
		//接收到一个部分文件块
		HttpGetResponseChunk,
		HttpGetResquestComplete,
		HttpGetResquestChunk,
		//处理过程
		HttpProgress,
		HttpClose
	};
	

	typedef void(*HTTP_CLI_HANDLER)(HTTP_S_STATUS status,HttpResponse *response, void *pdata);
	class HttpClient
	{
		/*friend void HTTP_CLI_ConnectHandler(sim::AsyncHandle handle, void*data);
		friend void HTTP_CLI_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data);
		friend void HTTP_CLI_CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data);
		friend void HTTP_CLI_RESPONSE_HANDLER(HttpParser*ctx, HttpResponse *response, void *pdata);*/
	public:
		HttpClient(SimAsync &async, bool http_body_callback = false)
			:async_(async),
			parser_(HTTP_CLI_RESPONSE_HANDLER,this, http_body_callback),
			handler_(NULL),
			pdata_(NULL),
			close_flag_(false),
			handle_(INVALID_SOCKET)
		{
			
		}
		HttpClient(bool http_body_callback = false) :async_(HttpGlobalCtx::Get()),
			parser_(HTTP_CLI_RESPONSE_HANDLER, this, http_body_callback),
			handler_(NULL),
			pdata_(NULL),
			close_flag_(false),
			handle_(INVALID_SOCKET) {};
		~HttpClient()
		{
			Close();
		}
		bool ConnectHttp(const char*ip, int port)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			if (handle_ == INVALID_SOCKET)
				return false;
			async_.SetConnectHandler(handle_, HTTP_CLI_ConnectHandler, this);
			async_.SetRecvDataHandler(handle_, HTTP_CLI_RecvDataHandler, this);
			async_.SetCloseHandler(handle_, HTTP_CLI_CloseHandler, this);
			if(SOCK_SUCCESS == async_.AddTcpConnect(handle_, ip, port))
				return true;
			return true;
		}
		bool ConnectHttps(const char*ip, int port)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			if (handle_ == INVALID_SOCKET)
				return false;
			async_.ConvertToSSL(handle_, false, true);
			async_.SetConnectHandler(handle_, HTTP_CLI_ConnectHandler, this);
			async_.SetRecvDataHandler(handle_, HTTP_CLI_RecvDataHandler, this);
			async_.SetCloseHandler(handle_, HTTP_CLI_CloseHandler, this);
			if (SOCK_SUCCESS == async_.AddTcpConnect(handle_, ip, port))
				return true;
			return true;
		}
		bool Connect(const char*host)
		{
			if (NULL == host)
				return false;
			HttpUrl out;
			if (!HttpParser::ParserUrl(host,out))
				return false;
			sim::Socket::Init();
			Str ip="";
			if (0 != sim::Socket::GetHostByName(out.host.c_str(), HttpClient::HTTP_CLI_GetHostByNameCallBack,&ip))
				return false;
			if (out.scheme == "https")
				return ConnectHttps(ip.c_str(), out.port);
			else if (out.scheme == "http")
				return ConnectHttp(ip.c_str(), out.port);
			return false;
		}
		bool SetHandler(HTTP_CLI_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
			return true;
		}
		bool Request(HttpRequest &req)
		{
			Str data = parser_.PrintRequest(&req);
			return async_.Send(handle_, data.c_str(), data.size()) == SOCK_SUCCESS;
		}
		bool Get(const Str&path)
		{
			HttpRequest req;
			req.Method = "GET";
			req.Url = path;
			req.Version = "HTTP/1.1";
			req.Head.Append("Connection", "Close");
			req.Head.Append("Server", "Sim.HttpApi");
			return Request(req);
		}
		bool Post(const Str&path, const Str&data)
		{
			HttpRequest req;
			req.Method = "POST";
			req.Url = path;
			req.Version = "HTTP/1.1";
			req.Head.Append("Connection", "Close");
			req.Head.Append("Server", "Sim.HttpApi");
			//req.Body = data;
			req.Content.chunk = data;
			req.Content.is_complete = true;
			req.Content.content_length = data.size();
			req.Content.offset = 0;
			return Request(req);
		}
		bool Close()
		{
			return async_.Close(handle_)==SOCK_SUCCESS;
		}
	public:
		//进度回调
		typedef bool(*SyncProgress)(ContentLength_t total_bytes, ContentLength_t now_bytes, HttpResponse *response,void*pdata);
		//上下文
		struct SyncCtx
		{
			HttpClient *cli;
			HttpRequest *request;
			HttpResponse *response;
			bool is_ok;
			bool is_close;//是否已经关闭
			//过程回调
			SyncProgress pro_handler;
			void* pdata;
		};
		static bool Request(const Str&Method, const Str &Url, const Str &Body,
			HttpResponse &last_response, int timeout_ms = 1000, HttpMap *exthead = NULL
		, SyncProgress pro_handler=NULL,void*pdata=NULL)
		{
			SyncCtx ctx;
			HttpClient cli(pro_handler !=NULL);

			//解析连接
			HttpUrl out;
			if (!HttpParser::ParserUrl(Url, out))
				return false;

			HttpRequest request;
			request.Method = Method;
			request.Url = out.path;
			request.Version = "HTTP/1.1";
			//request.Body = Body;
			request.Content.chunk = Body;
			request.Content.is_complete = true;
			request.Content.content_length = Body.size();
			request.Content.offset = 0;

			request.Head.Append("Connection", "Close");
			request.Head.Append("User-Agent", "Sim.HttpApi");
			request.Head.Append("Host", out.host);
			request.Head.Append("Accept", "*/*");
			if (exthead)
			{
				request.Head.AppendMap(*exthead);
			}

			//解析域名
			sim::Socket::Init();
			Str ip = "";
			if (0 != sim::Socket::GetHostByName(out.host.c_str(), HttpClient::HTTP_CLI_GetHostByNameCallBack, &ip))
				return false;
			
			ctx.cli = &cli;
			ctx.request = &request;
			ctx.response = &last_response;
			ctx.is_ok = false;
			ctx.is_close = false;
			ctx.pro_handler = pro_handler;
			ctx.pdata = pdata;

			cli.SetHandler((HTTP_CLI_HANDLER)HttpClient::HTTP_CLI_Sync_Response, &ctx);
			//连接
			if (out.scheme == "https")
			{
				if (!cli.ConnectHttps(ip.c_str(), out.port))
					return false;
			}
			else if (out.scheme == "http")
			{
				if (!cli.ConnectHttp(ip.c_str(), out.port))
					return false;
			}
			else
			{
				return false;
			}
			//等待
			const int wait_ms = 10;
			bool no_time_out = timeout_ms <= 0; //<=0 不退出
			while (timeout_ms > 0||no_time_out)
			{
				if (ctx.is_ok|| ctx.is_close)
					break;
				Time::Sleep(wait_ms);
				timeout_ms -= wait_ms;
			}
			return ctx.is_ok;
		}

		//静态通用接口
		static bool Get(const Str&Url, HttpResponse &response,int timeout_ms = 1000,
			HttpMap *exthead=NULL, SyncProgress pro_handler = NULL, void*pdata = NULL)
		{
			return Request("GET", Url, "", response, timeout_ms, exthead, pro_handler,pdata);
		}
		static bool Post(const Str&Url, const Str&Body, HttpResponse &response, int timeout_ms = 1000,
			HttpMap *exthead = NULL, SyncProgress pro_handler = NULL, void*pdata = NULL)
		{
			return Request("POST", Url, Body, response, timeout_ms, exthead, pro_handler, pdata);
		}
	private:
		static void HTTP_CLI_ConnectHandler(sim::AsyncHandle handle, void*data)
		{
			if (data)
			{
				HttpClient*cli = (HttpClient*)data;
				cli->OnHttpHandler(HttpConnect, NULL);
			}
		}
		static void HTTP_CLI_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			if (data)
			{
				HttpClient*cli = (HttpClient*)data;
				if (false == cli->parser_.Parser(buff, buff_len))
				{
					//关闭
					cli->Close();
				}
			}
		}
		static void HTTP_CLI_CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
		{
			if (data)
			{
				HttpClient*cli = (HttpClient*)data;
				cli->OnHttpHandler(HttpClose, NULL);
			}
		}
		static void HTTP_CLI_RESPONSE_HANDLER(HttpParser*ctx, HttpResponse *response, void *pdata)
		{
			if (pdata)
			{
				HttpClient*cli = (HttpClient*)pdata;
				if (ctx->GetParserStatus() == HTTP_COMPLETE)
				{
					cli->OnHttpHandler(HttpGetResponseComplete, response);
					if (HttpParser::ToLower(response->Head.GetCase(SIM_HTTP_CON, "Close")) == HttpParser::ToLower("Close"))
					{
						cli->Close();
					}
				}
				else if (ctx->GetParserStatus() == HTTP_CHUNK)
				{
					cli->OnHttpHandler(HttpGetResponseChunk, response);
				}
				else if (ctx->GetParserStatus() == HTTP_BODY)
				{
					cli->OnHttpHandler(HttpProgress, response);
				}
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
		//同步回调
		static void HTTP_CLI_Sync_Response(HTTP_S_STATUS status, HttpResponse *response, void *pdata)
		{
			if (pdata)
			{
				SyncCtx* pc = (SyncCtx*)pdata;
				if (status == HttpConnect)
				{
					pc->cli->Request(*pc->request);
				}
				else if (status == HttpGetResponseComplete)
				{
					if (pc->pro_handler)
					{
						if (!pc->pro_handler(HttpParser::GetHeadContentLen(response->Head), 
							response->Content.offset+response->Content.chunk.size(), response,pdata))
						{
							pc->is_close = true;
							return;
						}
					}

					*(pc->response) = (*response);
					pc->is_ok = true;
				}
				else if (status == HttpGetResponseChunk||status == HttpProgress)
				{
					if (pc->pro_handler)
					{
						if (!pc->pro_handler(HttpParser::GetHeadContentLen(response->Head),
							response->Content.offset + response->Content.chunk.size(),response,pdata))
						{
							pc->is_close = true;
						}
					}
				}
				else if (status == HttpClose)
				{
					pc->is_close = true;
				}
			}
		}
	private:
		void OnHttpHandler(HTTP_S_STATUS status,HttpResponse *response)
		{
			if (handler_)
				handler_(status,response, pdata_);
		}
		
		/*static bool Request(HttpClient &cli, HttpUrl &Url, HttpRequest &request,
			HttpResponse &response, int timeout_ms = 1000)*/
		
	private:
		bool close_flag_;
		HTTP_CLI_HANDLER handler_;
		void *pdata_;
		SimAsync &async_;
		AsyncHandle handle_;
		HttpParser parser_;
	};
	
	typedef void(*HTTP_SIMPLE_SERV_HANDLER)(HttpRequest*request, HttpResponse *response, void *pdata);
	class HttpSimpleServer
	{
		struct HttpSession
		{
			HttpParser parser;
			HttpResponse response;
			AsyncHandle handle;
			HttpSimpleServer &srv;
			bool close_flag;
			HttpSession(HttpSimpleServer &s, AsyncHandle cli)
				:srv(s), parser(HttpSimpleServer::HTTP_SERV_REQUEST_HANDLER,this), handle(cli), close_flag(false)
			{
				response.Clear();
			}
			void Close()
			{
				srv.async_.Close(handle);
			}
			void OnSendComplete()
			{
				if (close_flag)
				{
					Close();
				}
			}
		};
	public:
		HttpSimpleServer(SimAsync &async) :handler_(NULL), async_(async),
			pdata_(NULL),
			handle_(-1)
		{

		}
		HttpSimpleServer():handler_(NULL), async_(HttpGlobalCtx::Get()),
			pdata_(NULL),
			handle_(-1)
		{

		}
	public:
		bool ListenHttp(int port,
			const char*ip=NULL)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			//async_.SetRecvDataHandler(handle_, HTTP_SERV_RecvDataHandler, this);
			async_.SetAcceptHandler(handle_, HTTP_SERV_AcceptHandler, this);
			async_.AddTcpServer(handle_, ip, port);
			return true;
		}
		bool ListenHttps(int port,
			const char *pub_file = "cert.pem",
			const char *pri_file = "key.pem",
			const char*ip = NULL)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			async_.ConvertToSSL(handle_, true, true);
			async_.SetSSLKeyFile(handle_, pub_file, pri_file);
			//async_.SetRecvDataHandler(handle_, HTTP_SERV_RecvDataHandler, this);
			async_.SetAcceptHandler(handle_, HTTP_SERV_AcceptHandler, this);
			async_.AddTcpServer(handle_, ip, port);
			return true;
		}
		bool SetHandler(HTTP_SIMPLE_SERV_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
			return true;
		}
	private:
		static void HTTP_SERV_AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
		{
			printf("accept %d\n", client);
			if (data)
			{
				HttpSimpleServer*ss = (HttpSimpleServer*)data;
				HttpSession*pss = new HttpSession((*ss),client);
				ss->async_.SetCloseHandler(client, HTTP_SERV_CloseHandler, pss);
				ss->async_.SetRecvDataHandler(client, HTTP_SERV_RecvDataHandler, pss);
			}
		}
		static void HTTP_SERV_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			printf("recv %d\n", handle);
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				ss->parser.Parser(buff, buff_len);
			}
		}
		static void HTTP_SERV_CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
		{
			printf("close %d\n", handle);
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				delete ss;
			}
		}
		static void HTTP_SERV_REQUEST_HANDLER(HttpParser*ctx, HttpRequest *request, void *pdata)
		{
			if (pdata)
			{
				HttpSession*ss = (HttpSession*)pdata;

				if (ctx->GetParserStatus() == HTTP_COMPLETE)
				{
					ss->srv.OnHttpHandler(request, &ss->response);
					if (HttpParser::ToLower(ss->response.Head.GetCase(SIM_HTTP_CON, "Close")) == HttpParser::ToLower("Close"))
					{
						ss->close_flag = true;
					}

					Str data = ctx->PrintResponse(&ss->response);
					ss->srv.async_.Send(ss->handle, data.c_str(), data.size());
					ss->response.Clear();
				}
				else if (ctx->GetParserStatus() == HTTP_CHUNK)
				{
					ss->srv.OnHttpHandler(request, &ss->response);
				}
			}
		}
		static void HTTP_CLI_SendCompleteHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				ss->OnSendComplete();
			}
		}
	private:
		void OnHttpHandler(HttpRequest *request,HttpResponse *response)
		{
			if (handler_)
				 handler_(request, response,pdata_);
		}
	private:
		HTTP_SIMPLE_SERV_HANDLER handler_;
		void *pdata_;
		SimAsync &async_;
		AsyncHandle handle_;
		//解析器句柄
		//RbTree<RefObject<HttpParser> > parser_map_;
	};

	class HttpServer;

	class HttpSession
	{
		friend class HttpServer;
	public:
		//发送请求
		bool SendResponse(HttpResponse *response);
		//发送文件回复
		bool SendFile(const Str&filepath,HttpMap*ext_head=NULL,ContentLength_t buff_size =1024*1024*4);
		//发送裸数据
		bool SendData(const char*buff, unsigned int bufflen);
		HttpParserStatus GetStatus();
	private:
		HttpParser parser;
		AsyncHandle handle;
		HttpServer &srv;
		bool close_flag;
		HttpSession(HttpServer &s, AsyncHandle cli);
		void Close();
		void OnSendComplete()
		{
			if (close_flag)
			{
				Close();
			}
		}
	};
	typedef void(*HTTP_SERV_HANDLER)(HttpSession *ss,HttpRequest*request, void *pdata);
	class HttpServer
	{
		friend class HttpSession;
	public:
		HttpServer(SimAsync &async) :handler_(NULL), async_(async),
			pdata_(NULL),
			handle_(-1)
		{

		}
		HttpServer() :handler_(NULL), async_(HttpGlobalCtx::Get()),
			pdata_(NULL),
			handle_(-1)
		{

		}
	public:
		bool ListenHttp(int port,
			const char*ip = NULL)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			async_.SetAcceptHandler(handle_, HTTP_SERV_AcceptHandler, this);
			async_.AddTcpServer(handle_, ip, port);
			return true;
		}
		bool ListenHttps(int port,
			const char *pub_file = "cert.pem",
			const char *pri_file = "key.pem",
			const char*ip = NULL)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			async_.ConvertToSSL(handle_, true, true);
			async_.SetSSLKeyFile(handle_, pub_file, pri_file);
			async_.SetAcceptHandler(handle_, HTTP_SERV_AcceptHandler, this);
			async_.AddTcpServer(handle_, ip, port);
			return true;
		}
		bool SetHandler(HTTP_SERV_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
			return true;
		}
	private:
		static void HTTP_SERV_AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
		{
			printf("accept %d\n", client);
			if (data)
			{
				HttpServer*ss = (HttpServer*)data;
				HttpSession*pss = new HttpSession((*ss), client);
				ss->async_.SetCloseHandler(client, HTTP_SERV_CloseHandler, pss);
				ss->async_.SetRecvDataHandler(client, HTTP_SERV_RecvDataHandler, pss);
			}
		}
		static void HTTP_SERV_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			printf("recv %d\n", handle);
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				ss->parser.Parser(buff, buff_len);
			}
		}
		static void HTTP_SERV_CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
		{
			printf("close %d\n", handle);
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				delete ss;
			}
		}
		static void HTTP_SERV_REQUEST_HANDLER(HttpParser*ctx, HttpRequest *request, void *pdata)
		{
			if (pdata)
			{
				HttpSession*ss = (HttpSession*)pdata;

				if(ss)
					ss->srv.OnHttpHandler(ss, request);
			}
		}
		static void HTTP_CLI_SendCompleteHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			if (data)
			{
				HttpSession*ss = (HttpSession*)data;
				ss->OnSendComplete();
			}
		}
	private:
		void OnHttpHandler(HttpSession *ss, HttpRequest*request)
		{
			if (handler_)
				handler_(ss, request, pdata_);
		}
	private:
		HTTP_SERV_HANDLER handler_;
		void *pdata_;
		SimAsync &async_;
		AsyncHandle handle_;
		//解析器句柄
		//RbTree<RefObject<HttpParser> > parser_map_;
	};

	HttpSession::HttpSession(HttpServer &s, AsyncHandle cli)
		:srv(s), parser(HttpServer::HTTP_SERV_REQUEST_HANDLER, this), handle(cli), close_flag(false)
	{
	}
	void HttpSession::Close()
	{
		srv.async_.Close(handle);
	}
	bool HttpSession::SendResponse(HttpResponse *response)
	{
		Str data = HttpParser::PrintResponse(response);
		if (data.size() == 0)
		{
			return false;
		}
		if (HttpParser::IsClose(response->Head))
		{
			close_flag = true;
		}
		return SendData(data.c_str(), data.size());
	}
	inline bool HttpSession::SendFile(const Str & filepath, HttpMap * ext_head, ContentLength_t buff_size)
	{
		HttpResponse response;
		if(ext_head)
			response.Head.AppendMap(*ext_head);
		
		response.Head.Append(SIM_HTTP_CON, "Close");
		//
		response.Head.Append("Server", "Sim.HttpApi");

		FILE* fp = fopen(filepath.c_str(), "rb");
		if (NULL == fp)
		{
			//文件打开失败
			response.Status = "404";
			response.Reason = "Not Found";
			response.Head.Append(SIM_HTTP_CON, "Close");
			return SendResponse(&response);
		}
		fseek(fp, 0, SEEK_END);
		ContentLength_t size = ftell(fp);

		//返回开头 重新打开
		//fclose(fp);
		//fp = fopen(filepath.c_str(), "r");
		fseek(fp, 0, SEEK_SET);

		//设置body长度 SIM_HTTP_CL 后续支持range
		response.Head.Append(SIM_HTTP_CL, HttpParser::NumToStr<ContentLength_t>(size, "%llu"), HM_COVER);//直接覆盖掉
		
		//发送报文头
		Str Head = HttpParser::PrintStartLine(response.Version, response.Status, response.Reason) + SIM_HTTP_CRLF 
			+ HttpParser::PrintHead(response.Head) + SIM_HTTP_CRLF;
		if (false == SendData(Head.c_str(), Head.size()))
		{
			fclose(fp);
			return false;
		}
		
		//发送报文体
		RefBuff buff(buff_size);
		while (true)
		{
			int readlen = fread(buff.get(), sizeof(char), buff.size(), fp);
			if (readlen == 0)
			{
				fclose(fp);
				if (HttpParser::IsClose(response.Head))
				{
					close_flag = true;
				}
				return true;
			}
			if (readlen < 0)
			{
				fclose(fp);
				return false;
			}
			if (false == SendData(buff.get(), readlen))
			{
				fclose(fp);
				return false;
			}
		}
		
	}
	bool HttpSession::SendData(const char*buff, unsigned int bufflen)
	{
		return SOCK_SUCCESS == srv.async_.Send(handle, buff, bufflen);
	}
	HttpParserStatus HttpSession::GetStatus()
	{
		return parser.GetParserStatus();
	}
}
#endif