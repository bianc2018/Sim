/*
* һ���򵥵�JSON������
*/
#ifndef SIM_JSON_HPP_
#define SIM_JSON_HPP_
#include <string>

namespace sim
{
	//����ǰ��
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

	//������
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
        //׷��
		bool AddHead(JsonObjectPtr ptr);
        bool Append(JsonObjectPtr ptr);
        
		//ָ��λ�ú�����룬�Ҳ������ش���
        bool Insert(JsonObjectPtr ptr, const int after_index);
        bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        
		//����
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//ɾ��
		bool Del(const int index);
		bool Del(const JsonString& name);

		//���
		bool Clear();

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
		//���սڵ�
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
}
#endif