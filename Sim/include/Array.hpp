/*
* 数组
*/
#ifndef SIM_ARRAY_HPP_
#define SIM_ARRAY_HPP_
#include "Memory.hpp"

#include <string.h>

//日志宏
#ifndef USING_SIM_LOGGER
#include "Logger.hpp"
#else
	//格式
#define SIM_FORMAT_NUM(num,base,w,f)	
#define SIM_FORMAT_STR(str,w,f)			
#define SIM_HEX(num) 
#define SIM_FORMAT_STR0(str,w) 

//防止重名
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 

//新增输出流
#define SIM_LOG_ADD(Stream,...) 
//配置输出句柄
#define SIM_LOG_HANDLER(max_lv,handler,userdata) 

//配置控制台输出
#define SIM_LOG_CONSOLE(max_lv)

#define SIM_LOG(lv,x)
#define SIM_LDEBUG(x) 
#define SIM_LINFO(x) 
#define SIM_LWARN(x) 
#define SIM_LERROR(x)
#endif // !USING_SIM_LOGGER

namespace sim
{
	//数组长度
	typedef unsigned int array_size_t;

	//数组
	template<typename T>
	class Array:public MemoryBase
	{
	public:
		Array()
			:pbegin_(NULL), size_(0), capacity_(0)
		{
			
		}
		~Array()
		{
			for (int i = 0; i < size_; ++i)
			{
				//析构
				T* temp = pbegin_ + i;
				temp->~T();
			}
			//释放
			Free(pbegin_);
		}

	public:
		array_size_t Size()
		{
			return size_;
		}
		array_size_t Capacity()
		{
			return capacity_;
		}
		T* Begin()
		{
			return Ptr(0);
		}
		T* End()
		{
			return Ptr(size_);
		}
		T& operator[](array_size_t index)
		{
			return *Ptr(index);
		}
		T& At(array_size_t index)
		{
			return *Ptr(index);
		}
		//新增 
		bool Assign(const T& t)
		{
			//满了重新调整容量
			if (size_ == capacity_)
			{
				if (!Adjust())
				{
					return false;
				}
			}
			T* pt = Ptr(size_);
			//拷贝
			//*Ptr(size_) = t;
			pt = new(pt)T(t);
			//新增
			++size_;
			return true;
		}
		bool Erase(array_size_t index)
		{
			//越界了
			if (index >= size_)
				return false;
			//析构
			T* t = Ptr(index);
			t->~T();

			//移动
			if (index == size_ - 1)
			{
				//删除在尾部
				--size_;
				return true;
			}
			else
			{
				//移动
				::memcpy(t, Ptr(index + 1), (size_ - index-1)*sizeof(T));
				--size_;
				return true;
			}
		}
	protected:
		//容量调整
		bool Adjust()
		{
			if (pbegin_)
			{
				//1.5倍拓展 至少+1拓展
				capacity_ = capacity_ * 1.5+1;
				//重新申请
				pbegin_ = (T*)Realloc(pbegin_,sizeof(T)* capacity_);
				if (pbegin_ == NULL)
				{
					//申请内存异常
					return false;
				}
				return true;
			}
			else
			{
				//申请一个
				pbegin_ = (T*)Malloc(sizeof(T));
				if (pbegin_ == NULL)
				{
					//申请内存异常
					return false;
				}
				capacity_ = 1;
				return true;
			}
		}
		T* Ptr(array_size_t index)
		{
			//越界处理
			if (index > size_)
				throw "out of range";
			return (pbegin_ + index);
		}
	private:
		//内存块起始
		T* pbegin_;
		//大小
		array_size_t size_;
		//容量 capacity
		array_size_t capacity_;
	};

	
}
#endif