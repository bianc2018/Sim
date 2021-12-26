#include "Test.hpp"

#define _SIM_DISABLE_JSON_SERIALIZE
#include "File/Json.hpp"

struct MyStruct
{
	int i;
};
SIM_DEF_JSON_SERIALIZE_STRUCT(MyStruct)
    SIM_JSON_SERIALIZE_VALUE_2(i);
SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
SIM_TEST(JsonSerialize)
{
	MyStruct my,my1;
	my.i = 0;
	sim::JsonObjectPtr p=  sim::JsonObject::NewObject();
	SIM_ASSERT_IS_EQUAL(false ,p->Serialize(my));
	SIM_ASSERT_IS_EQUAL(false, p->DeSerialize(my1));
}
SIM_TEST_MAIN(sim::noisy)