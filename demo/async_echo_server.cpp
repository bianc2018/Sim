/*
	�첽���ӻظ����Է���˶�
*/
#include "Async.hpp"
#include "CmdLineParser.hpp"
#include "TaskPool.hpp"

#include <string>
#include <ctime>
struct EchoServerContext
{
	sim::SimAsync async;
	sim::TaskPool*pool;//�̳߳�
	//����
	//��������ַ
	std::string ip;
	int port;
	unsigned int active_num;
	unsigned int done_num;
	//��Ծ������
	unsigned int active_limit;
	//�߳�����
	unsigned int thread_num;
	bool exit_flag;
	EchoServerContext()
		:port(80),active_num(0), thread_num(1), done_num(0),
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
	printf("%d accept %d\n", handle, client);
	++ctx.active_num;
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	printf("%d ConnectHandler th %u\n", handle, sim::Thread::GetThisThreadId());
	
}
void RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	ctx.async.Send(handle, buff, buff_len);
}
void ErrorHandler(sim::AsyncHandle handle, int error, void*data)
{
	printf("%d error %d\n", handle, error);
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
	ctx.async.SetErrorHandler(handle, ErrorHandler, NULL);
	if (ctx.ip.empty())
		ctx.async.AddTcpServer(handle, NULL, ctx.port);
	else
		ctx.async.AddTcpServer(handle, ctx.ip.c_str(), ctx.port);
}
void* APoll(void* lpParam)
{
	while (!ctx.exit_flag)
	{
		ctx.async.Poll(100);
		printf("poll done %u active %u \n", ctx.done_num, ctx.active_num);
	}
	return NULL;
}
int main(int argc, char *argv[])
{
	//SIM_LOG_CONSOLE(sim::LDebug);

	sim::CmdLineParser cmd(argc, argv);
	ctx.ip = cmd.GetCmdLineParams("i", "");
	ctx.port = cmd.GetCmdLineParams("p", 8080);
	ctx.active_limit = cmd.GetCmdLineParams("l", 1000);

	//������
	if (cmd.HasParam("h") || cmd.HasParam("help")  || ctx.port <= 0 )
	{
		print_help();
		return -1;
	}

	ctx.thread_num = cmd.GetCmdLineParams("tn", 8);
	if (ctx.thread_num <= 0)
		ctx.thread_num = 1;
	ctx.pool = new sim::TaskPool(ctx.thread_num);
	for (int i = 0; i < ctx.thread_num; ++i)
		ctx.pool->Post(APoll, NULL, NULL);

	done();
	getchar();
	return 0;
}