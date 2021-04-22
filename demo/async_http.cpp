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
	sim::HttpResponseHead response;
	response.Head.Append("Connection", Head->Head.GetCase("Connection","Close"));
	sim::GlobalPoll<sim::AsyncHttp>::Get().Send(handle, response, content_lenght, offset, buff, len);
}
void print_help()
{
	printf("usg:-l URL ¼àÌýµØÖ· \n");
}
int main(int argc, char* argv[])
{
	//SIM_LOG_CONSOLE(sim::LDebug);
	cmd.Parser(argc, argv);

	sim::AsyncHttp &http = sim::GlobalPoll<sim::AsyncHttp>::Get();
	sim::AsyncHandle handle =  http.CreateSession();
	http.SetHttpRequestHandler(handle, ASYNC_HTTP_REQUEST_HANDLE, NULL);
	http.Listen(handle, cmd.GetCmdLineParams("l","http://:8080/").c_str(),"cert.pem","key.pem");
	sim::GlobalPoll<sim::AsyncHttp>::Wait();
	return 0;
}