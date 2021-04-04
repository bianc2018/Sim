#ifdef SIM_USE_OPENSSL
#include "SSLCtx.hpp"
#include "Socket.hpp"
#include "CmdLineParser.hpp"
#pragma comment (lib, "libcrypto.lib")  //加载 
#pragma comment (lib, "libssl.lib")  //加载 
int port = 8080;
void print_help()
{
	/*
	std::string ip;
	int port;
	std::string echomsg;
	unsigned int try_num;
	*/
	printf("usg:-p 8080\n");
}
int main(int argc, char*argv[])
{
	sim::CmdLineParser cmd(argc, argv);
	port = cmd.GetCmdLineParams("p", port);
	//检查参数
	if (cmd.HasParam("h") || cmd.HasParam("help") || port <= 0 )
	{
		print_help();
		return -1;
	}

	sim::Socket s(sim::TCP);

	sim::SSLCtx ctx(SSLv23_server_method());
	ctx.SetKeyFile("cert.pem", "key.pem");
	s.Bind(NULL, port);
	s.Listen(1024);
	printf("Listen:%d\n", port);

	sim::Socket c;

	const int recv_buff_size = 1024 * 4;
	char recv_buff[recv_buff_size];
	const int send_buff_size = 1024 * 4;
	char send_buff[send_buff_size];
	while (true)
	{
		c.Close();
		s.Accept(c);

		sim::SSLSession *ssl = ctx.NewSession(c.GetSocket());
		if (!(ssl&&ssl->HandShake()))
		{
			printf("HandShake fail");
			continue;
		}

		int recv = c.Recv(recv_buff, recv_buff_size);
		recv = ssl->Decrypt(recv_buff, recv, recv_buff, recv_buff_size);
		recv_buff[recv] = '\0';
		printf("recv:%d %s\n", recv, recv_buff);

		int send = ssl->Encrypt(recv_buff, recv, send_buff, send_buff_size);
		printf("echo %d\n", send);
		c.Send(send_buff, send);
		ctx.DelSession(ssl);
	}
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
