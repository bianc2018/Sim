/*
	全局运行的
*/
#ifndef SIM_GLOBAL_POLL_HPP_
#define SIM_GLOBAL_POLL_HPP_
#include "TaskPool.hpp"
#ifndef SIM_GLOBAL_POLL_WAIT_MS
#define SIM_GLOBAL_POLL_WAIT_MS 1000
#endif
#ifndef SIM_GLOBAL_POLL_TH_NUM
#define SIM_GLOBAL_POLL_TH_NUM 1
#endif
namespace sim
{
	template<typename PollService, int th_num = SIM_GLOBAL_POLL_TH_NUM>
	class GlobalPoll
	{
		GlobalPoll()
			:pool(th_num), exit_flag_(false)
		{
			for (int i = 0; i < th_num; ++i)
				pool.Post(APoll, this, NULL);
		}

	public:
		~GlobalPoll()
		{
			exit_flag_ = true;
			pool.WaitAllDone(SIM_GLOBAL_POLL_WAIT_MS*3);
		}
		static PollService &Get()
		{
			return GetInstance().async_;
		}
		static void Wait()
		{
			while (!GetInstance().exit_flag_)
			{
				ThreadSleep(SIM_GLOBAL_POLL_WAIT_MS);
			}
			return ;
		}
		static void Exit()
		{
			GetInstance().exit_flag_ = true;
			return;
		}
	private:
		static void* APoll(void* lpParam)
		{
			GlobalPoll* pc = (GlobalPoll*)lpParam;
			while (!pc->exit_flag_)
			{
				pc->async_.Poll(SIM_GLOBAL_POLL_WAIT_MS);
			}
			return NULL;
		}
		static GlobalPoll &GetInstance()
		{
			static GlobalPoll<PollService, th_num> ctx;
			return ctx;
		}
	private:
		bool exit_flag_;
		sim::TaskPool pool;//线程池
		PollService async_;
	};
}
#endif