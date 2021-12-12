/*
* 将原来的锁和线程模块合并
* 用于多线程编译
*/
#ifndef SIM_MULTI_THREAD_HPP_
#define SIM_MULTI_THREAD_HPP_

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
#include <Windows.h>
#include <process.h>
//锁原始句柄
typedef CRITICAL_SECTION MUTEX_HANDLE;
//线程原始句柄
typedef CRITICAL_SECTION THREAD_HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif  
#include <pthread.h>
typedef pthread_mutex_t MUTEX_HANDLE;
typedef pthread_t THREAD_HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

#else
#error "不支持的平台"
#endif

namespace sim
{
	//互斥锁
	class Mutex
	{
	private:
		Mutex(const Mutex& other) {};
		Mutex& operator=(const Mutex& other) {};

		MUTEX_HANDLE critical_section_;
	public:
		Mutex()
		{
#ifdef OS_WINDOWS
			InitializeCriticalSection(&critical_section_);
#else
			pthread_mutex_init(&critical_section_, NULL);
#endif
		}
		void lock()
		{
#ifdef OS_WINDOWS
			EnterCriticalSection(&critical_section_);
#else
			pthread_mutex_lock(&critical_section_);
#endif
		}
		void unlock()
		{
#ifdef OS_WINDOWS
			LeaveCriticalSection(&critical_section_);
#else
			pthread_mutex_unlock(&critical_section_);
#endif
		}
		virtual ~Mutex()
		{
#ifdef OS_WINDOWS
			DeleteCriticalSection(&critical_section_);
#else
			pthread_mutex_destroy(&critical_section_);
#endif
		}
		void swap(Mutex& other)
		{
			MUTEX_HANDLE temp = critical_section_;
			critical_section_ = other.critical_section_;
			other.critical_section_ = temp;
		}
	};

	class AutoMutex
	{
		AutoMutex(const AutoMutex& other) :lock_(other.lock_) {};
		AutoMutex operator=(const AutoMutex& other) {};

		Mutex& lock_;
	public:
		AutoMutex(Mutex& lock) :lock_(lock)
		{
			lock_.lock();
		}
		~AutoMutex()
		{
			lock_.unlock();
		}
	};


	class Thread;
	typedef void(*ThreadProc)(Thread&self,void* lpParam);

	class Thread
	{
		ThreadProc proc_;
		void* lpParam_;

		THREAD_HANDLE pth_;
		unsigned int th_id_;
		
	protected:
		//禁止拷贝
		Thread(const Thread& other) {};
		Thread operator=(const Thread& other) {};
		template<typename T>
		inline void ThSwap(T& t1, T& t2)
		{
			T temp = t1;
			t1 = t2;
			t2 = temp;
		}
	public:
		Thread()
			:pth_(INVALID_HANDLE_VALUE)
			, th_id_(-1)
			, lpParam_(NULL)
			, proc_(NULL)
		{

		}
		Thread(ThreadProc Proc, void* lpParam,bool start=true)
		{
			SetProc(Proc, lpParam);
			if (start)
				Start();
		}

		virtual ~Thread()
		{
#ifdef OS_WINDOWS
			CloseHandle(pth_);
#endif
			pth_ = INVALID_HANDLE_VALUE;
		}

		virtual bool Join(/*ThRet* ret_val = NULL*/)
		{
			if (!JoinAble())
				return false;

			THREAD_HANDLE hanle = pth_;
			pth_ = INVALID_HANDLE_VALUE;
#ifdef OS_WINDOWS
			//pth = CreateThread(NULL, 0, Proc, lpParam, 0, NULL);
			WaitForSingleObject(hanle, INFINITE);
			/*if (ret_val)
			{
				DWORD ret = 0;
				GetExitCodeThread(pth_, (LPDWORD)&ret);
				*ret_val = (ThRet)ret;
			}*/
			//GetExitCodeThread
			CloseHandle(hanle);
			return true;
#else
			int err = pthread_join(hanle, NULL);
			if (err != 0)
				return false;
			return true;
#endif
		}

		virtual bool JoinAble()
		{
			if (INVALID_HANDLE_VALUE == pth_)
				return false;
			return true;
		}

		virtual bool Detach()
		{
			if (INVALID_HANDLE_VALUE == pth_)
				return false;
#ifdef OS_WINDOWS
			CloseHandle(pth_);
			pth_ = INVALID_HANDLE_VALUE;
			return true;
#else
			int err = pthread_detach(pth_);
			pth_ = INVALID_HANDLE_VALUE;
			if (err != 0)
				return false;
			return true;
#endif
		}

		virtual void Swap(Thread& other)
		{
			ThSwap(pth_, other.pth_);
			ThSwap(th_id_, other.th_id_);
			ThSwap(proc_, other.proc_);
			ThSwap(lpParam_, other.lpParam_);
		}

		virtual unsigned int GetId()
		{
			return th_id_;
		}

		virtual void SetProc(ThreadProc Proc, void* lpParam)
		{
			proc_ = Proc;
			lpParam_ = lpParam;
		}

		virtual bool Start()
		{
			if (INVALID_HANDLE_VALUE != pth_)
				return false;
#ifdef OS_WINDOWS
			pth_ = (THREAD_HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)MyThreadProc, this, 0, &th_id_);
#else
			int err = pthread_create(&pth_, NULL, MyThreadProc, this);
			if (err != 0)
			{
				pth_ = INVALID_HANDLE_VALUE;
			}
			else
			{
				th_id_ = pth_;
			}
#endif
			return pth_ != INVALID_HANDLE_VALUE;
		}

		static unsigned GetThisThreadId();
	protected:
		//回调
#ifdef OS_WINDOWS
		//typedef unsigned(__stdcall* _beginthreadex_proc_type)(void*);
		static unsigned __stdcall MyThreadProc(void* arg);
#else
		static void*  MyThreadProc(void* arg);
#endif
		virtual void OnThreadProc()
		{
			if (proc_)
				proc_((*this), lpParam_);
		}
	};
	unsigned int sim::Thread::GetThisThreadId()
	{
#ifdef WIN32
		return::GetCurrentThreadId();
#else
		return pthread_self();
#endif
	}
#ifdef OS_WINDOWS
	//typedef unsigned(__stdcall* _beginthreadex_proc_type)(void*);
	static unsigned __stdcall sim::Thread::MyThreadProc(void* arg)
	{
		sim::Thread* self = (sim::Thread*)arg;
		if (self)
			self->OnThreadProc();
		return 0;
	}
#else
	static void* sim::Thread::MyThreadProc(void* arg)
	{
		sim::Thread* self = (sim::Thread*)arg;
		if (self)
			self->OnThreadProc();
		return NULL;
	}
#else
#endif

}
#endif //!SIM_MULTI_THREAD_HPP_