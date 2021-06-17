#include "AsyncSSH.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include <iostream>
sim::CmdLineParser cmd;
#define MY_THREAD 1
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
				if(channel)
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

class SshellChannelHandler :public sim::SshChannelHandler
{
	//打开通道 主动
	virtual void OnOpenConfirmation(sim::SshChannel* channel) 
	{
		//printf("channel[%u] is OnOpenConfirmation\n", channel->GetId());
		SIM_LINFO("channel "<<channel->GetId() << " OnOpenConfirmation ok");
		channel->PtyReq("vanilla",true);
		channel->Shell(true);
		SIM_LINFO("Sshell is ok");
		sim::GlobalPoll<AsyncInput, 1>::Get().channel = channel->GetRef();
		////channel->Env("FOO", "bar", true);
		////channel->Send("ll\n");
		//channel->Exec("ls -l",true);
		//channel->Close();
		return; 
	}

	virtual void OnOpenFailure(sim::SshChannel* channel, const sim::SshOpenChannelFailure&failure) 
	{ 
		SIM_LERROR("channel " << channel->GetId() 
			<< " OnOpenFailure reason_code "<<failure.reason_code
		<<" description: "<<failure.description);
		channel->Close();
		return; 
	}

	virtual void OnClosed(sim::SshChannel* channel) 
	{
		SIM_LINFO("channel " << channel->GetId() << " OnClosed ok");
		sim::GlobalPoll<AsyncInput, 1>::Get().channel.reset();
		channel->GetSession().Close();//关闭会话
		return;
	}

	virtual void OnData(sim::SshChannel* channel, const sim::Str&data)
	{
		//printf("channel[%u]OnData:%s\n", channel->GetId(),data.c_str());
		printf("%s", data.c_str());
		/*if(data.size()>2*1024)
			printf("%u\n", data.size());
		else
			printf("%s", data.c_str());*/
	}

