/*
* 一个简单的JSON解析器
*/
#ifndef SIM_JSON_HPP_
#define SIM_JSON_HPP_
#include <string>
#ifndef NOT_USE_SIM_REF_OBJECT
#include "RefObject.hpp"
#endif // !USING_SIM_REF_OBJECT


namespace sim
{
    enum JsonObjectType
    {
        JSON_NULL,
        JSON_BOOL,
        JSON_NUMBER,
        JSON_STRING,
        JSON_ARRAY,
        JSON_OBJECT,
    };

    typedef bool JsonBool;
    typedef double JsonNumber;
    typedef std::string JsonString;

    class JsonObject;

#ifndef NOT_USE_SIM_REF_OBJECT
    typedef sim::RefObject<JsonObject> JsonObjectPtr;
#else
    typedef JsonObject* JsonObjectPtr;
#endif // !USING_SIM_REF_OBJECT

    struct JsonArrayNode
    {
        JsonObjectPtr ptr;
        JsonArrayNode* next;
    };
    typedef JsonArrayNode* JsonArrayNodePtr;

    class JsonArray
    {
    public:
        //末尾追加
        bool Append(JsonObjectPtr ptr);
        //指定位置后面插入，找不到返回错误
        bool Insert(JsonObjectPtr ptr, const int after_index);
        bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        //覆盖
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);
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

    class Json
    {
    public:
        JsonObjectPtr CreateEmpty();//创建一个空的
        JsonObjectPtr Create(JsonNumber val);//创建一个空的
        JsonObjectPtr Create(JsonNumber val);//创建一个空的
    };

}
#endif