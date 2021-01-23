#include "Async.hpp"

sim::AsyncEventService service;
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
#include<iomanip>  
char *recvbuff = NULL;
char *buff = NULL;
void AsyncEventHandlerCB(sim::BaseAsyncSocket* sock, sim::Event* e, void* pdata)
{
	//printf("event %p flag 0x%08X\n", e,e->event_flag);
	SIM_LINFO("event " << (void*)e << " flag " << e->FormatFlag());
	if (e)
	{
		if (e->event_flag & ASYNC_FLAG_ERROR)
		{
			SIM_LERROR("erro event " << e->error << " flag " << e->FormatFlag());
			service.Destroy(&sock);
			return;
		}
		else
		{
			if (e->event_flag == ASYNC_FLAG_CONNECT)
			{
				SIM_LINFO("Connect ok");
				buff = new char[12]{ "hello world" };
				recvbuff = new char[128]{ 0 };
				sock->Send(buff, ::strlen(buff));
				sock->Recv(recvbuff, 128);
			}
			else if (e->event_flag == ASYNC_FLAG_SEND)
			{
				SIM_LINFO("send ok " << e->cache.ptr << " " << e->cache.size << " " << e->bytes_transfered);
				sock->Send(e->cache.ptr, e->cache.size);
			}
			else if (e->event_flag == ASYNC_FLAG_RECV)
			{
				SIM_LINFO("recv ok " << e->cache.ptr << " " << e->cache.size << " " << e->bytes_transfered);
				sock->Recv(e->cache.ptr, e->cache.size);
			}
		}

		if (e->event_flag & ASYNC_FLAG_RELEASE)
		{
			SIM_LERROR("Release Sock");
			//ÄÚ´æÊÍ·Å
			if (recvbuff)
				delete[]recvbuff;
			if (buff)
				delete[]buff;
		}
	}
}
int main(int argc, char* argv[])
{
	SIM_LOG_CONFIG(sim::LDebug, NULL, NULL);
	SIM_FUNC_DEBUG();

	sim::AsyncSocket *sock = service.CreateByType(sim::TCP);
	sock->SetHandler(AsyncEventHandlerCB, sock);
	//sock->Bind(NULL, 0);
	//sock->GetHostByName("www.baidu.com", GetHostByNameCallBack, sock);
	sim::SockRet ret = sock->Connect("127.0.0.1", 10000);
	SIM_LDEBUG("Connect 127.0.0.1,ret=" << ret);
	service.Run(1000);
	getchar();
	sock->Close();
	return 0;
}

