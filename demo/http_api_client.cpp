#include <stdio.h>
#include "HttpApi.hpp"
#include "CmdLineParser.hpp"
sim::CmdLineParser cmd;
sim::HttpClient cli;

void HTTP_CLI_HANDLER(sim::HTTP_S_STATUS status,sim::HttpResponse *response, void *pdata)
{
	if (status == sim::HttpConnect)
	{
		printf("Connect \n");
		cli.Get(cmd.GetCmdLineParams("path", "/").c_str());
	}
	else if (status == sim::HttpGetResponseComplete)
	{
		printf("Get Response %s %s body size=%u Connection %s\n",
			response->Status.c_str(), response->Reason.c_str(), response->Content.chunk.size()
		, response->Head.Get("Connection","").c_str());
		if (cmd.HasParam("print"))
		{
			printf("body :\n%s\n", response->Content.chunk.c_str());
		}
		sim::Str filename = cmd.GetCmdLineParams("save", "");
		if (!filename.empty())
		{
			FILE*f = fopen(filename.c_str(), "wb+");
			if (f == NULL)
			{
				printf("save to %s fail\n", filename.c_str());
				return;
			}
			fwrite(response->Content.chunk.c_str(), response->Content.chunk.size(), 1, f);
			fflush(f);
			fclose(f);
		}
	}
	else if (status == sim::HttpClose)
	{
		printf("close\n");
	}
}
bool GetHostByNameCallBack(const char* ip, void* pdata)
{
	cli.SetHandler(HTTP_CLI_HANDLER, NULL);
	if(cmd.HasParam("ssl"))
		cli.ConnectHttps(ip, cmd.GetCmdLineParams("port", 443));
	else
		cli.ConnectHttp(ip, cmd.GetCmdLineParams("port", 80));
	getchar();
	//
	return false;
}
void print_help()
{
	printf("usg:-host 主机域名 -port 连接端口 -path URL路径 -ssl 启用ssl -print 打印 -save filename 保存到filename -debug 打印详细日志\n");
}
int main(int argc, char* argv[])
{
#if 0
	cmd.InitCmdLineParams("host", "www.baidu.com")
		.InitCmdLineParams("ssl", "")
		//.InitCmdLineParams("debug", "")
		.InitCmdLineParams("port", 8080);
#endif
	if (!cmd.Parser(argc, argv) 
		|| cmd.HasParam("h") 
		|| cmd.HasParam("help") 
		|| !cmd.HasParam("host")
		|| cmd.GetCmdLineParams("port", 80) <= 0
		)
	{
		print_help();
		return -1;
	}
	if (cmd.HasParam("debug"))
	{
		SIM_LOG_CONSOLE(sim::LDebug);
		SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, "./debug_log/", "http_api_client", "txt");
	}

	sim::Socket::Init();

	sim::Socket::GetHostByName(cmd.GetCmdLineParams("host", "www.baidu.com").c_str(), GetHostByNameCallBack, NULL);


	getchar();
	return 0;
}