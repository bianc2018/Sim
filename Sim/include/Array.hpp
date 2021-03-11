/*
* ����
*/
#ifndef SIM_ARRAY_HPP_
#define SIM_ARRAY_HPP_
#include "Memory.hpp"

#include <string.h>

//��־��
//#ifndef SIM_NO_LOGGER
//#include "Logger.hpp"
//#else
//	//��ʽ
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
//#endif // !USING_SIM_LOGGER

namespace sim
{
	//���鳤��
	typedef unsigned int array_size_t;

	//����
	template<typename T>
	class Array :public MemoryBase
	{
	public:
		Array()
			:pbegin_(NULL), size_(0), capacity_(0)
		{

		}
		~Array()
		{
			Clear();
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

		//׷��
		bool Assign(const T* t, int size)
		{
			//�������µ�������
			if (size_+size >= capacity_)
			{
				capacity_ = size_ + size;
				if (!Adjust(1))
				{
					return false;
				}
			}
			for (int i = 0; i < size; ++i)
			{
				T* pt = Ptr(size_);
				//����
				//*Ptr(size_) = t;
				pt = new(pt)T(t[i]);
				//����
				++size_;
			}
			return true;
		}
		//���� 
		bool Assign(const T& t)
		{
			//�������µ�������
			if (size_ == capacity_)
			{
				if (!Adjust())
				{
					return false;
				}
			}
			T* pt = Ptr(size_);
			//����
			//*Ptr(size_) = t;
			pt = new(pt)T(t);
			//����
			++size_;
			return true;
		}
		bool Erase(array_size_t index)
		{
			//Խ����
			if (index >= size_)
				return false;
			ErasePtr(Ptr(index));
			return true;
			////����
			//T* t = Ptr(index);
			//t->~T();

			////�ƶ�
			//if (index == size_ - 1)
			//{
			//	//ɾ����β��
			//	--size_;
			//	return true;
			//}
			//else
			//{
			//	//�ƶ�
			//	::memcpy(t, Ptr(index + 1), (size_ - index-1)*sizeof(T));
			//	--size_;
			//	return true;
			//}
		}

		T* ErasePtr(T*ptr)
		{
			//Խ����
			if (NULL == ptr||ptr >= End()||ptr<Begin())
				return End();
			//����
			ptr->~T();
			::memcpy(ptr, ptr + 1, (End() - ptr - 1) * sizeof(T));
			--size_;
			return ptr;
		}

		bool Clear()
		{
			if (pbegin_)
			{
				for (int i = 0; i < size_; ++i)
				{
					//����
					T* temp = pbegin_ + i;
					temp->~T();
				}
				//�ͷ�
				Free(pbegin_);

				pbegin_ = NULL;
				size_ = 0; 
				capacity_ = 0;
			}
			return true;
		}
	protected:
		//��������
		bool Adjust(unsigned int x=1.5)
		{
			if (pbegin_)
			{
				//1.5����չ ����+1��չ
				capacity_ = capacity_ * x+1;
				//��������
				pbegin_ = (T*)Realloc(pbegin_,sizeof(T)* capacity_);
				if (pbegin_ == NULL)
				{
					//�����ڴ��쳣
					return false;
				}
				return true;
			}
			else
			{
				if (capacity_ == 0)
					capacity_ = 1;
				//����һ��
				pbegin_ = (T*)Malloc(sizeof(T)*(capacity_));
				if (pbegin_ == NULL)
				{
					capacity_ = 0;
					//�����ڴ��쳣
					return false;
				}
				return true;
			}
		}
		T* Ptr(array_size_t index)
		{
			//Խ�紦��
			if (index > size_)
				throw "out of range";
			return (pbegin_ + index);
		}
	private:
		//�ڴ����ʼ
		T* pbegin_;
		//��С
		array_size_t size_;
		//���� capacity
		array_size_t capacity_;
	};

	
}
#endif