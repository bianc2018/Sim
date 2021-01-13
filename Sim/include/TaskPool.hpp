/*
* 任务池，动态拓展
*/
#ifndef SIM_TASK_POOL_HPP_
#define SIM_TASK_POOL_HPP_

#include <time.h>

#include "Mutex.hpp"
#include "Queue.hpp"
#include "Thread.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#define  ThreadSleep(x) \
	while(x)\
	{\
		Sleep(x);\
		break;\
	}
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <sys/time.h>
#include <unistd.h>
#define  ThreadSleep(x)\
	while(x)\
	{\
		usleep(x*1000)\
		break;\
	}
#endif

namespace sim
{
#ifdef WIN32
	inline int gettimeofday(struct timeval* tp, void* tzp)
	{
		time_t clock;
		struct tm tm;
		SYSTEMTIME wtm;
		GetLocalTime(&wtm);
		tm.tm_year = wtm.wYear - 1900;
		tm.tm_mon = wtm.wMonth - 1;
		tm.tm_mday = wtm.wDay;
		tm.tm_hour = wtm.wHour;
		tm.tm_min = wtm.wMinute;
		tm.tm_sec = wtm.wSecond;
		tm.tm_isdst = -1;
		clock = mktime(&tm);
		tp->tv_sec = clock;
		tp->tv_usec = wtm.wMilliseconds * 1000;
		return (0);
	}
#endif

	//内存分配函数
	typedef void* (*TaskPoolMalloc)(unsigned int size);
	typedef void (*TaskPoolFree)(void*);

	//任务函数
	typedef void* (*TaskFunc)(void*);
	typedef void (*TaskComplete)(void* pUserData,void*ret);

	//任务
	struct Task
	{
		//运行函数
		TaskFunc pFunc;
		//返回函数
		TaskComplete pComplete;
		//用户数据
		void* pUserData;
	};
	
	inline ThRet TaskThreadProc(LPVOID lpParam);
	//任务执行函数
	class TaskWorker
	{
		//声明为友元
		friend ThRet TaskThreadProc(LPVOID lpParam);
		//状态
		enum WorkerStatus
		{
			sNoStart,//未启动
			sRUNING,//运行任务中
			sIDLE,//空闲
			sPAUSE,//暂停
			sEND,//结束
		};
	public:
		TaskWorker(unsigned int usleepms = 50)
			:uIdlems(0),
			eStatus(sNoStart),
			uSleepms(usleepms)
		{

		}
		
		~TaskWorker()
		{
			eStatus = sEND;
			if (thWorker.JoinAble())
				thWorker.Join();
		}
		//启动
		bool ReSume()
		{
			if (sNoStart == eStatus)
			{
				eStatus = sIDLE;
				Thread temp(TaskThreadProc,this);
				thWorker.Swap(temp);
				return true;
			}
			else if (sPAUSE == eStatus)
			{
				eStatus = sRUNING;
				return true;
			}
			return false;
		}
		
		//关闭
		bool Pause()
		{
			if (sRUNING == eStatus|| sPAUSE == eStatus|| sIDLE==eStatus)
			{
				eStatus = sPAUSE;
				return true;
			}
			return false;
		}

		//获取状态
		WorkerStatus GetStatus()
		{
			return eStatus;
		}

		//获取任务数量
		QueueSizeT GetTaskSize()
		{
			AutoMutex lk(mQueueLock);
			return qTaskQueue.size();
		}
		//推送任务
		bool Post(const Task& t)
		{
			//获取
			AutoMutex lk(mQueueLock);
			return qTaskQueue.PushBack(t);
		}
	private:
		//获取当前的毫秒时间
		unsigned long long GetCurrentMS()
		{
			struct timeval tp;
			memset(&tp, 0, sizeof(tp));
			gettimeofday(&tp, NULL);
			return (unsigned long long)tp.tv_sec * 1000 + 
				(double)tp.tv_usec/1000;
		}
		//线程运行
		void TaskProc()
		{
			unsigned long long last_run_ms=0;//上次运行结束的时间
			Task current_task;//当前运动
			
			while (true)
			{
				if (sPAUSE == eStatus)
				{
					;//no done
				}
				else if (sIDLE == eStatus)
				{
					memset(&current_task, 0, sizeof(current_task));
					bool isPop = false;//是否可以拿到任务
					{
						//获取
						AutoMutex lk(mQueueLock);
						isPop = qTaskQueue.PopFront(&current_task);
					}
					if (isPop)
					{
						eStatus = sRUNING;
						//执行
						if (current_task.pFunc)
						{
							void *pRet= current_task.pFunc(current_task.pUserData);
							if (current_task.pComplete)
								current_task.pComplete(current_task.pUserData, pRet);
						}
						//更新最后运行时间
						last_run_ms = GetCurrentMS();
						eStatus = sIDLE;
					}
				}
				else
				{
					//其他直接退出
					break;
				}
				//更新空闲时间
				uIdlems = GetCurrentMS() - last_run_ms;
				ThreadSleep(uSleepms);
			}
		}

	private:
		//运行线程
		Thread thWorker;
		
		//任务队列
		Mutex mQueueLock;
		Queue<Task> qTaskQueue;
		
		//空闲时间
		unsigned long long uIdlems;
		
		//睡眠时间
		unsigned int uSleepms;

		//运行标识
		WorkerStatus eStatus;

	};
	
	class TaskPool
	{
	public:
		TaskPool(unsigned int min_thr,
			unsigned int max_thr,
			unsigned int max_idle_time_ms);
		~TaskPool();

	private:
		bool Post(const Task& t);
	private:
		//运行队列
		Queue<TaskWorker*> busy_queue_;
		//空闲队列
		Queue<TaskWorker*> idle_queue_;
	};


	inline ThRet TaskThreadProc(LPVOID lpParam)
	{
		if (lpParam)
		{
			TaskWorker* pthiz = (TaskWorker*)lpParam;
			pthiz->TaskProc();
		}
		return ThRet(-1);
	}
}
#endif