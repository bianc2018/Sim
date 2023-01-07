#include "Test.hpp"
#include "Json.hpp"

using namespace sim;
//json 库测试
SIM_TEST(JsonNew)
{
	
	JsonObjectPtr ptr = JsonObject::NewNull();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_NULL, ptr->GetType());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewBoolen(false);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_BOOL, ptr->GetType());
	SIM_TEST_IS_EQUAL(false, ptr->GetBoolen());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewNumber(0.1234567);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, ptr->GetType());
	SIM_TEST_IS_EQUAL(0.1234567, ptr->GetNumber());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewString("String");
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_STRING, ptr->GetType());
	SIM_TEST_IS_EQUAL("String", ptr->GetString());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewObject();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_OBJECT, ptr->GetType());
	JsonObject::Free(ptr);

	ptr = JsonObject::NewArray();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_ARRAY, ptr->GetType());
	JsonObject::Free(ptr);
}

//解析
SIM_TEST(JsonParser)
{
	using namespace sim;
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
		SIM_ASSERT_IS_NOT_NULL(temp->FindByIndex(i));
		SIM_TEST_IS_EQUAL(i+1, temp->FindByIndex(i)->GetNumber());
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

	//FindByPath 接口
	JsonObjectPtr find_object=ptr->FindByPath("");
	SIM_TEST_IS_NULL(find_object);
	find_object = ptr->FindByPath("adad");
	SIM_TEST_IS_NULL(find_object);
	find_object = ptr->FindByPath("object1");
	SIM_ASSERT_IS_NOT_NULL(find_object);
	find_object = ptr->FindByPath("object1.4444");
	SIM_TEST_IS_NULL(find_object);

	find_object = ptr->FindByPath("object1.object2.object3.object4.object5");
	SIM_ASSERT_IS_NOT_NULL(find_object);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, find_object->GetType());
	SIM_TEST_IS_EQUAL(666, find_object->GetNumber());

	find_object = ptr->FindByPath("object1/object2/object3/object4/object5",'/');
	SIM_ASSERT_IS_NOT_NULL(find_object);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, find_object->GetType());
	SIM_TEST_IS_EQUAL(666, find_object->GetNumber());

	JsonObject::Free(ptr);

//	getchar();
}

enum MyEnumTest1
{
	MyEnumTest1_999=999,
	MyEnumTest1_444 = 444,
	MyEnumTest1_0 = 0,
};
SIM_DEF_JSON_SERIALIZE_TYPE_AS_ENUM(MyEnumTest1);
struct Test1chlids
{
	int val;
public:
	SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
		SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(val)
	SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
public:
	//比较
	bool operator != (const Test1chlids&t)
	{
		return val != t.val;
	}
};
struct Test1
{
	bool bool_val;
	char char_val;
	unsigned char uchar_val;
	int int_val;
	unsigned int uint_val;
	double double_val;
	float float_val ;
	size_t size_t_val;
	long long_val;
	long long llong_val;
	unsigned long long ullong_val ;
	std::string string_val;

	MyEnumTest1 enum_val;
	Test1chlids struct_val;
	std::vector<Test1chlids> vec_val;
	
	Test1():bool_val(false)
		, char_val(0)
		, uchar_val(0)
		, int_val(0)
		, uint_val(0)
		, double_val(0)
		, float_val(0)
		, size_t_val(0)
		, long_val(0)
		, llong_val(0)
		, ullong_val(0)
		, enum_val(MyEnumTest1_0)
	{

	}

public:
	//比较
	bool operator == (const Test1&t)
	{
		if (bool_val != t.bool_val)
			return false;
		if (char_val != t.char_val)
			return false;
		if (uchar_val != t.uchar_val)
			return false;
		if (int_val != t.int_val)
			return false;
		if (uint_val != t.uint_val)
			return false;
		if (double_val != t.double_val)
			return false;
		if (float_val != t.float_val)
			return false;
		if (size_t_val != t.size_t_val)
			return false;
		if (long_val != t.long_val)
			return false;
		if (llong_val != t.llong_val)
			return false;
		if (ullong_val != t.ullong_val)
			return false;
		if (string_val != t.string_val)
			return false;
		if (enum_val != t.enum_val)
			return false;
		if (struct_val != t.struct_val)
			return false;
		if (vec_val.size() != t.vec_val.size())
			return false;
		for (int i = 0; i < vec_val.size(); ++i)
			if (vec_val[i] != t.vec_val[i])
				return false;
		return true;
	}
};

