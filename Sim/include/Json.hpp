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
        
		////ָ��λ�ú�����룬�Ҳ������ش���
  //      bool Insert(JsonObjectPtr ptr, const int after_index);
  //      bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        
		//����
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//ɾ��
		bool Del(const int index);
		bool Del(const JsonString& name);

		//���
		bool Clear();

        //�����ӿ�preǰһ����NULL ����ͷ�ڵ�
        JsonArrayNodePtr Next(JsonArrayNodePtr pre=NULL);

        //���ش�С
        unsigned int Size();

        JsonObjectPtr operator[](const int index);
        JsonObjectPtr operator[](const JsonString& name);

        JsonArrayNodePtr FindByIndex(int index);
        JsonArrayNodePtr FindByName(const JsonString& name);
    private:
        //�½��սڵ�
        JsonArrayNodePtr NewEmptyNode();
		//���սڵ�
        void DeleteNode(JsonArrayNodePtr ptr);
    private:
        JsonArrayNodePtr pbeg_;
    };

    class JsonObject
    {
        //
        JsonObject():type(JSON_NULL), number(0.0){}
    public:
        ~JsonObject() {};
    public:
        //API
        //�½�һ���յĶ���
        static JsonObjectPtr New();
        static void Free(JsonObjectPtr ptr);
        //����json�����ַ�����ʧ�ܷ��ؿ�
        static JsonObjectPtr Parser(const JsonString&json);
    public:
        //���һ��
        bool Add(JsonObjectPtr ptr);
    private:

    public:
        //����
        JsonString name;
        //����
        JsonArray childs;
        //����
        JsonObjectType type;
        //JSON_BOOL,JSON_NUMBER,
        JsonNumber number;
        //JSON_STRING
        JsonString str;
    };

    //JsonArray
    inline JsonArray::JsonArray():pbeg_(NULL)
    {
    }

    inline JsonArray::~JsonArray()
    {
        Clear();
    }

    inline bool JsonArray::AddHead(JsonObjectPtr ptr)
    {
        if (NULL == ptr)
            return false;
        //�����µ�
        JsonArrayNodePtr New = NewEmptyNode();
        New->ptr = ptr;

        if (NULL == pbeg_)
        {
            pbeg_ = New;
        }
        else
        {
            New->next = pbeg_;
            pbeg_ = New;
        }

        return true;
    }

    inline bool JsonArray::Append(JsonObjectPtr ptr)
    {
        if (NULL == ptr)
            return false;
        //�����µ�
        JsonArrayNodePtr New = NewEmptyNode();
        New->ptr = ptr;

        if (NULL == pbeg_)
        {
            pbeg_ = New;
        }
        else
        {
            JsonArrayNodePtr iter = Next(NULL);
            while (iter->next != NULL)
            {
                iter = Next(iter);
            }
            iter->next = New;
        }
        
        return true;

    }

    inline bool JsonArray::Replace(JsonObjectPtr ptr, const int index)
    {
        if (NULL == ptr)
            return false;
        JsonArrayNodePtr dst = FindByIndex(index);
        if (dst)
        {
            JsonObject::Free(dst->ptr);
            dst->ptr = ptr;
            return true;
        }
        return false;
    }

    inline bool JsonArray::Replace(JsonObjectPtr ptr, const JsonString& name)
    {
        if (NULL == ptr)
            return false;
        JsonArrayNodePtr dst = FindByName(name);
        if (dst)
        {
            JsonObject::Free(dst->ptr);
            dst->ptr = ptr;
            return true;
        }
        return false;
    }

    inline bool JsonArray::Del(const int index)
    {
        if (index <= -1)
            return false;

        unsigned int i = 0;
        JsonArrayNodePtr pre = NULL;
        JsonArrayNodePtr iter = Next(pre);
        while (iter != NULL)
        {
            if (i == index)
            {
                if (pre)
                {
                    pre->next = iter->next;
                }
                else
                {
                    //ɾ��ͷ�ڵ�
                    pbeg_ = iter->next;
                }
                DeleteNode(iter);
                return true;
            }
            ++i;
            pre = iter;
            iter = Next(pre);
        }
        return false;
    }

    inline bool JsonArray::Del(const JsonString& name)
    {
        JsonArrayNodePtr pre = NULL;
        JsonArrayNodePtr iter = Next(pre);
        while (iter != NULL)
        {
            if (iter->ptr && name == iter->ptr->name)
            {
                if (pre)
                {
                    pre->next = iter->next;
                }
                else
                {
                    pbeg_ = iter->next;
                }
                DeleteNode(iter);
                return true;
            }
            pre = iter;
            iter = Next(pre);
        }
        return false;
    }

    inline bool JsonArray::Clear()
    {
        JsonArrayNodePtr iter = Next(NULL);
        while (iter != NULL)
        {
            JsonArrayNodePtr temp = iter;
            iter = Next(temp);
            DeleteNode(temp);
        }
        return true;
    }

    inline JsonArrayNodePtr JsonArray::Next(JsonArrayNodePtr pre)
    {
        if (pre)
            return pre->next;
        return pbeg_;
    }

    inline unsigned int JsonArray::Size()
    {
        unsigned int i = 0;
        JsonArrayNodePtr iter = Next(NULL);
        while (iter != NULL)
        {
            ++i;
            iter = Next(iter);
        }
        return i;
    }

    inline JsonObjectPtr JsonArray::operator[](const int index)
    {
        return FindByIndex(index);
    }

    inline JsonObjectPtr JsonArray::operator[](const JsonString& name)
    {
        return FindByName(name);
    }

    inline JsonArrayNodePtr JsonArray::FindByIndex(int index)
    {
        if (index <= -1)
            return NULL;
        int i = 0;
        JsonArrayNodePtr iter = Next(NULL);
        while (iter != NULL)
        {
            if (i == index)
                return iter;
            ++i;
            iter = Next(iter);
        }
        return iter;
    }

    inline JsonArrayNodePtr JsonArray::FindByName(const JsonString& name)
    {
        JsonArrayNodePtr iter = Next(NULL);
        while (iter != NULL)
        {
            if (iter->ptr&&name == iter->ptr->name)
                return iter;
            iter = Next(iter);
        }
        return iter;
    }

    inline JsonArrayNodePtr JsonArray::NewEmptyNode()
    {
        JsonArrayNodePtr ptr = new JsonArrayNode;
        ptr->next = NULL;
        ptr->ptr = NULL;
        return ptr;
    }

    inline void JsonArray::DeleteNode(JsonArrayNodePtr ptr)
    {
        //ֻ�ǻ�����Դ,������������
        if (ptr && ptr->ptr)
        {
            JsonObject::Free(ptr->ptr);
            delete ptr;
        }
    }

    //JsonObject
    inline JsonObjectPtr JsonObject::New()
    {
        return new JsonObject();
    }
    inline void JsonObject::Free(JsonObjectPtr ptr)
    {
        if(ptr)
            delete ptr;
    }
}
#endif