/*
* ����أ���̬��չ
*/
#ifndef SIM_TASK_POOL_HPP_
#define SIM_TASK_POOL_HPP_

#include <time.h>

#include "Mutex.hpp"
#include "Queue.hpp"
#include "Thread.hpp"
//#include "stdio.h"
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
		usleep(x*1000);\
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
	//class TaskWorker;

	//�ڴ���亯��
	/*typedef void* (*TaskPoolMalloc)(unsigned int size);
	typedef void (*TaskPoolFree)(void*);*/

	//������
	typedef void* (*TaskFunc)(void*);
	typedef void (*TaskComplete)(void* pUserData,void*ret);

	//����
	struct Task
	{
		//���к���
		TaskFunc pFunc;
		//���غ���
		TaskComplete pComplete;
		//�û�����
		void* pUserData;
	};
	
	//����ִ�к���
	//class TaskWorker :protected Thread
	//{
	//	//����Ϊ��Ԫ
	//	friend ThRet TaskThreadProc(LPVOID lpParam);
	//	//״̬
	//	enum WorkerStatus
	//	{
	//		sRUNING,//����������
	//		sIDLE,//����
	//		sPAUSE,//��ͣ
	//		sEND,//����
	//	};
	//public:
	//	TaskWorker(unsigned int usleepms = 50)
	//		:uIdlems(0),
	//		eStatus(sIDLE),
	//		uSleepms(usleepms),
	//		Thread(TaskThreadProc,this)
	//	{
	//	}
	//	
	//	~TaskWorker()
	//	{
	//		End();
	//	}
	//	//����
	//	bool ReSume()
	//	{
	//		if (sPAUSE == eStatus)
	//		{
	//			eStatus = sIDLE;
	//			return true;
	//		}
	//		return false;
	//	}
	//	
	//	//�ر�
	//	bool Pause()
	//	{
	//		if (sRUNING == eStatus|| sPAUSE == eStatus|| sIDLE==eStatus)
	//		{
	//			eStatus = sPAUSE;
	//			return true;
	//		}
	//		return false;
	//	}
	//	bool End()
	//	{
	//		eStatus = sEND;
	//		if (JoinAble())
	//			Join();
	//		return true;
	//	}
	//	//��ȡ״̬
	//	WorkerStatus GetStatus()
	//	{
	//		return eStatus;
	//	}
	//	//��ȡ��������
	//	QueueSizeT GetTaskSize()
	//	{
	//		AutoMutex lk(mQueueLock);
	//		return qTaskQueue.Size();
	//	}
	//	
	//	//��������
	//	bool Post(const Task& t)
	//	{
	//		//��ȡ
	//		AutoMutex lk(mQueueLock);
	//		return qTaskQueue.PushBack(t);
	//	}
	//	//����
	//	/*void Swap(TaskWorker& other)
	//	{
	//		ThSwap(uIdlems, other.uIdlems);
	//		ThSwap(uSleepms, other.uSleepms);
	//		ThSwap(eStatus, other.eStatus);
	//		Thread::Swap(other);
	//		mQueueLock.swap(other.mQueueLock);
	//		qTaskQueue.Swap(other.qTaskQueue);
	//	}*/
	//private:
	//	//��ȡ��ǰ�ĺ���ʱ��
	//	unsigned long long GetCurrentMS()
	//	{
	//		struct timeval tp;
	//		memset(&tp, 0, sizeof(tp));
	//		gettimeofday(&tp, NULL);
	//		return (unsigned long long)tp.tv_sec * 1000 + 
	//			(double)tp.tv_usec/1000;
	//	}
	//	//�߳�����
	//	void TaskProc()
	//	{
	//		unsigned long long last_run_ms=0;//�ϴ����н�����ʱ��
	//		Task current_task;//��ǰ�˶�
	//		
	//		while (true)
	//		{
	//			if (sPAUSE == eStatus)
	//			{
	//				;//no done
	//			}
	//			else if (sIDLE == eStatus|| sRUNING ==eStatus)
	//			{
	//				memset(&current_task, 0, sizeof(current_task));
	//				bool isPop = false;//�Ƿ�����õ�����
	//				{
	//					//��ȡ
	//					AutoMutex lk(mQueueLock);
	//					isPop = qTaskQueue.PopFront(&current_task);
	//				}
	//				if (isPop)
	//				{
	//					eStatus = sRUNING;
	//					//ִ��
	//					if (current_task.pFunc)
	//					{
	//						void *pRet= current_task.pFunc(this,current_task.pUserData);
	//						if (current_task.pComplete)
	//							current_task.pComplete(this,current_task.pUserData, pRet);
	//					}
	//					//�����������ʱ��
	//					last_run_ms = GetCurrentMS();
	//					if(sRUNING == eStatus)
	//						eStatus = sIDLE;
	//					uIdlems = 0;
	//					continue;//���ȴ�
	//				}
	//			}
	//			else
	//			{
	//				//����ֱ���˳�
	//				break;
	//			}
	//			//���¿���ʱ��
	//			uIdlems = GetCurrentMS() - last_run_ms;
	//			ThreadSleep(uSleepms);
	//		}
	//	}
	//private:
	//	
	//	
	//	
	//	//����ʱ��
	//	unsigned long long uIdlems;
	//	
	//	//˯��ʱ��
	//	unsigned int uSleepms;
	//	//���б�ʶ
	//	WorkerStatus eStatus;
	//};
	
	inline ThRet TaskThreadProc(LPVOID lpParam);

	class TaskPool
	{
		friend ThRet TaskThreadProc(LPVOID lpParam);
	public:
		TaskPool(unsigned int thread_num, unsigned int sleep_ms=10);
		~TaskPool();

		bool Post(TaskFunc pFunc, void* pUserData,TaskComplete pComplete);

	private:
		bool PostTask(const Task& t);

		void TaskProc();
	private:
		//���ж���
		Queue<Thread*> th_queue_;
		//�������
		Mutex task_lock_;
		Queue<Task> task_queue_;

		unsigned int thread_num_;
		unsigned int sleep_ms_;
		bool run_flag_;
	};


	inline ThRet TaskThreadProc(LPVOID lpParam)
	{
		if (lpParam)
		{
			TaskPool* pthiz = (TaskPool*)lpParam;
			pthiz->TaskProc();
		}
		return ThRet(-1);
	}
	
	void sim::TaskPool::TaskProc()
	{
		Task current_task;//��ǰ�˶�
		while (run_flag_)
		{
			memset(&current_task, 0, sizeof(current_task));
			bool isPop = false;//�Ƿ�����õ�����
			{
				//��ȡ
				AutoMutex lk(task_lock_);
				isPop = task_queue_.PopFront(&current_task);
			}
			if (isPop)
			{
				//ִ��
				if (current_task.pFunc)
				{
					void* pRet = current_task.pFunc(current_task.pUserData);
					if (current_task.pComplete)
						current_task.pComplete(current_task.pUserData, pRet);
				}
				
				continue;//���ȴ�
			}
			ThreadSleep(sleep_ms_);
		}
	
		//printf("end %u\n", sim::Thread::GetThisThreadId());
	}
	
	inline TaskPool::TaskPool(unsigned int thread_num,
		unsigned int sleep_ms)
		:thread_num_(thread_num),
		sleep_ms_(sleep_ms),
		run_flag_(true)
	{
		if (thread_num_ <= 0)
			thread_num_ = 8;
		if (sleep_ms_ <= 0)
			sleep_ms_ = 10;
		//�����߳�
		for (int i = 0; i < thread_num_; ++i)
		{
			Thread* pt = new Thread(TaskThreadProc, this);
			th_queue_.PushBack(pt);
		}
	}

	inline TaskPool::~TaskPool()
	{
		run_flag_ = false;
		Thread* pt;
		while (true)
		{
			pt = NULL;
			if (th_queue_.PopFront(&pt))
			{
				if (pt->JoinAble())
					pt->Join();
				delete pt;
				pt = NULL;
			}
			else
			{
				break;
			}
		}
	}

	inline bool TaskPool::Post(TaskFunc pFunc, void* pUserData, TaskComplete pComplete)
	{
		if (NULL == pFunc)
			return false;
		Task t;
		t.pFunc = pFunc;
		t.pComplete = pComplete;
		t.pUserData = pUserData;
		return PostTask(t);
	}

	inline bool TaskPool::PostTask(const Task& t)
	{
		{
			//��ȡ
			AutoMutex lk(task_lock_);
			return task_queue_.PushBack(t);
		}
	}
}
#endif