//SIM_DEF_JSON_SERIALIZE_TYPE_AS_NUM(char);
//SIM_DEF_JSON_SERIALIZE_TYPE_AS_NUM(unsigned char);

SIM_DEF_JSON_SERIALIZE_STRUCT(Test1)
	SIM_JSON_SERIALIZE_VALUE_2(bool_val)
	SIM_JSON_SERIALIZE_VALUE_2(char_val)
	SIM_JSON_SERIALIZE_VALUE_2(uchar_val)
	SIM_JSON_SERIALIZE_VALUE_2(int_val)
	SIM_JSON_SERIALIZE_VALUE_2(uint_val)
	SIM_JSON_SERIALIZE_VALUE_2(double_val)
	SIM_JSON_SERIALIZE_VALUE_2(float_val)
	SIM_JSON_SERIALIZE_VALUE_2(size_t_val)
	SIM_JSON_SERIALIZE_VALUE_2(long_val)
	SIM_JSON_SERIALIZE_VALUE_2(llong_val)
	SIM_JSON_SERIALIZE_VALUE_2(ullong_val)
	SIM_JSON_SERIALIZE_VALUE_2(string_val)
	SIM_JSON_SERIALIZE_VALUE_2(enum_val)
	SIM_JSON_SERIALIZE_VALUE_2(struct_val)
	SIM_JSON_SERIALIZE_VALUE_2(vec_val)
SIM_DEF_JSON_SERIALIZE_STRUCT_END(Test1)

