/************************************************************************
     共享指针
************************************************************************/
#ifndef _REFCOUNTED_INCLUDED_
#define _REFCOUNTED_INCLUDED_
#include <cassert>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
	#include <Windows.h>
	typedef LONG RefCountType;
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif  
	#include <pthread.h>
	typedef int RefCountType;
#else
	#error "不支持的平台"
#endif

//引入日志
#ifdef USING_SIM_LOGGER
#include "Logger.hpp"
#else
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 
#define SIM_LOG_CONFIG(max_lv,handler,userdata)
#define SIM_LOG(lv,x) 
#define SIM_LDEBUG(x) 
#define SIM_LINFO(x) 
#define SIM_LWARN(x) 
#define SIM_LERROR(x) 
#endif // USING_SIM_LOGGER

namespace sim
{

	class RefCountable 
	{
	public:
		RefCountType addRef(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedIncrement(&refCount_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = ++refCount_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType decRef(void) 
		{
#ifdef OS_WINDOWS
				return ::InterlockedDecrement(&refCount_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = --refCount_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
			/*assert(r >= 0);
			if (0 == r)
				delete this;
			return r;*/
		}

		RefCountType getRefCount(void) const
		{ 
			return refCount_; 
		}

	protected:
		RefCountable(void) : refCount_(0) {}
		virtual ~RefCountable(void) { /*assert(0 == refCount_);*/ }

	private:
		RefCountType volatile refCount_;
#ifdef OS_LINUX
		//linux上面用锁实现
		pthread_mutex_t lock_;
#endif
	private:
		RefCountable(const RefCountable&);
		RefCountable& operator = (const RefCountable&) {return *this};

	};
}
 #endif // ifndef _REFCOUNTED_INCLUDED
