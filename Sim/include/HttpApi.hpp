/*
	http api
*/
#ifndef SIM_HTTP_API_HTTP_
#define SIM_HTTP_API_HTTP_
#include "HttpParser.hpp"
#include "Async.hpp"
#include "TaskPool.hpp"

//全局环境句柄
#define SIM_HTTP_GLOBAL_CTX_THREAD_NUM 1
#define SIM_HTTP_GLOBAL_CTX_POLL_WAIT_MS 10000
namespace sim
{
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
		HttpConnect,
		HttpGetResponse,
		HttpGetResquest,
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
		HttpClient(SimAsync &async)
			:async_(async),
			parser_(HTTP_CLI_RESPONSE_HANDLER,this),
			handler_(NULL),
			pdata_(NULL),
			handle_(-1)
		{
			
		}
		HttpClient() :HttpClient(HttpGlobalCtx::Get()) {};
		~HttpClient()
		{
			async_.Close(handle_);
		}
		bool ConnectHttp(const char*ip, int port)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			async_.SetConnectHandler(handle_, HTTP_CLI_ConnectHandler, this);
			async_.SetRecvDataHandler(handle_, HTTP_CLI_RecvDataHandler, this);
			async_.SetCloseHandler(handle_, HTTP_CLI_CloseHandler, this);
			async_.AddTcpConnect(handle_, ip, port);
			return true;
		}
		bool ConnectHttps(const char*ip, int port)
		{
			async_.Close(handle_);
			handle_ = async_.CreateHandle(sim::TCP);
			
			async_.ConvertToSSL(handle_, false, true);
			async_.SetConnectHandler(handle_, HTTP_CLI_ConnectHandler, this);
			async_.SetRecvDataHandler(handle_, HTTP_CLI_RecvDataHandler, this);
			async_.SetCloseHandler(handle_, HTTP_CLI_CloseHandler, this);
			async_.AddTcpConnect(handle_, ip, port);
			return true;
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
			req.Head.Append("Connection", "Keep-Alive");
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
			req.Body = data;
			return Request(req);
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
				cli->parser_.Parser(buff, buff_len);
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
				cli->OnHttpHandler(HttpGetResponse, response);
			}
		}
	private:
		void OnHttpHandler(HTTP_S_STATUS status,HttpResponse *response)
		{
			if (handler_)
				handler_(status,response, pdata_);
		}
	private:
		HTTP_CLI_HANDLER handler_;
		void *pdata_;
		SimAsync &async_;
		AsyncHandle handle_;
		HttpParser parser_;
	};
	

	typedef bool(*HTTP_SERV_HANDLER)(HttpRequest*request, HttpResponse *response, void *pdata);
	class HttpServer
	{
		struct HttpSession
		{
			HttpParser parser;
			AsyncHandle handle;
			HttpServer &srv;
			HttpSession(HttpServer &s, AsyncHandle cli)
				:srv(s), parser(HttpServer::HTTP_SERV_REQUEST_HANDLER,this), handle(cli)
			{

			}
		};
	public:
		HttpServer(SimAsync &async) :handler_(NULL), async_(async),
			pdata_(NULL),
			handle_(-1)
		{

		}
		HttpServer() :HttpServer(HttpGlobalCtx::Get())
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
				HttpResponse response;
				response.Status = "200";
				response.Reason = "OK";
				response.Version = "HTTP/1.1";
				if (ss->srv.OnHttpHandler(request, &response))
				{
					Str data =  ctx->PrintResponse(&response);
					ss->srv.async_.Send(ss->handle, data.c_str(), data.size());
				}
				else
				{
					ss->srv.async_.Close(ss->handle);
				}

			}
		}
	private:
		bool OnHttpHandler(HttpRequest *request,HttpResponse *response)
		{
			if (handler_)
				return handler_(request, response,pdata_);
			return false;
		}
	private:
		HTTP_SERV_HANDLER handler_;
		void *pdata_;
		SimAsync &async_;
		AsyncHandle handle_;
		//解析器句柄
		//RbTree<RefObject<HttpParser> > parser_map_;
	};

}
#endif