//序列化
SIM_TEST(JsonSerialize)
{
	Test1 t2,t1;

	//创建一个空的
	JsonObjectPtr ptr = JsonObject::NewNull();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL(JSON_NULL, ptr->GetType());

	//初始化
	t1.bool_val = true;
	t1.char_val = 'A';
	t1.double_val = -0.123456;
	t1.enum_val = MyEnumTest1_444;
	t1.float_val = 0.123;
	t1.int_val = -1;
	t1.llong_val = -123456;
	t1.long_val = -11;
	t1.size_t_val = 99999;
	t1.string_val = "ni hao shijie";
	t1.struct_val.val = 22;
	t1.uchar_val = 32;
	t1.uint_val = 10;
	t1.ullong_val = 9999999;
	Test1chlids temp;
	temp.val = 10;
	t1.vec_val.push_back(temp);
	//序列化
	SIM_TEST_IS_TRUE(ptr->Serialize(t1));

	//对比值
	JsonObjectPtr pobj = ptr->FindByName("bool_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_BOOL, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.bool_val, pobj->GetBoolen());

	pobj = ptr->FindByName("char_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.char_val, (char)pobj->GetNumber());

	pobj = ptr->FindByName("double_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.double_val, pobj->GetNumber());

	pobj = ptr->FindByName("enum_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.enum_val, pobj->GetNumber());

	pobj = ptr->FindByName("float_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.float_val, pobj->GetNumber());

	pobj = ptr->FindByName("int_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.int_val, pobj->GetNumber());


	pobj = ptr->FindByName("llong_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.llong_val, pobj->GetNumber());

	pobj = ptr->FindByName("long_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.long_val, pobj->GetNumber());

	pobj = ptr->FindByName("size_t_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.size_t_val, pobj->GetNumber());

	pobj = ptr->FindByName("string_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_STRING, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.string_val, pobj->GetString());

	pobj = ptr->FindByName("struct_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	JsonObjectPtr pobj1 = pobj->FindByName("val");
	SIM_ASSERT_IS_NOT_NULL(pobj1);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj1->GetType());
	SIM_TEST_IS_EQUAL(t1.struct_val.val, pobj1->GetNumber());

	pobj = ptr->FindByName("uchar_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.uchar_val, (unsigned char)pobj->GetNumber());

	pobj = ptr->FindByName("uint_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.uint_val, pobj->GetNumber());

	pobj = ptr->FindByName("ullong_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.ullong_val, (unsigned long long)pobj->GetNumber());

	pobj = ptr->FindByName("vec_val");
	SIM_ASSERT_IS_NOT_NULL(pobj);
	SIM_TEST_IS_EQUAL(JSON_ARRAY, pobj->GetType());
	SIM_TEST_IS_EQUAL(t1.vec_val.size(), pobj->Size());
	for (int i = 0; i < t1.vec_val.size(); ++i)
	{
		SIM_ASSERT_IS_NOT_NULL(pobj->FindByIndex(i));
		JsonObjectPtr pobj2 = pobj->FindByIndex(i)->FindByName("val");
		SIM_ASSERT_IS_NOT_NULL(pobj2);
		SIM_TEST_IS_EQUAL(JSON_NUMBER, pobj2->GetType());
		SIM_TEST_IS_EQUAL(t1.vec_val[i].val, pobj2->GetNumber());
	}

	//反序列化
	SIM_TEST_IS_TRUE(ptr->DeSerialize(t2));

	//对比值
	SIM_TEST_IS_TRUE(t1 == t2);

	JsonObject::Free(ptr);
}

//JSON打印
SIM_TEST(JsonPrint)
{
	JsonObjectPtr ptr = JsonObject::NewObject();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("{}", ptr->Print(false));
	SIM_TEST_IS_EQUAL("{\n\n}", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":{}", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : \n{\n\n}", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewArray();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("[]", ptr->Print(false));
	SIM_TEST_IS_EQUAL("[\n\n]", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":[]", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : \n[\n\n]", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewNull();
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("", ptr->Print(false));
	SIM_TEST_IS_EQUAL("", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : ", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewBoolen(true);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("true", ptr->Print(false));
	SIM_TEST_IS_EQUAL("true", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":true", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : true", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewNumber(123456);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("123456", ptr->Print(false));
	SIM_TEST_IS_EQUAL("123456", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":123456", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : 123456", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewNumber(0.123456);
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("0.123456", ptr->Print(false));
	SIM_TEST_IS_EQUAL("0.123456", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":0.123456", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : 0.123456", ptr->Print(true));
	JsonObject::Free(ptr);

	ptr = JsonObject::NewString("string");
	SIM_ASSERT_IS_NOT_NULL(ptr);
	SIM_TEST_IS_EQUAL("\"string\"", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"string\"", ptr->Print(true));
	ptr->SetName("name");
	SIM_TEST_IS_EQUAL("\"name\":\"string\"", ptr->Print(false));
	SIM_TEST_IS_EQUAL("\"name\" : \"string\"", ptr->Print(true));
	JsonObject::Free(ptr);
}

//文件读写
SIM_TEST(JsonFile)
{
	Test1 t2, t1;

	//创建一个空的
	JsonObjectPtr ptr = JsonObject::NewNull();
	SIM_ASSERT_IS_NOT_NULL(ptr);

	//初始化
	t1.bool_val = true;
	t1.char_val = 'A';
	t1.double_val = -0.123456;
	t1.enum_val = MyEnumTest1_444;
	t1.float_val = 0.123;
	t1.int_val = -1;
	t1.llong_val = -123456;
	t1.long_val = -11;
	t1.size_t_val = 99999;
	t1.string_val = "ni hao shijie";
	t1.struct_val.val = 22;
	t1.uchar_val = 32;
	t1.uint_val = 10;
	t1.ullong_val = 9999999;
	Test1chlids temp;
	temp.val = 10;
	t1.vec_val.push_back(temp);

	//序列化
	SIM_TEST_IS_TRUE(ptr->Serialize(t1));
	//写文件
	SIM_TEST_IS_TRUE(ptr->SaveFile("JsonFile.json"));

	JsonObject::Free(ptr);

	//读文件
	JsonObjectPtr ptr1 = JsonObject::ReadFile("JsonFile.json");
	SIM_ASSERT_IS_NOT_NULL(ptr1);

	//反序列化
	SIM_TEST_IS_TRUE(ptr1->DeSerialize(t2));

	JsonObject::Free(ptr1);

	//对比值
	SIM_TEST_IS_TRUE(t1 == t2);
	
}

//元素操作
SIM_TEST(JsonItemOperator)
{
	//创建一个空的
	JsonObjectPtr ptr = JsonObject::NewObject();
	SIM_ASSERT_IS_NOT_NULL(ptr);

	SIM_TEST_IS_TRUE(ptr->ObjectAddBoolen("bool", true));
	SIM_TEST_IS_TRUE(ptr->ObjectAddNumber("number", 123456789));
	SIM_TEST_IS_TRUE(ptr->ObjectAddString("string", "str"));
	
	SIM_TEST_IS_EQUAL(3, ptr->Size());

	JsonObjectPtr temp = ptr->FindByName("bool");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(true, temp->GetBoolen());

	temp = ptr->FindByName("number");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL(123456789, (long long)temp->GetNumber());

	temp = ptr->FindByName("string");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL("str", temp->GetString());

	SIM_TEST_IS_TRUE(ptr->Replace(JsonObject::NewString("ddd"), "string"));

	temp = ptr->FindByName("string");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL("ddd", temp->GetString());

	SIM_TEST_IS_TRUE(ptr->Replace(JsonObject::NewString("ddd1","d"), "string"));
	temp = ptr->FindByName("string");
	SIM_ASSERT_IS_NULL(temp);
	temp = ptr->FindByName("d");
	SIM_ASSERT_IS_NOT_NULL(temp);
	SIM_TEST_IS_EQUAL("ddd1", temp->GetString());

	SIM_TEST_IS_TRUE(ptr->Del("d"));
	SIM_TEST_IS_EQUAL(2, ptr->Size());
	temp = ptr->FindByName("d");
	SIM_ASSERT_IS_NULL(temp);


	JsonObject::Free(ptr);
}
//内存测试
SIM_TEST(JsonItemMemory)
{
	for (unsigned long long i = 0; i < 10000000; ++i)
	{
		//创建一个空的
		//JsonObjectPtr ptr = JsonObject::NewNull();
		////SIM_ASSERT_IS_NOT_NULL(ptr);

		//Test1 t2, t1;
		////初始化
		//t1.bool_val = true;
		//t1.char_val = 'A';
		//t1.double_val = -0.123456;
		//t1.enum_val = MyEnumTest1_444;
		//t1.float_val = 0.123;
		//t1.int_val = -1;
		//t1.llong_val = -123456;
		//t1.long_val = -11;
		//t1.size_t_val = 99999;
		//t1.string_val = "ni hao shijie";
		//t1.struct_val.val = 22;
		//t1.uchar_val = 32;
		//t1.uint_val = 10;
		//t1.ullong_val = 9999999;
		//Test1chlids temp;
		//temp.val = 10;
		//t1.vec_val.push_back(temp);

		////序列化
		//ptr->Serialize(t1);
		//写文件
		//std::string msg = ptr->Print();

		JsonObjectPtr ptr = JsonObject::NewObject();
		ptr->ObjectAddObject("null", JsonObject::NewBoolen(false));
		JsonObject::Free(ptr);
	}
}

SIM_TEST_MAIN(sim::noisy)