#include "Test.hpp"
#include "Json.hpp"

using namespace sim;
//json 库测试
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

//解析
SIM_TEST(JsonParser)
{
	JsonString json = "";
	JsonObjectPtr ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NULL(ptr);

	//空对象
	json = "{}";
	ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_OBJECT, ptr->GetType());
	JsonObject::Free(ptr);

	json = "[]";
	ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_ARRAY, ptr->GetType());
	JsonObject::Free(ptr);

	//错误
	json = "{adhadkla}";
	ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NULL(ptr);

	//正常解析
	json = "{\"string\":\"string\","
		"\"double\":34.32,"
		"\"int\":222,"
		"\"bool\":true,"
		"\"null\":,"
		"\"null2\":null,"
		"\"array\":[1,2,3,4,5,6],"
		"\"object\":{\"object_child\":\"child\"}"
		"}";
	ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_OBJECT, ptr->GetType());
	
	JsonObjectPtr temp = ptr->FindByName("string");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_STRING, temp->GetType());
	SIM_TEST_IS_EQUAL("string", temp->GetString());

	temp = ptr->FindByName("double");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, temp->GetType());
	SIM_TEST_IS_EQUAL(34.32, temp->GetNumber());

	temp = ptr->FindByName("int");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, temp->GetType());
	SIM_TEST_IS_EQUAL(222, temp->GetNumber());

	temp = ptr->FindByName("bool");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_BOOL, temp->GetType());
	SIM_TEST_IS_TRUE(temp->GetBoolen());

	temp = ptr->FindByName("null");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_NULL, temp->GetType());
	SIM_TEST_IS_TRUE(temp->IsNull());

	temp = ptr->FindByName("null2");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_NULL, temp->GetType());
	SIM_TEST_IS_TRUE(temp->IsNull());

	temp = ptr->FindByName("array");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_ARRAY, temp->GetType());
	SIM_TEST_IS_EQUAL(6, temp->Size());
	for (int i = 0; i < temp->Size(); ++i)
	{
		SIM_ASSERT_IS_NOT_NULL((*temp)[i]);
		SIM_TEST_IS_EQUAL(i+1, (*temp)[i]->GetNumber());
	}

	temp = ptr->FindByName("object");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(JSON_OBJECT, temp->GetType());
	//object_child
	JsonObjectPtr temp1 = temp->FindByName("object_child");
	SIM_ASSERT_IS_NOT_NULL(temp1);
	SIM_TEST_IS_EQUAL(JSON_STRING, temp1->GetType());
	SIM_TEST_IS_EQUAL("child", temp1->GetString());

	JsonObject::Free(ptr);

	//多重嵌套
	json = "{"
				"\"object1\":{"
					"\"object2\":{"
						"\"object3\":{"
							"\"object4\":{"
								"\"object5\":666,"
								"\"array5\":[{\"object6\":\"object6\"}]"
							"}"
						"}"
					"}"
				"}"
		"}";
	ptr = JsonObject::Parser(json);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_OBJECT, ptr->GetType());
	JsonObjectPtr object1 = NULL, object2 = NULL,\
		object3 = NULL, object4 = NULL, object5 = NULL, object6 = NULL,\
		array5 = NULL;
	object1 = ptr->FindByName("object1");
	SIM_ASSERT_IS_NOT_NULL(object1);
	object2 = object1->FindByName("object2");
	SIM_ASSERT_IS_NOT_NULL(object2);
	object3 = object2->FindByName("object3");
	SIM_ASSERT_IS_NOT_NULL(object3);
	object4 = object3->FindByName("object4");
	SIM_ASSERT_IS_NOT_NULL(object4);
	object5 = object4->FindByName("object5");
	SIM_ASSERT_IS_NOT_NULL(object5);
	array5 = object4->FindByName("array5");
	SIM_ASSERT_IS_NOT_NULL(array5);
	SIM_ASSERT_IS_NOT_NULL(array5->FindByIndex(0));
	object6 = array5->FindByIndex(0)->FindByName("object6");
	SIM_ASSERT_IS_NOT_NULL(object6);

	SIM_TEST_IS_EQUAL(JSON_NUMBER, object5->GetType());
	SIM_TEST_IS_EQUAL(666, object5->GetNumber());

	SIM_TEST_IS_EQUAL(JSON_STRING, object6->GetType());
	SIM_TEST_IS_EQUAL("object6", object6->GetString());

	JsonObject::Free(ptr);
}
SIM_TEST_MAIN(sim::noisy)