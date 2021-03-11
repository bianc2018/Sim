#include "Test.hpp"
#include "Queue.hpp"
SIM_TEST(QueueSetAlloc)
{
	sim::Queue<int> q;
	SIM_TEST_IS_FALSE(q.SetAlloc(NULL, NULL));
}

SIM_TEST(QueueEmpty)
{
	sim::Queue<int> q;

	SIM_TEST_IS_TRUE(q.isEmpty());
	SIM_TEST_IS_EQUAL(0, q.Size());
}

SIM_TEST(QueueNext)
{
	sim::Queue<int> q;
	int data = 1;
	int data1 = 2;
	SIM_TEST_IS_EQUAL((sim::QueueNode<int> *)NULL, q.Next(NULL));

	q.PushBack(data);
	sim::QueueNode<int> *phead = q.Next(NULL);
	SIM_ASSERT_IS_NOT_EQUAL((sim::QueueNode<int> *)NULL, phead);
	SIM_TEST_IS_EQUAL(phead->data, data);

	q.PushBack(data1);
	sim::QueueNode<int> *phead1 = q.Next(phead);
	SIM_ASSERT_IS_NOT_EQUAL((sim::QueueNode<int> *)NULL, phead1);
	SIM_TEST_IS_EQUAL(phead1->data, data1);

	SIM_TEST_IS_EQUAL((sim::QueueNode<int> *)NULL, q.Next(phead1));
}

SIM_TEST(QueuePushAndPop)
{
	sim::Queue<int> q;
	int data = 1;
	q.PushBack(data);
	SIM_TEST_IS_EQUAL(1, q.Size());
	int d = -1;
	SIM_TEST_IS_TRUE(q.PopFront(NULL));
	SIM_TEST_IS_EQUAL(0, q.Size());
	q.PushBack(data);
	SIM_TEST_IS_TRUE(q.PopFront(&d));
	SIM_TEST_IS_EQUAL(data, d);
}

struct QueuePushAndPopStruct
{
	int i ;
	double j;
	std::string str;
	QueuePushAndPopStruct():
		i(0),j(0.0),str("")
	{

	}
};
SIM_TEST(QueuePushAndPopStruct)
{
	sim::Queue<QueuePushAndPopStruct> q;
	QueuePushAndPopStruct data;
	data.i = 1;
	data.j = 1.123;
	data.str = "abcdef";

	q.PushBack(data);
	QueuePushAndPopStruct d;
	SIM_TEST_IS_TRUE(q.PopFront(&d));

	SIM_TEST_IS_EQUAL(data.i, d.i);
	SIM_TEST_IS_EQUAL(data.j, d.j);
	SIM_TEST_IS_EQUAL(data.str, d.str);
}

SIM_TEST(QueueSize)
{
	sim::Queue<int> q;
	unsigned int size = 10000;
	int data = 1;

	for (unsigned int i = 0; i < size; ++i)
	{
		q.PushBack(data);
	}
	SIM_TEST_IS_EQUAL(size, q.Size());

	int d = -1;
	SIM_TEST_IS_TRUE(q.PopFront(&d));
	SIM_TEST_IS_EQUAL(data, d);
	SIM_TEST_IS_EQUAL(size-1, q.Size());

	//Clear
	q.Clear();
	SIM_TEST_IS_EQUAL(0, q.Size());
}

SIM_TEST(QueueSwap)
{
	sim::Queue<int> q1,q2;
	unsigned int size = 10000;
	int data1 = 1, data2 = 2;
	
	q1.PushBack(data1);
	q2.PushBack(data2);
	q1.Swap(q2);

	//查看数值有没有交换
	int d = -1;
	SIM_TEST_IS_TRUE(q1.PopFront(&d));
	SIM_TEST_IS_EQUAL(data2, d);

	d = -1;
	SIM_TEST_IS_TRUE(q2.PopFront(&d));
	SIM_TEST_IS_EQUAL(data1, d);
}


SIM_TEST_MAIN(sim::noisy)