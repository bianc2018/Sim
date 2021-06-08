#define SIM_PARSER_MULTI_THREAD 1
#include "SSHv2.hpp"
#include "Async.hpp"
#include "GlobalPoll.hpp"
#define MY_THREAD_NUM 1
#include <string>
sim::SshAuthentication ssh_parser(sim::SshClient);
sim::AsyncHandle handle = 0;
sim::SimAsync& Get()
{
	return sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Get();
}
void SSH_TRANS_HANDLER_SSH_MSG_VERSION(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
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

void SSH_TRANS_HANDLER_SSH_MSG_KEXINIT(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("kex_init\n");
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
	sim::Str kex_dh_init = parser->PrintKexDHInit();
	if (kex_dh_init.empty())
	{
		printf("PrintKexInit falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	Get().Send(handle, kex_dh_init.c_str(), kex_dh_init.size());
}

void SSH_TRANS_HANDLER_SSH_MSG_KEXDH_REPLY(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("kex_dh reply\n");
	sim::SSHKexDHReply reply;
	if (!parser->ParserKexDHReply(payload_data, payload_data_len, reply))
	{
		printf("ParserKexDHReply falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	if (!parser->KeyExchange(reply))
	{
		printf("KeyExchange falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	sim::Str newkey = parser->PrintNewKeys();
	if (newkey.empty())
	{
		printf("PrintNewKeys falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	Get().Send(handle, newkey.c_str(), newkey.size());
}

void SSH_TRANS_HANDLER_SSH_MSG_NEWKEYS(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("new key\n");
	if (!parser->NewKeys())
	{
		printf("NewKeys falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}

	sim::Str req = parser->PrintServiceRequest("ssh-userauth");
	if (req.empty())
	{
		printf("PrintServiceRequest falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	Get().Send(handle, req.c_str(), req.size());
}

//SSH_MSG_SERVICE_ACCEPT
void SSH_TRANS_HANDLER_SSH_MSG_SERVICE_ACCEPT(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	sim::Str service;
	parser->ParserServiceAccept(payload_data, payload_data_len, service);
	printf("accept %s\n", service.c_str());

	sim::SshAuthentication*auth = (sim::SshAuthentication*)parser;
	sim::SshAuthRequest auth_req;
	//ssh-connection
	auth_req.user_name = "root";
	auth_req.service_name = "ssh-connection";
	auth_req.method = SSH_AUTH_PASSWORD;
	auth_req.method_fields.password.flag = false;
	auth_req.method_fields.password.password = "q1051576073@";
	sim::Str req = auth->PrintAuthRequset(auth_req);
	if (req.empty())
	{
		printf("PrintServiceRequest falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	Get().Send(handle, req.c_str(), req.size());
}

/*
#define SSH_MSG_USERAUTH_FAILURE			51
#define SSH_MSG_USERAUTH_SUCCESS			52
#define SSH_MSG_USERAUTH_BANNER				53
*/
void SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_FAILURE(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("SSH_MSG_USERAUTH_FAILURE \n");
}
void SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_SUCCESS(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("SSH_MSG_USERAUTH_SUCCESS \n");
}
void SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_BANNER(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("SSH_MSG_USERAUTH_BANNER \n");
}

void SSH_TRANS_HANDLER_SSH_MSG_ERROR(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{

}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	ssh_parser.SetHandler(SSH_MSG_VERSION,SSH_TRANS_HANDLER_SSH_MSG_VERSION, NULL);
	ssh_parser.SetHandler(SSH_MSG_KEXINIT, SSH_TRANS_HANDLER_SSH_MSG_KEXINIT, NULL);
	ssh_parser.SetHandler(SSH_MSG_KEXDH_REPLY, SSH_TRANS_HANDLER_SSH_MSG_KEXDH_REPLY, NULL);
	ssh_parser.SetHandler(SSH_MSG_NEWKEYS, SSH_TRANS_HANDLER_SSH_MSG_NEWKEYS, NULL);
	ssh_parser.SetHandler(SSH_MSG_SERVICE_ACCEPT, SSH_TRANS_HANDLER_SSH_MSG_SERVICE_ACCEPT, NULL);
	ssh_parser.SetHandler(SSH_MSG_USERAUTH_FAILURE, SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_FAILURE, NULL);
	ssh_parser.SetHandler(SSH_MSG_USERAUTH_SUCCESS, SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_SUCCESS, NULL);
	ssh_parser.SetHandler(SSH_MSG_USERAUTH_BANNER, SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_BANNER, NULL);
	ssh_parser.SetHandler(SSH_MSG_ERR, SSH_TRANS_HANDLER_SSH_MSG_ERROR, NULL);

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
	SSL_library_init();
	ssh_parser.GeneratePriKey(sim::SshPublicKeyType::SshDsa, "dsa.pri");
	/*ssh_parser.LoadPriKey(sim::SshRsa, "./ssh_rsa.pem");
	ssh_parser.LoadPriKey(sim::SshDsa, "./ssh_dsa.pem");*/

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
	getchar();
	return 0;

}