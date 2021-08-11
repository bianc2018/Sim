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
		JsonArrayNodePtr next;
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
		bool Replace(JsonObjectPtr ptr, const int index);
		bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//删除
		bool Del(const int index);
		bool Del(const JsonString& name);

		//清空
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
		//
		JsonObject(JsonObjectType t= JSON_NULL) :type(t), number(0.0) {}
	public:
		~JsonObject() {};
	public:
		//API
		//新建一个空的对象
		static JsonObjectPtr NewObject();
		static JsonObjectPtr NewArray();
		static JsonObjectPtr NewString(const JsonString&json);
		static JsonObjectPtr NewNumber(const JsonNumber&str);
		static JsonObjectPtr NewBoolen(bool b);
		static void Free(JsonObjectPtr ptr);
		
		//解析json数据字符串，失败返回空
		static JsonObjectPtr Parser(const JsonString&json);
		JsonString Print(bool f=true,unsigned w=0);

		//从文件中加载
		static JsonObjectPtr ReadFile(const JsonString&filename);
		bool SaveFile(const JsonString & filename,bool f = true);
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

		//清空
		bool Clear();

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
		p->str = str;
		return p;
	}
	inline JsonObjectPtr JsonObject::NewNumber(const JsonNumber & number)
	{
		JsonObjectPtr p = new JsonObject(JSON_NUMBER);
		p->number = number;
		return p;
	}
	inline JsonObjectPtr JsonObject::NewBoolen(bool b)
	{
		JsonObjectPtr p = new JsonObject(JSON_BOOL);
		if(b)
			p->number = 1;
		else
			p->number = 0;
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
		if (!name.empty())
			json += "\"" + name + "\""+ space +":"+ space;

		switch (type)
		{
		case sim::JSON_NULL:
			break;
		case sim::JSON_BOOL:
		{
			if (number == 0)
				json += "false";
			else
				json += "true";
			break;
		}
		case sim::JSON_NUMBER:
		{
			const int buff_size = 256;
			char buff[buff_size] = { 0 };
			if (floor(number + 0.5) == number)
			{
				//整数
				snprintf(buff, buff_size, "%lld", static_cast<long long>(number));

			}
			else
			{
				//浮点数
				snprintf(buff, buff_size, "%lf", number);
			}
			json += buff;
			break;
		}
		case sim::JSON_STRING:
		{
			json += "\""+str+"\"";
			break;
		}
		case sim::JSON_ARRAY:
		case sim::JSON_OBJECT:
		{
			if (!name.empty())
				json += crlf;

			bool isFirst = true;
			if (type == JSON_ARRAY)
				json += start+ "[" + crlf;
			else
				json += start+ "{" + crlf;

			JsonArrayNodePtr iter = childs.Next(NULL);
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
				iter = childs.Next(iter);
			}
			if (type == JSON_ARRAY)
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
		if (type == JSON_ARRAY)
		{
			ptr->name = "";
			return childs.Append(ptr);
		}
		else if (type == JSON_OBJECT)
		{
			if (name.empty())
				return false;
			ptr->name = name;
			return childs.Append(ptr);
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
		ptr->name = name;
		return ObjectAddObject(name, ptr);
	}

	inline bool JsonObject::ObjectAddBoolen(const JsonString & name, bool b)
	{
		JsonObjectPtr ptr = NewBoolen(b);
		ptr->name = name;
		return ObjectAddObject(name, ptr);
	}

	inline bool JsonObject::AddHead(JsonObjectPtr ptr)
	{
		return childs.AddHead(ptr);
	}

	inline bool JsonObject::Append(JsonObjectPtr ptr)
	{
		return childs.Append(ptr);
	}

	inline bool JsonObject::Replace(JsonObjectPtr ptr, const int index)
	{
		return childs.Replace(ptr, index);
	}

	inline bool JsonObject::Replace(JsonObjectPtr ptr, const JsonString & name)
	{
		return childs.Replace(ptr, name);
	}

	inline bool JsonObject::Del(const int index)
	{
		return childs.Del(index);
	}

	inline bool JsonObject::Del(const JsonString & name)
	{
		return childs.Del(name);
	}

	inline bool JsonObject::Clear()
	{
		return childs.Clear();
	}

	inline unsigned int JsonObject::Size()
	{
		return childs.Size();
	}

	inline JsonObjectPtr JsonObject::operator[](const int index)
	{
		return childs[index];
	}

	inline JsonObjectPtr JsonObject::operator[](const JsonString & name)
	{
		return childs[name];
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
				type = JSON_NULL;
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
		return FindStringEnd(pdata, len, offset, '"', name);
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
			type = JSON_OBJECT;
			return ParserChilds(pdata, len, offset, false);
		}
		else if (start == '[')
		{
			offset++;
			type = JSON_ARRAY;
			return  ParserChilds(pdata, len, offset, true);
		}
		else if (start == '"')
		{
			offset++;
			type = JSON_STRING;
			return  FindStringEnd(pdata, len, offset, '"', str);
		}
		else if ((start >= '0'&&start <= '9') || start == '-' || start == '+')//数值
		{
			type = JSON_NUMBER;
			return  ParserNumber(pdata, len, offset, number);
		}
		else if (start == 'T' || start == 't' || start == 'F' || start == 'f')//数值
		{
			type = JSON_BOOL;
			return  ParserBoolen(pdata, len, offset, number);
		}
		else if (start == 'N' || start == 'n')//数值
		{
			type = JSON_NULL;
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
				childs.Append(child);
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
	
}
#endif