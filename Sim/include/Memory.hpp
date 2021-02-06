/*
*	内存管理
*/
#ifndef SIM_MEMORY_HPP_
#define SIM_MEMORY_HPP_
#include <stdlib.h>
namespace sim
{
	//内存分配函数
	typedef void* (*MemoryMalloc)(size_t size);
	typedef void* (*MemoryRealloc)(void *mem_address, size_t newsize);
	typedef void(*MemoryFree)(void*);
	class Memory
	{
		// malloc_(::malloc),free_(::free)
	public:
		Memory() :malloc_(::malloc), free_(::free), realloc_(::realloc) {};
		virtual ~Memory() {};

		//设置内存申请函数
		virtual bool SetAlloc(MemoryMalloc m, MemoryFree f, MemoryRealloc r)
		{
			malloc_ = m;
			free_ = f;
			realloc_ = r;
			return true;
		}

		virtual void* Malloc(size_t size)
		{
			if (malloc_)
				return malloc_(size);
			return NULL;
		}
		virtual void Free(void* p)
		{
			if (free_&& p)
				free_(p);
		}
		virtual void* Realloc(void* p, size_t size)
		{
			if (realloc_)
				return realloc_(p, size);
			return NULL;
		}

		//在c++03中，__cplusplus定义为199711L；在c++11中，__cplusplus定义为201103L
#if __cplusplus >= 201103L
		template<typename T, typename... Args>
		T* New(Args...args)
		{
			void*p = Malloc(sizeof(T));

			if (NULL == p)
				return NULL;

			T*pt = new(p) T(args...);
			return pt;
		}
#endif
		template<typename T>
		T* New()
		{
			void*p = Malloc(sizeof(T));

			if (NULL == p)
				return NULL;

			T*pt = new(p) T();
			return pt;
		}

		template<typename T>
		void Delete(T* p)
		{
			//析构
			p->~T();
			Free((void*)p);
		}
	protected:
		MemoryMalloc malloc_;
		MemoryFree free_;
		MemoryRealloc realloc_;
	};

	//基础类 不暴露其他接口
	class MemoryBase:protected Memory
	{
	public:
		virtual bool SetAlloc(MemoryMalloc m,
			MemoryFree f,
			MemoryRealloc r)
		{
			return Memory::SetAlloc(m, f, r);
		}
	};
#if __cplusplus >= 201103L
#define SIM_MEM_NEW(type,memory,...)  memory->New<type>(__VA_ARGS__)
#else
#define SIM_MEM_NEW(type,memory,...)  new(memory->Malloc(sizeof(type))) type(__VA_ARGS__)
#endif
//释放内存
#define SIM_MEM_DEL(memory,p)  memory->Delete(p)
}
#endif