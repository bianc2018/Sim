/*
	时间相关函数 
*/
#ifndef SIM_TIMER_HPP_
#define SIM_TIMER_HPP_
#include<time.h>
#include <stdio.h>
#include <sys/timeb.h>

#include "RbTree.hpp"
#include "Mutex.hpp"
#include "Queue.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
#define OS_WINDOWS
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  
#endif
#include <WinSock2.h>
#include <Windows.h>

#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif  
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#else
#error "不支持的平台"
#endif

namespace sim
{
	//时间函数
	class Time
	{
#ifdef OS_WINDOWS
		class WsInit
		{
		public:
			WsInit()
			{
				WSADATA wsaData;
				WSAStartup(MAKEWORD(2, 2), &wsaData);
			}
			~WsInit()
			{
				//终止 DLL 的使用
				WSACleanup();
			}
		};
#endif
	public:
		Time() {}
		~Time() {}
	public:
		//程序启动之后的毫秒计时
		static unsigned long long GetTickMS()
		{

#ifdef _MSC_VER
			_timeb timebuffer;
			_ftime(&timebuffer);
			unsigned long long ret = timebuffer.time;
			ret = ret * 1000 + timebuffer.millitm;
			return ret;
#else
			timeval tv;
			::gettimeofday(&tv, 0);
			unsigned long long ret = tv.tv_sec;
			return ret * 1000 + tv.tv_usec / 1000;
#endif
		}
		static void Sleep(unsigned int ms)
		{
#ifdef OS_WINDOWS
			static WsInit ws;
#endif
			struct timeval tv;
			tv.tv_sec = ms / 1000;
			tv.tv_usec = (ms % 1000) * 1000;

			int sock = socket(AF_INET, SOCK_STREAM, 0);
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(sock, &fds);

			int ret =select(sock +1, NULL, &fds, NULL, &tv);
#ifdef OS_WINDOWS
			::closesocket(sock);
#endif

#ifdef OS_LINUX
			::close(sock);
#endif
		}
	private:

	};

	class TimeSpan
	{
	public:
		TimeSpan() :start_(Time::GetTickMS())
		{

		}
		void ReSet()
		{
			start_ = Time::GetTickMS();
		}
		unsigned long long Get()
		{
			return Time::GetTickMS() - start_;
		}
		~TimeSpan() {};

	private:
		unsigned long long start_;
	};

	typedef unsigned long long timer_id;
	typedef void(*TIMER_OUT_HANDLER)(timer_id timer_id, void *userdata);
	//计时器管理
	class Timer
	{
	public:
		Timer(unsigned long long interval_ms, TIMER_OUT_HANDLER handler, void *userdata,bool is_once =true)
			:interval_ms_(interval_ms)
			, timer_id_(CreateTimerId())
			, userdata_(userdata)
			, handler_(handler)
			, is_once_(is_once)
		{

		}
		Timer()
			:interval_ms_(0)
			, timer_id_(0)
			, userdata_(NULL)
			, handler_(NULL)
			, is_once_(true)
		{

		}
		~Timer()
		{

		}

		void OnTimerOut()
		{
			if (handler_)
				handler_(timer_id_, userdata_);
		}
		timer_id GetID() { return timer_id_; };
		bool IsOnce() { return is_once_; }
		unsigned long long GetInterval() {return interval_ms_;}
	private:
		static timer_id CreateTimerId()
		{
			static sim::Mutex _id_inc_lock;
			static timer_id  _id_inc = 0;

			sim::AutoMutex lk(_id_inc_lock);
			++_id_inc;
			if (_id_inc == 0)
				_id_inc = 1;
			return _id_inc;
		}
	private:
		unsigned long long interval_ms_;
		timer_id timer_id_;
		void *userdata_;
		bool is_once_;
		TIMER_OUT_HANDLER handler_;
	};

