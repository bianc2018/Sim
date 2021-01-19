/*
	异步socket 由iocp封装
*/
#ifndef SIM_IOCP_HPP_
#define SIM_IOCP_HPP_
#include "Async.hpp"
namespace sim
{
	class IocpAsyncEventService :public BaseAsyncEventService
	{
	public:
		IocpAsyncEventService(unsigned int thread_num=0);
		virtual ~IocpAsyncEventService() {};

		virtual SockRet AddSock(BaseAsyncSocket*psock) { return 0; };
		virtual SockRet RemoveSock(BaseAsyncSocket*psock) { return 0; };
		virtual SockRet Run() { return 0; };
	private:

	};

	//class BaseAsyncSocket;
	class IocpAsyncSocket:public BaseAsyncSocket
	{
	public:
		//构造函数
		IocpAsyncSocket();

		IocpAsyncSocket(IocpAsyncEventService* service, SOCKET sock);
		// SOCK_STREAM tcp SOCK_DGRAM udp
		IocpAsyncSocket(IocpAsyncEventService* service, SockType type);

		IocpAsyncSocket(IocpAsyncEventService* service, int af, int type, int protocol);
	private:

	};
}
#endif