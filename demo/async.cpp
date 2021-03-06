#include "Async.hpp"
#include <string>
void AcceptHandler(sim::AsyncHandle handle, sim::AsyncHandle client, void*data)
{
	printf("%d accept %d\n", handle, client);
}
void ConnectHandler(sim::AsyncHandle handle, void*data)
{
	printf("%d connect \n", handle);
}
void RecvDataHandler(sim::AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
{

	sim::SimAsync*pasync = (sim::SimAsync*)data;
	printf("%d recv %u %s\n", handle, buff_len,std::string(buff,buff_len).c_str());
	pasync->Send(handle, buff, buff_len);

}
void CloseHandler(sim::AsyncHandle handle, sim::AsyncCloseReason reason, int error, void*data)
{
	SIM_LERROR("close " << handle << " error " << error << " reason " << reason);
}
int main(int argc, char*argv[])
{

	sim::SimAsync async;
	sim::AsyncHandle handle = async.CreateTcpHandle();
	async.ConvertToSSL(handle, true, true);
	async.SetSSLKeyFile(handle,"cert.pem", "key.pem");
	async.SetAcceptHandler(handle, AcceptHandler,NULL);
	async.SetConnectHandler(handle, ConnectHandler, NULL);
	async.SetRecvDataHandler(handle, RecvDataHandler, &async);
	async.SetCloseHandler(handle, CloseHandler, NULL);
	async.AddTcpServer(handle, NULL, 8080);

	while (true)
	{
		async.Poll(100);
	}
	return 0;

}