/*
* 一个简单的JSON解析器
*/
#ifndef SIM_JSON_HPP_
#define SIM_JSON_HPP_
#include <string>

namespace sim
{
	//声明前置
	struct JsonArrayNode;
	class JsonArray;
	class JsonObject;

    enum JsonObjectType
    {
        JSON_NULL,
        JSON_BOOL,
        JSON_NUMBER,
        JSON_STRING,
        JSON_ARRAY,
        JSON_OBJECT,
    };

	//重命名
    typedef double JsonNumber;
    typedef std::string JsonString;
	typedef JsonArrayNode* JsonArrayNodePtr;
	typedef JsonObject* JsonObjectPtr;

    struct JsonArrayNode
    {
        JsonObjectPtr ptr;
        JsonArrayNode* next;
    };
    

    class JsonArray
    {
	public:
		JsonArray();
		~JsonArray();

    public:
        //追加
		bool AddHead(JsonObjectPtr ptr);
        bool Append(JsonObjectPtr ptr);
        
		//指定位置后面插入，找不到返回错误
        bool Insert(JsonObjectPtr ptr, const int after_index);
        bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        
		//覆盖
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//删除
		bool Del(const int index);
		bool Del(const JsonString& name);

		//清空
		bool Clear();

        //遍历接口pre前一个，NULL 返回头节点
        JsonArrayNodePtr Next(JsonArrayNodePtr* pre=NULL);

        //返回大小
        unsigned int Size();

        JsonObjectPtr operator[](const int index);
        JsonObjectPtr operator[](const JsonString& name);

        JsonArrayNodePtr FindByIndex(int index);
        JsonArrayNodePtr FindByName(const JsonString& name);
    private:
        //新建空节点
        JsonArrayNodePtr NewEmptyNode()
        {
            JsonArrayNodePtr ptr = new JsonArrayNode;
            ptr->next = NULL;
            ptr->ptr = NULL;
        }
		//回收节点
		void DeleteNode(JsonArrayNodePtr ptr)
		{
			delete ptr;
		}
    private:
        JsonArrayNodePtr pbeg_;
    };

    class JsonObject
    {
    public:
    private:
        //名称
        JsonString name_;
        //子项
        JsonArray childs_;
        //类型
        JsonObjectType type_;
    private:
        JsonNumber number_;
        JsonString string_;
    };
}
#endif