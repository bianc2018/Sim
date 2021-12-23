/*
* SysBase
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
typedef HANDLE THREAD_HANDLE;
typedef CONDITION_VARIABLE COND_HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif  
#include <pthread.h>
#include <unistd.h>
typedef pthread_mutex_t MUTEX_HANDLE;
typedef pthread_t THREAD_HANDLE;
typedef pthread_cond_t  COND_HANDLE;
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

	//条件变量
	class Cond
	{
		COND_HANDLE cv_;
		MUTEX_HANDLE mtx_;
#ifdef OS_LINUX
		//属性设置
		pthread_condattr_t cattr_;
#endif // OS_LINUX

		Cond(const Cond& other) {};
		Cond& operator=(const Cond& other) {};
	public:
		Cond()
		{
#ifdef OS_WINDOWS
			InitializeCriticalSection(&mtx_);
			InitializeConditionVariable(&cv_);
#else
			pthread_mutex_init(&mtx_, NULL);

		    //采用相对时间等待。可以避免：因系统调整时间，导致等待时间出现错误。
			int ret = pthread_condattr_init(&cattr_);
			ret = pthread_condattr_setclock(&cattr_, CLOCK_MONOTONIC);
			pthread_cond_init(&cv_, &cattr_);
#endif
		}
		virtual ~Cond()
		{
#ifdef OS_WINDOWS
			//Broadcast();

			DeleteCriticalSection(&mtx_);
#else
			pthread_mutex_destroy(&mtx_);
			pthread_condattr_destroy(&(cattr_));
			pthread_cond_destroy(&cv_);
#endif
		}

		virtual void Signal()
		{
#ifdef OS_WINDOWS
			LeaveCriticalSection(&mtx_);
			WakeConditionVariable(&cv_);
			LeaveCriticalSection(&mtx_);
#else
			pthread_mutex_lock(&mtx_);
			pthread_cond_signal(&cv_);
			pthread_mutex_unlock(&mtx_);
#endif
		}

		virtual void Broadcast()
		{
#ifdef OS_WINDOWS
			LeaveCriticalSection(&mtx_);
			WakeAllConditionVariable(&cv_);
			LeaveCriticalSection(&mtx_);
#else
			pthread_mutex_lock(&mtx_);
			pthread_cond_broadcast(&cv_);
			pthread_mutex_unlock(&mtx_);
#endif
		}
		virtual bool Wait()
		{
#ifdef OS_WINDOWS
			LeaveCriticalSection(&mtx_);
			BOOL ret = SleepConditionVariableCS(&cv_, &mtx_, INFINITE);
			LeaveCriticalSection(&mtx_);
			return ret == TRUE;
#else
			pthread_mutex_lock(&mtx_);
			int ret = pthread_cond_wait(&cv_, &mtx_);
			pthread_mutex_unlock(&mtx_);
			return true;
#endif
		}
		virtual bool Wait(unsigned long dwMilliseconds)
		{
#ifdef OS_WINDOWS
			LeaveCriticalSection(&mtx_);
			BOOL ret = SleepConditionVariableCS(&cv_, &mtx_, dwMilliseconds);
			LeaveCriticalSection(&mtx_);
			return ret == TRUE;
#else
			//获取时间
			struct timespec outtime;
			clock_gettime(CLOCK_MONOTONIC, &outtime);
			//ms为毫秒，换算成秒
			outtime.tv_sec += dwMilliseconds / 1000;

			//在outtime的基础上，增加ms毫秒
			//outtime.tv_nsec为纳秒，1微秒=1000纳秒
			//tv_nsec此值再加上剩余的毫秒数 ms%1000，有可能超过1秒。需要特殊处理
			unsigned long long  us = outtime.tv_nsec / 1000 + 1000 * (dwMilliseconds % 1000); //微秒
			//us的值有可能超过1秒，
			outtime.tv_sec += us / 1000000;

			us = us % 1000000;
			outtime.tv_nsec = us * 1000;//换算成纳秒

			pthread_mutex_lock(&mtx_);
			int ret = pthread_cond_timedwait(&cv_, &mtx_, &outtime);
			pthread_mutex_unlock(&mtx_);
			return true;
#endif
		}
		
	};

	//原子操作
	template<typename T>
	class Atomic
	{
		T t_;
		Mutex mtx_;
	public:
		Atomic(const T& t) :t_(t) {};
		void Set(const T& t) {
			AutoMutex(mtx_);
			t_ = t;
		}
		T Get() {
			AutoMutex(mtx_);
			return t_;
		}
		Atomic& operator=(const T& t)
		{
			t_ = t;
			return (*this);
		}
		operator T()
		{
			return t_;
		}
	};

	//原子数字
	class AtomicNumber
	{
		/*
		*	type __sync_fetch_and_add(type*ptr,type value,...);// m+n
			type __sync_fetch_and_sub(type*ptr,type value,...);// m-n
			type __sync_fetch_and_or(type*ptr,type value,...);  // m|n
			type __sync_fetch_and_and(type*ptr,type value,...);// m&n
			type __sync_fetch_and_xor(type*ptr,type value,...);// m^n
			type __sync_fetch_and_nand(type*ptr,type value,...);// (~m)&n
		*/
		typedef long long ato_number;
		ato_number volatile num_;
	public:
		AtomicNumber(const ato_number& n=0) :num_(n) {};
		virtual ~AtomicNumber() {};
		ato_number Add(const ato_number& n)
		{
#ifdef OS_WINDOWS
			return ::InterlockedExchangeAdd64(&num_,n);
#else
			__sync_fetch_and_add(&num_, n);
			return num_;
#endif
		}
		ato_number Sub(const ato_number& n)
		{
#ifdef OS_WINDOWS
			return ::InterlockedExchangeAdd64(&num_, -n);
#else
			__sync_fetch_and_sub(&num_, n);
			return num_;
#endif
		}
		ato_number Or(const ato_number& n)
		{
#ifdef OS_WINDOWS
			return ::InterlockedOr64(&num_, n);
#else
			__sync_fetch_and_or(&num_, n);
			return num_;
#endif
		}
		ato_number And (const ato_number & n)
		{
#ifdef OS_WINDOWS
			return ::InterlockedAnd64(&num_, n);
#else
			__sync_fetch_and_and(&num_, n);
			return num_;
#endif
		}
		ato_number XOr(const ato_number& n)
		{
#ifdef OS_WINDOWS
			return ::InterlockedXor64(&num_, n);
#else
			__sync_fetch_and_xor(&num_, n);
			return num_;
#endif
		}

		/*AtomicNumber& operator=(const ato_number& t)
		{
			num_ = t;
			return (*this);
		}
		AtomicNumber& operator+(const ato_number& t)
		{
			Add(t);
			return (*this);
		}
		AtomicNumber& operator-(const ato_number& t)
		{
			Sub(t);
			return (*this);
		}
		AtomicNumber& operator|(const ato_number& t)
		{
			Or(t);
			return (*this);
		}
		AtomicNumber& operator&(const ato_number& t)
		{
			And(t);
			return (*this);
		}
		AtomicNumber& operator^(const ato_number& t)
		{
			XOr(t);
			return (*this);
		}*/

		operator ato_number()
		{
			return num_;
		}

	};

	//线程
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

		Thread()
			:pth_(INVALID_HANDLE_VALUE)
			, th_id_(-1)
			, lpParam_(NULL)
			, proc_(NULL)
		{

		}
	public:
		Thread(ThreadProc Proc, void* lpParam,bool start=true)
			:pth_(INVALID_HANDLE_VALUE)
			, th_id_(-1)
			, lpParam_(NULL)
			, proc_(NULL)
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

		/*virtual void Swap(Thread& other)
		{
			ThSwap(pth_, other.pth_);
			ThSwap(th_id_, other.th_id_);
			ThSwap(proc_, other.proc_);
			ThSwap(lpParam_, other.lpParam_);
		}*/

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

		static void Sleep(unsigned int ms);
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
	inline void Thread::Sleep(unsigned int ms)
	{
#ifdef OS_WINDOWS
		//timeBeginPeriod(1); //设置精度为1毫秒
		::Sleep(ms);         //当前线程挂起ms毫秒
		//timeEndPeriod(1);   //结束精度设置
#else
		usleep(ms * 1000);
#endif
	}
#ifdef OS_WINDOWS
	//typedef unsigned(__stdcall* _beginthreadex_proc_type)(void*);
	unsigned __stdcall sim::Thread::MyThreadProc(void* arg)
	{
		sim::Thread* self = (sim::Thread*)arg;
		if (self)
			self->OnThreadProc();
		return 0;
	}
#else
	void* sim::Thread::MyThreadProc(void* arg)
	{
		sim::Thread* self = (sim::Thread*)arg;
		if (self)
			self->OnThreadProc();
		return NULL;
	}
#endif


}
#endif //!SIM_MULTI_THREAD_HPP_