/*
*  简单的链表
*/
#ifndef SIM_LIST_HPP_
#define SIM_LIST_HPP_

#include <stdlib.h>
#include <string.h>
#define USING_NEW
#ifdef USING_NEW
#include <new>
#endif // USING_NEW

namespace sim
{
	template<typename T>
	struct QueueNode
	{
		T data;
		QueueNode* pNext;
		QueueNode(const T& t):data(t),pNext(NULL)
		{

		}
	};

	//内存分配函数
	typedef void* (*QueueMalloc)(size_t size);
	typedef void (*QueueFree)(void*);

	typedef unsigned long QueueSizeT;
	template<typename T>
	class Queue
	{
		//交换函数
		template<typename ST>
		inline void qSwap(ST& t1, ST& t2)
		{
			ST temp = t1;
			t1 = t2;
			t2 = temp;
		}
	public:
		Queue(const Queue<T>&other) :pHead(NULL), pTail(NULL)
			, qMalloc(::malloc)
			, qFree(::free)
			, qsize(0)
		{
			operator=(other);
		}
		Queue<T>& operator=(const Queue<T>&other)
		{
			//自己赋值给自己的情况不算
			if (this != &other)
			{
				Clear();
				QueueNode<T>*pn =  other.Next(NULL);
				while (pn)
				{
					PushBack(pn->data);
					pn = other.Next(pn);
				}
			}
			return (*this);
		}

		Queue();
		virtual ~Queue();
		//出列
		virtual bool PopFront(T* t);

		virtual bool PushBack(const T& t);

		virtual bool isEmpty();

		virtual bool Clear();

		virtual QueueSizeT Size();

		virtual void Swap(Queue& other);

		virtual bool SetAlloc(QueueMalloc m, QueueFree f);

		//返回迭代器 NULL 标识结束
		QueueNode<T>* Next(QueueNode<T>*p)const;
		//遍历 返回false终止
		typedef bool(*TraverseFunc)(T* Now, void*pdata);
		bool Traverse(TraverseFunc func, void*pdata);
	private:
		QueueNode<T>* NewNode(const T& t);
		bool FreeNode(QueueNode<T>* pnode);
	private:
		QueueNode<T>* pHead;
		QueueNode<T>* pTail;
		QueueSizeT qsize;
	private:
		QueueMalloc qMalloc;
		QueueFree qFree;
	};

	template<typename T>
	inline Queue<T>::Queue()
		:pHead(NULL), pTail(NULL)
		, qMalloc(::malloc)
		, qFree(::free)
		, qsize(0)
	{

	}

	template<typename T>
	inline Queue<T>::~Queue()
	{
		//释放
		Clear();
	}

	template<typename T>
	inline bool Queue<T>::PopFront(T * t)
	{
		if (isEmpty())
			return false;

		if (NULL == pHead)
			return false;

		if (t)
			*t = pHead->data;

		QueueNode<T>* pTemp = pHead;

		if (pHead == pTail)
		{
			pHead = pTail = NULL;
			qsize = 0;
			//return true;
		}
		else
		{
			pHead = pHead->pNext;
			--qsize;
		}
		FreeNode(pTemp);
		return true;
	}
	template<typename T>
	inline bool Queue<T>::PushBack(const T & t)
	{
		QueueNode<T>* pnode = NewNode(t);
		if (NULL == pnode)
			return false;

		//添加首个
		if (isEmpty())
		{
			pHead = pTail = pnode;
			qsize = 1;
			return true;
		}
		else
		{
			if (pTail)
			{
				//前移
				pTail->pNext = pnode;
				pTail = pnode;
				++qsize;
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	template<typename T>
	inline bool Queue<T>::isEmpty()
	{
		return 0 == qsize;
	}
	template<typename T>
	inline bool Queue<T>::Clear()
	{
		while (pHead)
		{
			QueueNode<T>* pTemp = pHead;
			pHead = pHead->pNext;
			FreeNode(pTemp);
		}

		pHead = pTail = NULL;
		qsize = 0;
		return true;
	}
	template<typename T>
	inline QueueSizeT Queue<T>::Size()
	{
		return qsize;
	}
	template<typename T>
	inline void Queue<T>::Swap(Queue & other)
	{
		qSwap(pHead, other.pHead);
		qSwap(pTail, other.pTail);
		qSwap(qsize, other.qsize);
		qSwap(qMalloc, other.qMalloc);
		qSwap(qFree, other.qFree);
	}
	template<typename T>
	inline bool Queue<T>::SetAlloc(QueueMalloc m, QueueFree f)
	{
		if (m && f)
		{
			qMalloc = m;
			qFree = f;
		}
		return false;
	}
	template<typename T>
	inline QueueNode<T>* Queue<T>::Next(QueueNode<T>* p)const
	{
		if (NULL == p)
			return pHead;
		return p->pNext;
	}
	template<typename T>
	inline bool Queue<T>::Traverse(TraverseFunc func, void * pdata)
	{
		if (NULL == func)
			return false;

		QueueNode<T>* pt = Next(NULL);
		while (pt)
		{
			if (!func(&pt->data, pdata))
				break;
			pt = Next(pt);
		}
		return true;
	}
	template<typename T>
	inline QueueNode<T>* Queue<T>::NewNode(const T & t)
	{
		if (NULL == qMalloc)
		{
			return NULL;
		}
		//申请一个新节点
		QueueNode<T>* newnode = (QueueNode<T>*)qMalloc(sizeof(QueueNode<T>));
		if (NULL == newnode)
			return NULL;
		
		//初始化
		::memset(newnode, 0, sizeof(QueueNode<T>));
#ifdef USING_NEW
		newnode = new(newnode) QueueNode<T>(t);
#else
		newnode->data = t;
#endif // USING_NEW
		//newnode->data = newnode->data->T::T(0);
		return newnode;
	}
	template<typename T>
	inline bool Queue<T>::FreeNode(QueueNode<T>* pnode)
	{
		if (NULL == qFree && NULL == pnode)
		{
			return false;
		}
#ifdef USING_NEW
		pnode->~QueueNode<T>();
#endif // USING_NEW
		qFree(pnode);
		return true;
	}
}
#endif