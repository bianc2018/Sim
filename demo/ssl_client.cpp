/*
	异步连接回复测试服务端端
*/
#ifdef SIM_USE_OPENSSL
//#define SIM_NO_LOGGER
#include <string>
#include "CmdLineParser.hpp"
#include "Logger.hpp"
#include "SSLCtx.hpp"
#include "Socket.hpp"
#pragma comment (lib, "libcrypto.lib")  //加载 
#pragma comment (lib, "libssl.lib")  //加载 
std::string ip;
char *msg=NULL;
int size = 0;
int port;

void print_help()
{
	/*
	std::string ip;
	int port;
	std::string echomsg;
	unsigned int try_num;
	*/
	printf("usg:-i 127.0.0.1 -p 8080 -s 124\n");
}
void done()
{
	sim::Socket s(sim::TCP);

	sim::SSLCtx ctx(SSLv23_client_method());

	int ret = s.Connect(ip.c_str(), port);
	sim::SSLSession*ssl=ctx.NewSession(s.GetSocket());
	if (!(ssl&&ssl->HandShake()))
	{
		printf("HandShake fail");
		return;
	}
	char buff[1024] = { 0 };

	if (msg)
	{
		ssl->InEncrypt(msg, size);
		//printf("send:%s\n", msg);
		while (true)
		{
			int ret = ssl->OutEncrypt(buff, 1024);
			if (ret == -1)
				break;
			s.Send(buff, ret);
		}
	}
	while (true)
	{
		int len = s.Recv(buff, 1024,10000);
		if (len > 0)
		{
			 ssl->InDecrypt(buff, len);
			 len = ssl->OutDecrypt(buff, 1023);
			if (len <= 0)
			{
				continue;
			}
			buff[len] = '\0';
			printf("recv:%s\n", buff);
		}
		else
		{
			printf("recv %d\n", len);
			break;
		}
		
	}
	ctx.DelSession(ssl);

}
int main(int argc, char *argv[])
{
	SIM_LOG_CONSOLE(sim::LInfo);
	//SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, "./debug_log/", "async_echo_server", "txt");

	sim::CmdLineParser cmd(argc, argv);
	ip = cmd.GetCmdLineParams("i", "49.234.220.213");
	port = cmd.GetCmdLineParams("p", 8080);
	std::string  t= cmd.GetCmdLineParams("s", std::string(1024 * 10,'c'));
	
	//检查参数
	if (cmd.HasParam("h") || cmd.HasParam("help") || port <= 0 || ip.empty())
	{
		print_help();
		return -1;
	}
	if (t.size() > 0)
	{
		size = t.size();
		msg = new char[t.size()+1];
		memset(msg, 0, t.size() + 1);
		strcpy(msg, t.c_str());

	}
	done();
	getchar();
	return 0;
}

#else
#include <stdio.h>
int main(int argc, char*argv[])
{
	printf("not def SIM_USE_OPENSSL\n");
	return -1;
}
#endif