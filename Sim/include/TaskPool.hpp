/*
* ����أ���̬��չ
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
	class TaskWorker;

	//�ڴ���亯��
	/*typedef void* (*TaskPoolMalloc)(unsigned int size);
	typedef void (*TaskPoolFree)(void*);*/

	//������
	typedef void* (*TaskFunc)(TaskWorker *self,void*);
	typedef void (*TaskComplete)(TaskWorker* self,void* pUserData,void*ret);

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
	
	inline ThRet TaskThreadProc(LPVOID lpParam);
	//����ִ�к���
	class TaskWorker :protected Thread
	{
		//����Ϊ��Ԫ
		friend ThRet TaskThreadProc(LPVOID lpParam);
		//״̬
		enum WorkerStatus
		{
			sRUNING,//����������
			sIDLE,//����
			sPAUSE,//��ͣ
			sEND,//����
		};
	public:
		TaskWorker(unsigned int usleepms = 50)
			:uIdlems(0),
			eStatus(sIDLE),
			uSleepms(usleepms),
			Thread(TaskThreadProc,this)
		{

		}
		
		~TaskWorker()
		{
			End();
		}
		//����
		bool ReSume()
		{
			if (sPAUSE == eStatus)
			{
				eStatus = sIDLE;
				return true;
			}
			return false;
		}
		
		//�ر�
		bool Pause()
		{
			if (sRUNING == eStatus|| sPAUSE == eStatus|| sIDLE==eStatus)
			{
				eStatus = sPAUSE;
				return true;
			}
			return false;
		}

		bool End()
		{
			eStatus = sEND;
			if (JoinAble())
				Join();
			return true;
		}
		//��ȡ״̬
		WorkerStatus GetStatus()
		{
			return eStatus;
		}

		//��ȡ��������
		QueueSizeT GetTaskSize()
		{
			AutoMutex lk(mQueueLock);
			return qTaskQueue.Size();
		}
		
		//��������
		bool Post(const Task& t)
		{
			//��ȡ
			AutoMutex lk(mQueueLock);
			return qTaskQueue.PushBack(t);
		}

		//����
		/*void Swap(TaskWorker& other)
		{
			ThSwap(uIdlems, other.uIdlems);
			ThSwap(uSleepms, other.uSleepms);
			ThSwap(eStatus, other.eStatus);
			Thread::Swap(other);

			mQueueLock.swap(other.mQueueLock);
			qTaskQueue.Swap(other.qTaskQueue);
		}*/
	private:
		//��ȡ��ǰ�ĺ���ʱ��
		unsigned long long GetCurrentMS()
		{
			struct timeval tp;
			memset(&tp, 0, sizeof(tp));
			gettimeofday(&tp, NULL);
			return (unsigned long long)tp.tv_sec * 1000 + 
				(double)tp.tv_usec/1000;
		}
		//�߳�����
		void TaskProc()
		{
			unsigned long long last_run_ms=0;//�ϴ����н�����ʱ��
			Task current_task;//��ǰ�˶�
			
			while (true)
			{
				if (sPAUSE == eStatus)
				{
					;//no done
				}
				else if (sIDLE == eStatus|| sRUNING ==eStatus)
				{
					memset(&current_task, 0, sizeof(current_task));
					bool isPop = false;//�Ƿ�����õ�����
					{
						//��ȡ
						AutoMutex lk(mQueueLock);
						isPop = qTaskQueue.PopFront(&current_task);
					}
					if (isPop)
					{
						eStatus = sRUNING;
						//ִ��
						if (current_task.pFunc)
						{
							void *pRet= current_task.pFunc(this,current_task.pUserData);
							if (current_task.pComplete)
								current_task.pComplete(this,current_task.pUserData, pRet);
						}
						//�����������ʱ��
						last_run_ms = GetCurrentMS();
						if(sRUNING == eStatus)
							eStatus = sIDLE;
						uIdlems = 0;
						continue;//���ȴ�
					}
				}
				else
				{
					//����ֱ���˳�
					break;
				}
				//���¿���ʱ��
				uIdlems = GetCurrentMS() - last_run_ms;
				ThreadSleep(uSleepms);
			}
		}

	private:
		
		//�������
		Mutex mQueueLock;
		Queue<Task> qTaskQueue;
		
		//����ʱ��
		unsigned long long uIdlems;
		
		//˯��ʱ��
		unsigned int uSleepms;

		//���б�ʶ
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
		Mutex queue_lock_;
		//���ж���
		Queue<TaskWorker*> busy_queue_;
		//���ж���
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