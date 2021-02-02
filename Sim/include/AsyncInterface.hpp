//interface
/*
	Òì²½ÍøÂç½Ó¿Ú
*/
#ifndef SIM_ASYNC_INTERFACE_HPP_
#define SIM_ASYNC_INTERFACE_HPP_
#include "EpollAsync.hpp"
#include "IocpAsync.hpp"

namespace sim
{
	enum AsyncType
	{
#ifdef OS_WINDOWS
		IOCP=0,
#endif
#ifdef OS_LINUX
		EPOLL=1,
#endif
		SELECT=2
	};
	Async * GetAsyncService(AsyncType type)
	{
		switch (type)
		{
#ifdef OS_WINDOWS
		case sim::IOCP:
			return new IocpAsync();
			break;
#endif
		case sim::SELECT:
			break;
		default:
			break;
		}
		return NULL;
	}
	void ReleaseAsyncService(Async **p)
	{
		if (p&&*p)
		{
			delete *p;
			*p = NULL;
		}
	}
}
#endif