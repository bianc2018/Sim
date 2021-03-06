#include "Socket.hpp"

//#include <MSWSock.h>
//#include <WS2tcpip.h>

bool GetHostByNameCallBack(const char* ip, void* pdata)
{
	sim::Socket* s = (sim::Socket*)pdata;
	sim::SockRet ret= s->ConnectTimeOut(ip, 80,1000);
	printf("ip:%s %d\n", ip,ret);
	if (ret == 0)
	{
		const int buff_size = 1024;
		char buff[buff_size] = { 0 };
		ret = s->Recv(buff, buff_size,10000);
		printf("recv [%d]%s\n", ret, buff);
		ret = s->Send(buff, buff_size);
		printf("send [%d]%s\n", ret, buff);
	}
	return true;
}
int main(int argc, char* argv[])
{
	sim::Socket::Init();
	//WSAID_CONNECTEX;
	sim::Socket s(sim::TCP);
	
	sim::SockRet ret = s.ConnectTimeOut("127.0.0.1", 80, -1);
	//s.SetNonBlock(true);
	s.GetHostByName("www.baidu.com", GetHostByNameCallBack, &s);
	getchar();
	return 0;
}