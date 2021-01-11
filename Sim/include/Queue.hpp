/*
*  简单的链表
*/
#ifndef SIM_LIST_HPP_
#define SIM_LIST_HPP_

#include <stdlib.h>
#include <string.h>

namespace sim
{
	template<typename T>
	struct QueueNode
	{
		T data;
		QueueNode* pNext;
		QueueNode(T t):data(t),pNext(NULL)
		{

		}
	};

	//内存分配函数
	typedef void* (*QueueMalloc)(unsigned int size);
	typedef void (*QueueFree)(void*);
	typedef unsigned long QueueSizeT;
	template<typename T>
	class Queue
	{
	public:
		Queue() 
			:pHead(NULL), pTail(NULL)
			, qMalloc(::malloc)
			, qFree(::free)
			, qsize(0)
		{
			
		}
		virtual ~Queue()
		{
			//释放
			clear();
		}
		//出列
		virtual bool PopFront(T* t)
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
				return true;
			}
			else
			{
				pHead = pHead->pNext;
				--qsize;
			}
			FreeNode(pTemp);
			return true;
		}

		virtual bool PushBack(const T& t)
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

		virtual bool isEmpty()
		{
			return 0 == qsize;
		}

		bool clear()
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

		virtual QueueSizeT size()
		{
			return qsize;
		}

		virtual bool SetAlloc(QueueMalloc m, QueueFree f)
		{
			if (m && f)
			{
				qMalloc = m;
				qFree = f;
			}
			return false;
		}
	private:
		QueueNode<T>* NewNode(const T& t)
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
			newnode->data = t;
			return newnode;
		}
		bool FreeNode(QueueNode<T>* pnode)
		{
			if (NULL == qFree&&NULL==pnode)
			{
				return false;
			}
			qFree(pnode);
			return true;
		}
	private:
		QueueNode<T>* pHead;
		QueueNode<T>* pTail;
		QueueSizeT qsize;
	private:
		QueueMalloc qMalloc;
		QueueFree qFree;
	};

}
#endif