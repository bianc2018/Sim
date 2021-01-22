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
	SIM_LINFO("event "<<(void*)e<< " flag "<<e->FormatFlag());
	if (e)
	{
		if (e->event_flag == ASYNC_FLAG_RECV && !(e->event_flag&ASYNC_FLAG_ERROR))
		{
			printf("recv ok %s %u %ld\n", e->cache.ptr, e->cache.size, e->bytes_transfered);
			sock->Recv(e->cache.ptr, e->cache.size);
		}
		if (e->event_flag == ASYNC_FLAG_SEND && !(e->event_flag&ASYNC_FLAG_ERROR))
		{
			printf("send ok %s %u %ld\n", e->cache.ptr, e->cache.size, e->bytes_transfered);
			//delete[] e->cache.ptr;
			//关闭之后不可用
			//sock->Close();
			//sock->Send(e->cache.ptr, e->cache.size);
		}
		if (e->event_flag == ASYNC_FLAG_CONNECT && !(e->event_flag&ASYNC_FLAG_ERROR))
		{
			printf("Connect happen ok %d\n", e->error);
			buff = new char[12]{ "hello world" };
			recvbuff = new char[128] {0};
			sock->Send(buff, ::strlen(buff));
			sock->Recv(recvbuff, 128);
			//sock->Close();
		}
		//释放
		if (e->event_flag&ASYNC_FLAG_RELEASE)
		{
			printf("socket Release %d %p\n", e->error,sock);
			if(recvbuff)
				delete[]recvbuff;
			if (buff)
				delete[]buff;
			//sock->Close();
			return;
		}
		if (e->event_flag&ASYNC_FLAG_DISCONNECT)
		{
			printf("socket offline %d\n", e->error);
			//delete[] e->cache.ptr;
			//sock->Close();
			sock->Close();
			return;
		}
		if (e->event_flag&ASYNC_FLAG_ERROR)
		{
			printf("erro happen %d\n", e->error);
			return;
		}
	}
}
int main(int argc, char* argv[])
{
	SIM_LOG_CONFIG(sim::LInfo, NULL, NULL);
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

