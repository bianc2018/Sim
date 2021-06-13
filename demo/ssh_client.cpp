#include "AsyncSSH.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#define MY_THREAD 3
class myHandler:public sim::SshSessionHandler
{
public:

	////连接建立
	virtual void OnConnectEstablished(sim::SshSession* session)
	{
		printf("%p OnConnectEstablished ok\n",session);
		session->LoginPassWord("root", "q1051576073@");
	}

	//连接会话关闭
	virtual void OnConnectClosed(sim::SshSession* session)
	{
		printf("%p OnConnectClosed \n", session);
	}

	//客户端验证结果
	virtual void OnAuthResponse(sim::SshSession* session, bool success)
	{
		printf("%p OnAuthResponse %s \n", session,success?"success":"fail");
	}

	virtual void OnAuthRequest(sim::SshSession* session,
		const sim::SshAuthRequest& req, int& res_status)
	{
		if (req.method != SSH_AUTH_PASSWORD)
		{
			res_status = -1;
			return;
		}
		if(req.user_name == "root"&&req.method_fields.password.password == "root")
		{
			res_status = 1;
			return;
		}
		res_status = -1;
		return;
	}
};
int main(int argc, char*argv[])
{
	SIM_LOG_CONSOLE(sim::LError);

	sim::AsyncSsh&ssh =sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Get();
	sim::RefObject<sim::SshSession> session=ssh.CreateSession();
	session->SetHandler(sim::RefObject<sim::SshSessionHandler>(new myHandler));
	
	//sim::SshTransport::WriteKey(sim::SshTransport::GenerateRsaKey(2048), "./ssh_rsa.pem");
	//sim::SshTransport::WriteKey(sim::SshTransport::GenerateDsaKey(2048), "./ssh_dsa.pem");

	session->LoadHostPrivateKey("./ssh_rsa.pem");
	session->LoadHostPrivateKey("./ssh_dsa.pem");
	//session->Connect("49.234.220.213");
	session->Accept(8080);
	sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Wait();
	return 0;
}