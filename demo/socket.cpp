#include "Socket.hpp"
bool GetHostByNameCallBack(const char* ip, void* pdata)
{
	sim::Socket* s = (sim::Socket*)pdata;
	sim::SockRet ret= s->Connect(ip, 80);
	printf("ip:%s %d\n", ip,ret);
	if (ret == 0)
	{
		const int buff_size = 1024;
		char buff[buff_size] = { 0 };
		ret = s->Recv(buff, buff_size);
		printf("recv [%d]%s\n", ret, buff);
	}
	return true;
}
int main(int argc, char* argv[])
{
	sim::Socket::Init();

	sim::Socket s(sim::TCP);
	s.GetHostByName("www.baidu.com", GetHostByNameCallBack, &s);
	getchar();
	return 0;
}