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

#include <new>

////引入日志
//#ifdef SIM_NO_LOGGER
//#include "Logger.hpp"
//#else
////格式
//#define SIM_FORMAT_NUM(num,base,w,f)	
//#define SIM_FORMAT_STR(str,w,f)			
//#define SIM_HEX(num) 
//#define SIM_FORMAT_STR0(str,w) 
//
////防止重名
//#define SIM_FUNC(lv)
//#define SIM_FUNC_DEBUG() 
//
////新增输出流
//#define SIM_LOG_ADD(Stream,...) 
////配置输出句柄
//#define SIM_LOG_HANDLER(max_lv,handler,userdata) 
//
////配置控制台输出
//#define SIM_LOG_CONSOLE(max_lv)
//
//#define SIM_LOG(lv,x)
//#define SIM_LDEBUG(x) 
//#define SIM_LINFO(x) 
//#define SIM_LWARN(x) 
//#define SIM_LERROR(x)
//#endif // USING_SIM_LOGGER

namespace sim
{
	template <typename T>
	class WeakObject;

	//引用计数对象
	class RefCountable 
	{
	public:
		RefCountType add_ref(void)
		{
#ifdef OS_WINDOWS
			::InterlockedIncrement(&weak_count_);
			return ::InterlockedIncrement(&ref_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			++weak_count_;
			RefCountType t = ++ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType dec_ref(void) 
		{
#ifdef OS_WINDOWS
			::InterlockedDecrement(&weak_count_);
			return ::InterlockedDecrement(&ref_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			--weak_count_;
			RefCountType t = --ref_count_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
		}

		RefCountType add_weak_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedIncrement(&weak_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = ++weak_count_;
			pthread_mutex_unlock(&lock_);
			return t;
#endif

		}

		RefCountType dec_weak_ref(void)
		{
#ifdef OS_WINDOWS
			return ::InterlockedDecrement(&weak_count_);
#else
			//linux上面用锁实现
			pthread_mutex_lock(&lock_);
			RefCountType t = --weak_count_;
			pthread_mutex_unlock(&lock_);
			return t;

#endif
		}

		RefCountType get_ref_count(void) const
		{ 
			return ref_count_;
		}
		RefCountType get_weak_count(void) const
		{
			return weak_count_;
	}

		RefCountable(RefCountType count=1) : ref_count_(count) , weak_count_(count)
		{
#ifdef OS_LINUX
			pthread_mutex_init(&lock_, NULL);
#endif
		}
		virtual ~RefCountable(void) { /*assert(0 == refCount_);*/ }
	private:
		RefCountable(const RefCountable&);
		RefCountable& operator = (const RefCountable&) { return *this; };

	private:
		//强引用
		RefCountType volatile ref_count_;
		//弱引用
		RefCountType volatile weak_count_;
#ifdef OS_LINUX
		//linux上面用锁实现
		pthread_mutex_t lock_;
#endif
	};

	//引用归零的时候使用的删除器
	typedef void(*RefObjectDelete)(void* ptr, void*pdata);

	template <typename T>
	class RefObject
	{
		friend class WeakObject<T>;
	public:
		//weak 使用
		RefObject(T* p, RefObjectDelete deleter, void* pdata, RefCountable* ref_count_ptr)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(ref_count_ptr), pdata_(pdata)
		{
		}

		RefObject(T* p=NULL, RefObjectDelete deleter=NULL, void*pdata=NULL)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(new RefCountable(1)), pdata_(pdata)
		{
			//新增引用
			//add_ref();
		}
		virtual ~RefObject()
		{
			//释放
			release();
		}
		//// 浅拷贝
		RefObject(const RefObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			ref_count_ptr_->add_ref();
		}

		//// 浅拷贝
		virtual RefObject<T>& operator=(const RefObject<T>& rhs)
		{
			//自己赋值给自己的情况不算
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();

				//释放
				release();

				//赋值
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}
		
		//比较
		virtual bool operator==(const RefObject<T>& rhs)
		{
			return ptr_ == rhs.ptr_;
		}

		virtual RefCountType getcount()
		{
			return ref_count_ptr_->get_ref_count();
		}

		virtual void reset(T* p = NULL)
		{
			release();
			ptr_ = p;
			ref_count_ptr_ = new RefCountable(1);
		}

		virtual T* operator->()
		{
			return ptr_;
		}
		virtual T& operator*()
		{
			return *ptr_;
		}
		//获取指针
		virtual T* get()
		{
			return ptr_;
		}
		virtual operator bool()
		{
			return NULL != ptr_;
		}
		/*operator nullptr()
		{
			return ptr_;
		}*/

		//类型转换
		template<typename OT>
		RefObject<OT> cast()
		{
			if (ref_count_ptr_)
			{
				ref_count_ptr_->add_ref();

				return RefObject<OT>((OT*)(ptr_), deleter_, pdata_, ref_count_ptr_);
			}
			else
			{
				return NULL;
			}
		}
		
	protected:
		void release()
		{
			//释放
			if (ref_count_ptr_->dec_ref() <= 0)
			{
				if (ptr_)
				{
					if (deleter_)
						deleter_(ptr_, pdata_);
					else
						delete ptr_;
				}
				ptr_ = NULL;

				//回收计数指针
				if (ref_count_ptr_->get_weak_count() <= 0)
				{
					delete ref_count_ptr_;
				}
				ref_count_ptr_ = NULL;
			}
		}
		
	protected:
		//指针
		T* ptr_;
		RefObjectDelete deleter_;
		void*pdata_;
		//引用计数指针
		RefCountable*ref_count_ptr_;
	};

	//弱引用对象
	template <typename T>
	class WeakObject
	{
		friend class RefObject<T>;
	public:
		WeakObject(const RefObject<T>& orig) :ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			if (ref_count_ptr_)
				ref_count_ptr_->add_weak_ref();
		}
		WeakObject(const WeakObject<T>& orig) :ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			if (ref_count_ptr_)
				ref_count_ptr_->add_weak_ref();
		}
		virtual WeakObject<T>& operator=(const RefObject<T>& rhs)
		{
			rhs.ref_count_ptr_->add_weak_ref();

			//释放
			release();

			//赋值
			ptr_ = rhs.ptr_;
			ref_count_ptr_ = rhs.ref_count_ptr_;
			deleter_ = rhs.deleter_;
			pdata_ = rhs.pdata_;

			return *this;
		}
		virtual WeakObject<T>& operator=(const WeakObject<T>& rhs)
		{
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_weak_ref();

				//释放
				release();

				//赋值
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}
		virtual RefObject<T> lock()
		{
			if (ref_count_ptr_&& ref_count_ptr_->get_ref_count()>0)
			{
				ref_count_ptr_->add_ref();
				return RefObject<T>(ptr_, deleter_, pdata_, ref_count_ptr_);
			}
			else
			{
				return NULL;
			}
		}
		virtual RefCountType getcount()
		{
			if (ref_count_ptr_)
				return ref_count_ptr_->get_ref_count();
			return 0;
		}
	protected:
		void release()
		{
			//释放
			if (ref_count_ptr_&&ref_count_ptr_->dec_weak_ref()<=0)
			{
				delete ref_count_ptr_;
			}
			ref_count_ptr_ = NULL;
			ptr_ = NULL;
			deleter_ = NULL;
			pdata_ = NULL;
		}
	protected:
		//指针
		T* ptr_;
		RefObjectDelete deleter_;
		void* pdata_;
		//引用计数指针
		RefCountable* ref_count_ptr_;
	};

