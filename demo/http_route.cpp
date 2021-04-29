/*
	http请求测试
*/
#include <stdio.h>
#include <map>
#include "Timer.hpp"
#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include "CodeConvert.hpp"
#define MY_THREAD_NUM 8
//控制参数输入
sim::CmdLineParser cmd;

void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void* data);

void TIMER_OUT_HANDLER(sim::timer_id timer_id, void* userdata);

void ConnectHandler(sim::AsyncHandle handle, void* data);

void ASYNC_HTTP_RESPONSE_HANDLE(sim::AsyncHandle handle, sim::HttpResponseHead* Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset,
	const char* buff, sim::ContentLength_t len, void* pdata);

void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead* Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset,
	const char* buff, sim::ContentLength_t len,
	void* pdata);

struct LinkCache
{
	sim::HttpRequestHead Head;
	sim::ContentLength_t content_lenght;
	sim::ContentLength_t offset;
	sim::RefBuff buff;
};

struct GlobalConfig
{
	sim::Str Listen;
	sim::Str Srv;
	sim::Str pub_key;
	sim::Str pri_key;

	int timeout;
	int thread_num;//线程数

	sim::Mutex cache_lock_;
	sim::RbTree<sim::Queue<LinkCache>*> caches_;
};

GlobalConfig conf;


struct LinkCtx
{
	//主链接 客户端到代理服务器(本机)
	sim::AsyncHandle sub;
	//定时器id
	sim::timer_id timer_id;

	LinkCtx()
		:sub(-1), timer_id(0)
	{

	}
	void ReSetTimer(sim::AsyncHandle main)
	{
		if (conf.timeout > 0)
		{
			if (timer_id != 0)
			{
				sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(timer_id);
				timer_id = 0;
			}

			timer_id = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(conf.timeout, TIMER_OUT_HANDLER, (void*)main);
		}
	}
	void DelTimer()
	{
		if (conf.timeout > 0)
		{
			if (timer_id != 0)
			{
				sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(timer_id);
				timer_id = 0;
			}
		}
	}
};

sim::Str print_bytes(double bytes)
{
	const int buff_size = 1024;
	char buff[buff_size] = { 0 };
	if (bytes < 1024)
	{
		snprintf(buff, buff_size, "%0.3lf bytes", bytes);
	}
	else if (bytes < 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf kb", double(bytes) / 1024);
	}
	else if (bytes < 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf mb", double(bytes) / (1024 * 1024));
	}
	else if (bytes > 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf gb", double(bytes) / (1024 * 1024 * 1024));
	}
	return buff;
}
sim::AsyncHttp& GetHttp()
{
	return sim::GlobalPoll<sim::AsyncHttp, MY_THREAD_NUM>::Get();
}

void send_error(sim::AsyncHandle handle, const sim::Str& status, const sim::Str& reason)
{
	sim::HttpResponseHead res;
	res.Head.Append(SIM_HTTP_CON, "Close");
	res.Status = status;
	res.Reason = reason;
	GetHttp().Send(handle, res, NULL, 0);
	return;
}

LinkCtx * create_link_ctx(sim::AsyncHandle handle,LinkCtx *Main)
{
	//建立连接
	LinkCtx *Sub = new LinkCtx;

	sim::AsyncHandle sub = GetHttp().CreateSession();

	Main->sub = sub;
	Sub->sub = handle;

	GetHttp().SetCloseHandler(sub, CloseHandler, Sub);
	GetHttp().SetConnectHandler(sub, ConnectHandler, Sub);
	GetHttp().SetHttpResponseHandler(sub, ASYNC_HTTP_RESPONSE_HANDLE, Sub);

	if (false == GetHttp().Connect(sub, conf.Srv.c_str()))
	{
		printf("%d connect bad\n", sub);
		//链接失败
		GetHttp().Close(sub);
		//send_error(handle, "502", "Bad Gateway");
		return NULL;
	}
	//GetHttp().SetSSLKeyFile(sub, "cert.pem", "key.pem");
	//Main->ReSetTimer(client);
	Sub->ReSetTimer(sub);
	return Sub;
}

//缓存报文
bool add_cache(LinkCtx *Main,sim::HttpRequestHead* Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset,
	const char* buff, sim::ContentLength_t len)
{
	LinkCache cache;
	cache.buff = sim::RefBuff(buff, len);
	cache.content_lenght = content_lenght;
	cache.Head = *Head;
	cache.Head.Head.Del("Host");
	cache.offset = offset;
	//加到缓存里面
	sim::AutoMutex lk(conf.cache_lock_);
	sim::Queue<LinkCache> *p_cache_queue = NULL;
	if (false == conf.caches_.Find(Main->sub, &p_cache_queue))
	{
		p_cache_queue = new sim::Queue<LinkCache>();
		conf.caches_.Add(Main->sub, p_cache_queue);
	}
	p_cache_queue->PushBack(cache);
	return true;
}

void TIMER_OUT_HANDLER(sim::timer_id timer_id, void* userdata)
{
	if (userdata)
	{
		sim::AsyncHandle handle = (long)userdata;
		printf("%d time out [timer id %llu]\n", handle, timer_id);
		sim::GlobalPoll<sim::AsyncHttp>::Get().Close(handle);
	}
	
}

