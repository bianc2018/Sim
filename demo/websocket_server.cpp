#include "AsyncHttp.hpp"
#include "GlobalPoll.hpp"
#include "CmdLineParser.hpp"

sim::CmdLineParser cmd;

void ASYNC_HTTP_REQUEST_HANDLE(sim::AsyncHandle handle, sim::HttpRequestHead *Head,
	sim::ContentLength_t content_lenght, sim::ContentLength_t offset, const char*buff, sim::ContentLength_t len,
	void *pdata)
{
	printf("handle %d request method %s url %s version %s content_lenght %llu offset %llu buff %s len %llu\n",
		handle, Head->Method.c_str(), Head->Url.c_str(), Head->Version.c_str(), content_lenght, offset, buff, len);
	
}
void ASYNC_WS_HANDLER(sim::AsyncHandle handle, sim::WebSocketFrameHead* pFrame,
	sim::PayLoadLength_t payload_offset,
	const char*payload_data, sim::PayLoadLength_t data_len,
	void* pdata)
{
	printf("handle %d ws frame fin %d mask %d masking_key[%x,%x,%x,%x] opcode %u"
		" content_lenght %llu offset %llu buff %s len %llu\n",
		handle, pFrame->fin, pFrame->mask,pFrame->masking_key[0]
		, pFrame->masking_key[1], pFrame->masking_key[2], pFrame->masking_key[3]
		, pFrame->opcode,
		pFrame->payload_length, payload_offset, payload_data, data_len);
	sim::WebSocketFrameHead resFrame;
	resFrame.fin = true;
	resFrame.mask = true;
	resFrame.opcode = SIM_WS_OPCODE_TEXT;
	resFrame.payload_length = data_len;
	sim::GlobalPoll<sim::AsyncHttp>::Get().Send(handle, resFrame, payload_offset, payload_data, data_len);
}
void print_help()
{
	printf("usg:-l URL ¼àÌýµØÖ· \n");
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
	sim::AsyncHttp &http = sim::GlobalPoll<sim::AsyncHttp>::Get();
	sim::AsyncHandle handle = http.CreateSession();
	http.SetHttpRequestHandler(handle, ASYNC_HTTP_REQUEST_HANDLE, NULL);
	http.SetWsFrameHandler(handle, ASYNC_WS_HANDLER, NULL);
	http.Listen(handle, cmd.GetCmdLineParams("l", "ws://127.0.0.1:8080").c_str(), "cert.pem", "key.pem");
	sim::GlobalPoll<sim::AsyncHttp>::Wait();
	return 0;
}