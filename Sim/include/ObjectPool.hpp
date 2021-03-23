/*
*	对象池
*/
#ifndef SIM_OBJECT_POOL_HPP_
#define SIM_OBJECT_POOL_HPP_
#include <stdlib.h>
#include "Queue.hpp"
#include "Memory.hpp"
namespace sim
{

	template<typename T>
	class ObjectPool :public Memory
	{
		//不允许拷贝
		ObjectPool(const ObjectPool&) {};
		ObjectPool& operator=(const ObjectPool&) {}
	public:
		ObjectPool(size_t max_object):
			max_object_(max_object),init_flag_(false)
		{
			
		}
		virtual ~ObjectPool()
		{
			Clear();
		}

		//在c++03中，__cplusplus定义为199711L；在c++11中，__cplusplus定义为201103L
#if __cplusplus >= 201103L|true
		template<typename... Args>
		T* Generate(Args...args)
		{
			return SIM_MEM_NEW(T, this, (args)...);
		}
#endif
		T* Generate()
		{
			return SIM_MEM_NEW(T, this)
		}

		void ReCycle(T* p)
		{
			return SIM_MEM_DEL(this, p);
		}

		bool Clear()
		{
			//释放缓存
			void* temp = NULL;
			while (free_queue_.PopFront(&temp))
			{
				if (free_)
					free_(temp);
			}
			return true;
		}
	private:
		virtual void* Malloc(size_t size)
		{
			if (false == init_flag_)
			{
				//初始化
				init_flag_ = true;
				void*p = NULL;
				for (int i = 0; i < max_object_; ++i)
				{
					if (malloc_)
						p =  malloc_(size);
					if (NULL == p)
						return NULL;
					free_queue_.PushBack(p);
				}
			}

			if (free_queue_.isEmpty())
			{
				if (malloc_)
					return malloc_(size);
			}
			else
			{
				void* temp = NULL;
				free_queue_.PopFront(&temp);
				return temp;
			}
			return NULL;
		}
		virtual void Free(void* p)
		{
			if (free_queue_.Size() >= max_object_)
			{
				if (free_)
					free_(p);
			}
			else
			{
				free_queue_.PushBack(p);
			}
		}
	private:
		Queue<void*> free_queue_;

		unsigned int max_object_;

		bool init_flag_;
	};
}
#endif