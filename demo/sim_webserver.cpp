/*
	http请求测试
*/
#include <stdio.h>
#include <map>
#include "Timer.hpp"
#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include "CodeConvert.hpp"
//控制参数输入
sim::CmdLineParser cmd;

struct GlobalConfig
{
	sim::Str Listen;
	sim::Str Mount;
	sim::Str pub_key;
	sim::Str pri_key;

	int timeout;
	int thread_num;//线程数
};
GlobalConfig conf;
std::map<std::string, std::string> content_type_map;
sim::AsyncHttp &GetHttp()
{
	return sim::GlobalPoll<sim::AsyncHttp, 8>::Get();
}

void init_content_type_map()
{
	//https://www.w3school.com.cn/media/media_mimeref.asp
	content_type_map[".html"] = "text/html";
	content_type_map[".js"] = "application/x-javascript";
	content_type_map[".css"] = "text/css";
	content_type_map[".jpeg"] = "image/jpeg";
	content_type_map[".jpg"] = "image/jpeg";
}

sim::Str get_content_type(const sim::Str& filename)
{
	sim::Str ext = filename.substr(filename.find_last_of('.'));
	std::map<std::string, std::string>::iterator iter= content_type_map.find(ext);
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
//发送文件
void send_file(sim::AsyncHandle handle, const sim::Str& filename)
{
	FILE *pf = fopen(filename.c_str(), "rb");
	if (NULL == pf)
	{
		printf("filename %s is not exist\n", filename.c_str());
		send_error(handle, "404", "Not Found");
		return;
	}

	//长度
	fseek(pf, 0, SEEK_END);
	sim::ContentLength_t content_lenght = ftell(pf);
	fseek(pf, 0, SEEK_SET);
	sim::HttpResponseHead res;
	res.Head.Append(SIM_HTTP_CT, get_content_type(filename));

	sim::ContentLength_t content_offset = 0;
	const int buff_size = 32 * 1024;
	char buff[buff_size] = { 0 };
	while (true)
	{
		unsigned int readlen = fread(buff, sizeof(char), buff_size, pf);
		if (readlen <= 0)
			break;
		GetHttp().Send(handle, res, content_lenght, content_offset, buff, readlen);
	}
	fclose(pf);
}

void TIMER_OUT_HANDLER(sim::timer_id timer_id, void *userdata)
{
	//关闭
	sim::AsyncHandle handle = (sim::AsyncHandle)((long long)userdata);
	sim::GlobalPoll<sim::AsyncHttp>::Get().Close(handle);
	printf("request time out \n");
}
//code_convert.cpp
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	/*
	//主动关闭 （本级调用close关闭）
		CloseActive,
		//会话正常接收关闭 （连接接收到eof）
		ClosePassive,
		//异常关闭（发生了错误，平台错误码error）
		CloseError=-1,
	*/
	printf("handle %d %d reason %d\n", handle, __LINE__, reason);
	static sim::Str reasons[3] = {"CloseActive","ClosePassive","CloseError"};
	printf("%d close reason %s error %d\n", handle, reasons[reason].c_str(), error);
	if (data)
	{
		delete (sim::timer_id*)data;
	}
}

void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	sim::timer_id* ptimer = NULL;
	if (pdata)
	{
		ptimer = (sim::timer_id*)pdata;
		sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(*ptimer);
	}

	if (Head->Method == "GET")
	{
		sim::Str uri = sim::FromUtf8(sim::HttpParser::DecodeUrl(Head->Url));
		sim::Str path;
		sim::KvMap params;
		sim::HttpParser::ParserRequestUri(uri,path,params);

		sim::Str filename = conf.Mount + path;
		printf("request:%s\n", filename.c_str());
		send_file(handle, filename);
		return;
	}
	else
	{
		send_error(handle, "400", "Bad Request");
	}

	//计时
	if (ptimer)
	{
		*ptimer = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(conf.timeout, TIMER_OUT_HANDLER, (void*)handle);
	}
}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);

	sim::timer_id* ptimer = NULL;
	if (conf.timeout > 0)
	{
		ptimer = new sim::timer_id;
		*ptimer = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(conf.timeout, TIMER_OUT_HANDLER,(void*)client);
	}
	GetHttp().SetCloseHandler(client, CloseHandler, ptimer);
	GetHttp().SetHttpRequestHandler(client, ASYNC_HTTP_REQUEST_HANDLE, ptimer);
}

void print_help()
{
	printf("usg:-l 监听地址 -m 挂载位置 -pub_key 公钥 -pri_key 私钥 -timeout 连接超时间隔 \n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("timeout", 5*60*1000);
	cmd.InitCmdLineParams("l", "https://:8080/");
#endif

	if (!cmd.Parser(argc, argv)|| cmd.HasParam("h")|| cmd.HasParam("help"))
	{
		print_help();
		return -1;
	}

	conf.Listen = cmd.GetCmdLineParams("l", "http://:8080/");
	conf.Mount = cmd.GetCmdLineParams("m", ".");
	conf.pub_key = cmd.GetCmdLineParams("pub_key", "cert.pem");
	conf.pri_key = cmd.GetCmdLineParams("pri_key", "key.pem");
	conf.timeout = cmd.GetCmdLineParams("timeout", -1);

	if (conf.Listen.empty() || conf.Mount.empty())
	{
		print_help();
		return -1;
	}

	init_content_type_map();

	sim::AsyncHandle serv = GetHttp().CreateSession();
	GetHttp().SetAcceptHandler(serv, AcceptHandler, NULL);
	if (false == GetHttp().Listen(serv, conf.Listen.c_str(), conf.pub_key.c_str(), conf.pri_key.c_str()))
	{
		printf("Listent %s fail,please check params.\n", conf.Listen.c_str());
		return -1;
	}
	sim::GlobalPoll<sim::AsyncHttp, 8>::Wait();
	return 0;
}