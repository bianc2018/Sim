#include "Test.hpp"

//对象数目
static size_t _use_obj_num = 0;
#define JSON_NEW(T) (++_use_obj_num,new T())
#define JSON_FREE(T) {if(T)delete T;--_use_obj_num;}
#include "File/Json.hpp"

SIM_TEST(AutoJsonObject)
{
	SIM_ASSERT_IS_EQUAL(0, _use_obj_num);
	{
		sim::AutoJsonObject t1(sim::JsonObject::NewObject());
		sim::AutoJsonObject t2(sim::JsonObject::NewObject());
		SIM_ASSERT_IS_EQUAL(2, _use_obj_num);
		t1->ObjectAddObject("name", t2.Release());
		SIM_ASSERT_IS_TRUE(t2.IsNull());
		sim::AutoJsonObject t3(NULL);
		SIM_ASSERT_IS_TRUE(t2.IsNull());
	}
	SIM_ASSERT_IS_EQUAL(0, _use_obj_num);
}
SIM_TEST_MAIN(sim::noisy)