//code_convert.cpp
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void* data)
{
	static sim::Str reasons[3] = {"CloseError","CloseActive","ClosePassive"};
	printf("%d close reason %s error %d\n", handle, reasons[reason].c_str(), error);
	if (data)
	{
		LinkCtx* pl = (LinkCtx*)data;
		sim::GlobalPoll<sim::AsyncHttp>::Get().Close(pl->sub);
		pl->DelTimer();
		delete pl;
	}
}

//遍历 返回false终止
bool TraverseFunc(LinkCache* Now, void*pdata)
{
	if (Now&&pdata)
	{
		sim::AsyncHandle handle = (long)(pdata);
		GetHttp().Send(handle, Now->Head, Now->content_lenght, Now->offset, Now->buff.get(), Now->buff.size());
	}
	return true;
}
void ConnectHandler(sim::AsyncHandle handle, void* data)
{
	printf("%d Connect\n", handle);

	//发送缓存数据
	LinkCache cache;
	//加到缓存里面
	sim::AutoMutex lk(conf.cache_lock_);
	sim::Queue<LinkCache> *p_cache_queue = NULL;
	if (conf.caches_.Find(handle, &p_cache_queue))
	{
		conf.caches_.Del(handle);
		p_cache_queue->Traverse(TraverseFunc, (void*)handle);
		delete p_cache_queue;
	}
}

void ASYNC_HTTP_RESPONSE_HANDLE(sim::AsyncHandle handle, sim::HttpResponseHead* Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset,
	const char* buff, sim::ContentLength_t len, void* pdata)
{
	printf("%d response %s(%s/%s)\n", handle, print_bytes(len).c_str(),
		print_bytes(offset).c_str(), print_bytes(content_lenght).c_str());
	if (pdata)
	{
		LinkCtx *link = (LinkCtx *)pdata;
		link->ReSetTimer(handle);
		//转发
		GetHttp().Send(link->sub, *Head, content_lenght, offset, buff, len);
	}
}

void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead* Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, 
	const char* buff, sim::ContentLength_t len,
	void* pdata)
{
	printf("%d request %s\n", handle, Head->Url.c_str());
	if (pdata)
	{
		LinkCtx *link = (LinkCtx *)pdata;
		//link->ReSetTimer(handle);
		while (true)
		{
			if (GetHttp().IsHas(link->sub))
			{
				if (GetHttp().IsActive(link->sub))
				{
					printf("sub %d is active\n", link->sub);
					break;
				}
			}
			else
			{
				printf("sub %d is close\n", link->sub);
				//send_error(handle, "502", "Bad Gateway");
				//建立连接
				LinkCtx *Sub = create_link_ctx(handle, link);
				if (NULL == Sub)
				{
					send_error(handle, "502", "Bad Gateway");
					return ;
				}
				if (false == add_cache(link, Head, content_lenght, offset, buff, len))
				{
					send_error(handle, "502", "Bad Gateway");
					return;
				}
				return;
			}
			//等待连接建立 或者超时
			if (!GetHttp().IsActive(handle))
			{
				printf("%d is no active\n", handle);
				return;
			}
			printf("wait sub %d\n", link->sub);
			if (false == add_cache(link, Head, content_lenght, offset, buff, len))
			{
				send_error(handle, "502", "Bad Gateway");
				return;
			}
			return;
		}
		Head->Head.Del("Host");
		//转发
		GetHttp().Send(link->sub, *Head, content_lenght, offset, buff, len);
	}
}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void* data)
{
	printf("%d accept %d\n", handle, client);
	
	LinkCtx *Main=new LinkCtx;

	GetHttp().SetCloseHandler(client,CloseHandler, Main);
	GetHttp().SetHttpRequestHandler(client, ASYNC_HTTP_REQUEST_HANDLE, Main);

	LinkCtx *Sub = create_link_ctx(client, Main);
	if (NULL == Sub)
	{
		//GetHttp().Close(client);
		send_error(handle, "502", "Bad Gateway");
		return;
	}
}

void print_help()
{
	printf("usg:-l 监听地址 -s 目标服务器 -pub_key 公钥 -pri_key 私钥 -timeout 连接超时间隔 \n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("timeout", 60 * 1000);
	cmd.InitCmdLineParams("l", "http://:8080/");
	cmd.InitCmdLineParams("s", "https://github.com/");
#endif

	if (!cmd.Parser(argc, argv) || cmd.HasParam("h") || cmd.HasParam("help"))
	{
		print_help();
		return -1;
	}

	conf.Listen = cmd.GetCmdLineParams("l", "http://:8080/");
	conf.Srv = cmd.GetCmdLineParams("s", "");
	conf.pub_key = cmd.GetCmdLineParams("pub_key", "cert.pem");
	conf.pri_key = cmd.GetCmdLineParams("pri_key", "key.pem");
	conf.timeout = cmd.GetCmdLineParams("timeout", -1);

	if (conf.Listen.empty() || conf.Srv.empty())
	{
		print_help();
		return -1;
	}

	sim::AsyncHandle serv = GetHttp().CreateSession();
	GetHttp().SetAcceptHandler(serv, AcceptHandler, NULL);
	if (false == GetHttp().Listen(serv, conf.Listen.c_str(), conf.pub_key.c_str(), conf.pri_key.c_str()))
	{
		printf("Listent %s fail,please check params.\n", conf.Listen.c_str());
		return -1;
	}
	sim::GlobalPoll<sim::AsyncHttp, MY_THREAD_NUM>::Wait();
	return 0;
}