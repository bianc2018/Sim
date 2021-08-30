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
//测试继承
struct RefObjectTest1 :public RefObjectTest
{
	RefObjectTest1(int &i) :RefObjectTest(i)
	{

	}
	int p_;
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
	sim::RefObject<int> int_ptr= SIM_MAKE_REF(int,(0));
	SIM_TEST_IS_TRUE(int_ptr);
	SIM_TEST_IS_FALSE(NULL == int_ptr);
	SIM_TEST_IS_FALSE(NULL == int_ptr.get());
	SIM_TEST_IS_EQUAL(1, int_ptr.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr);

	sim::RefObject<int> int_ptr_0(new int(0));
	SIM_TEST_IS_FALSE(int_ptr_0 == int_ptr);

	sim::RefObject<int> int_ptr2(int_ptr);
	SIM_TEST_IS_EQUAL(int_ptr.getcount(), int_ptr2.getcount());
	SIM_TEST_IS_EQUAL(2, int_ptr2.getcount());
	SIM_TEST_IS_EQUAL(0, *int_ptr2);

	SIM_TEST_IS_TRUE(int_ptr2 == int_ptr);

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
	SIM_TEST_IS_TRUE(buff == buff1);

	const char* test1 = "hello world";
	sim::RefBuff buff2(test1);
	//SIM_TEST_IS_EQUAL(test1, buff2.get());
	SIM_TEST_IS_EQUAL(strlen(test1), buff2.size());
	for(int i=0;i< buff2.size();++i)
		SIM_TEST_IS_EQUAL(test1[i], buff2[i]);

	SIM_TEST_IS_FALSE(buff == buff2);

	sim::RefBuff buff3(buff2);
	buff1 = buff3;
	SIM_TEST_IS_EQUAL(strlen(test1), buff1.size());
	for (int i = 0; i < buff1.size(); ++i)
		SIM_TEST_IS_EQUAL(test1[i], buff1[i]);
	SIM_TEST_IS_EQUAL(3, buff1.getcount());
}

SIM_TEST(WeakObject)
{
	sim::RefObject<int> int_ptr(new int(0));
	SIM_TEST_IS_TRUE(int_ptr);

	sim::WeakObject<int> weak_ptr(int_ptr);
	SIM_TEST_IS_EQUAL(1, int_ptr.getcount());
	SIM_TEST_IS_EQUAL(weak_ptr.getcount(), int_ptr.getcount());

	sim::RefObject<int> int_ptr1 = weak_ptr.lock();
	SIM_TEST_IS_EQUAL(2, int_ptr1.getcount());
	SIM_TEST_IS_TRUE(int_ptr1==int_ptr);

	int_ptr.reset();
	int_ptr1.reset();
	SIM_TEST_IS_EQUAL(0, weak_ptr.getcount());
	SIM_TEST_IS_NULL(weak_ptr.lock());
	sim::WeakObject<int> weak_ptr1 = weak_ptr;
	SIM_TEST_IS_EQUAL(0, weak_ptr1.getcount());
	SIM_TEST_IS_NULL(weak_ptr1.lock());
}
//Type conversion
SIM_TEST(TypeConversion)
{
	int i = 0;
	{
		sim::RefObject<RefObjectTest1> object_ptr(new RefObjectTest1(i));
		sim::RefObject<RefObjectTest> object_ptr1 = NULL;
		object_ptr1 = object_ptr.cast<RefObjectTest>();;
	}
	SIM_TEST_IS_EQUAL(0, i);
}

//EnableRefFormThis
class EnableRefFormThisTest:public sim::EnableRefFormThis<EnableRefFormThisTest>
{
public:
	sim::RefObject<EnableRefFormThisTest> test()
	{
		return ref();
	}
	EnableRefFormThisTest() {};
};
SIM_TEST(EnableRefFormThis)
{
	int i = 0;
	{
		EnableRefFormThisTest test;
		SIM_TEST_IS_NULL(test.test());
		sim::RefObject<EnableRefFormThisTest> object_ptr(new EnableRefFormThisTest());
		SIM_TEST_IS_NULL(object_ptr->test());
		sim::RefObject<EnableRefFormThisTest> object_ptr1 = sim::EnableRefFormThis<EnableRefFormThisTest>::make_ref(new EnableRefFormThisTest());
		SIM_TEST_IS_NOT_NULL(object_ptr1->test());
	}
	SIM_TEST_IS_EQUAL(0, i);
}
SIM_TEST_MAIN(sim::noisy)