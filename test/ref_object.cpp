#include "Test.hpp"
#include "RefObject.hpp"

//测试
struct RefObjectTest
{
	int &i_ ;
	RefObjectTest(int &i)
		:i_(i)
	{
		++i_;
	}
	~RefObjectTest()
	{
		--i_;
	}
};

SIM_TEST(RefObjectEmpty)
{
	sim::RefObject<int> int_ptr;
	SIM_TEST_IS_FALSE(int_ptr);
	SIM_TEST_IS_NULL(int_ptr);
	SIM_TEST_IS_NULL(int_ptr.get());
	SIM_TEST_IS_EQUAL(1, int_ptr.getcount());
	int_ptr.reset();
	SIM_TEST_IS_NULL(int_ptr.get());
	SIM_TEST_IS_EQUAL(1, int_ptr.getcount());
}
SIM_TEST(RefObjectBuildIn)
{
	sim::RefObject<int> int_ptr(new int(0));
	SIM_TEST_IS_TRUE(int_ptr);
	SIM_TEST_IS_FALSE(NULL == int_ptr);
	SIM_TEST_IS_FALSE(NULL == int_ptr.get());
	SIM_TEST_IS_EQUAL(1, int_ptr.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr);

	sim::RefObject<int> int_ptr2(int_ptr);
	SIM_TEST_IS_EQUAL(int_ptr.getcount(), int_ptr2.getcount());
	SIM_TEST_IS_EQUAL(2, int_ptr2.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr2);

	sim::RefObject<int> int_ptr3(new int(3));
	int_ptr3 = int_ptr;//这里会释放一个
	SIM_TEST_IS_EQUAL(int_ptr.getcount(), int_ptr3.getcount());
	SIM_TEST_IS_EQUAL(3, int_ptr3.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr3);

	int_ptr.reset();
	SIM_TEST_IS_FALSE(int_ptr);
	SIM_TEST_IS_EQUAL(int_ptr2.getcount(), int_ptr3.getcount());
	SIM_TEST_IS_EQUAL(2, int_ptr3.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr3);
}

SIM_TEST(RefObjectObject)
{
	int i = 0;
	{
		sim::RefObject<RefObjectTest> object_ptr(new RefObjectTest(i));
		sim::RefObject<RefObjectTest> object_ptr1(object_ptr);
		sim::RefObject<RefObjectTest> object_ptr2;
		object_ptr2 = object_ptr1;
		object_ptr1 = object_ptr;
	}
	SIM_TEST_IS_EQUAL(0, i);
}

SIM_TEST(RefBuff)
{
	sim::RefBuff buff;
	SIM_TEST_IS_NULL(buff);
	SIM_TEST_IS_NULL(buff.get());
	SIM_TEST_IS_EQUAL(0,buff.size());
	SIM_TEST_IS_EQUAL(1,buff.getcount());

	sim::RefBuff buff1(buff);
	SIM_TEST_IS_NULL(buff);
	SIM_TEST_IS_NULL(buff.get());
	SIM_TEST_IS_EQUAL(0, buff.size());
	SIM_TEST_IS_EQUAL(2, buff.getcount());

	const char* test1 = "hello world";
	sim::RefBuff buff2(test1);
	//SIM_TEST_IS_EQUAL(test1, buff2.get());
	SIM_TEST_IS_EQUAL(strlen(test1), buff2.size());
	for(int i=0;i< buff2.size();++i)
		SIM_TEST_IS_EQUAL(test1[i], buff2[i]);

	sim::RefBuff buff3(buff2);
	buff1 = buff3;
	SIM_TEST_IS_EQUAL(strlen(test1), buff1.size());
	for (int i = 0; i < buff1.size(); ++i)
		SIM_TEST_IS_EQUAL(test1[i], buff1[i]);
	SIM_TEST_IS_EQUAL(3, buff1.getcount());
}

SIM_TEST_MAIN(sim::noisy)