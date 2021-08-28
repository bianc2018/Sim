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

////������־
//#ifdef SIM_NO_LOGGER
//#include "Logger.hpp"
//#else
////��ʽ
//#define SIM_FORMAT_NUM(num,base,w,f)	
//#define SIM_FORMAT_STR(str,w,f)			
//#define SIM_HEX(num) 
//#define SIM_FORMAT_STR0(str,w) 
//
////��ֹ����
//#define SIM_FUNC(lv)
//#define SIM_FUNC_DEBUG() 
//
////���������
//#define SIM_LOG_ADD(Stream,...) 
////����������
//#define SIM_LOG_HANDLER(max_lv,handler,userdata) 
//
////���ÿ���̨���
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

		RefCountable(RefCountType count=1) : ref_count_(count) 
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
		RefCountType volatile ref_count_;
#ifdef OS_LINUX
		//linux��������ʵ��
		pthread_mutex_t lock_;
#endif
	};

	//���ù����ʱ��ʹ�õ�ɾ����
	typedef void(*RefObjectDelete)(void* ptr, void*pdata);

	template <typename T>
	class RefObject
	{
	public:
		RefObject(T* p=NULL, RefObjectDelete deleter=NULL, void*pdata=NULL)
			:ptr_(p), deleter_(deleter), ref_count_ptr_(new RefCountable(1)), pdata_(pdata)
		{
			//��������
			//add_ref();
		}
		virtual ~RefObject()
		{
			//�ͷ�
			release();
		}
		//// ǳ����
		RefObject(const RefObject<T>& orig)
			:ptr_(orig.ptr_),
			deleter_(orig.deleter_),
			ref_count_ptr_(orig.ref_count_ptr_),
			pdata_(orig.pdata_)
		{
			ref_count_ptr_->add_ref();
		}
		
		//// ǳ����
		virtual RefObject<T>& operator=(const RefObject<T>& rhs)
		{
			//�Լ���ֵ���Լ����������
			if (this != &rhs)
			{
				rhs.ref_count_ptr_->add_ref();

				//�ͷ�
				release();

				//��ֵ
				ptr_ = rhs.ptr_;
				ref_count_ptr_ = rhs.ref_count_ptr_;
				deleter_ = rhs.deleter_;
				pdata_ = rhs.pdata_;
			}
			return *this;
		}
		
		//�Ƚ�
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
		//��ȡָ��
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
	protected:
		void release()
		{
			//�ͷ�
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

				//���ռ���ָ��
				delete ref_count_ptr_;
				ref_count_ptr_ = NULL;
			}
		}
	protected:
		//ָ��
		T* ptr_;
		RefObjectDelete deleter_;
		void*pdata_;
		//���ü���ָ��
		RefCountable*ref_count_ptr_;
	};

	//���û���
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
		
		//// ǳ����
		RefBuff(const RefBuff& orig):RefObject<char>(orig), buff_size_(orig.buff_size_)
		{
		}
		
		//// ǳ����
		virtual RefBuff& operator=(const RefBuff& rhs)
		{
			//�Լ���ֵ���Լ����������
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
		//�Ƚ�
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
		
		//�������
		virtual char& operator[](unsigned int index)
		{
			return ptr_[index];
		}

		virtual unsigned int size()
		{
			return buff_size_;
		}

		//����ֵ
		virtual void set(int _val)
		{
			memset(ptr_, _val, buff_size_);
		}
	private:
		unsigned int buff_size_;
	};
}
 #endif // ifndef _REFCOUNTED_INCLUDED
