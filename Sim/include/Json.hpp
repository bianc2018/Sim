/*
* һ���򵥵�JSON������
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
        //ĩβ׷��
        bool Append(JsonObjectPtr ptr);
        //ָ��λ�ú�����룬�Ҳ������ش���
        bool Insert(JsonObjectPtr ptr, const int after_index);
        bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        //����
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);
        //�����ӿ�preǰһ����NULL ����ͷ�ڵ�
        JsonArrayNodePtr Next(JsonArrayNodePtr* pre=NULL);
        //���ش�С
        unsigned int Size();

        JsonObjectPtr operator[](const int index);
        JsonObjectPtr operator[](const JsonString& name);

        JsonArrayNodePtr FindByIndex(int index);
        JsonArrayNodePtr FindByName(const JsonString& name);
    private:
        //�½��սڵ�
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
        //����
        JsonString name_;
        //����
        JsonArray childs_;
        //����
        JsonObjectType type_;
    private:
        JsonNumber number_;
        JsonString string_;
    };

    class Json
    {
    public:
        JsonObjectPtr CreateEmpty();//����һ���յ�
        JsonObjectPtr Create(JsonNumber val);//����һ���յ�
        JsonObjectPtr Create(JsonNumber val);//����һ���յ�
    };

}
#endif