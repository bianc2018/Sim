#include "Json.hpp"
struct MyStruct
{
public:
	double num;
	std::string  helloword;
public:
	//序列化函数
	template<typename archive>
	bool JsonSerializeFunc(archive& ar, bool isSerialize)
	{
		if (false == ar.Serialize("num", num, isSerialize, true))
			return false;
		if (false == ar.Serialize("helloword", helloword, isSerialize, true))
			return false;
		return true;
	}
};

struct MyStruct2
{
public:
	int num;
	std::string  helloword;
	MyStruct mystruct;
public:
	//序列化函数
	template<typename archive>
	bool JsonSerializeFunc(archive& ar, bool isSerialize)
	{
		if (false == ar.Serialize("num", num, isSerialize, true))
			return false;
		if (false == ar.Serialize("helloword", helloword, isSerialize, true))
			return false;
		if (false == ar.Serialize("mystruct", mystruct, isSerialize, true))
			return false;
		return true;
	}
};
int main(int argc, char *argv[])
{
	sim::JsonObjectPtr ptr=sim::JsonObject::NewObject();
	MyStruct2 t;
	t.helloword = "aaa";
	t.num = 100;
	t.mystruct.helloword = "hellow";
	t.mystruct.num = 20.1234567;
	ptr->Serialize(t);
	ptr->SaveFile("test.json", true);

	sim::JsonObject::Free(ptr);
	
	return 0;
}


