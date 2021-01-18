#include "Test.hpp"
#include "Queue.hpp"
SIM_TEST(Queue)
{
	sim::Queue<int> q;
	SIM_TEST_IS_FALSE(q.SetAlloc(NULL, NULL));

	SIM_TEST_IS_TRUE(q.isEmpty());
	SIM_TEST_IS_EQUAL(0, q.Size());
	q.PushBack(1);
	SIM_TEST_IS_EQUAL(1, q.Size());
	int d = -1;
	SIM_TEST_IS_TRUE(q.PopFront(NULL));
	SIM_TEST_IS_EQUAL(0, q.Size());
	q.PushBack(1);
	SIM_TEST_IS_TRUE(q.PopFront(&d));
	
}
SIM_TEST_MAIN(sim::noisy)