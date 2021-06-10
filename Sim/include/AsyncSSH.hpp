/*
	异步ssh 实现
*/
#ifndef SIM_ASYNC_SSH_HPP_
#define SIM_ASYNC_SSH_HPP_

#include "Async.hpp"
#define SIM_PARSER_MULTI_THREAD
#include "SSHv2.hpp"

namespace sim
{
	class AsyncSsh;

	class SshSession
	{
	public:
		AsyncSsh& ssh_;
	};

	class AsyncSsh :protected SimAsync
	{
	public:
		virtual int Poll(unsigned int wait_ms)
		{
			return SimAsync::Poll(wait_ms);
		}
		//生成一个空的会话
		virtual AsyncHandle CreateSession()
		{

		}

		//设置回调
	};
}
#endif