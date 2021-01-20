#include "Async.hpp"

//#include <MSWSock.h>
//#include <WS2tcpip.h>
bool GetHostByNameCallBack(const char* ip, void* pdata)
{
	//Sleep(1000);
	sim::AsyncSocket * s = (sim::AsyncSocket *)pdata;
	sim::SockRet ret = s->Connect("127.0.0.1", 10000);
	printf("ip:%s %d\n", ip, ret);
	if (ret == 0)
	{
		const int buff_size = 1024;
		char buff[buff_size] = { 0 };
		ret = s->Recv(buff, buff_size);
		printf("recv [%d]%s\n", ret, buff);
	}
	return false;
}
void AsyncEventHandlerCB(sim::BaseAsyncSocket* sock, sim::Event* e, void* pdata)
{
	printf("event %p \n", e);
}
int main(int argc, char* argv[])
{
	sim::AsyncEventService service;
	sim::AsyncSocket *sock = service.CreateByType(sim::TCP);
	sock->SetHandler(AsyncEventHandlerCB, sock);
	//sock->Bind(NULL, 0);
	//sock->GetHostByName("www.baidu.com", GetHostByNameCallBack, sock);
	sim::SockRet ret = sock->Connect("127.0.0.1", 10000);
	service.Run(1000);
	getchar();
	service.Release(sock);
	return 0;
}

