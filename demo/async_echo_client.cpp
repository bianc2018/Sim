/*
	异步连接回复测试客户端
*/
#include "Async.hpp"
#include "CmdLineParser.hpp"
#include "TaskPool.hpp"

#include <string>
#include <ctime>
struct EchoClientContext
{
	sim::SimAsync async;
	sim::TaskPool*pool;//线程池
	//参数
	//服务器地址
	std::string ip;
	int port;
	std::string echomsg;
	unsigned int try_num;
	//活跃数限制
	unsigned int active_limit;
	//线程数量
	unsigned int thread_num;
	
	bool ssl;
	//统计信息
	unsigned int done_num;
	unsigned int fail_num;
	unsigned int active_num;
	EchoClientContext() 
		:port(80), try_num(1), done_num(0),fail_num(0), active_num(0), thread_num(1),
		active_limit(0), pool(NULL), ssl(false)
	{

	}
	~EchoClientContext()
	{
		if (pool)
		{
			pool->WaitAllDone(100);
			delete pool;
		}
	}
};
EchoClientContext ctx;

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	//printf("%d accept %d\n", handle, client);
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	SIM_LINFO(handle << " ConnectHandler th " << sim::Thread::GetThisThreadId());
	if (ctx.echomsg.empty())
	{
		++ctx.done_num;
		--ctx.active_num;
		ctx.async.Close(handle);
	}
	else
	{
		ctx.async.Send(handle, ctx.echomsg.c_str(), ctx.echomsg.size());
	}
}
void RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	if (std::string(buff, buff_len) == ctx.echomsg)
	{
		++ctx.done_num;
	}
	else
	{
		++ctx.fail_num;
	}
	--ctx.active_num;
	ctx.async.Close(handle);
}
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	SIM_LERROR("close " << handle << " error " << error << " reason " << reason);
	++ctx.fail_num;
	--ctx.active_num;
	ctx.async.Close(handle);
}

void print_help()
{
	/*
	std::string ip;
	int port;
	std::string echomsg;
	unsigned int try_num;
	*/
	printf("usg:-i 127.0.0.1 -p 8080 -e asdadsda -n 1000 -l 1000 -tn 1 -ssl\n");
}
void done()
{
	//并发线程数
	if (ctx.active_limit >0&&ctx.active_num>=ctx.active_limit)
		return;
	
	if (ctx.try_num <= 0)
		return;

	--ctx.try_num;
	sim::AsyncHandle handle = ctx.async.CreateTcpHandle();
	if (SOCK_FAILURE == handle)
	{
		++ctx.fail_num;
		return;
	}
	ctx.async.SetAcceptHandler(handle, AcceptHandler, NULL);
	ctx.async.SetConnectHandler(handle, ConnectHandler, NULL);
	ctx.async.SetRecvDataHandler(handle, RecvDataHandler, NULL);
	ctx.async.SetCloseHandler(handle, CloseHandler, NULL);
	if (ctx.ssl)
	{
		ctx.async.ConvertToSSL(handle, false, true);
	}
	int ret = ctx.async.AddTcpConnect(handle, ctx.ip.c_str(), ctx.port);
	if (ret != SOCK_SUCCESS)
	{
		++ctx.fail_num;
		return;
	}
	++ctx.active_num;
}
void* APoll(void* lpParam)
{
	unsigned int  all = (long)(lpParam);
	while (all > (ctx.done_num + ctx.fail_num))
		ctx.async.Poll(100);
	return NULL;
}
int main(int argc, char *argv[])
{
	SIM_LOG_CONSOLE(sim::LInfo);
	SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, ".", "async_echo_client","txt");

	sim::CmdLineParser cmd(argc, argv);
	ctx.ip = cmd.GetCmdLineParams("i", "127.0.0.1");
	ctx.port = cmd.GetCmdLineParams("p", 8080);
	ctx.echomsg = cmd.GetCmdLineParams("e", "");
	ctx.try_num = cmd.GetCmdLineParams("n", 10000);
	ctx.active_limit = cmd.GetCmdLineParams("l", 1000);
	if (cmd.HasParam("ssl"))
	{
#ifdef SIM_USE_OPENSSL
		ctx.ssl = true;
#else
		SIM_LWARN("Not supoort -ssl");
#endif // SIM_USE_OPENSSL

		
	}
	//检查参数
	if (cmd.HasParam("h")|| cmd.HasParam("help") || ctx.ip.empty()|| ctx.port<=0|| ctx.try_num<=0)
	{
		print_help();
		return -1;
	}

	ctx.thread_num = cmd.GetCmdLineParams("tn", 8);
	if (ctx.thread_num <= 0)
		ctx.thread_num = 1;
	ctx.pool = new sim::TaskPool(ctx.thread_num);
	for (int i = 0; i < ctx.thread_num; ++i)
		ctx.pool->Post(APoll, (void*)ctx.try_num,NULL);

	time_t t1 = time(NULL);
	unsigned all = ctx.try_num;
	while (all > (ctx.done_num+ctx.fail_num))
	{
		done();
		SIM_LINFO("poll done "<< ctx.done_num<<" fail "<< ctx.fail_num<<" active "<< ctx.active_num<<" no try "<<ctx.try_num);
	}
	time_t us_s = time(NULL) - t1;
	SIM_LINFO("poll done "<<ctx.done_num + ctx.fail_num<<" use "<< us_s<<" s "<<double(us_s)/ double(ctx.done_num + ctx.fail_num));
	getchar();
	return 0;
}