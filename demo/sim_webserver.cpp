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
#define MY_THREAD_NUM 8
struct GlobalConfig
{
	sim::Str Listen;
	sim::Str Mount;
	sim::Str index;
	sim::Str pub_key;
	sim::Str pri_key;

	int timeout;
	int thread_num;//线程数
};
GlobalConfig conf;
std::map<std::string, std::string> content_type_map;
sim::AsyncHttp &GetHttp()
{
	return sim::GlobalPoll<sim::AsyncHttp, MY_THREAD_NUM>::Get();
}

struct WebSession
{
	//超时
	sim::timer_id timer;
	//表单
	sim::HttpFormDataParser form_parser;
};

void init_content_type_map()
{
	//https://www.w3school.com.cn/media/media_mimeref.asp
	content_type_map[".html"] = "text/html";
	content_type_map[".js"] = "application/x-javascript";
	content_type_map[".css"] = "text/css";
	content_type_map[".jpeg"] = "image/jpeg";
	content_type_map[".jpg"] = "image/jpeg";
	//image/gif：GIF格式的图片
	content_type_map[".gif"] = "image/gif";
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
void send_file(sim::AsyncHandle handle, sim::HttpResponseHead &res, const sim::Str& filename)
{
	FILE *pf = fopen(filename.c_str(), "rb");
	if (NULL == pf)
	{
		printf("filename %s is not exist\n", filename.c_str());
		send_error(handle, "404", "Not Found");
		return;
	}

	//长度
	sim::ContentLength_t content_lenght = 0;
	/*fseek(pf, 0, SEEK_END);
	content_lenght = ftell(pf);
	fseek(pf, 0, SEEK_SET);*/
	//sim::HttpResponseHead res;

	res.Head.Append(SIM_HTTP_CT, get_content_type(filename));
	res.Head.Append("Transfer-Encoding", "chunked");

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
	//发送结束
	if(sim::HttpParser::IsChunked(res.Head))
		GetHttp().Send(handle, res, 0, content_offset, NULL, 0);

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
	//printf("handle %d %d reason %d\n", handle, __LINE__, reason);
	static sim::Str reasons[3] = {"CloseActive","ClosePassive","CloseError"};
	printf("%d close reason %s error %d\n", handle, reasons[reason].c_str(), error);
	if (data)
	{
		delete (WebSession*)data;
	}
}
void OnHttpFormFileUpload(sim::HttpFormDataParser *pp, sim::HttpFormDataHead *pformdata,
	sim::ContentLength_t offset,
	const char*data, sim::ContentLength_t len, bool fin, void*pdata)
{
	FILE*fp = (FILE*)pdata;

	if (offset == 0)
	{
		sim::Str name, filename;
		//初始化
		pp->GetFormDataDisposition(*pformdata, name, filename);
		if (filename.size() == 0)
			return;
		sim::Str filepath = conf.Mount +"/"+ sim::FromUtf8(filename);
		printf("file upload:%s\n", filepath.c_str());
		
		if (fp)
		{
			fclose(fp);
		}

		fp = fopen(filepath.c_str(), "wb");
		if (NULL == fp)
		{
			printf("file open:%s error\n", filepath.c_str());
			return;
		}
		pp->SetHandler(OnHttpFormFileUpload, fp);
	}

	if (fp&&len>0&& data!=NULL)
	{
		fwrite(data, len, sizeof(const char), fp);
	}
	if (fin&&fp)
	{
		fclose(fp);
		pp->SetHandler(OnHttpFormFileUpload, NULL);
		
	}
}
void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	WebSession* ps = (WebSession*)pdata;
	if (NULL == ps)
	{
		send_error(handle, "500", "HTTP-Internal Server Error");
		return;
	}
	
	if (conf.timeout > 0)
	{
		sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(ps->timer);
	}

	//sim::Str  data = sim::Str(buff, len);
	sim::Str uri = sim::FromUtf8(sim::HttpParser::DecodeUrl(Head->Url));
	sim::Str path;
	sim::KvMap params;
	sim::HttpParser::ParserRequestUri(uri, path, params);

	sim::HttpResponseHead res;
	res.Head.Append(SIM_HTTP_CON, Head->Head.GetCase(SIM_HTTP_CON, "Close"));

	if (Head->Method == "GET")
	{
		if ("/" == path)
		{
			res.Status = "302";
			res.Reason = "Moved Temporarily";
			res.Head.Append("Location", conf.index);
			GetHttp().Send(handle, res, NULL, 0);
		}
		else
		{
			sim::HttpResponseHead res;
			sim::Str filename = conf.Mount + path;
			printf("%d request:%s\n", handle, filename.c_str());
			send_file(handle, res, filename);
		}
		//return;
	}
	else if (Head->Method == "POST")
	{
		if ("/file_upload" == path)
		{
			if (!ps->form_parser.Parser(buff, len))
			{
				send_error(handle, "400", "Bad Request");
			}
			if (sim::HttpFormDataParser::IsComplete(Head->Head, offset, len))
			{
				res.Status = "302";
				res.Reason = "Moved Temporarily";
				res.Head.Append("Location", "fileupload.html");
				GetHttp().Send(handle, res, NULL, 0);
			}
		}
		else
		{
			send_error(handle, "403", "Forbidden");
		}
	}
	else
	{
		send_error(handle, "400", "Bad Request");
	}

	if (conf.timeout > 0)
	{
		ps->timer = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(conf.timeout, TIMER_OUT_HANDLER, (void*)handle);
	}
}

void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);

	WebSession* ps = new WebSession();
	if (conf.timeout > 0)
	{
		ps->timer = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(conf.timeout, TIMER_OUT_HANDLER,(void*)client);
	}
	ps->form_parser.SetHandler(OnHttpFormFileUpload, NULL);
	GetHttp().SetCloseHandler(client, CloseHandler, ps);
	GetHttp().SetHttpRequestHandler(client, ASYNC_HTTP_REQUEST_HANDLE, ps);
}

void print_help()
{
	printf("usg:-l 监听地址 -m 挂载位置 -index 首页 -pub_key 公钥 -pri_key 私钥 -timeout 连接超时间隔 \n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("timeout", 5*60*1000);
	cmd.InitCmdLineParams("l", "http://:8080/");
	cmd.InitCmdLineParams("log", "");
#ifdef OS_WINDOWS
	cmd.InitCmdLineParams("m", "F:/Sim/res/WWW");
#endif
#ifdef OS_LINUX
	cmd.InitCmdLineParams("m", "/code/Sim-github/res/WWW");
#endif

#endif

	if (!cmd.Parser(argc, argv)|| cmd.HasParam("h")|| cmd.HasParam("help"))
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