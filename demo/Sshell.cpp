#include "AsyncSSH.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include <iostream>
#define MY_THREAD 3
//异步服务对象基类
class AsyncInput
{
public:
	//运行接口，取事件和分发
	virtual int Poll(unsigned int wait_ms)
	{
		while (channel&&channel->Status() != sim::SSH_CHANNEL_CLOSED)
		{
			char c = getchar();
			//printf("%c\n", c);
			//channel->Send(input + "\n");
			input += c;
			if (c == '\n'|| c == '\t')
			{
				channel->Send(input);
				input = "";
			}

		}
		return 0;
	}
public:
	sim::Str input;
	sim::RefObject<sim::SshChannel> channel;
};

class myChannelHandler :public sim::SshChannelHandler
{
	//打开通道 主动
	virtual void OnOpenConfirmation(sim::SshChannel* channel) 
	{
		printf("channel[%u] is OnOpenConfirmation\n", channel->GetId());
		channel->PtyReq("vanilla",true);
		channel->Shell(true);
		
		sim::GlobalPoll<AsyncInput, 1>::Get().channel = channel->GetRef();
		////channel->Env("FOO", "bar", true);
		////channel->Send("ll\n");
		//channel->Exec("ls -l",true);
		//channel->Close();
		return; 
	}

	virtual void OnOpenFailure(sim::SshChannel* channel, const sim::SshOpenChannelFailure&failure) 
	{ 
		printf("channel[%u] is OnOpenFailure\n", channel->GetId());
		return; 
	}

	virtual void OnClosed(sim::SshChannel* channel) 
	{
		printf("channel[%u] is OnClosed\n", channel->GetId());
		return;
	}

	virtual void OnData(sim::SshChannel* channel, const sim::Str&data)
	{
		//printf("channel[%u]OnData:%s\n", channel->GetId(),data.c_str());
		printf("%s", data.c_str());
	}

	virtual void OnExtData(sim::SshChannel* channel, sim::uint32_t data_type_code, const sim::Str& data)
	{
		//printf("channel[%u]OnExtData %u:%s\n", channel->GetId(), data_type_code, data.c_str());
		printf("%s", data.c_str());
	}

	virtual void OnRequest(sim::SshChannel* channel, const sim::SshChannelRequest&req)
	{
		//printf("channel[%u]OnRequest %s\n", channel->GetId(), req.type.c_str());
	}

	virtual void OnResponse(sim::SshChannel* channel, bool success)
	{
		//printf("channel[%u]OnResponse %s\n", channel->GetId(), success?"success":"failure");
	}
};

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
		if (success)
		{
			sim::RefObject<sim::SshChannel> channel = session->CreateChannel();
			channel->Open(SSH_CHANNEL_TYPE_SESSION);
		}
	}

	/*virtual void OnAuthRequest(sim::SshSession* session,
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
	}*/
};
int main(int argc, char*argv[])
{
	SIM_LOG_CONSOLE(sim::LError);

	sim::AsyncSsh&ssh =sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Get();
	sim::RefObject<sim::SshSession> session=ssh.CreateSession();
	session->SetHandler(new myHandler);
	session->SetChannelHandler(new myChannelHandler());
	//sim::SshTransport::WriteKey(sim::SshTransport::GenerateRsaKey(2048), "./ssh_rsa.pem");
	//sim::SshTransport::WriteKey(sim::SshTransport::GenerateDsaKey(2048), "./ssh_dsa.pem");

	//session->LoadHostPrivateKey("./ssh_rsa.pem");
	//session->LoadHostPrivateKey("./ssh_dsa.pem");
	session->Connect("49.234.220.213");
	//session->Accept(8080);
	sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Wait();
	return 0;
}