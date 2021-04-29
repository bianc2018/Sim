/*
	http请求测试
*/
#include <stdio.h>
#include "Timer.hpp"
#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"

sim::CmdLineParser cmd;
sim::TimeSpan ts;//计时
sim::KvMap *pex = NULL;
FILE*f = NULL;
bool is_print_head = true;
sim::Str method, url, body,path="/";
sim::timer_id t_id=0;//定时器id
int timeout;
bool is_complete = false;

sim::Str print_bytes(double bytes)
{
	const int buff_size = 1024;
	char buff[buff_size] = { 0 };
	if (bytes < 1024)
	{
		snprintf(buff, buff_size, "%0.3lf bytes", bytes);
	}
	else if (bytes < 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf kb", double(bytes) / 1024);
	}
	else if (bytes < 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf mb", double(bytes) / (1024 * 1024));
	}
	else if (bytes > 1024 * 1024 * 1024)
	{
		snprintf(buff, buff_size, "%0.3lf gb", double(bytes) / (1024 * 1024 * 1024));
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

void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void* data)
{
	if (false == is_complete)
	{
		static sim::Str reasons[3] = { "CloseError","CloseActive","ClosePassive" };
		printf("request error:socket %d close reason %s error %d\n", handle, reasons[reason].c_str(), error);
	}
	sim::GlobalPoll<sim::AsyncHttp>::Exit();
}

void TIMER_OUT_HANDLER(sim::timer_id timer_id, void *userdata)
{
	//关闭
	sim::AsyncHandle handle = (sim::AsyncHandle)((long long)userdata);
	sim::GlobalPoll<sim::AsyncHttp>::Get().Close(handle);
	printf("request time out \n");
}

void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	if (timeout >= 0)
	{
		sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(t_id);
		t_id = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(timeout, TIMER_OUT_HANDLER, (void*)handle);
	}

	if (cmd.HasParam("print"))
	{
		printf("connect ok\n");
	}
	sim::HttpRequestHead request;
	request.Method = method;
	request.Url = path;
	request.Head.Append(SIM_HTTP_CON, "Close");
	sim::ContentLength_t offset=0;
	sim::GlobalPoll<sim::AsyncHttp>::Get().Send(handle, request, body.size(), offset, body.c_str(), body.size());

}

void ASYNC_HTTP_RESPONSE_HANDLE(sim::AsyncHandle handle, sim::HttpResponseHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	if (timeout >= 0)
	{
		sim::GlobalPoll<sim::TimerMgr>::Get().RemoveTimer(t_id);
		t_id = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(timeout, TIMER_OUT_HANDLER, (void*)handle);
	}

	if (cmd.HasParam("print"))
	{
		if (is_print_head)
		{
			printf("response head :%s %s use %llu ms\n", Head->Status.c_str(), Head->Reason.c_str(), ts.Get());
			printf("response head:\n");
			sim::KvMap::KvMapNode *pn = Head->Head.pHead;
			int index = 0;
			while (pn)
			{
				printf("\t%d %s:%s\n", ++index, pn->Key.c_str(), pn->Value.c_str());
				pn = pn->next;
			}
			printf("response body[%s (%llu bytes)]\n",
				print_bytes(content_lenght).c_str(), content_lenght);
			is_print_head = false;
		}

		unsigned long long use_ms = ts.Get();
		//进度百分比
		double ps = content_lenght == 0 ? 100 : (double(offset+ len) / content_lenght) * 100;
		double speed = use_ms == 0 ? 0 : double(offset + len) / (use_ms / double(1000));
		printf("\rrecv body(%%%0.3lf) %s [total %s] use %0.3lf s speed %s/s",
			ps,
			print_bytes(offset+ len).c_str(), print_bytes(content_lenght).c_str(),
			double(use_ms) / 1000,
			print_bytes(speed).c_str());

	}
	if (f)
	{
		fwrite(buff,len, 1, f);
	}
	if (sim::HttpParser::IsComplete(Head->Head,offset,len))
	{
		is_complete = true;
		if (cmd.HasParam("print"))
		{
			printf("\n");
			printf("end request :%s %s offset %llu len %llu content_lenght %llu\n", method.c_str()
				, url.c_str(), offset, len, content_lenght);
			printf("response:%s %s use %llu ms\n", Head->Status.c_str(), Head->Reason.c_str(), ts.Get());
		}
		//close
		sim::GlobalPoll<sim::AsyncHttp>::Exit();
	}
	//return true;
}


void print_help()
{
	printf("usg:-m 请求方法(GET、POST) -u URL -body 消息体 -timeout 超时单位毫秒 -log 输出日志 -save filename 保存报文 -print 打印详情 -no_print_body 不打印消息体 -ext_head 拓展头 \"key:value:key1:value1\"\n");
}

int main(int argc, char* argv[])
{
#if 1
	cmd.InitCmdLineParams("u", "https://github.com")
		.InitCmdLineParams("print", "")
		//.InitCmdLineParams("m", "HEAD")
		//.InitCmdLineParams("log", "")
		.InitCmdLineParams("timeout", -1)
		.InitCmdLineParams("no_print_body", "")
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

	method = cmd.GetCmdLineParams("m", "GET");
	url = cmd.GetCmdLineParams("u", "");
	body = cmd.GetCmdLineParams("body", "");
	timeout = cmd.GetCmdLineParams("timeout", -1);
	init_exhead(cmd.GetCmdLineParams("ext_head", ""));
	sim::HttpUrl out;
	if (!sim::HttpParser::ParserUrl(url, out))
		return false;
	path = out.path;

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
			printf("body[%u bytes]\n", body.size());
		else
			printf("body[%u bytes]:%s\n", body.size(), body.c_str());
		printf("\n");

	}

	ts.ReSet();
	sim::AsyncHttp &http = sim::GlobalPoll<sim::AsyncHttp>::Get();
	sim::AsyncHandle handle=http.CreateSession();
	http.SetConnectHandler(handle,ConnectHandler, NULL);
	http.SetHttpResponseHandler(handle, ASYNC_HTTP_RESPONSE_HANDLE, NULL);
	http.SetCloseHandler(handle, CloseHandler, NULL);
	if (timeout >= 0)
		t_id = sim::GlobalPoll<sim::TimerMgr>::Get().AddTimer(timeout, TIMER_OUT_HANDLER, (void*)handle);
	if (!http.Connect(handle,url.c_str()))
	{
		printf("\n");
		printf("Request error use %llu ms\n", ts.Get());
		getchar();
		return -1;
	}
	sim::GlobalPoll<sim::AsyncHttp>::Wait();


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