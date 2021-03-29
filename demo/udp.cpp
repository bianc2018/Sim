/*
	异步连接回复测试服务端端
*/

//#define SIM_NO_LOGGER
#include "Logger.hpp"
#include "CmdLineParser.hpp"
#include "Socket.hpp"
#include "TaskPool.hpp"

#include <string>
#include <ctime>
struct EchoServerContext
{
	sim::TaskPool*pool;//线程池
	//参数
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

void print_help()
{
	/*
	std::string ip;
	int port;
	std::string echomsg;
	unsigned int try_num;
	*/
	printf("usg: -p 8080 \n");
}

void* APoll(void* lpParam)
{
	sim::Socket udp(sim::UDP);
	int ret = udp.Bind(NULL, ctx.port);
	if (ret != SOCK_SUCCESS)
	{
		SIM_LERROR("udp.Bind error ret=" << ret << " port=" << ctx.port);
		return NULL;
	}
	const int buff_size = 4 * 1024 * 1024;
	char *buff = new char[buff_size];
	const unsigned int len = 128;
	char ip[len] = "127.0.0.1";
	unsigned short port = 0;
	while (true)
	{
		//int readlen = udp.Recv(buff, buff_size, -1);
		int readlen = udp.Recvfrom(buff, buff_size, ip, len,&port);
		if (readlen > 0)
		{
			SIM_LINFO("Recv ["<<ip<<":"<<port<<" "<<readlen<<"]:" << std::string(buff, readlen));

		}
		else
		{
			SIM_LERROR("udp.Recv error ret=" << readlen << " port=" << ctx.port << "  WSAGetLastError()=" << WSAGetLastError());
		}
	}
	delete[]buff;
	return NULL;
}
int main(int argc, char *argv[])
{
	SIM_LOG_CONSOLE(sim::LInfo);
	SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, "./debug_log/", "async_echo_server", "txt");

	sim::CmdLineParser cmd(argc, argv);
	ctx.port = cmd.GetCmdLineParams("p", 8080);
	ctx.active_limit = cmd.GetCmdLineParams("l", 10000);

	//检查参数
	if (cmd.HasParam("h") || cmd.HasParam("help") || ctx.port <= 0)
	{
		print_help();
		return -1;
	}

	ctx.thread_num = 1;// cmd.GetCmdLineParams("tn", 1);
	if (ctx.thread_num <= 0)
		ctx.thread_num = 1;
	ctx.pool = new sim::TaskPool(ctx.thread_num);
	for (int i = 0; i < ctx.thread_num; ++i)
		ctx.pool->Post(APoll, NULL, NULL);

	getchar();
	return 0;
}