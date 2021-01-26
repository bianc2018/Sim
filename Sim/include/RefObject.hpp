/************************************************************************
     ����ָ��
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
	#error "��֧�ֵ�ƽ̨"
#endif

#include <new>

//������־
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
	//���ü�������
	class RefCountable 
	{
	public:
		RefCountType add_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedIncrement(&ref_count_);
#else
			//linux��������ʵ��
			pthread_mutex_lock(&lock_);
			RefCountType t = ++ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType dec_ref(void) 
		{
#ifdef OS_WINDOWS
			return ::InterlockedDecrement(&ref_count_);
#else
			//linux��������ʵ��
			pthread_mutex_lock(&lock_);
			RefCountType t = --ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
		}

		RefCountType get_ref_count(void) const
		{ 
			return ref_count_;
		}

		RefCountable(RefCountType count=1) : ref_count_(count) {}
		virtual ~RefCountable(void) { /*assert(0 == refCount_);*/ }
	private:
		RefCountable(const RefCountable&);
		RefCountable& operator = (const RefCountable&) { return *this; };

	private:
		RefCountType volatile ref_count_;
#ifdef OS_LINUX
		//linux��������ʵ��
		pthread_mutex_t lock_;
#endif
	};

	//
	template <typename T>
	class RefObject
	{
		//���ù����ʱ��ʹ�õ�ɾ����
		typedef void(*RefObjectDelete)(T* ptr);

	public:
		RefObject(T* p=NULL, RefObjectDelete deleter=NULL)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(new RefCountable(1))
		{
			//��������
			//add_ref();
		}
		~RefObject()
		{
			//�ͷ�
			release();
		}
		//// ǳ����
		RefObject(const RefObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_)
		{
			ref_count_ptr_->add_ref();
		}
		//// ǳ����
		RefObject<T>& operator=(const RefObject<T>& rhs)
		{
			//�Լ���ֵ���Լ����������
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
			}
			return *this;
		}
		
		RefCountType getcount()
		{
			return ref_count_ptr_->get_ref_count();
		}

		void reset(T* p = NULL)
		{
			release();
			ptr_ = p;
			ref_count_ptr_ = new RefCountable(1);
		}

		T* operator->()
		{
			return ptr_;
		}
		T& operator*()
		{
			return *ptr_;
		}
	private:
		void release()
		{
			//�ͷ�
			if (ref_count_ptr_->dec_ref() <= 0)
			{
				if (ptr_)
				{
					if (deleter_)
						deleter_(ptr_);
					else
						delete ptr_;
				}
				ptr_ = NULL;

				//���ռ���ָ��
				delete ref_count_ptr_;
				ref_count_ptr_ = NULL;
			}
		}
	private:
		//ָ��
		T* ptr_;
		RefObjectDelete deleter_;

		//���ü���ָ��
		RefCountable*ref_count_ptr_;
	};
}
 #endif // ifndef _REFCOUNTED_INCLUDED
