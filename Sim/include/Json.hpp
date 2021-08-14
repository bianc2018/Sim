/*
* 一个简单的JSON解析器
*/
#ifndef SIM_JSON_HPP_
#define SIM_JSON_HPP_
#include <string>
#include <sstream>
#include <vector>

//实用的JSON序列化定义宏
#define SIM_DEF_JSON_SERIALIZE_TYPE(type) \
bool SerializeValueFormJson(sim::JsonObjectPtr pjson, type & t, bool isSerialize)\
{

#define SIM_DEF_JSON_SERIALIZE_TYPE_END(type) \
}

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,jsontype)\
   SIM_DEF_JSON_SERIALIZE_TYPE(type)\
        return sim::serialize::SerializeValueFormJson(pjson, t,jsontype, isSerialize);\
   SIM_DEF_JSON_SERIALIZE_TYPE_END(type)

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_ENUM(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE(type)\
        int temp=0;\
        if(isSerialize) temp=(int)(t);\
        bool ret=sim::serialize::SerializeValueFormJson(pjson, temp,sim::JsonObjectType::JSON_NUMBER, isSerialize);\
        if(!isSerialize) t=(type)(temp);\
        return ret;\
   SIM_DEF_JSON_SERIALIZE_TYPE_END(type)

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_NUM(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JsonObjectType::JSON_NUMBER)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_BOOL(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JsonObjectType::JSON_BOOL)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_STR(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JsonObjectType::JSON_STRING)

//定义序列化函数 在结构体外部
#define SIM_DEF_JSON_SERIALIZE_STRUCT(type) \
template<typename archive>\
bool JsonSerializeFunc(archive& ar, type& t, bool isSerialize)\
{
#define SIM_JSON_SERIALIZE_VALUE(name,value,ismust)\
	if (false == ar.Serialize(name, t.value, isSerialize, ismust))\
	    return false;
#define SIM_JSON_SERIALIZE_VALUE_1(name,ismust) SIM_JSON_SERIALIZE_VALUE(#name,name,ismust)
#define SIM_JSON_SERIALIZE_VALUE_2(name) SIM_JSON_SERIALIZE_VALUE_1(name,true)
#define SIM_DEF_JSON_SERIALIZE_STRUCT_END(type) \
       return true;\
}

//定义序列化函数 在结构体内部
#define SIM_DEF_JSON_SERIALIZE_IN_STRUCT() \
template<typename archive>\
bool JsonSerializeFunc(archive& ar, bool isSerialize)\
{
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT(name,value,ismust)\
	if (false == ar.Serialize(name, value, isSerialize, ismust))\
	    return false;
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_1(name,ismust) SIM_JSON_SERIALIZE_VALUE_IN_STRUCT(#name,name,ismust)
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(name) SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_1(name,true)
#define SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END() \
       return true;\
}
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
		JsonArrayNodePtr next;
	};


	class JsonArray
	{
		friend class JsonObject;
		JsonArray();
		~JsonArray();		
	public:
		bool CopyTo(JsonArray&arr);
		//追加
		bool AddHead(JsonObjectPtr ptr);
		bool Append(JsonObjectPtr ptr);

		////指定位置后面插入，找不到返回错误
  //      bool Insert(JsonObjectPtr ptr, const int after_index);
  //      bool Insert(JsonObjectPtr ptr, const JsonString& after_name);

		//覆盖
		bool Replace(JsonObjectPtr ptr, const int index);
		bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//删除
		bool Del(const int index);
		bool Del(const JsonString& name);

		//清空子节点
		bool Clear();

		//遍历接口pre前一个，NULL 返回头节点
		JsonArrayNodePtr Next(JsonArrayNodePtr pre = NULL);

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
		friend class JsonArray;
		//
		JsonObject(JsonObjectType t= JSON_NULL) :type_(t), number_(0.0) {}
	public:
		~JsonObject();
	public:
		//API
		//新建一个空的对象
		static JsonObjectPtr NewNull();
		static JsonObjectPtr NewObject();
		static JsonObjectPtr NewArray();
		static JsonObjectPtr NewString(const JsonString&json);
		static JsonObjectPtr NewNumber(const JsonNumber&str);
		static JsonObjectPtr NewBoolen(bool b);
		static void Free(JsonObjectPtr ptr);
		
		//拷贝；
		static JsonObjectPtr Copy(JsonObjectPtr src);

		//解析json数据字符串，失败返回空
		static JsonObjectPtr Parser(const JsonString&json);
		JsonString Print(bool f=true,unsigned w=0);

		//从文件中加载
		static JsonObjectPtr ReadFile(const JsonString&filename);
		bool SaveFile(const JsonString & filename,bool f = true);

		//结构到JSON
		template<typename T>
		bool Serialize(T&t);

		//JSON到结构
		template<typename T>
		bool DeSerialize(T&t);
	public:
		JsonObjectPtr Copy();

		//重置对象
		bool Reset();
		//重置值为空
		bool ResetValue();
		//数据接口
		JsonString GetName();
		void SetName(const JsonString&str);

		//获取数值，会自动转换类型
		/*template<typename T>
		T GetValue();
		template<typename T>
		void SetValue(const T&t);*/

		//对象类型 
		JsonObjectType GetType();
		//设置类型 if type!=JSON_NULL&&reset=false return false
		bool SetType(JsonObjectType t, bool reset = false);

		//获取字符串 if type == JSON_STRING
		JsonString GetString();
		// if type!=JSON_STRING&&reset=false return false
		bool SetString(const JsonString& str, bool reset = false);

		JsonNumber GetNumber();
		bool SetNumber(const JsonNumber& num, bool reset = false);

		bool GetBoolen();
		bool SetBoolen(const bool& b, bool reset = false);

		//判断是否为 NULL
		bool IsNull();
	public:
		bool ArrayAddItem(JsonObjectPtr ptr);
		bool ArrayAddString(const JsonString&str);
		bool ArrayAddNumber(const JsonNumber&number);
		bool ArrayAddBoolen(bool b);
		//添加一个对象
		bool ObjectAddObject(const JsonString&name,JsonObjectPtr ptr);
		bool ObjectAddString(const JsonString&name,const JsonString&str);
		bool ObjectAddNumber(const JsonString&name, const JsonNumber&number);
		bool ObjectAddBoolen(const JsonString&name, bool b);
	public:
		//追加
		bool AddHead(JsonObjectPtr ptr);
		bool Append(JsonObjectPtr ptr);

		//覆盖
		bool Replace(JsonObjectPtr ptr, const int index);
		bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//删除
		bool Del(const int index);
		bool Del(const JsonString& name);

		//清空子节点
		bool ClearChilds();

		//返回大小
		unsigned int Size();

		JsonObjectPtr operator[](const int index);
		JsonObjectPtr operator[](const JsonString& name);

	private:
		bool Parser(const char*pdata, unsigned int len, unsigned int &offset);
		bool ParserName(const char*pdata, unsigned int len, unsigned int &offset);
		//解析值
		bool ParserValue(const char*pdata, unsigned int len, unsigned int &offset);
		//解析子节点
		bool ParserChilds(const char*pdata, unsigned int len, unsigned int &offset, bool isArrays);
		bool ParserNumber(const char*pdata, unsigned int len, unsigned int &offset, JsonNumber &num);
		bool ParserBoolen(const char*pdata, unsigned int len, unsigned int &offset, JsonNumber &num);
		bool ParserNull(const char*pdata, unsigned int len, unsigned int &offset);
		//跳过空格
		bool IsSpace(char end);
		void SkipSpace(const char*pdata, unsigned int len, unsigned int &offset);
		bool FindStringEnd(const char*pdata, unsigned int len, unsigned int &offset, char end, JsonString& str);
		//大小写
		JsonString ToLower(const JsonString&s)
		{
			JsonString res;
			for (int i = 0; i < s.size(); ++i)
				if (s[i] > 'A'&&s[i] < 'Z')
					res += ('a' + s[i] - 'A');
				else
					res += s[i];
			return res;
		}
	private:
		//名称
		JsonString name_;
		//子项
		JsonArray childs_;
		//类型
		JsonObjectType type_;
		//JSON_BOOL,JSON_NUMBER,
		JsonNumber number_;
		//JSON_STRING
		JsonString string_;
	};

	//序列化
	namespace serialize
	{
		//stringstream 进行类型转换
		template<typename SrcType, typename DstType>
		bool TypeCast(const SrcType &t1, DstType &t2);

		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, T& t, bool isSerialize);

		//自动转换
		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, T& t, JsonObjectType jsontype, bool isSerialize);

		template<typename T>
		bool SerializeValueFormJsonNum(JsonObjectPtr pjson, T& t, bool isSerialize);

		bool SerializeValueFormJson(JsonObjectPtr pjson, bool& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, int& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, float& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, double& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, std::string& t, bool isSerialize);
		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, std::vector<T>& arrs, bool isSerialize);
		//序列化函数
		template<typename archive, typename T>
		bool JsonSerializeFunc(archive& ar, T& t, bool isSerialize);

		//序列化
		class JsonSerialize
		{
		public:
			JsonSerialize(JsonObjectPtr ref) :ptr_(ref), json_object_is_release_(false) {};
			JsonSerialize() :ptr_(JsonObject::NewObject()), json_object_is_release_(true) {};
			~JsonSerialize() { if (ptr_&&json_object_is_release_)JsonObject::Free(ptr_); }
			//序列化接口
			template<typename T>
			bool Serialize(const JsonString&key,T&t, bool isSerialize, bool isMust);
			template<typename T>
			bool Serialize(T&t, bool isSerialize);
		private:
			JsonObjectPtr ptr_;
			bool json_object_is_release_;
		};
		
		template<typename SrcType, typename DstType>
		bool TypeCast(const SrcType& t1, DstType& t2)
		{
			try
			{
				std::stringstream ss;
				ss << t1;
				ss >> t2;
				return true;
			}
			catch (const std::exception&)
			{
				return false;
			}
			
		}
		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, T& t,
			JsonObjectType jsontype, bool isSerialize)
		{
			if (isSerialize)
			{
				pjson->SetType(jsontype, true);
				if (jsontype == JSON_BOOL)
				{
					bool val = false;
					if (false == TypeCast(t, val))
						return false;
					if (false == pjson->SetBoolen(val))
						return false;
				}
				else if (jsontype == JSON_NUMBER)
				{
					JsonNumber val = 0.0;
					if (false == TypeCast(t, val))
						return false;
					if (false == pjson->SetNumber(val))
						return false;
				}
				else if (jsontype == JSON_STRING)
				{
					JsonString val = "";
					if (false == TypeCast(t, val))
						return false;
					if (false == pjson->SetString(val))
						return false;
				}
				else
				{
					return false;
				}
				return true;
			}
			else
			{
				if (pjson->GetType() != jsontype)
				{
					return false;
				}
				if (jsontype == JSON_BOOL)
				{
					bool val = pjson->GetBoolen();
					if (false == TypeCast(val, t))
						return false;
				}
				else if (jsontype == JSON_NUMBER)
				{
					JsonNumber val = pjson->GetNumber();
					if (false == TypeCast(val, t))
						return false;
				}
				else if (jsontype == JSON_STRING)
				{
					JsonString val = pjson->GetString();
					if (false == TypeCast(val, t))
						return false;
				}
				else
				{
					return false;
				}
				return true;
			}
		}

		template<typename T>
		inline bool  JsonSerialize::Serialize(const JsonString & key, T & t, bool isSerialize,bool isMust)
		{
			// TODO: 在此处插入 return 语句
			if (NULL == ptr_)
				return false;
			JsonObjectPtr childs = NULL;
			if (isSerialize)
			{
				//转换为字符串
				childs = JsonObject::NewNull();
				if (NULL == childs)
					return false;
				if (false == ptr_->ObjectAddObject(key, childs))
					return false;
				
			}
			else
			{
				childs = (*ptr_)[key];
				if (NULL == childs)
				{
					if (isMust)
						return false;
					else
						return true;
				}
			}
			
			return SerializeValueFormJson(childs, t, isSerialize);
		}

		template<typename T>
		inline bool  JsonSerialize::Serialize(T & t, bool isSerialize)
		{
			// TODO: 在此处插入 return 语句
			return JsonSerializeFunc(*this, t, isSerialize);
		}

		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, T & t, bool isSerialize)
		{
			if (isSerialize)
				pjson->SetType(JSON_OBJECT, true);
			JsonSerialize ar(pjson);
			return ar.Serialize(t,isSerialize);
		}

		template<typename T>
		bool SerializeValueFormJsonNum(JsonObjectPtr pjson, T & t, bool isSerialize)
		{
			if (isSerialize)
			{
				pjson->SetType(JSON_NUMBER, true);
				pjson->SetNumber((double)(t));
				return true;
			}
			else
			{
				if (pjson->GetType() == JSON_NUMBER)
				{
					t = (T)(pjson->GetNumber());
					return true;
				}
			}
			return false;
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, bool & t, bool isSerialize)
		{
			if (isSerialize)
			{
				pjson->SetType(JSON_BOOL, true);
				pjson->SetBoolen(t);
				return true;
			}
			else
			{
				if (pjson->GetType() == JSON_BOOL)
				{
					t = pjson->GetBoolen();
					return true;
				}
			}
			return false;
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, int & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, long & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned long & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, float & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, double & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, std::string & t, bool isSerialize)
		{
			if (isSerialize)
			{
				pjson->SetType(JSON_STRING, true);
				pjson->SetString(t);
				return true;
			}
			else
			{
				if (pjson->GetType() == JSON_NUMBER)
				{
					t = pjson->GetString();
					return true;
				}
			}
			return false;
		}

		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, std::vector<T>& arrs, bool isSerialize)
		{
			if (isSerialize)
			{
				pjson->SetType(JSON_ARRAY, true);
				size_t size = arrs.size();
				for (int i = 0; i < size; ++i)
				{
					JsonObjectPtr item = JsonObject::NewNull();
					if (false == SerializeValueFormJson(item, arrs[i], isSerialize))
					{
						JsonObject::Free(item);
						return false;
					}
					pjson->ArrayAddItem(item);
				}
				return true;
			}
			else
			{
				if (pjson->GetType() == JSON_ARRAY)
				{
					size_t size = pjson->Size();
					for (int i = 0; i < size; ++i)
					{
						T temp;
						if (false == SerializeValueFormJson(pjson->operator[](i), temp, isSerialize))
						{
							return false;
						}
						arrs.push_back(temp);
					}
					return true;
				}
			}
			return false;
		}

		template<typename archive, typename T>
		bool JsonSerializeFunc(archive & ar, T & t, bool isSerialize)
		{
			return t.JsonSerializeFunc(ar, isSerialize);
		}


		//定义通用接口
	}

	//JsonArray
	inline JsonArray::JsonArray() :pbeg_(NULL)
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
			if (iter->ptr && name == iter->ptr->name_)
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
		pbeg_ = NULL;
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
		JsonArrayNodePtr ptr = FindByIndex(index);
		if (ptr)
			return ptr->ptr;
		return NULL;
	}

	inline JsonObjectPtr JsonArray::operator[](const JsonString& name)
	{
		JsonArrayNodePtr ptr = FindByName(name);
		if (ptr)
			return ptr->ptr;
		return NULL;
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
			if (iter->ptr&&name == iter->ptr->name_)
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
			//delete ptr;
		}
	}

	inline JsonObject::~JsonObject()
	{
		Reset();
	}

	inline JsonObjectPtr JsonObject::NewNull()
	{
		return new JsonObject(JSON_NULL);
	}

	inline bool JsonArray::CopyTo(JsonArray& arr)
	{
		arr.Clear();

		JsonArrayNodePtr iter = Next(NULL);
		while (iter != NULL)
		{
			JsonObjectPtr newit = iter->ptr->Copy();
			if (NULL == newit)
				return false;
			arr.Append(newit);
			iter = Next(iter);
		}
		return true;
	}

	//JsonObject
	inline JsonObjectPtr JsonObject::NewObject()
	{
		return new JsonObject(JSON_OBJECT);
	}
	inline JsonObjectPtr JsonObject::NewArray()
	{
		return new JsonObject(JSON_ARRAY);
	}
	inline JsonObjectPtr JsonObject::NewString(const JsonString & str)
	{
		JsonObjectPtr p= new JsonObject(JSON_STRING);
		p->string_ = str;
		return p;
	}
	inline JsonObjectPtr JsonObject::NewNumber(const JsonNumber & number)
	{
		JsonObjectPtr p = new JsonObject(JSON_NUMBER);
		p->number_ = number;
		return p;
	}
	inline JsonObjectPtr JsonObject::NewBoolen(bool b)
	{
		JsonObjectPtr p = new JsonObject(JSON_BOOL);
		if(b)
			p->number_ = 1;
		else
			p->number_ = 0;
		return p;
	}
	inline void JsonObject::Free(JsonObjectPtr ptr)
	{
		if (ptr)
			delete ptr;
	}

	inline JsonObjectPtr JsonObject::Parser(const JsonString & json)
	{
		JsonObjectPtr ptr = JsonObject::NewObject();
		if (NULL == ptr)
			return NULL;
		unsigned int offset = 0;
		if (ptr->Parser(json.c_str(), json.size(), offset))
			return ptr;
		JsonObject::Free(ptr);
		return NULL;
	}

	inline JsonString JsonObject::Print(bool f, unsigned w)
	{
		JsonString space,crlf,tab,start;
		
		if (f)
		{
			space = " ";
			crlf = "\n";
			tab = "\t";
			for (int i = 0; i < w; ++i)
				start += tab;
		}
		JsonString json= start;
		if (!name_.empty())
			json += "\"" + name_ + "\""+ space +":"+ space;

		switch (type_)
		{
		case sim::JSON_NULL:
			break;
		case sim::JSON_BOOL:
		{
			if (number_ == 0)
				json += "false";
			else
				json += "true";
			break;
		}
		case sim::JSON_NUMBER:
		{
			const int buff_size = 256;
			char buff[buff_size] = { 0 };
			if (floor(number_ + 0.5) == number_)
			{
				//整数
				snprintf(buff, buff_size, "%lld", static_cast<long long>(number_));

			}
			else
			{
				//浮点数
				snprintf(buff, buff_size, "%lf", number_);
			}
			json += buff;
			break;
		}
		case sim::JSON_STRING:
		{
			json += "\""+string_+"\"";
			break;
		}
		case sim::JSON_ARRAY:
		case sim::JSON_OBJECT:
		{
			if (!name_.empty())
				json += crlf;

			bool isFirst = true;
			if (type_ == JSON_ARRAY)
				json += start+ "[" + crlf;
			else
				json += start+ "{" + crlf;

			JsonArrayNodePtr iter = childs_.Next(NULL);
			while (iter != NULL)
			{
				if (isFirst)
				{
					isFirst = false;
					json += iter->ptr->Print(f, w+1);
				}
				else
				{
					json +=","+ crlf +iter->ptr->Print(f, w+1);
				}
				iter = childs_.Next(iter);
			}
			if (type_ == JSON_ARRAY)
				json += crlf+start+"]";
			else
				json += crlf + start +"}";
			break;
		}
		default:
			break;
		}
		return json;
	}

	inline JsonObjectPtr JsonObject::ReadFile(const JsonString & filename)
	{
		FILE *file = fopen(filename.c_str(), "r");
		if (NULL == file)
			return NULL;

		JsonString  json;
		const size_t buff_size = 1024;
		char buff[buff_size] = { 0 };

		while (true)
		{
			size_t readed = fread(buff, sizeof(char), buff_size, file);
			if (readed == 0)
				break;
			json += JsonString(buff, readed);
		}
		fclose(file);
		return Parser(json);
	}

	inline bool JsonObject::SaveFile(const JsonString & filename,bool f)
	{
		FILE *file = fopen(filename.c_str(), "w+");
		if (NULL == file)
			return false;

		JsonString  json=Print(f);
		if (json.empty())
		{
			fclose(file);
			return false;
		}
		fwrite(json.c_str(), sizeof(char), json.size(), file);
		fclose(file);
		return true;
	}

	inline bool JsonObject::Reset()
	{
		name_ = "";
		return ResetValue();
	}

	inline bool JsonObject::ResetValue()
	{
		//类型
		type_ = JSON_NULL;
		//JSON_BOOL,JSON_NUMBER,
		number_ = 0.0;
		string_ = "";
		return ClearChilds();
	}

	inline JsonString JsonObject::GetName()
	{
		return name_;
	}

	inline void JsonObject::SetName(const JsonString& str)
	{
		name_ = str;
	}

	inline JsonObjectType JsonObject::GetType()
	{
		return type_;
	}

	inline bool JsonObject::SetType(JsonObjectType t, bool reset)
	{
		if (t == type_)
			return true;
		if (!IsNull())
		{
			if (reset)
			{
				ResetValue();
			}
			return false;
		}
		else
		{
			type_ = t;
			return true;
		}
		
	}

	inline JsonString JsonObject::GetString()
	{
		if(type_ !=JSON_STRING)
			return JsonString();
		return string_;
	}

	inline bool JsonObject::SetString(const JsonString& str, bool reset)
	{
		if (type_ == JSON_NULL)
			type_ = JSON_STRING;
		if (type_ != JSON_STRING)
		{
			if (reset)
			{
				ResetValue();
				return SetString(str, false);
			}
			return false;
		}
		string_ = str;
		return true;
	}

	inline JsonNumber JsonObject::GetNumber()
	{
		if (type_ != JSON_NUMBER)
			return 0.0;
		return number_;
	}

	inline bool JsonObject::SetNumber(const JsonNumber& num, bool reset)
	{
		if (type_ == JSON_NULL)
			type_ = JSON_NUMBER;
		if (type_ != JSON_NUMBER)
		{
			if (reset)
			{
				ResetValue();
				return SetNumber(num, false);
			}
			return false;
		}
		number_ = num;
		return true;
	}

	inline bool JsonObject::GetBoolen()
	{
		if (type_ != JSON_BOOL)
			return false;
		return number_!=0.0;//非0为true
	}

	inline bool JsonObject::SetBoolen(const bool& b, bool reset)
	{
		if (type_ == JSON_NULL)
			type_ = JSON_BOOL;
		if (type_ != JSON_BOOL)
		{
			if (reset)
			{
				ResetValue();
				return SetBoolen(b, false);
			}
			return false;
		}
		if (b)
			number_ = 1;
		else
			number_ = 0.0;
		return true;
	}

	inline bool JsonObject::IsNull()
	{
		return type_ == JSON_NULL;
	}

	inline bool JsonObject::ArrayAddItem(JsonObjectPtr ptr)
	{
		return ObjectAddObject("",ptr);
	}

	inline bool JsonObject::ArrayAddString(const JsonString & str)
	{
		return ObjectAddString("",str);
	}

	inline bool JsonObject::ArrayAddNumber(const JsonNumber & number)
	{
		return ObjectAddNumber("",number);
	}

	inline bool JsonObject::ArrayAddBoolen(bool b)
	{
		return ObjectAddBoolen("",b);
	}

	inline bool JsonObject::ObjectAddObject(const JsonString&name, JsonObjectPtr ptr)
	{
		if (type_ == JSON_ARRAY)
		{
			ptr->name_ = "";
			return childs_.Append(ptr);
		}
		else if (type_ == JSON_OBJECT)
		{
			if (name.empty())
				return false;
			ptr->name_ = name;
			return childs_.Append(ptr);
		}
		else
		{
			return false;
		}
	}

	inline bool JsonObject::ObjectAddString(const JsonString & name, const JsonString & str)
	{
		JsonObjectPtr ptr = NewString(str);
		return ObjectAddObject(name,ptr);
	}

	inline bool JsonObject::ObjectAddNumber(const JsonString & name, const JsonNumber & number)
	{
		JsonObjectPtr ptr = NewNumber(number);
		ptr->name_ = name;
		return ObjectAddObject(name, ptr);
	}

	inline bool JsonObject::ObjectAddBoolen(const JsonString & name, bool b)
	{
		JsonObjectPtr ptr = NewBoolen(b);
		ptr->name_ = name;
		return ObjectAddObject(name, ptr);
	}

	inline bool JsonObject::AddHead(JsonObjectPtr ptr)
	{
		return childs_.AddHead(ptr);
	}

	inline bool JsonObject::Append(JsonObjectPtr ptr)
	{
		return childs_.Append(ptr);
	}

	inline bool JsonObject::Replace(JsonObjectPtr ptr, const int index)
	{
		return childs_.Replace(ptr, index);
	}

	inline bool JsonObject::Replace(JsonObjectPtr ptr, const JsonString & name)
	{
		return childs_.Replace(ptr, name);
	}

	inline bool JsonObject::Del(const int index)
	{
		return childs_.Del(index);
	}

	inline bool JsonObject::Del(const JsonString & name)
	{
		return childs_.Del(name);
	}

	inline bool JsonObject::ClearChilds()
	{
		return childs_.Clear();
	}

	inline unsigned int JsonObject::Size()
	{
		return childs_.Size();
	}

	inline JsonObjectPtr JsonObject::operator[](const int index)
	{
		return childs_[index];
	}

	inline JsonObjectPtr JsonObject::operator[](const JsonString & name)
	{
		return childs_[name];
	}

	inline bool JsonObject::Parser(const char * pdata, unsigned int len, unsigned int & offset)
	{
		SkipSpace(pdata, len, offset);
		if (offset >= len)
			return false;
		//{,[."
		if (pdata[offset] == '"')
		{
			++offset;
			if (false == ParserName(pdata, len, offset))
				return false;
			SkipSpace(pdata, len, offset);
			if (offset >= len)
				return false;
			if (pdata[offset] != ':')
				return false;
			++offset;
			//结束了，是空的
			if (pdata[offset] == ',' || pdata[offset] == '}' || pdata[offset] == ']')
			{
				type_ = JSON_NULL;
				return true;
			}
			return ParserValue(pdata, len, offset);
		}
		else
		{
			return ParserValue(pdata, len, offset);
		}
	}

	inline bool JsonObject::ParserName(const char * pdata, unsigned int len, unsigned int & offset)
	{
		return FindStringEnd(pdata, len, offset, '"', name_);
	}

	inline bool JsonObject::ParserValue(const char * pdata, unsigned int len, unsigned int & offset)
	{
		SkipSpace(pdata, len, offset);
		if (offset >= len)
			return false;
		//{,[,",number,T,F,Nn
		char start = pdata[offset];
		if (start == '{')
		{
			offset++;
			type_ = JSON_OBJECT;
			return ParserChilds(pdata, len, offset, false);
		}
		else if (start == '[')
		{
			offset++;
			type_ = JSON_ARRAY;
			return  ParserChilds(pdata, len, offset, true);
		}
		else if (start == '"')
		{
			offset++;
			type_ = JSON_STRING;
			return  FindStringEnd(pdata, len, offset, '"', string_);
		}
		else if ((start >= '0'&&start <= '9') || start == '-' || start == '+')//数值
		{
			type_ = JSON_NUMBER;
			return  ParserNumber(pdata, len, offset, number_);
		}
		else if (start == 'T' || start == 't' || start == 'F' || start == 'f')//数值
		{
			type_ = JSON_BOOL;
			return  ParserBoolen(pdata, len, offset, number_);
		}
		else if (start == 'N' || start == 'n')//数值
		{
			type_ = JSON_NULL;
			return  ParserNull(pdata, len, offset);
		}
		return false;
	}

	inline bool JsonObject::ParserChilds(const char * pdata, unsigned int len, unsigned int & offset, bool isArrays)
	{
		for (; offset < len;)
		{
			SkipSpace(pdata, len, offset);
			if (offset >= len)
				return false;

			if (pdata[offset] == '}')
			{
				return true;
			}
			else if (pdata[offset] == ',')
			{
				++offset;
				JsonObjectPtr child = NewObject();
				childs_.Append(child);
				if (false == child->Parser(pdata, len, offset))
					return false;
			}
			else if (pdata[offset] == '\"')
			{
				JsonObjectPtr child = NewObject();
				childs_.Append(child);
				if (false == child->Parser(pdata, len, offset))
					return false;
			}
			else
			{
				return false;
			}
		}
		return false;
	}

	inline bool JsonObject::ParserNumber(const char * pdata, unsigned int len, unsigned int & offset, JsonNumber & num)
	{
		JsonString temp;
		for (; offset < len; ++offset)
		{
			if (IsSpace(pdata[offset])
				|| ',' == pdata[offset]
				|| '}' == pdata[offset]
				|| ']' == pdata[offset])
				break;
			temp += pdata[offset];

		}
		if(temp.empty())
			return false;
		num = atof(temp.c_str());
		return true;
	}

	inline bool JsonObject::ParserBoolen(const char * pdata, unsigned int len, unsigned int & offset, JsonNumber & num)
	{
		JsonString temp;
		for (; offset < len; ++offset)
		{
			if (IsSpace(pdata[offset])
				|| ',' == pdata[offset]
				|| '}' == pdata[offset]
				|| ']' == pdata[offset])
				break;
			temp += pdata[offset];

		}

		if (ToLower(temp) == "true")
			num = 1;
		else if (ToLower(temp) == "false")
			num = 0;
		else
			return false;
		return true;
	}

	inline bool JsonObject::ParserNull(const char * pdata, unsigned int len, unsigned int & offset)
	{
		JsonString temp;
		for (; offset < len; ++offset)
		{
			if (IsSpace(pdata[offset])
				|| ',' == pdata[offset]
				|| '}' == pdata[offset]
				|| ']' == pdata[offset])
				break;
			temp += pdata[offset];

		}

		if (ToLower(temp) == "null")
			return true;
		return false;
	}

	inline bool JsonObject::IsSpace(char end)
	{
		if (end == ' '
			|| end == '\t'
			|| end == '\n'
			|| end == '\r')
			return true;
		return false;
	}

	inline void JsonObject::SkipSpace(const char * pdata, unsigned int len, unsigned int & offset)
	{
		for (; offset < len; ++offset)
			if (!IsSpace(pdata[offset]))
				break;
	}

	inline bool JsonObject::FindStringEnd(const char * pdata, unsigned int len, unsigned int & offset, char end, JsonString & str)
	{
		for (; offset < len; ++offset)
		{
			if (pdata[offset] == end)
			{
				//不要结束符
				++offset;
				return true;
			}
			else
			{
				str += pdata[offset];
			}
		}
		return false;
	}
	
	//序列化，结构体到JSON
	template<typename T>
	inline bool JsonObject::Serialize(T & t)
	{
		Reset();
		SetType(JSON_OBJECT, true);
		serialize::JsonSerialize ar(this);
		return ar.Serialize(t, true);
	}

	//反序列化，JSON到结构体
	template<typename T>
	inline bool JsonObject::DeSerialize(T & t)
	{
		//OBJECT才可以反序列化其他不可以。
		if (type_ != JSON_OBJECT)
			return false;

		serialize::JsonSerialize ar(this);
		return ar.Serialize(t, false);
	}

	inline JsonObjectPtr JsonObject::Copy()
	{
		JsonObjectPtr ptr = NewNull();
		if (NULL == ptr)
			return NULL;
		ptr->SetType(type_, true);
		ptr->SetName(name_);

		switch (type_)
		{
		case sim::JSON_NULL:
			break;
		case sim::JSON_BOOL:
		{
			ptr->SetBoolen(GetBoolen());
			break;
		}
		case sim::JSON_NUMBER:
		{
			ptr->SetNumber(GetNumber());
			break;
		}
		case sim::JSON_STRING:
		{
			ptr->SetString(GetString());
			break;
		}
		case sim::JSON_ARRAY:
		case sim::JSON_OBJECT:
		{
			if (false == childs_.CopyTo(ptr->childs_))
			{
				Free(ptr);
				return NULL;
			}
			break;
		}
		default:
		{
			Free(ptr);
			return NULL;
		}
		}
		return ptr;
	}
}


#endif