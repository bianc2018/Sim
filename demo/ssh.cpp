#include "AsyncSSH.hpp"
#include "Async.hpp"
#include "GlobalPoll.hpp"
#define MY_THREAD_NUM 1
#include <string>
sim::SshTransport ssh_parser(sim::SshClient);
sim::AsyncHandle handle = 0;
sim::SimAsync& Get()
{
	return sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Get();
}
void SSH_TRANS_HANDLER(sim::SshTransport*parser,
	std::uint8_t message_code,
	const char*payload_data, std::uint32_t payload_data_len, void*pdata)
{
	if (SSH_MSG_VERSION == message_code)
	{
		printf("version:%s\n", sim::Str(payload_data, payload_data_len).c_str());
		sim::SSHVersion ver;
		if (!parser->ParserVersion(payload_data, payload_data_len, ver))
		{
			printf("ParserVersion falt\n");
			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
			return;
		}
		if (!parser->VersionExchange(ver))
		{
			printf("VersionExchange falt\n");
			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
			return;
		}
		sim::Str kex_init = parser->PrintKexInit();
		if (kex_init.empty())
		{
			printf("PrintKexInit falt\n");
			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
			return;
		}
		Get().Send(handle, kex_init.c_str(), kex_init.size());
	}
	else if (SSH_MSG_KEXINIT == message_code)
	{
		sim::SSHKexInit kex_init;
		if (!parser->ParserKexInit(payload_data, payload_data_len, kex_init))
		{
			printf("ParserKexInit falt\n");
			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
			return;
		}
		if (!parser->KexInit(kex_init))
		{
			printf("KexInit falt\n");
			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
			return;
		}
	}
	else
	{
		printf("cannt find message_code %d\n", message_code);
	}
}
void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	ssh_parser.SetHandler(SSH_TRANS_HANDLER, NULL);
	sim::Str ver = ssh_parser.PrintProtocolVersion();
	if (ver.empty())
	{
		printf("PrintProtocolVersion falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	Get().Send(handle, ver.c_str(), ver.size());
	printf("%d connect \n", handle);
}
void RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	ssh_parser.Parser(buff, buff_len);
}
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	SIM_LERROR("close " << handle << " error " << error << " reason " << reason);
}
int main(int argc, char*argv[])
{
	sim::SimAsync &async= Get();
	/*sim::AsyncHandle */handle = async.CreateTcpHandle();
	/*async.ConvertToSSL(handle, true, true);
	async.SetSSLKeyFile(handle, "cert.pem", "key.pem");*/
	async.SetAcceptHandler(handle, AcceptHandler, NULL);
	async.SetConnectHandler(handle, ConnectHandler, NULL);
	async.SetRecvDataHandler(handle, RecvDataHandler, NULL);
	async.SetCloseHandler(handle, CloseHandler, NULL);
//	async.AddTcpServer(handle, NULL, 8080);

	if (async.AddTcpConnect(handle, "49.234.220.213", 22) != SOCK_SUCCESS)
		return -1;
	sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Wait();

	return 0;

}