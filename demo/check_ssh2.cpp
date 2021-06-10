/*
*/
#define SIM_PARSER_MULTI_THREAD 1
#include "SSHv2.hpp"
#include "Async.hpp"
#include "GlobalPoll.hpp"

#define MY_THREAD_NUM 1
sim::SimAsync& Get()
{
	return sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Get();
}

sim::SshAuthentication ssh_c(sim::SshClient);
sim::SshAuthentication ssh_s(sim::SshServer);

sim::AsyncHandle c_handle=0;
sim::AsyncHandle s_handle=0;

void SendToClient(const sim::Str&data)
{
	Get().Send(c_handle, data.c_str(), data.size());
}
void SendToServer(const sim::Str&data)
{
	Get().Send(s_handle, data.c_str(), data.size());
}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);
	if (c_handle != 0)
	{
		Get().Close(client);
		return;//²»½ÓÊÜ
	}
	c_handle = client;
	SendToClient(ssh_s.PrintProtocolVersion());
}
void c_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	ssh_c.Parser(buff, buff_len);
}
void s_RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{
	ssh_s.Parser(buff, buff_len);
}

//void c_SSH_TRANS_HANDLER(sim::SshTransport*parser,
//	std::uint8_t message_code,
//	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
//{
//	printf("c_SSH_TRANS_HANDLER message_code %d\n", message_code);
//	if (SSH_MSG_VERSION == message_code)
//	{
//		printf("c_SSH_TRANS_HANDLER version:%s\n", sim::Str(payload_data, payload_data_len).c_str());
//		sim::SSHVersion ver;
//		if (!parser->ParserVersion(payload_data, payload_data_len, ver))
//		{
//			printf("c_SSH_TRANS_HANDLER ParserVersion falt\n");
//			return;
//		}
//		if (!parser->VersionExchange(ver))
//		{
//			printf("c_SSH_TRANS_HANDLER VersionExchange falt\n");
//			return;
//		}
//
//		sim::Str pro_ver = parser->PrintProtocolVersion();
//		if (pro_ver.empty())
//		{
//			printf("c_SSH_TRANS_HANDLER PrintProtocolVersion falt\n");
//			return;
//		}
//		SendToServer(pro_ver);
//
//		sim::Str kex_init = parser->PrintKexInit();
//		if (kex_init.empty())
//		{
//			printf("c_SSH_TRANS_HANDLER PrintKexInit falt\n");
//			return;
//		}
//		SendToServer(kex_init);
//	}
//	else if (SSH_MSG_KEXINIT == message_code)
//	{
//		sim::SSHKexInit kex_init;
//		if (!parser->ParserKexInit(payload_data, payload_data_len, kex_init))
//		{
//			printf("c_SSH_TRANS_HANDLER ParserKexInit falt\n");
//			return;
//		}
//		if (!parser->KexInit(kex_init))
//		{
//			printf("c_SSH_TRANS_HANDLER KexInit falt\n");
//			return;
//		}
//		sim::Str kex_dh_init = parser->PrintKexDHInit();
//		if (kex_dh_init.empty())
//		{
//			printf("c_SSH_TRANS_HANDLER PrintKexInit falt\n");
//			return;
//		}
//		SendToServer(kex_dh_init);
//	}
//	else if (SSH_MSG_KEXDH_REPLY == message_code)
//	{
//		sim::SSHKexDHReply reply;
//		if (!parser->ParserKexDHReply(payload_data, payload_data_len, reply))
//		{
//			printf("c_SSH_TRANS_HANDLER ParserKexDHReply falt\n");
//			return;
//		}
//		if (!parser->KeyExchange(reply))
//		{
//			printf("c_SSH_TRANS_HANDLER KeyExchange falt\n");
//			return;
//		}
//		printf("c_SSH_TRANS_HANDLER KeyExchange ok\n");
//		
//	}
//	else if (SSH_MSG_NEWKEYS == message_code)
//	{
//		sim::Str newkey = parser->PrintNewKeys();
//		if (newkey.empty())
//		{
//			printf("PrintNewKeys falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//		printf("new keys\n");
//		if (!parser->NewKeys())
//		{
//			printf("NewKeys falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//		SendToServer(newkey);
//	}
//	else
//	{
//		printf("cannt find message_code %d\n", message_code);
//	}
//}
//void s_SSH_TRANS_HANDLER(sim::SshTransport*parser,
//	std::uint8_t message_code,
//	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
//{
//	printf("s_SSH_TRANS_HANDLER message_code %d\n", message_code);
//	if (SSH_MSG_VERSION == message_code)
//	{
//		printf("s_SSH_TRANS_HANDLER version:%s\n", sim::Str(payload_data, payload_data_len).c_str());
//		sim::SSHVersion ver;
//		if (!parser->ParserVersion(payload_data, payload_data_len, ver))
//		{
//			printf("s_SSH_TRANS_HANDLER ParserVersion falt\n");
//			return;
//		}
//		if (!parser->VersionExchange(ver))
//		{
//			printf("s_SSH_TRANS_HANDLER VersionExchange falt\n");
//			return;
//		}
//		return;
//	}
//	else if (SSH_MSG_KEXINIT == message_code)
//	{
//		sim::SSHKexInit kex_init;
//		if (!parser->ParserKexInit(payload_data, payload_data_len, kex_init))
//		{
//			printf("s_SSH_TRANS_HANDLER ParserKexInit falt\n");
//			return;
//		}
//		if (!parser->KexInit(kex_init))
//		{
//			printf("s_SSH_TRANS_HANDLER KexInit falt\n");
//			return;
//		}
//
//		sim::Str str_kex_init = parser->PrintKexInit();
//		if (str_kex_init.empty())
//		{
//			printf("s_SSH_TRANS_HANDLER PrintKexInit falt\n");
//			return;
//		}
//		SendToClient(str_kex_init);
//		return;
//	}
//	else if (SSH_MSG_KEXDH_INIT == message_code)
//	{
//		sim::SSHKexDHInit dh_init;
//		if (!parser->ParserKexDHInit(payload_data, payload_data_len, dh_init))
//		{
//			printf("s_SSH_TRANS_HANDLER ParserKexDHInit falt\n");
//			return;
//		}
//		sim::SSHKexDHReply dh_reply;
//		if (!parser->KeyExchange(dh_init, dh_reply))
//		{
//			printf("s_SSH_TRANS_HANDLER KeyExchange falt\n");
//			return;
//		}
//		sim::Str reply = parser->PrintKexDHReply(dh_reply);
//		if (reply.empty())
//		{
//			printf("s_SSH_TRANS_HANDLER PrintKexDHReply falt\n");
//			return;
//		}
//		SendToClient(reply);
//
//		sim::Str newkey = parser->PrintNewKeys();
//		if (newkey.empty())
//		{
//			printf("PrintNewKeys falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//		SendToClient(newkey);
//		return;
//	}
//	else if (SSH_MSG_NEWKEYS == message_code)
//	{
//		printf("new keys\n");
//		if (!parser->NewKeys())
//		{
//			printf("NewKeys falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//
//	}
//	else if (SSH_MSG_SERVICE_REQUEST == message_code)
//	{
//		sim::Str service;
//		if (!parser->ParserServiceRequest(payload_data, payload_data_len, service))
//		{
//			printf("s_SSH_TRANS_HANDLER ParserServiceRequest falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//		printf("request service %s\n", service.c_str());
//		sim::Str accept = parser->PrintServiceAccept(service);
//		if (accept.empty())
//		{
//			printf("PrintServiceAccept falt\n");
//			sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
//			return;
//		}
//		SendToClient(accept);
//	}
//	else
//	{
//		printf("cannt find message_code %d\n", message_code);
//	}
//}

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
	//Get().Send(handle, kex_init.c_str(), kex_init.size());
	SendToClient(kex_init);
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
}

