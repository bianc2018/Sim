#include "Test.hpp"
#include "Async/RefObject.hpp"

struct TestObject
{
    static int i;
public:
    TestObject()
    {
        ++i;
    }
    ~TestObject()
    {
        --i;
    }
};
int TestObject::i =0;
//单线程基本功能
SIM_TEST(RefObjectSimpleThread)
{
   SIM_ASSERT_IS_EQUAL(TestObject::i, 0);
   sim::RefObject<TestObject>  ref = SIM_MAKE_REF(TestObject);
   SIM_ASSERT_IS_EQUAL(TestObject::i, 1);
   SIM_ASSERT_IS_EQUAL(ref.getcount(), 1);
   {
       sim::RefObject<TestObject>  ref1 = ref;
       SIM_ASSERT_IS_EQUAL(TestObject::i, 1);
       SIM_ASSERT_IS_EQUAL(ref1.getcount(), 2);
       SIM_ASSERT_IS_EQUAL(ref.getcount(), 2);
   }
   SIM_ASSERT_IS_EQUAL(TestObject::i, 1);
   SIM_ASSERT_IS_EQUAL(ref.getcount(), 1);
}

template<typename T>
struct RefArrayNode
{
    sim::RefObject<TestObject> ref;
    RefArrayNode<T>* pnext;

    RefArrayNode():pnext(NULL)
    {}
};
SIM_TEST(RefObjectTestUseTimes)
{
    SIM_ASSERT_IS_EQUAL(TestObject::i, 0);

    RefArrayNode<TestObject> head;
    head.pnext = NULL;
    head.ref = SIM_MAKE_REF(TestObject);
    const int num = 10000000;

    SIM_TEST_TIME_START(use_time_ref);
    RefArrayNode<TestObject>* cur = &head;
    for (int i = 0; i < num; ++i)
    {
        cur->pnext = new RefArrayNode<TestObject>();
        cur = cur->pnext;
        cur->ref = head.ref;//添加引用
        cur->pnext = NULL;
        //下一个
    }
    SIM_TEST_TIME_END_LT(use_time_ref,10000);

    SIM_ASSERT_IS_EQUAL(head.ref.getcount(), num+1);
    SIM_ASSERT_IS_EQUAL(TestObject::i, 1);
    //释放
    head.ref.reset();
    cur = head.pnext;
    while (cur)
    {
        RefArrayNode<TestObject>* temp = cur;
        cur = cur->pnext;
        delete temp;
    }
    SIM_ASSERT_IS_EQUAL(TestObject::i, 0);
}

SIM_TEST_MAIN(sim::noisy)