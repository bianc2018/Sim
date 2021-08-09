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
        
		////指定位置后面插入，找不到返回错误
  //      bool Insert(JsonObjectPtr ptr, const int after_index);
  //      bool Insert(JsonObjectPtr ptr, const JsonString& after_name);
        
		//覆盖
        bool Replace(JsonObjectPtr ptr,const int index);
        bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//删除
		bool Del(const int index);
		bool Del(const JsonString& name);

		//清空
		bool Clear();

        //遍历接口pre前一个，NULL 返回头节点
        JsonArrayNodePtr Next(JsonArrayNodePtr pre=NULL);

        //返回大小
        unsigned int Size();

        JsonObjectPtr operator[](const int index);
        JsonObjectPtr operator[](const JsonString& name);

        JsonArrayNodePtr FindByIndex(int index);
        JsonArrayNodePtr FindByName(const JsonString& name);
    private:
        //新建空节点
        JsonArrayNodePtr NewEmptyNode();
		//回收节点
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
        //新建一个空的对象
        static JsonObjectPtr New();
        static void Free(JsonObjectPtr ptr);
        //解析json数据字符串，失败返回空
        static JsonObjectPtr Parser(const JsonString&json);
    public:
        //添加一个
        bool Add(JsonObjectPtr ptr);
    private:

    public:
        //名称
        JsonString name;
        //子项
        JsonArray childs;
        //类型
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
        //创建新的
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
        //创建新的
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
                    //删除头节点
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
        //只是回收资源,不管其他事情
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