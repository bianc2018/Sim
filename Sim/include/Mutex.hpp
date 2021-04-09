#ifndef SIM_MUTEX_HPP_
#define SIM_MUTEX_HPP_

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  
#endif
#include <Windows.h>

#else
#include <pthread.h>
typedef pthread_mutex_t CRITICAL_SECTION ;
#endif

namespace sim
{
	//»¥³âËø
	class Mutex
	{
	private:
		Mutex(const Mutex &other) {};
		Mutex &operator=(const Mutex &other) {};
		
		CRITICAL_SECTION critical_section_;
	public:
		Mutex()
		{
#ifdef WIN32
			InitializeCriticalSection(&critical_section_);
#else
			pthread_mutex_init(&critical_section_, NULL);
#endif
		}
		void lock()
		{
#ifdef WIN32
			EnterCriticalSection(&critical_section_);
#else
			pthread_mutex_lock(&critical_section_);
#endif
		}
		void unlock()
		{
#ifdef WIN32
			LeaveCriticalSection(&critical_section_);
#else
			pthread_mutex_unlock(&critical_section_);
#endif
		}
		virtual ~Mutex()
		{
#ifdef WIN32
			DeleteCriticalSection(&critical_section_);
#else
			pthread_mutex_destroy(&critical_section_);
#endif
		}
		void swap(Mutex& other)
		{
			CRITICAL_SECTION temp = critical_section_;
			critical_section_ = other.critical_section_;
			other.critical_section_ = temp;
		}
	};

	class AutoMutex
	{
		AutoMutex(const AutoMutex &other):lock_(other.lock_){};
		AutoMutex operator=(const AutoMutex &other) {};

		Mutex &lock_;
	public:
		AutoMutex(Mutex&lock):lock_(lock)
		{
			lock_.lock();
		}
		~AutoMutex()
		{
			lock_.unlock();
		}
	};
}
#endif