/*
	http请求测试
*/
#include <stdio.h>
#include "HttpApi.hpp"
#include "CmdLineParser.hpp"
sim::CmdLineParser cmd;
sim::TimeSpan ts;//计时
sim::KvMap *pex = NULL;
FILE*f =NULL;
bool is_print_head = true;
sim::Str print_bytes(double bytes)
{
	const int buff_size = 1024;
	char buff[buff_size] = { 0 };
	if (bytes < 1024)
	{
		snprintf(buff, buff_size, "%u bytes", bytes);
	}
	else if (bytes < 1024*1024)
	{
		snprintf(buff, buff_size, "%0.3lf kb", double(bytes)/1024);
	}
	else if (bytes < 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf mb", double(bytes) / (1024*1024));
	}
	else if (bytes > 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf gb", double(bytes) / (1024 * 1024* 1024));
	}
	return buff;
}

void init_exhead(const sim::Str&map)
{
	bool is_key = true;
	sim::Str key, value;
	for (int i = 0; i < map.size(); ++i)
	{
		if (map[i] == ':')
		{
			if (is_key)
			{
				is_key = false;
			}
			else
			{
				is_key = true;
				if (key.size() != 0)
				{
					if (NULL == pex)
						pex = new sim::KvMap;
					pex->Append(key, value);
				}
				key = value = "";
			}
			continue;
		}
		if (is_key)
			key += map[i];
		else
			value += map[i];
	}
	if (!is_key)
	{
		if (key.size() != 0)
		{
			if (NULL == pex)
				pex = new sim::KvMap;
			pex->Append(key, value);
		}
	}
}

bool SyncProgress(sim::ContentLength_t total_bytes, sim::ContentLength_t now_bytes,sim::HttpResponse *response, void*pdata)
{
	 if (cmd.HasParam("print"))
	 {
		 if (is_print_head)
		 {
			 printf("response head :%s %s use %llu ms\n", response->Status.c_str(), response->Reason.c_str(), ts.Get());
			 printf("response head:\n");
			 sim::KvMap::KvMapNode *pn = response->Head.pHead;
			 int index = 0;
			 while (pn)
			 {
				 printf("\t%d %s:%s\n", ++index, pn->Key.c_str(), pn->Value.c_str());
				 pn = pn->next;
			 }
			 printf("response body[%s (%llu bytes)]\n", 
				 print_bytes(response->Content.content_length).c_str(),response->Content.content_length);
			 is_print_head = false;
		 }

		 unsigned long long use_ms = ts.Get();
		 //进度百分比
		 double ps = total_bytes==0?100:(double(now_bytes) / total_bytes) * 100;
		 double speed = use_ms==0?0:double(now_bytes) / (use_ms / double(1000));
		 printf("\rrecv body(%%%0.3lf) %s [total %s] use %0.3lf s speed %s/s",
			 ps,
			 print_bytes(now_bytes).c_str(), print_bytes(total_bytes).c_str(),
			 double(use_ms)/1000,
			 print_bytes(speed).c_str());
		
	 }
	 if (f&&response->Content.is_last)
	 {
		 fwrite(response->Content.chunk.c_str(), response->Content.chunk.size(), 1, f);
	 }
	 return true;
}
void print_help()
{
	printf("usg:-m 请求方法(GET、POST) -u URL -body 消息体 -timeout 超时单位毫秒 -log 输出日志 -save filename 保存报文 -print 打印详情 -no_print_body 不打印消息体 -ext_head 拓展头 \"key:value:key1:value1\"\n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("u", "http://mirrors.aliyun.com/centos/7/isos/x86_64/CentOS-7-x86_64-Everything-2009.iso")
		.InitCmdLineParams("print","")
		//.InitCmdLineParams("m", "HEAD")
		//.InitCmdLineParams("log", "")
		.InitCmdLineParams("timeout", -1)
		.InitCmdLineParams("no_print_body","")
	.InitCmdLineParams("save", "CentOS-7-x86_64-Everything-2009.iso");

#endif
	if (!cmd.Parser(argc, argv)
		|| cmd.HasParam("h")
		|| cmd.HasParam("help")
		|| !cmd.HasParam("u")
		)
	{
		print_help();
		return -1;
	}

	if (cmd.HasParam("log"))
	{
		SIM_LOG_CONSOLE(sim::LDebug);
		SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, "./debug_log/", "http_request", "txt");
	}

	sim::Str filename = cmd.GetCmdLineParams("save", "");
	if (!filename.empty())
	{
		f = fopen(filename.c_str(), "wb+");
		if (f == NULL)
		{
			printf("save to %s fail\n", filename.c_str());
			return -1;
		}
	}

	sim::Str method = cmd.GetCmdLineParams("m", "GET");
	sim::Str url = cmd.GetCmdLineParams("u", "");
	sim::Str body = cmd.GetCmdLineParams("body", "");
	int timeout = cmd.GetCmdLineParams("timeout", -1);
	init_exhead(cmd.GetCmdLineParams("ext_head", ""));

	//打印
	if (cmd.HasParam("print"))
	{
		printf("start request :%s %s timeout[%d ms]\n", method.c_str(), url.c_str(), timeout);
		if (!filename.empty())
		{
			printf("save %s\n", filename.c_str());
		}
		if (pex)
		{
			printf("ex head:\n");
			sim::KvMap::KvMapNode *pn = pex->pHead;
			int index = 0;
			while (pn)
			{
				printf("\t%d %s:%s\n", ++index, pn->Key.c_str(), pn->Value.c_str());
				pn = pn->next;
			}
		}
		//no_print_body
		if (cmd.HasParam("no_print_body"))
			printf("body[%u bytes]\n",body.size());
		else
			printf("body[%u bytes]:%s\n", body.size(), body.c_str());
		printf("\n");

	}
	
	ts.ReSet();

	sim::HttpResponse response;
	if (!sim::HttpClient::Request(method, url, body, response, timeout, pex, SyncProgress,NULL))
	{
		printf("\n");
		printf("Request error use %llu ms\n", ts.Get());
		getchar();
		return -1;
	}
	
	if (cmd.HasParam("print"))
	{
		printf("\n");
		printf("end request :%s %s \n", method.c_str(), url.c_str());
		printf("response:%s %s use %llu ms\n", response.Status.c_str(), response.Reason.c_str(), ts.Get());
	}

	if (f)
	{
		fflush(f);
		fclose(f);
	}
	getchar();
	if (pex)
		delete pex;
	return 0;
}