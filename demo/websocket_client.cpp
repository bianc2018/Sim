#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"
#include <iostream>

sim::CmdLineParser cmd;
//输入
class Input
{
public:
	sim::AsyncHandle handle;
public:
	void Poll(unsigned int wait_ms = 0)
	{
		sim::Str data;
		//printf("\nhandle %d >>", handle);
		std::cin >> data;
		sim::GlobalPoll<sim::AsyncHttp>::Get().SendWebSocketFrame(handle, data.c_str(),data.size());
	}
private:

};

void ASYNC_HTTP_RESPONSE_HANDLE(sim::AsyncHandle handle, sim::HttpResponseHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	printf("handle %d connect ok\n", handle);
	if(cmd.HasParam("i"))
		sim::GlobalPoll<Input>::Get().handle = handle;
}
void ASYNC_WS_HANDLER(sim::AsyncHandle handle, sim::WebSocketFrameHead* pFrame,
	sim::PayLoadLength_t payload_offset,
	const char*payload_data, sim::PayLoadLength_t data_len,
	void* pdata)
{
	/*printf("handle %d ws frame fin %d mask %d masking_key[%x,%x,%x,%x] opcode %u"
		" content_lenght %llu offset %llu buff %s len %llu\n",
		handle, pFrame->fin, pFrame->mask, pFrame->masking_key[0]
		, pFrame->masking_key[1], pFrame->masking_key[2], pFrame->masking_key[3]
		, pFrame->opcode,
		pFrame->payload_length, payload_offset, payload_data, data_len);*/
	printf("handle %d recv[%llu] %s\n",
		handle, data_len,sim::Str(payload_data, data_len).c_str());
}
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void* data)
{
	printf("handle %d closed\n",
		handle);
}
void print_help()
{
	printf("usg:-l URL 链接 -i 交互\n");
}
int main(int argc, char* argv[])
{
#ifdef OS_WINDOWS
	system("chcp 65001");//显示utf8
	cmd.InitCmdLineParams("l", " ws://49.234.18.41:8866")
		.InitCmdLineParams("i", "");
#endif
	

	//SIM_LOG_CONSOLE(sim::LDebug);
	cmd.Parser(argc, argv);
	if (cmd.HasParam("h") || cmd.HasParam("help")||!cmd.HasParam("l"))
	{
		print_help();
		return -1;
	}

	sim::AsyncHttp &http = sim::GlobalPoll<sim::AsyncHttp>::Get();
	sim::AsyncHandle handle = http.CreateSession();
	http.SetHttpResponseHandler(handle, ASYNC_HTTP_RESPONSE_HANDLE, NULL);
	http.SetWsFrameHandler(handle, ASYNC_WS_HANDLER, NULL);
	http.SetCloseHandler(handle, CloseHandler, NULL);

	http.Connect(handle, cmd.GetCmdLineParams("l", "").c_str());

	sim::GlobalPoll<sim::AsyncHttp>::Wait();
	return 0;
}