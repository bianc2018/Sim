#include "Json.hpp"

enum MyEnum
{
	E1, E2, E3,KKKKKK=-9999
};
SIM_DEF_JSON_SERIALIZE_TYPE_AS_ENUM(MyEnum);
struct MyStruct
{
public:
	double num;
	std::string  helloword;
	MyEnum enums;
	std::vector<std::string> strs;
public:
};

SIM_DEF_JSON_SERIALIZE_STRUCT(MyStruct)
    SIM_JSON_SERIALIZE_VALUE_2(num)
	SIM_JSON_SERIALIZE_VALUE_2(helloword)
	SIM_JSON_SERIALIZE_VALUE_2(enums)
	SIM_JSON_SERIALIZE_VALUE_2(strs)
SIM_DEF_JSON_SERIALIZE_STRUCT_END(MyStruct)

struct MyStruct2
{
public:
	int num;
	std::string  helloword;
	MyStruct mystruct;
public:
	SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
		SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(num)
		SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(helloword)
		SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(mystruct)
	SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
	//ÐòÁÐ»¯º¯Êý
	/*template<typename archive>
	bool JsonSerializeFunc(archive& ar, bool isSerialize)
	{
		if (false == ar.Serialize("num", num, isSerialize, true))
			return false;
		if (false == ar.Serialize("helloword", helloword, isSerialize, true))
			return false;
		if (false == ar.Serialize("mystruct", mystruct, isSerialize, true))
			return false;
		return true;
	}*/
};

int main(int argc, char *argv[])
{
	sim::JsonObjectPtr ptr=sim::JsonObject::NewObject();
	//sim::JsonObjectPtr ptr = sim::JsonObject::ReadFile("test.json");

	MyStruct2 t;
	t.helloword = "hhh";
	t.num =12345;
	t.mystruct.enums = E1;
	t.mystruct.helloword = "aaa";
	t.mystruct.num = 10011;
	t.mystruct.strs = { "t1","t","t3","t4" };
	ptr->Serialize(t);


	ptr->SaveFile("test.json", true);
	sim::JsonObjectPtr ptr1 = ptr->Copy();
	sim::JsonObject::Free(ptr);
	ptr1->SaveFile("test1.json", true);
	sim::JsonObject::Free(ptr1);
	
	
	return 0;
}


