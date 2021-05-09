/*
	http�������
*/
#include <stdio.h>
#include <map>
#include "Timer.hpp"
#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include "CodeConvert.hpp"
//���Ʋ�������
sim::CmdLineParser cmd;
#define MY_THREAD_NUM 8
struct GlobalConfig
{
	sim::Str Listen;
	sim::Str Mount;
	sim::Str index;
	sim::Str pub_key;
	sim::Str pri_key;

	int timeout;
	int thread_num;//�߳���
};
GlobalConfig conf;
std::map<std::string, std::string> content_type_map;
sim::AsyncHttp &GetHttp()
{
	return sim::GlobalPoll<sim::AsyncHttp, MY_THREAD_NUM>::Get();
}
void HttpFormDataHANDLER(sim::HttpFormDataParser *pp, sim::HttpFormData *pformdata, void*pdata)
{
	return;
}
void init_content_type_map()
{
	//https://www.w3school.com.cn/media/media_mimeref.asp
	content_type_map[".html"] = "text/html";
	content_type_map[".js"] = "application/x-javascript";
	content_type_map[".css"] = "text/css";
	content_type_map[".jpeg"] = "image/jpeg";
	content_type_map[".jpg"] = "image/jpeg";
	//image/gif��GIF��ʽ��ͼƬ
	content_type_map[".gif"] = "image/gif";
}

sim::Str get_content_type(const sim::Str& filename)
{
	sim::Str ext = filename.substr(filename.find_last_of('.'));
	std::map<std::string, std::string>::iterator iter = content_type_map.find(ext);
	if (iter == content_type_map.end())
	{
		return "application/octet-stream";
	}
	return iter->second;

}
void send_error(sim::AsyncHandle handle, const sim::Str& status, const sim::Str& reason)
{
	sim::HttpResponseHead res;
	res.Status = status;
	res.Reason = reason;
	GetHttp().Send(handle, res, NULL, 0);
	return;
}

//code_convert.cpp
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	/*
	//�����ر� ����������close�رգ�
		CloseActive,
		//�Ự�������չر� �����ӽ��յ�eof��
		ClosePassive,
		//�쳣�رգ������˴���ƽ̨������error��
		CloseError=-1,
	*/
	//printf("handle %d %d reason %d\n", handle, __LINE__, reason);
	static sim::Str reasons[3] = { "CloseActive","ClosePassive","CloseError" };
	printf("%d close reason %s error %d\n", handle, reasons[reason].c_str(), error);
	if (data)
	{
		delete (sim::HttpFormDataParser*)data;
	}
}

void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	sim::HttpFormDataParser *parser = NULL;
	if (pdata)
	{
		parser = (sim::HttpFormDataParser *)pdata;

		sim::Str  data = sim::Str(buff, len);
		if (Head->Method == "POST")
		{
			sim::HttpResponseHead res;
			res.Head.Append(SIM_HTTP_CON, Head->Head.GetCase(SIM_HTTP_CON, "Close"));

			sim::Str uri = sim::FromUtf8(sim::HttpParser::DecodeUrl(Head->Url));
			sim::Str path;
			sim::KvMap params;
			sim::HttpParser::ParserRequestUri(uri, path, params);
			if ("/form_test" == path)
			{
				if (!parser->Parser(buff, len))
				{
					send_error(handle, "400", "Bad Request");
					return;
				}
				if (sim::HttpFormDataParser::IsComplete(Head->Head, offset, len))
				{
					GetHttp().Send(handle, res, NULL, 0);
				}
			}
			else
			{
				send_error(handle, "403", "Forbidden");
			}
			return;
		}
		else
		{
			send_error(handle, "400", "Bad Request");
		}
	}
	else
	{
		
		send_error(handle, "500", "HTTP-Internal Server Error");
	}
}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);

	sim::HttpFormDataParser *parser = new sim::HttpFormDataParser();
	parser->SetHandler(HttpFormDataHANDLER, NULL);
	GetHttp().SetCloseHandler(client, CloseHandler, parser);
	GetHttp().SetHttpRequestHandler(client, ASYNC_HTTP_REQUEST_HANDLE, parser);
}

void print_help()
{
	printf("usg:-l ������ַ -m ����λ�� -index ��ҳ -pub_key ��Կ -pri_key ˽Կ -timeout ���ӳ�ʱ��� \n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("timeout", 5 * 60 * 1000);
	cmd.InitCmdLineParams("l", "http://:8080/");
	cmd.InitCmdLineParams("log", "");
#ifdef OS_WINDOWS
	cmd.InitCmdLineParams("m", "F:/Sim/res/WWW");
#endif
#ifdef OS_LINUX
	cmd.InitCmdLineParams("m", "/code/Sim-github/res/WWW");
#endif

#endif

	if (!cmd.Parser(argc, argv) || cmd.HasParam("h") || cmd.HasParam("help"))
	{
		print_help();
		return -1;
	}

	conf.Listen = cmd.GetCmdLineParams("l", "http://:8080/");
	conf.Mount = cmd.GetCmdLineParams("m", ".");
	conf.index = cmd.GetCmdLineParams("index", "index.html");
	conf.pub_key = cmd.GetCmdLineParams("pub_key", "cert.pem");
	conf.pri_key = cmd.GetCmdLineParams("pri_key", "key.pem");
	conf.timeout = cmd.GetCmdLineParams("timeout", -1);

	if (conf.Listen.empty() || conf.Mount.empty())
	{
		print_help();
		return -1;
	}
	if (cmd.HasParam("log"))
	{
		SIM_LOG_CONSOLE(sim::LError);
		//SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, ".", "test", "txt", 32);
		//SIM_FUNC(sim::LDebug);
	}
	init_content_type_map();

	sim::AsyncHandle serv = GetHttp().CreateSession();
	GetHttp().SetAcceptHandler(serv, AcceptHandler, NULL);
	if (false == GetHttp().Listen(serv, conf.Listen.c_str(), conf.pub_key.c_str(), conf.pri_key.c_str()))
	{
		printf("Listent %s fail,please check params.\n", conf.Listen.c_str());
		return -1;
	}
	sim::GlobalPoll<sim::AsyncHttp, MY_THREAD_NUM>::Wait();
	return 0;
}