	//引用缓存
	class RefBuff :public RefObject<char>
	{
		//typedef void(*RefObjectDelete)(void* ptr, void*pdata);
		static void RefBuffDelete(void* ptr, void*pdata)
		{
			delete[]((char*)ptr);
		}
	public:
		RefBuff(unsigned int buff_size) 
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			
		};
		
		RefBuff(unsigned int buff_size,char _val)
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			set(_val);
		};
		
		RefBuff(const char*pdata, unsigned int buff_size) 
			:RefObject<char>(new char[buff_size], &RefBuff::RefBuffDelete), buff_size_(buff_size)
		{
			::memcpy(get(), pdata, buff_size);
		}
		
		RefBuff(const char*pdata)
			:RefObject<char>(new char[::strlen(pdata)], &RefBuff::RefBuffDelete), buff_size_(::strlen(pdata))
		{
			::memcpy(get(), pdata, buff_size_);
		}

		RefBuff() :RefObject<char>(), buff_size_(0)
		{
		}
		
		//// 浅拷贝
		RefBuff(const RefBuff& orig):RefObject<char>(orig), buff_size_(orig.buff_size_)
		{
		}
		
		//// 浅拷贝
		virtual RefBuff& operator=(const RefBuff& rhs)
		{
			//自己赋值给自己的情况不算
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();

				release();

				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
				buff_size_ = rhs.buff_size_;
			}
			return *this;
		}
		//比较
		virtual bool operator==(const RefBuff& rhs)
		{
			return (buff_size_ == rhs.buff_size_)&&(ptr_ == rhs.ptr_);
		}
		virtual RefBuff operator+(const RefBuff& rhs)
		{
			if (this->buff_size_ + rhs.buff_size_ <= 0)
				return RefBuff();

			RefBuff temp(this->buff_size_ + rhs.buff_size_);
			if(this->buff_size_>0)
				memcpy(temp.ptr_, this->ptr_, this->buff_size_);
			if (rhs.buff_size_ > 0)
				memcpy(temp.ptr_+ this->buff_size_, rhs.ptr_, rhs.buff_size_);
			return temp;
		}
		
		//不做检查
		virtual char& operator[](unsigned int index)
		{
			return ptr_[index];
		}

		virtual unsigned int size()
		{
			return buff_size_;
		}

		//设置值
		virtual void set(int _val)
		{
			memset(ptr_, _val, buff_size_);
		}
	private:
		unsigned int buff_size_;
	};
}
 #endif // ifndef _REFCOUNTED_INCLUDED
