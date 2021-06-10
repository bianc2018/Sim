/*
	�첽ssh ʵ��
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
		//����һ���յĻỰ
		virtual AsyncHandle CreateSession()
		{

		}

		//���ûص�
	};
}
#endif