#include "IptClient.hpp"
#include "IptServer.hpp"

#include "Thread.hpp"
#include "CmdLineParser.hpp"
#include <iostream>

sim::CmdLineParser cmd;
void print_help()
{
	printf("usg:服务端 -t s -p 端口\n");
	printf("usg:客户端 -t c -p 本地监听端口 -local 本地id -srv_ip 服务端ip -srv_port 服务端端口 -peer 对端id\n");
}
void RunServer();

ThRet ReadClientInputThreadProc(LPVOID lpParam);

void RunClient();

int main(int argc, char* argv[])
{
#if 0
	cmd.InitCmdLineParams("t", "s")
	.InitCmdLineParams("p", "9999");
#else
	// -c -p 9998 -local 1 -srv_ip 127.0.0.1 -srv_port 9999 -peer 2
	cmd.InitCmdLineParams("t", "c")
		.InitCmdLineParams("p", "19998")
		.InitCmdLineParams("local", "1")
		.InitCmdLineParams("srv_ip", "127.0.0.1")
		.InitCmdLineParams("srv_port", "9999")
		.InitCmdLineParams("peer", "2");
#endif

	SIM_LOG_CONSOLE(sim::LDebug);
	cmd.Parser(argc, argv);
	if (cmd.HasParam("h") || cmd.HasParam("help") )
	{
		print_help();
		return -1;
	}
	if (cmd.GetCmdLineParams("t","") == "s")
	{
		RunServer();
	}
	else if (cmd.GetCmdLineParams("t", "") == "c")
	{
		RunClient();
	}
	else
	{
		print_help();
		return -1;
	}
    return 0;
}

void RunServer()
{
	ipt::CIpcServer cServer(cmd.GetCmdLineParams<int>("p", 9999));
	if (false == cServer.RunAccept())
	{
		SIM_LERROR("RunAccept error");
	}
}

ThRet ReadClientInputThreadProc(LPVOID lpParam)
{
	ipt::CIpcClient* pcClient = (ipt::CIpcClient*)lpParam;
	if (pcClient)
	{
		while (true)
		{
			std::string strMsg;
			std::cin >> strMsg;
			pcClient->SendMsg(strMsg);
		}
		
		
	}
	return ThRet(); 
}

void RunClient()
{
	UdpAddr stListenAddr;
	stListenAddr.strIp = "0.0.0.0";
	stListenAddr.nPort = cmd.GetCmdLineParams<int>("p", 9999);
	stListenAddr.strCliID = cmd.GetCmdLineParams<std::string>("local", "cli1");

	UdpAddr stSrvAddr;
	stSrvAddr.strIp = cmd.GetCmdLineParams<std::string>("srv_ip", "");
	stSrvAddr.nPort = cmd.GetCmdLineParams<int>("srv_port", 9999);

	ipt::CIpcClient cClient(stListenAddr, stSrvAddr);
	if (false == cClient.Connect(cmd.GetCmdLineParams<std::string>("peer", "")))
	{
		SIM_LERROR("Connect error peer:"<< cmd.GetCmdLineParams<std::string>("peer", ""));
		return;
	}
	sim::Thread thRead(ReadClientInputThreadProc, &cClient);

	//读数据并且返回
	while (true)
	{
		std::string strMsg;
		cClient.RecvMsg(strMsg);
		if (strMsg.empty())
			continue;
		printf("Peer:%s\n", strMsg.c_str());
	}
}