	virtual void OnExtData(sim::SshChannel* channel, sim::uint32_t data_type_code, const sim::Str& data)
	{
		//printf("channel[%u]OnExtData %u:%s\n", channel->GetId(), data_type_code, data.c_str());
		printf("%s", data.c_str());
		//printf("%u\n", data.size());
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

class SshellSessionHandler:public sim::SshSessionHandler
{
public:

	////连接建立
	virtual void OnConnectEstablished(sim::SshSession* session)
	{
		SIM_LINFO(session->GetHandle() << " OnConnectEstablished ok");

		sim::Str username = cmd.GetCmdLineParams("u", "root");
		sim::Str auth_method = cmd.GetCmdLineParams("a", "password");
		SIM_LINFO("Login " << username << " by " << auth_method);
		if ("password" == auth_method)
		{
			sim::Str password = cmd.GetCmdLineParams("password", "");	
			if (!session->LoginPassWord(username, password))
			{
				SIM_LERROR("Start LoginPassWord Error");
				session->Close();
				return;
			}
		}
		else if ("publickey" == auth_method)
		{
			sim::Str publickey = cmd.GetCmdLineParams("publickey", "");
			if (!session->LoginPublicKey(username, publickey))
			{
				SIM_LERROR("Start LoginPublicKey Error");
				session->Close();
				return;
			}
		}
		else
		{
			SIM_LERROR("未知的鉴权方式 " << auth_method);
			session->Close();
			return;
		}
		
	}

	//连接会话关闭
	virtual void OnConnectClosed(sim::SshSession* session)
	{
		//printf("%p OnConnectClosed \n", session);
		SIM_LINFO(session->GetHandle() << " OnConnectClosed");
		sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Exit();
	}

	//客户端验证结果
	virtual void OnAuthResponse(sim::SshSession* session, bool success)
	{
		SIM_LINFO(session->GetHandle() << " OnAuthResponse ok "<<(session, success ? "success" : "fail"));
		if (success)
		{
			sim::RefObject<sim::SshChannel> channel = session->CreateChannel();
			if (!channel->Open(SSH_CHANNEL_TYPE_SESSION))
			{
				SIM_LERROR(session->GetHandle() << " Open Channel error");
				session->Close();
				return;
			}
			
		}
		else
		{
			session->Close();
		}
	}

	virtual void OnPassWordChanged(sim::SshSession*session, sim::Str&new_password)
	{
		SIM_LWARN(session->GetHandle() << "the password is changed!please input new password");
		std::cout << "new_password:";
		std::cin >> new_password;
		if (new_password.size() == 0)
		{
			SIM_LERROR(session->GetHandle() << " new_password is Empty,may be Close");
			session->Close();
			return;
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

void PrintHelp()
{
	printf("use as:\n"
		"\t-h host 目标主机\n"
		"\t-p port 访问端口（默认22）\n"
		"\t-a 验证方法（password or publickey 默认password）\n"
		"\t-u 用户名（默认root）\n"
		"\t-password 密码\n"
		"\t-publickey 登陆密钥文件\n"
		"\t-log 日志级别(INFO,ERROR,WARNING,NONE,DEBUG 默认 ERROR)\n"
		"\t-output 日志输出路径\n"
		"\t-help   帮助\n");
	getchar();
}

bool CheckConfig(sim::CmdLineParser &cmd)
{
	if (cmd.HasParam("help"))
	{
		return false;
	}

	if (!cmd.HasParam("h"))
	{
		printf("请输入目标主机 -h\n");
		return false;
	}
	sim::Str auth_method = cmd.GetCmdLineParams("a", "password");
	if ("password" == auth_method)
	{
		if (!cmd.HasParam("password"))
		{
			printf("请输入目标主机密码 -password\n");
			return false;
		}
	}
	else if ("publickey" == auth_method)
	{
		if (!cmd.HasParam("publickey"))
		{
			printf("请输入目标主机公钥 -publickey\n");
			return false;
		}
	}
	else
	{
		printf("未知的鉴权方式 %s\n",auth_method.c_str());
		return false;
	}

	sim::Str log = cmd.GetCmdLineParams("log", "ERROR");
	sim::Str output = cmd.GetCmdLineParams("output", "");
	sim::LogLevel ll = sim::LError;

	if ("DEBUG" == log)
		ll = sim::LDebug;
	else if ("INFO" == log)
		ll = sim::LInfo;
	else if ("WARNING" == log)
		ll = sim::LWarn;
	else if ("ERROR" == log)
		ll = sim::LError;
	else if ("NONE" == log)
		ll = sim::LNone;
	else
	{
		printf("错误的日志级别 %s\n", log.c_str());
		return false;
	}

	SIM_LOG_CONSOLE(ll);
	if(output.size()!=0)
		SIM_LOG_ADD(sim::LogFileStream, ll, output, "Sshell");
	return true;
}

int main(int argc, char*argv[])
{
#if 1
	cmd.InitCmdLineParams("h", "49.234.220.213");
	cmd.InitCmdLineParams("password", "q1051576073@");
	cmd.InitCmdLineParams("log", "ERROR");
	cmd.InitCmdLineParams("output", "./log/");
#endif
	int p = 4 % 2;
	//test
	/*FILE*fp = fopen("./0.d", "w");
	const int size = 1 * 1024 * 1024;
	sim::Str data(size, '0');
	for (int i = 0; i < 1024; ++i)
		fwrite(data.c_str(), sizeof(char), size,fp);
	fclose(fp);*/
	//test

	//配置初始化
	if (!cmd.Parser(argc, argv)||!CheckConfig(cmd))
	{
		PrintHelp();
		return -1;
	}
	sim::AsyncSsh&ssh =sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Get();
	sim::RefObject<sim::SshSession> session=ssh.CreateSession();
	session->SetHandler(new SshellSessionHandler);
	session->SetChannelHandler(new SshellChannelHandler());
	sim::Str host = cmd.GetCmdLineParams("h", "");
	int port = cmd.GetCmdLineParams("p", 22);
	SIM_LINFO("Connect " << host << ":" << port);
	session->Connect(host,port);
	sim::GlobalPoll<sim::AsyncSsh, MY_THREAD>::Wait();
	getchar();
	return 0;
}