	//计时器管理
	class TimerMgr
	{
		struct TraverseData
		{
			Queue<unsigned long long>*pdelete_index;
			Queue<timer_id>*presult;
		};
		//不允许复制拷贝
		TimerMgr(const TimerMgr &other) {};
		TimerMgr operator=(const TimerMgr &other) {};
	public:
		TimerMgr() {};

		timer_id AddTimer(unsigned long long interval_ms, TIMER_OUT_HANDLER handler, void *userdata, bool is_once = true)
		{
			Timer t(interval_ms, handler, userdata, is_once);
			AddTimer(t);
			return t.GetID();
		}

		bool RemoveTimer(timer_id id)
		{
			AutoMutex lk(timers_lock_);
			return timers_.Del(id);
		}

		void Poll()
		{
			Queue<timer_id>result;
			Queue<Timer>timers;

			unsigned int size = GetTimeOutTriggers(&result);
			if (size > 0)
			{
				{
					AutoMutex lk(timers_lock_);
					timer_id id = 0;
					while (result.PopFront(&id))
					{
						Timer tmp;
						if (timers_.Find(id, &tmp))
						{
							timers.PushBack(tmp);
						}
					}
				}
				//处理
				Timer tmp;
				while (timers.PopFront(&tmp))
				{
					tmp.OnTimerOut();
					if (tmp.IsOnce())
					{
						//一次性的删除
						RemoveTimer(tmp.GetID());
					}
					else
					{
						AddTriggers(tmp);
					}
				}
			}
		}

	private:
		bool AddTimer(Timer &t)
		{
			{
				AutoMutex lk(timers_lock_);
				timers_.Add(t.GetID(), t);
			}
			return AddTriggers(t);
		}

		bool AddTriggers(Timer &t)
		{
			{
				AutoMutex lk(timer_triggers_lock_);
				unsigned long long interval = t.GetInterval() + Time::GetTickMS();
				Queue<timer_id>*tq=NULL;
				if (timer_triggers_.Find(interval, &tq)&&tq)
				{
					tq->PushBack(t.GetID());
				}
				else
				{
					tq = new Queue<timer_id>();
					tq->PushBack(t.GetID());
					timer_triggers_.Add(interval, tq);
				}

			}
			return true;
		}

		static bool TimeOutTriggersTreeTraverseFunc(RbTreeNode<Queue<timer_id>*>* Now, void*pdata)
		{
			//超时
			if (Now->Key < Time::GetTickMS())
			{
				TraverseData *pd = (TraverseData *)(pdata);
				pd->pdelete_index->PushBack(Now->Key);
				if (Now->Data)
				{
					QueueNode<timer_id>*pn = Now->Data->Next(NULL);
					while (pn)
					{
						pd->presult->PushBack(pn->data);
						pn = Now->Data->Next(pn);
					}
					delete Now->Data;
					Now->Data = NULL;
				}
				//继续
				return true;
			}
			else
			{
				//不再遍历
				return false;
			}

		}
		
		unsigned int GetTimeOutTriggers(Queue<timer_id>*result)
		{
			if (NULL == result)
				return 0;
			Queue<unsigned long long> delete_index;
			TraverseData pdata;
			pdata.pdelete_index = &delete_index;
			pdata.presult = result;
			{
				AutoMutex lk(timer_triggers_lock_);
				//TraverseTypeLDR
				timer_triggers_.TraverseTree(TimeOutTriggersTreeTraverseFunc, (void*)&pdata, TraverseTypeLDR);
				//删除
				QueueNode<unsigned long long>*pn=delete_index.Next(NULL);
				while (pn)
				{
					timer_triggers_.Del(pn->data);
					pn = delete_index.Next(pn);
				}
			}
			return result->Size();
		}

	private:
		//id->timer
		Mutex timers_lock_;
		RbTree<Timer> timers_;
		//触发器
		Mutex timer_triggers_lock_;
		RbTree<Queue<timer_id>*> timer_triggers_;
		
	};
}
#endif