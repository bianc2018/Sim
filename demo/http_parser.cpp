/*
	http ½âÎö
*/
#include "HttpParser.hpp"
void HTTP_REQUEST_HANDLER(sim::HttpParser*ctx, sim::HttpRequest *request, void *pdata)
{
	printf("Get\n");
}
int main(int argc, char*argv[])
{
	//sim::HttpParser ctx(HTTP_REQUEST_HANDLER,NULL);
	sim::HttpRequest req;
	req.Method = "GET";
	req.Url = "/";
	req.Version = "HTTP/1.1";
	req.Head.Append("Accept", "text/plain");
	req.Body = "123132132132131313132";
	sim::Str data =  sim::HttpParser::PrintRequest(&req);
	printf("req:\n%s\n", data.c_str());
	data += data;
	data += data;
	sim::HttpParser ctx(HTTP_REQUEST_HANDLER,NULL);
	ctx.Parser(data.c_str(), data.size());
	getchar();
	return 0;

}