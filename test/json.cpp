#include "Test.hpp"
#include "Json.hpp"

using namespace sim;
//json ¿â²âÊÔ
SIM_TEST(JsonNew)
{
	JsonObjectPtr ptr = JsonObject::NewNull();
	SIM_TEST_IS_EQUAL(JSON_NULL, ptr->GetType());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewBoolen(false);
	SIM_TEST_IS_EQUAL(JSON_BOOL, ptr->GetType());
	SIM_TEST_IS_EQUAL(false, ptr->GetBoolen());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewNumber(0.1234567);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, ptr->GetType());
	SIM_TEST_IS_EQUAL(0.1234567, ptr->GetNumber());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewString("String");
	SIM_TEST_IS_EQUAL(JSON_STRING, ptr->GetType());
	SIM_TEST_IS_EQUAL("String", ptr->GetString());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewObject();
	SIM_TEST_IS_EQUAL(JSON_OBJECT, ptr->GetType());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewArray();
	SIM_TEST_IS_EQUAL(JSON_ARRAY, ptr->GetType());
	JsonObject::Free(ptr);
}
SIM_TEST_MAIN(sim::noisy)