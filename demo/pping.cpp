#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include "Logger.hpp"
#include "Array.hpp"
#include "Socket.hpp"
#include "Queue.hpp"
#include "TaskPool.hpp"
struct Context;
//一个连接任务
struct PpContext
{
	std::string ip;
	int port;
	int wait_ms;
	int ret;

	Context*ctx;
};

struct Context
{
	std::string host;
	int beg_port;
	int end_port;
	int wait_ms;
	int count;

	//ip队列
	sim::Array<std::string> ips;

	//已处理队列
	sim::Mutex done_lock;
	sim::Queue<PpContext*> done;
	unsigned long do_num ;
	unsigned long do_ok ;
	unsigned long do_fail ;

	sim::TaskPool pool;

	Context()
		:pool(0), beg_port(0), end_port(65535), wait_ms(10), count(1)
		, do_num(0), do_ok(0), do_fail(0)
	{

	}
	~Context()
	{
		PpContext*p;
		while (done.PopFront(&p))
		{
			delete (p);
		}
	}
};
void* DonePp(void*content)
{
	PpContext*pc = (PpContext*)content;
	sim::Socket s(sim::TCP);
	pc->ret = s.ConnectTimeOut(pc->ip.c_str(), pc->port, pc->wait_ms);
	s.Close();
	SIM_LINFO("ConnectTimeOut " << pc->ip << ":" << pc->port << " wait " << pc->wait_ms << " ret " << pc->ret);
	sim::AutoMutex lock(pc->ctx->done_lock);
	if (pc->ret != 0)
	{
		++pc->ctx->do_fail;
		delete pc;
	}
	else
	{
		++pc->ctx->do_ok;
		if (pc->ctx->count == 1)
			pc->ctx->done.PushBack(pc);
		else
			delete pc;
	}
	return NULL;
}
bool GetHostByNameCallBack(const char* ip, void* pdata)
{
	Context*ctx = (Context*)pdata;
	SIM_LINFO("GetHostByName:" << ip);
	ctx->ips.Assign(ip);
	return true;
}

template<typename T1, typename T2>
T2 SimTo(const T1 &t)
{
	std::stringstream oss;
	oss << t;
	T2 t2;
	oss >> t2;
	return t2;
}
int string_to_int(const std::string&num)
{
	return SimTo<std::string, int>(num);
}



void print_help()
{
	printf("use as:pping  host bport eport  waitms count\n");
}
//pping  host bport eport  waitms count
int main(int argc, char* argv[])
{
	SIM_LOG_CONSOLE(sim::LDebug);
	Context ctx;
#if 1
	if (argc == 1)
	{
		ctx.host = "49.234.220.213";
		ctx.beg_port = 8080;
		ctx.end_port = 8080;
		ctx.wait_ms = 1000;
		ctx.count = 100000;
	}
	else
#endif
		if (argc <= 3)
		{
			print_help();
			return -1;
		}

	if (argc > 3)
	{
		ctx.host = argv[1];
		ctx.beg_port = string_to_int(argv[2]);
		ctx.end_port = string_to_int(argv[3]);
		if (ctx.end_port < ctx.beg_port)
		{
			SIM_LERROR("ctx.end_port " << ctx.end_port << " < ctx.beg_port " << ctx.beg_port);
			return -1;
		}
	}
	if (argc > 4)
	{
		ctx.wait_ms = string_to_int(argv[4]);
	}
	if (argc > 5)
	{
		ctx.count = string_to_int(argv[5]);
	}

	SIM_LINFO("Pp: host " << ctx.host
		<< " port [" << ctx.beg_port
		<< "," << ctx.end_port << ")"
		<< " wait " << ctx.wait_ms << " ms"
		<< " try count " << ctx.count << " one port");

	sim::Socket s(sim::TCP);
	s.GetHostByName(ctx.host.c_str(), GetHostByNameCallBack, &ctx);

	int ip_size = ctx.ips.Size();
	if (ip_size == 0)
	{
		SIM_LERROR("GetHostByName " << ctx.host << " fail");
		return -1;
	}

	for (int i = 0; i < ip_size; ++i)
	{
		for (int p = ctx.beg_port; p <= ctx.end_port; ++p)
		{
			for (int c = 0; c < ctx.count; ++c)
			{
				PpContext* cc = new PpContext();
				cc->ctx = &ctx;
				cc->ip = ctx.ips[i];
				cc->port = p;
				cc->ret = 0;
				cc->wait_ms = ctx.wait_ms;

				++ctx.do_num;
				ctx.pool.Post(DonePp, cc, NULL);
				SIM_LINFO("Post:" << ctx.do_num << " ok " << ctx.do_ok << " fail " << ctx.do_fail);
			}
		}
	}

	while (ctx.pool.GetTaskNum()||(ctx.do_num>ctx.do_fail+ctx.do_ok))
	{
		ThreadSleep(10);
		SIM_LINFO("Post:" << ctx.do_num << " ok " << ctx.do_ok << " fail " << ctx.do_fail);
	}
	SIM_LINFO("Post:" << ctx.do_num << " ok " << ctx.do_ok << " fail " << ctx.do_fail);
	getchar();
	if (ctx.count == 1)
	{
		std::string temp;
		PpContext*p;
		while (ctx.done.PopFront(&p))
		{
			if (temp.empty())
				temp += p->ip+":"+SimTo<int,std::string>(p->port)+"\n";
			else
				temp += std::string(",") + p->ip + ":" + SimTo<int, std::string>(p->port) + "\n";
			delete (p);
		}
		SIM_LINFO("Post:ports {" << temp << "}");
		return 0;
	}
}