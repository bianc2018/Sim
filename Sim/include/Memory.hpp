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
			if (free_)
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

	//对象池
//	template<typename T>
//	class ObjectPool :public Memory
//	{
//	public:
//		ObjectPool(size_t capacity)
//			:capacity_(0)
//			, pbeg_(NULL)
//			, size_(0)
//			, use_map_(NULL)
//		{
//			resize(capacity);
//		}
//		virtual ~ObjectPool()
//		{
//			release();
//		}
//
//		//在c++03中，__cplusplus定义为199711L；在c++11中，__cplusplus定义为201103L
//#if __cplusplus >= 201103L|true
//		template<typename... Args>
//		T* MallocArgs(Args...args)
//		{
//			void*p = get_free_memory();
//			if (NULL == p)
//				return NULL;
//			//printf("get_free_memory p=%p\n", p);
//			T*pt = new(p) T(args...);
//			return pt;
//		}
//#endif
//		T* Malloc()
//		{
//			void*p = get_free_memory();
//			if (NULL == p)
//				return NULL;
//			//printf("get_free_memory p=%p\n", p);
//			T*pt = new(p) T();
//			return pt;
//		}
//
//		void Free(T* p)
//		{
//			if (p&&(p>=(T*)pbeg_)&& (p < (T*)pbeg_+sizeof(T)*capacity_))
//			{
//				int i = ((T*)pbeg_ - p) / sizeof(T);
//				//析构
//				p->~T();
//				use_map_[i] = false;
//				--size_;
//			}
//			else
//			{
//				printf("Free p=%p\n", p);
//				return ;
//			}
//		}
//
//		bool Clear()
//		{
//			if (use_map_&&pbeg_)
//			{
//				for (int i = 0; i < capacity_; ++i)
//				{
//					if (use_map_[i])
//					{
//						//析构
//						T*p = (T*)pbeg_ + sizeof(T)*i;
//						p->~T();
//						use_map_[i] = false;
//					}
//				}
//				size_ = 0;
//				return true;
//			}
//			return false;
//		}
//	private:
//		//重新申请内存
//		bool resize(size_t new_size)
//		{
//			if (new_size == 0)
//			{
//				return false;
//			}
//			//printf("resize capacity_ %u ->new_size %u\n", capacity_, new_size);
//			if (capacity_ == new_size)
//				return true;
//			else if (capacity_ < new_size)
//			{
//				//拓容
//				if (0 == capacity_)
//				{
//					//初始化
//					capacity_ = new_size;
//					size_ = 0;
//					pbeg_ = Memory::Malloc(sizeof(T)*capacity_);
//					use_map_ = (bool*)Memory::Malloc(sizeof(bool)*capacity_);
//					memset(use_map_, false, sizeof(bool)*capacity_);
//				}
//				else
//				{
//					//拓展
//					pbeg_ = Memory::Realloc(pbeg_,sizeof(T)*new_size);
//					//printf("pbeg_=%p\n", pbeg_);
//					use_map_ = (bool*)Memory::Realloc(use_map_,sizeof(bool)*new_size);
//					memset(use_map_+ capacity_, false, sizeof(bool)*new_size-sizeof(bool)*capacity_);
//					capacity_ = new_size;
//				}
//				return true;
//			}
//			//缩小
//			else  if (capacity_ > new_size)
//			{
//				//保证数据是不用的
//				int i = capacity_-1;
//				for (; i >= new_size; --i)
//				{
//					if (use_map_[i] == true)
//						break;
//				}
//				if (i == capacity_ - 1)
//					return false;//不可拓展
//				new_size = i;
//				//拓展
//				pbeg_ = Memory::Realloc(pbeg_, sizeof(T)*new_size);
//				use_map_ = (bool*)Memory::Realloc(use_map_, sizeof(bool)*new_size);
//				capacity_ = new_size;
//				return true;
//			}
//
//			return false;
//		}
//
//		bool release()
//		{
//			if (use_map_&&pbeg_)
//			{
//				Clear();
//				Memory::Free((void*)use_map_);
//				Memory::Free(pbeg_);
//				capacity_ = 0;
//				size_ = 0;
//				use_map_ = NULL;
//				pbeg_ = NULL;
//				return true;
//			}
//			return false;
//		}
//
//		void* get_free_memory()
//		{
//			if (size_ == capacity_)
//			{
//				if (!resize(capacity_ == 0 ? 1 : capacity_ * 2))
//					return NULL;
//				
//				use_map_[size_] = true;
//				T* temp = (T*)pbeg_ + sizeof(T)*size_;
//				//printf("2 i=%u %p\n", size_, temp);
//				++size_;
//				
//				return temp;
//			}
//			else
//			{
//				for (int i = 0; i < capacity_; ++i)
//				{
//					if (false == use_map_[i])
//					{
//						use_map_[i] = true;
//						++size_;
//						//printf("2 i=%u %p\n", i, (T*)pbeg_ + sizeof(T)*i);
//						return (T*)pbeg_ + sizeof(T)*i;
//					}
//				}
//				return false;
//			}
//		}
//	private:
//		//内存开始指针
//		void*pbeg_;
//		//对象数量
//		size_t size_;
//
//		//使用池
//		size_t capacity_;
//		bool *use_map_;
//	};

#if __cplusplus >= 201103L
#define SIM_MEM_NEW(type,memory,...)  memory->New<type>(__VA_ARGS__)
#else
#define SIM_MEM_NEW(type,memory,...)  new(memory->Malloc(sizeof(type))) type(__VA_ARGS__)
#endif
//释放内存
#define SIM_MEM_DEL(memory,p)  memory->Delete(p)
}
#endif