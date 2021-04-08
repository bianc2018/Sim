/*
	http请求测试
*/
#include <stdio.h>
#include "HttpApi.hpp"
#include "CmdLineParser.hpp"
sim::CmdLineParser cmd;
sim::HttpMap *pex = NULL;
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
						pex = new sim::HttpMap;
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
				pex = new sim::HttpMap;
			pex->Append(key, value);
		}
	}
}
void print_help()
{
	printf("usg:-m 请求方法(GET、POST) -u URL -body 消息体 -timeout 超时单位毫秒 -log 输出日志 -save filename 保存报文 -print 打印详情 -no_print_body 不打印消息体 -ext_head 拓展头 \"key:value:key1:value1\"\n");
}

int main(int argc, char* argv[])
{
#if 0
	cmd.InitCmdLineParams("u", "http://www.baidu.com")
		.InitCmdLineParams("print","")
		.InitCmdLineParams("timeout", 10000)
		.InitCmdLineParams("no_print_body","");
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
	sim::Str method = cmd.GetCmdLineParams("m", "GET");
	sim::Str url = cmd.GetCmdLineParams("u", "");
	sim::Str body = cmd.GetCmdLineParams("body", "");
	int timeout = cmd.GetCmdLineParams("timeout", -1);
	init_exhead(cmd.GetCmdLineParams("ext_head", ""));

	//打印
	if (cmd.HasParam("print"))
	{
		printf("start request :%s %s timeout[%d ms]\n", method.c_str(), url.c_str(), timeout);
		if (pex)
		{
			printf("ex head:\n");
			sim::HttpMap::HttpMapNode *pn = pex->pHead;
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

	}
	sim::TimeSpan ts;//计时

	sim::HttpResponse response;
	if (!sim::HttpClient::Request(method, url, body, response, timeout, pex))
	{
		printf("Request error use %llu ms\n", ts.Get());
		getchar();
		return -1;
	}
	
	if (cmd.HasParam("print"))
	{
		printf("end request :%s %s \n", method.c_str(), url.c_str());
		printf("response:%s %s use %llu ms\n", response.Status.c_str(), response.Reason.c_str(), ts.Get());
		printf("response head:\n");
		sim::HttpMap::HttpMapNode *pn = response.Head.pHead;
		int index = 0;
		while (pn)
		{
			printf("\t%d %s:%s\n", ++index, pn->Key.c_str(), pn->Value.c_str());
			pn = pn->next;
		}
		//no_print_body
		if (cmd.HasParam("no_print_body"))
			printf("response body[%u bytes]\n", response.Body.size());
		else
			printf("response body[%u bytes]:%s\n", response.Body.size(), response.Body.c_str());

	}

	sim::Str filename = cmd.GetCmdLineParams("save", "");
	if (!filename.empty())
	{
		FILE*f = fopen(filename.c_str(), "wb+");
		if (f == NULL)
		{
			printf("save to %s fail\n", filename.c_str());
			return -1;
		}
		fwrite(response.Body.c_str(), response.Body.size(), 1, f);
		fflush(f);
		fclose(f);
	}
	getchar();
	if (pex)
		delete pex;
	return 0;
}