void SSH_TRANS_HANDLER_SSH_MSG_KEXDH_INIT(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	printf("kex_dh init\n");
	sim::SSHKexDHInit dh_init;
	if (!parser->ParserKexDHInit(payload_data, payload_data_len, dh_init))
	{
		printf("s_SSH_TRANS_HANDLER ParserKexDHInit falt\n");
		return;
	}
	sim::SSHKexDHReply dh_reply;
	if (!parser->KeyExchange(dh_init, dh_reply))
	{
		printf("s_SSH_TRANS_HANDLER KeyExchange falt\n");
		return;
	}
	sim::Str reply = parser->PrintKexDHReply(dh_reply);
	if (reply.empty())
	{
		printf("s_SSH_TRANS_HANDLER PrintKexDHReply falt\n");
		return;
	}
	SendToClient(reply);

	sim::Str newkey = parser->PrintNewKeys();
	if (newkey.empty())
	{
		printf("PrintNewKeys falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	SendToClient(newkey);
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
	//Get().Send(handle, newkey.c_str(), newkey.size());
	SendToClient(newkey);
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
}

//#define SSH_MSG_SERVICE_REQUEST				5
void SSH_TRANS_HANDLER_SSH_MSG_SERVICE_REQUEST(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	sim::Str service;
	parser->ParserServiceRequest(payload_data, payload_data_len, service);
	printf("request %s\n", service.c_str());

	sim::Str response = parser->PrintServiceAccept(service);
	if (response.empty())
	{
		printf("PrintNewKeys falt\n");
		sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Exit();
		return;
	}
	//Get().Send(handle, newkey.c_str(), newkey.size());
	SendToClient(response);
}
//#define SSH_MSG_USERAUTH_REQUEST			50
void SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_REQUEST(sim::SshTransport*parser,
	const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
{
	sim::SshAuthRequest req;
	sim::SshAuthentication *auth = (sim::SshAuthentication*)parser;
	if (!auth->ParserAuthRequset(payload_data, payload_data_len, req))
	{
		printf("ParserAuthRequset Failed\n");
		return;
	}
	sim::Str response;
	if (req.method != SSH_AUTH_PUB_KEY)
	{
		printf("auth method %s not support\n", req.method.c_str());
		response = auth->PrintAuthResponseFailure(SSH_AUTH_PUB_KEY, false);
	}
	else
	{
		if (req.method_fields.publickey.flag)
		{
			if (auth->VerifyAuthRequest(req))
			{
				printf("auth success!\n");
				response = auth->PrintAuthResponseSuccess();
			}
			else
			{
				printf("VerifyAuthRequest Failed!\n");
				response = auth->PrintAuthResponseFailure(SSH_AUTH_PUB_KEY, false);
			}
		}
		else
		{
			printf("no sig\n");
			response = auth->PrintPkOK(req.method_fields.publickey.key_algorithm_name, 
				req.method_fields.publickey.key_blob);
		}
	}
	SendToClient(response);
}
int main(int argc, char*argv[])
{
	/*ssh_c.SetHandler(c_SSH_TRANS_HANDLER, NULL);
	ssh_s.SetHandler(s_SSH_TRANS_HANDLER, NULL);*/

	ssh_s.SetHandler(SSH_MSG_VERSION, SSH_TRANS_HANDLER_SSH_MSG_VERSION, NULL);
	ssh_s.SetHandler(SSH_MSG_KEXINIT, SSH_TRANS_HANDLER_SSH_MSG_KEXINIT, NULL);
	ssh_s.SetHandler(SSH_MSG_KEXDH_INIT, SSH_TRANS_HANDLER_SSH_MSG_KEXDH_INIT, NULL);
	ssh_s.SetHandler(SSH_MSG_KEXDH_REPLY, SSH_TRANS_HANDLER_SSH_MSG_KEXDH_REPLY, NULL);
	ssh_s.SetHandler(SSH_MSG_NEWKEYS, SSH_TRANS_HANDLER_SSH_MSG_NEWKEYS, NULL);
	ssh_s.SetHandler(SSH_MSG_SERVICE_REQUEST, SSH_TRANS_HANDLER_SSH_MSG_SERVICE_REQUEST, NULL);
	ssh_s.SetHandler(SSH_MSG_USERAUTH_REQUEST, SSH_TRANS_HANDLER_SSH_MSG_USERAUTH_REQUEST, NULL);

	ssh_s.LoadPriKey(sim::Rsa, "./ssh_rsa.pem");
	ssh_s.LoadPriKey(sim::Dsa, "./ssh_dsa.pem");

	sim::SimAsync &async = Get();
	sim::AsyncHandle handle = async.CreateTcpHandle();
	/*async.ConvertToSSL(handle, true, true);
	async.SetSSLKeyFile(handle, "cert.pem", "key.pem");*/
	async.SetAcceptHandler(handle, AcceptHandler, NULL);
	async.SetRecvDataHandler(handle, s_RecvDataHandler, NULL);

	async.AddTcpServer(handle, NULL, 8080);

	/*s_handle = async.CreateTcpHandle();
	async.SetRecvDataHandler(s_handle, c_RecvDataHandler, NULL);
	async.AddTcpConnect(s_handle, "127.0.0.1", 8080);*/

	sim::GlobalPoll<sim::SimAsync, MY_THREAD_NUM>::Wait();
	getchar();
	return 0;
}