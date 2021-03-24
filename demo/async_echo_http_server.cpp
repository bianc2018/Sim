/*
	异步连接回复测试服务端端
*/
#include "Async.hpp"
#include "CmdLineParser.hpp"
#include "TaskPool.hpp"

#include <string>
#include <ctime>
struct EchoServerContext
{
	sim::SimAsync async;
	sim::TaskPool*pool;//线程池
	//参数
	//服务器地址
	std::string ip;
	int port;
	sim::Mutex active_lock;
	unsigned int active_num;
	unsigned int done_num;
	//活跃数限制
	unsigned int active_limit;
	//线程数量
	unsigned int thread_num;
	bool exit_flag;
	EchoServerContext()
		:port(80), active_num(0), thread_num(1), done_num(0),
		active_limit(0), pool(NULL), exit_flag(false)
	{

	}
	~EchoServerContext()
	{
		exit_flag = true;
		if (pool)
		{
			pool->WaitAllDone(100);
			delete pool;
		}
	}
};
EchoServerContext ctx;

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	SIM_LINFO(handle << " accept " << client);
	sim::AutoMutex lk(ctx.active_lock);
	++ctx.active_num;
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	SIM_LINFO(handle << " ConnectHandler th " << sim::Thread::GetThisThreadId());

}
void RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	static std::string response = "HTTP/1.0 200 OK\r\n"
							"Server: Sim\r\n"
							"Connection: Close\r\n"
							"Content-Length: 0\r\n"
							"\r\n\0";
	ctx.async.Send(handle, response.c_str(), response.size());
}

void SendCompleteHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	//sim::AutoMutex lk(ctx.active_lock);
	SIM_LERROR("send complete " << handle );
	//--ctx.active_num;
	ctx.async.Close(handle);
}
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	sim::AutoMutex lk(ctx.active_lock);
	SIM_LERROR("close " << handle << " error " << error << " reason " << reason);
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
	printf("usg:-i 127.0.0.1 -p 8080 -l 1000 -tn 1\n");
}
void done()
{
	sim::AsyncHandle handle = ctx.async.CreateTcpHandle();
	ctx.async.SetAcceptHandler(handle, AcceptHandler, NULL);
	ctx.async.SetConnectHandler(handle, ConnectHandler, NULL);
	ctx.async.SetRecvDataHandler(handle, RecvDataHandler, NULL);
	ctx.async.SetCloseHandler(handle, CloseHandler, NULL);
	ctx.async.SetSendCompleteHandler(handle, SendCompleteHandler, NULL);
	if (ctx.ip.empty())
		ctx.async.AddTcpServer(handle, NULL, ctx.port,128);
	else
		ctx.async.AddTcpServer(handle, ctx.ip.c_str(), ctx.port,128);
}
void* APoll(void* lpParam)
{
	while (!ctx.exit_flag)
	{
		ctx.async.Poll(10000);
		SIM_LINFO("poll done " << ctx.done_num << " active " << ctx.active_num);
	}
	return NULL;
}
int main(int argc, char *argv[])
{
	SIM_LOG_CONSOLE(sim::LInfo);
	SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, "./debug_log/", "async_echo_server", "txt");

	sim::CmdLineParser cmd(argc, argv);
	ctx.ip = cmd.GetCmdLineParams("i", "");
	ctx.port = cmd.GetCmdLineParams("p", 8080);
	ctx.active_limit = cmd.GetCmdLineParams("l", 10000);

	//检查参数
	if (cmd.HasParam("h") || cmd.HasParam("help") || ctx.port <= 0)
	{
		print_help();
		return -1;
	}

	ctx.thread_num = cmd.GetCmdLineParams("tn",8);
	if (ctx.thread_num <= 0)
		ctx.thread_num = 1;
	ctx.pool = new sim::TaskPool(ctx.thread_num);
	for (int i = 0; i < ctx.thread_num; ++i)
		ctx.pool->Post(APoll, NULL, NULL);

	done();
	getchar();
	return 0;
}