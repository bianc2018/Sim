#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"

sim::CmdLineParser cmd;
sim::Str SendData;
void SendDataToServer(sim::AsyncHandle handle)
{
	sim::WebSocketFrameHead resFrame;
	resFrame.fin = true;
	resFrame.mask = true;
	resFrame.opcode = SIM_WS_OPCODE_TEXT;
	resFrame.payload_length = SendData.size();
	sim::PayLoadLength_t payload_offset = 0;
	sim::GlobalPoll<sim::AsyncHttp>::Get().Send(handle, resFrame, payload_offset, SendData.c_str(), SendData.size());
}
void ASYNC_HTTP_RESPONSE_HANDLE(sim::AsyncHandle handle, sim::HttpResponseHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	printf("handle %d response status %s reason %s version %s content_lenght %llu offset %llu buff %s len %llu\n",
		handle, Head->Status.c_str(), Head->Reason.c_str(), Head->Version.c_str(), content_lenght, offset, buff, len);

	SendDataToServer(handle);
}
void ASYNC_WS_HANDLER(sim::AsyncHandle handle, sim::WebSocketFrameHead* pFrame,
	sim::PayLoadLength_t payload_offset,
	const char*payload_data, sim::PayLoadLength_t data_len,
	void* pdata)
{
	printf("handle %d ws frame fin %d mask %d masking_key[%x,%x,%x,%x] opcode %u"
		" content_lenght %llu offset %llu buff %s len %llu\n",
		handle, pFrame->fin, pFrame->mask, pFrame->masking_key[0]
		, pFrame->masking_key[1], pFrame->masking_key[2], pFrame->masking_key[3]
		, pFrame->opcode,
		pFrame->payload_length, payload_offset, payload_data, data_len);

	SendDataToServer(handle);
}
void print_help()
{
	printf("usg:-l URL 监听地址 -s 发送的数据\n");
}
int main(int argc, char* argv[])
{
	//SIM_LOG_CONSOLE(sim::LDebug);
	cmd.Parser(argc, argv);
	if (cmd.HasParam("h") || cmd.HasParam("help"))
	{
		print_help();
		return -1;
	}
	SendData = cmd.GetCmdLineParams("s", "hello world");
	sim::AsyncHttp &http = sim::GlobalPoll<sim::AsyncHttp>::Get();
	sim::AsyncHandle handle = http.CreateSession();
	http.SetHttpResponseHandler(handle, ASYNC_HTTP_RESPONSE_HANDLE, NULL);
	http.SetWsFrameHandler(handle, ASYNC_WS_HANDLER, NULL);
	http.Connect(handle, cmd.GetCmdLineParams("l", "ws://49.234.220.213:8080").c_str());
	sim::GlobalPoll<sim::AsyncHttp>::Wait();
	return 0;
}