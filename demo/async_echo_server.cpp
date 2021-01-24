#include "Async.hpp"

sim::AsyncEventService service;
#include<iomanip>  
char* recvbuff = NULL;
char* buff = NULL;
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
			if (e->cache.ptr)
				delete[]e->cache.ptr;
			return;
		}
		else
		{
			if (e->event_flag == ASYNC_FLAG_ACCEPT)
			{
				sock->Accept(NULL);

				SIM_LINFO("accept "<<e->accept_client->GetSocket());
				const int buff_size = 102400;
				char* pbuff = new char[buff_size];
				e->accept_client->Recv(pbuff, buff_size);
				//service.Destroy(&e->accept_client);
			}
			else if (e->event_flag == ASYNC_FLAG_SEND)
			{
				SIM_LINFO("send ok " << std::string(e->cache.ptr, e->bytes_transfered) << " " << e->bytes_transfered);
				/*if (e->cache.ptr)
					delete[]e->cache.ptr;*/
				sock->Recv(e->cache.ptr, e->cache.size);
			}
			else if (e->event_flag == ASYNC_FLAG_RECV)
			{
				SIM_LINFO("recv ok " <<std::string(e->cache.ptr, e->bytes_transfered) << " " << e->bytes_transfered);
				sock->Send(e->cache.ptr, e->bytes_transfered);
			}
		}

		if (e->event_flag & ASYNC_FLAG_RELEASE)
		{
			SIM_LERROR("Release Sock");
			
		}
	}
}
int main(int argc, char* argv[])
{
	SIM_LOG_CONFIG(sim::LInfo, NULL, NULL);
	SIM_FUNC_DEBUG();

	sim::AsyncSocket* sock = service.CreateByType(sim::TCP);
	sock->SetHandler(AsyncEventHandlerCB, sock);
	sock->SetReusePort(true);
	sim::SockRet ret = sock->Bind(NULL, 10000);
	SIM_LINFO("Bind 127.0.0.1,ret=" << ret);
	ret = sock->Listen(1024);
	SIM_LINFO("Listen 127.0.0.1,ret=" << ret);
	ret = sock->Accept(NULL);
	SIM_LINFO("Accept 127.0.0.1,s=" << ret);
	//service.Destroy(&sock);
	service.Run(1000);
	return 0;
}

