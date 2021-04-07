#include "HttpApi.hpp"
#include "CmdLineParser.hpp"

#define SIM_DEFUALT_PORT 8080

sim::CmdLineParser cmd;
sim::HttpServer Srv;

bool HTTP_SERV_HANDLER(sim::HttpRequest*request, sim::HttpResponse *response, void *pdata)
{
	printf("recv request path:%s\n", request->Url.c_str());
	response->Body = "Hello world";
	return true;
}
void print_help()
{
	
	printf("usg:-i ip地址 -p 监听端口 -ssl 启用ssl -pub_key 公钥 -pri_key 私钥\n");
}
int main(int argc, char* argv[])
{
	if (!cmd.Parser(argc, argv)|| cmd.HasParam("h") || cmd.HasParam("help") || cmd.GetCmdLineParams("p", SIM_DEFUALT_PORT) <= 0)
	{
		print_help();
		return -1;
	}
	Srv.SetHandler(HTTP_SERV_HANDLER, &Srv);

	char *c_ip = NULL;
	sim::Str ip = cmd.GetCmdLineParams("i", "");
	if (!ip.empty())
		c_ip = (char*)ip.c_str();
	printf("Listen %d SSL %s\n", cmd.GetCmdLineParams("p", SIM_DEFUALT_PORT), cmd.HasParam("ssl")?"ON":"OFF");
	if (!cmd.HasParam("ssl"))
	{
		Srv.ListenHttp(cmd.GetCmdLineParams("p", SIM_DEFUALT_PORT), c_ip);
	}
	else
	{
		Srv.ListenHttps(cmd.GetCmdLineParams("p", SIM_DEFUALT_PORT),
			cmd.GetCmdLineParams("pub_key","cert.pem").c_str(),
			cmd.GetCmdLineParams("pri_key", "key.pem").c_str(),c_ip);
	}
	
	getchar();
	return 0;
}
