/*
* һ���򵥵�JSON������
*/
#ifndef SIM_JSON_HPP_
#define SIM_JSON_HPP_
#include <math.h>
#include <stdio.h> 
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>

//#define JSON_NEW(T) (T*)JsonMemoryHook::GetMemoryHook().fmalloc(sizeof(T))
#ifndef JSON_NEW
#define JSON_NEW(T) new T()
#endif
//#define JSON_FREE(T) {if(T)JsonMemoryHook::GetMemoryHook().ffree((void*)(T));}
#ifndef JSON_FREE
#define JSON_FREE(T) {if(T)delete T;}
#endif
//�ⲿ���� _SIM_DISABLE_JSON_SERIALIZE �� ʹ�����л�����Ч
//_SIM_DISABLE_JSON_SERIALIZE

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
}
#ifndef _SIM_DISABLE_JSON_SERIALIZE
//���л���ֵ���� ���
template<typename T>
bool SerializeValueFormJson(sim::JsonObjectPtr pjson, T& t, bool isSerialize);

//���л�����
template<typename archive, typename T>
bool JsonSerializeFunc(archive& ar, T& t, bool isSerialize);
#endif
namespace sim
{
	//��������
	/*struct JsonMemoryHook
	{
		struct FuncHook
		{
			typedef void* (*FMalloc)(size_t size);
			typedef void (*FFree)(void*p);
			FMalloc fmalloc;
			FFree ffree;
			FuncHook(FMalloc _fmalloc, FFree _ffree)
				:fmalloc(_fmalloc), ffree(_ffree) {}
			FuncHook()
				:fmalloc(::malloc), ffree(::free) {}
		};

		static FuncHook& GetMemoryHook()
		{
			static FuncHook func;
			return func;
		}
	};*/

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
		//׷��
		bool AddHead(JsonObjectPtr ptr);
		bool Append(JsonObjectPtr ptr);

		////ָ��λ�ú�����룬�Ҳ������ش���
  //      bool Insert(JsonObjectPtr ptr, const int after_index);
  //      bool Insert(JsonObjectPtr ptr, const JsonString& after_name);

		//����
		bool Replace(JsonObjectPtr ptr, const int index);
		bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//ɾ��
		bool Del(const int index);
		bool Del(const JsonString& name);

		//����ӽڵ�
		bool Clear();

		//�����ӿ�preǰһ����NULL ����ͷ�ڵ�
		JsonArrayNodePtr Next(JsonArrayNodePtr pre = NULL);

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
		friend class JsonArray;
		//
		JsonObject(const JsonString& name="",JsonObjectType t= JSON_NULL) :name_(name),type_(t), number_(0.0) {}
	public:
		~JsonObject();
	public:
		//API
		//�½�һ���յĶ���
		static JsonObjectPtr NewNull(const JsonString& name = "");
		static JsonObjectPtr NewObject(const JsonString& name = "");
		static JsonObjectPtr NewArray(const JsonString& name = "");
		static JsonObjectPtr NewString(const JsonString&json, const JsonString& name = "");
		static JsonObjectPtr NewNumber(const JsonNumber&str, const JsonString& name = "");
		static JsonObjectPtr NewBoolen(bool b, const JsonString& name = "");
		static void Free(JsonObjectPtr ptr);
		
		//������
		static JsonObjectPtr Copy(JsonObjectPtr src);

		//����json�����ַ�����ʧ�ܷ��ؿ�
		static JsonObjectPtr Parser(const JsonString&json);

		JsonString Print(bool f=true);

		//���ļ��м���
		static JsonObjectPtr ReadFile(const JsonString&filename);
		bool SaveFile(const JsonString & filename,bool f = true);

		//�ṹ��JSON
		template<typename T>
		bool Serialize(T&t);

		//JSON���ṹ
		template<typename T>
		bool DeSerialize(T&t);
	public:
		JsonObjectPtr Copy();

		//���ö���
		bool Reset();
		//����ֵΪ��
		bool ResetValue();
		//���ݽӿ�
		JsonString GetName();
		void SetName(const JsonString&str);

		//��ȡ��ֵ�����Զ�ת������
		/*template<typename T>
		T GetValue();
		template<typename T>
		void SetValue(const T&t);*/

		//�������� 
		JsonObjectType GetType();
		//�������� if type!=JSON_NULL&&reset=false return false
		bool SetType(JsonObjectType t, bool reset = false);

		//��ȡ�ַ��� if type == JSON_STRING
		JsonString GetString();
		// if type!=JSON_STRING&&reset=false return false
		bool SetString(const JsonString& str, bool reset = false);

		JsonNumber GetNumber();
		bool SetNumber(const JsonNumber& num, bool reset = false);

		bool GetBoolen();
		bool SetBoolen(const bool& b, bool reset = false);

		//�ж��Ƿ�Ϊ NULL
		bool IsNull();
	public:
		bool ArrayAddItem(JsonObjectPtr ptr);
		bool ArrayAddString(const JsonString&str);
		bool ArrayAddNumber(const JsonNumber&number);
		bool ArrayAddBoolen(bool b);
		//����һ������
		bool ObjectAddObject(const JsonString&name,JsonObjectPtr ptr);
		bool ObjectAddString(const JsonString&name,const JsonString&str);
		bool ObjectAddNumber(const JsonString&name, const JsonNumber&number);
		bool ObjectAddBoolen(const JsonString&name, bool b);
	public:
		//׷��
		bool AddHead(JsonObjectPtr ptr);
		bool Append(JsonObjectPtr ptr);

		//����
		bool Replace(JsonObjectPtr ptr, const int index);
		bool Replace(JsonObjectPtr ptr, const JsonString& name);

		//ɾ��
		bool Del(const int index);
		bool Del(const JsonString& name);

		//����ӽڵ�
		bool ClearChilds();

		//���ش�С
		unsigned int Size();

		/*JsonObjectPtr operator[](const int index);
		JsonObjectPtr operator[](const JsonString& name);*/

		JsonObjectPtr FindByIndex(int index);
		JsonObjectPtr FindByName(const JsonString& name);
		//����·�������ӽڵ� ��.Ϊ�ֽڵ� �� child.child1.child2
		JsonObjectPtr FindByPath(const JsonString& path,char spliter='.');
	private:
		JsonString PrintJson(bool f,unsigned w);

		bool Parser(const char*pdata, unsigned int len, unsigned int &offset);
		bool ParserName(const char*pdata, unsigned int len, unsigned int &offset);
		//����ֵ
		bool ParserValue(const char*pdata, unsigned int len, unsigned int &offset);
		//�����ӽڵ�
		bool ParserChilds(const char*pdata, unsigned int len, unsigned int &offset, bool isArrays);
		bool ParserNumber(const char*pdata, unsigned int len, unsigned int &offset, JsonNumber &num);
		bool ParserBoolen(const char*pdata, unsigned int len, unsigned int &offset, JsonNumber &num);
		bool ParserNull(const char*pdata, unsigned int len, unsigned int &offset);
		//�����ո�
		bool IsSpace(char end);
		void SkipSpace(const char*pdata, unsigned int len, unsigned int &offset);
		bool FindStringEnd(const char*pdata, unsigned int len, unsigned int &offset, char end, JsonString& str);
		//��Сд
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
		//����
		JsonString name_;
		//����
		JsonArray childs_;
		//����
		JsonObjectType type_;
		//JSON_BOOL,JSON_NUMBER,
		JsonNumber number_;
		//JSON_STRING
		JsonString string_;
	};
#ifndef _SIM_DISABLE_JSON_SERIALIZE
	//���л�
	namespace serialize
	{
		//stringstream ��������ת��
		template<typename SrcType, typename DstType>
		bool TypeCast(const SrcType &t1, DstType &t2);

		//�Զ�ת��
		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, T& t, JsonObjectType jsontype, bool isSerialize);

		template<typename T>
		bool SerializeValueFormJsonNum(JsonObjectPtr pjson, T& t, bool isSerialize);

		bool SerializeValueFormJson(JsonObjectPtr pjson, bool& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, char& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned char& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, int& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, float& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, double& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, long long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned long long& t, bool isSerialize);
		bool SerializeValueFormJson(JsonObjectPtr pjson, std::string& t, bool isSerialize);
		template<typename T>
		bool SerializeValueFormJson(JsonObjectPtr pjson, std::vector<T>& arrs, bool isSerialize);
		

		//���л�
		class JsonSerialize
		{
		public:
			JsonSerialize(JsonObjectPtr ref) :ptr_(ref), json_object_is_release_(false) {};
			JsonSerialize() :ptr_(JsonObject::NewObject()), json_object_is_release_(true) {};
			~JsonSerialize() { if (ptr_&&json_object_is_release_)JsonObject::Free(ptr_); }
			//���л��ӿ�
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
			// TODO: �ڴ˴����� return ���
			if (NULL == ptr_)
				return false;
			JsonObjectPtr childs = NULL;
			if (isSerialize)
			{
				//ת��Ϊ�ַ���
				childs = JsonObject::NewNull();
				if (NULL == childs)
					return false;
				if (false == ptr_->ObjectAddObject(key, childs))
					return false;
				
			}
			else
			{
				childs = ptr_->FindByName(key);
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
			// TODO: �ڴ˴����� return ���
			return JsonSerializeFunc(*this, t, isSerialize);
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

		bool SerializeValueFormJson(JsonObjectPtr pjson, char & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned char & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
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

		bool SerializeValueFormJson(JsonObjectPtr pjson, long long & t, bool isSerialize)
		{
			return SerializeValueFormJsonNum(pjson, t, isSerialize);
		}

		bool SerializeValueFormJson(JsonObjectPtr pjson, unsigned long long & t, bool isSerialize)
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
				if (pjson->GetType() == JSON_STRING)
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
						if (false == SerializeValueFormJson(pjson->FindByIndex(i), temp, isSerialize))
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

		
		//����ͨ�ýӿ�
	}
#endif

	//�Զ��ͷŵ�json���� ��Ҫ�Ǹ���ʹ��
	class AutoJsonObject
	{
		//����������
		AutoJsonObject() {};
		AutoJsonObject(const AutoJsonObject&other) {};
	public:
		//����
		AutoJsonObject(JsonObjectPtr ptr);
		//����
		virtual ~AutoJsonObject();
		//�������
		virtual JsonObjectPtr operator->();
		virtual JsonObject& operator*();
		//�ͷ�
		virtual void Reset(JsonObjectPtr ptr = NULL);
		//������ã������ͷŵ�
		virtual JsonObjectPtr Release();
		//�ƶ�
		virtual void Move(AutoJsonObject& other);
		//�ж��Ƿ�Ϊ��
		virtual bool IsNull();
	private:
		JsonObjectPtr ptr_;
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
			if (dst->ptr)
			{
				if (ptr->name_.empty())
					ptr->name_ = dst->ptr->name_;
				JsonObject::Free(dst->ptr);
			}
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
			if (dst->ptr)
			{
				if (ptr->name_.empty())
					ptr->name_ = dst->ptr->name_;
				JsonObject::Free(dst->ptr);
			}
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
		//JsonArrayNodePtr ptr = new JsonArrayNode;
		JsonArrayNodePtr ptr = JSON_NEW(JsonArrayNode);//(JsonArrayNodePtr)JsonMemoryHook::GetMemoryHook().fmalloc(sizeof(JsonArrayNode));
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
			//delete ptr;
		}
		JSON_FREE(ptr);
	}

	inline JsonObject::~JsonObject()
	{
		Reset();
	}

	inline JsonObjectPtr JsonObject::NewNull(const JsonString& name)
	{
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_NULL, true);
		}
		//return new JsonObject(name,JSON_NULL);
		return ptr;
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
	inline JsonObjectPtr JsonObject::NewObject(const JsonString& name)
	{
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_OBJECT, true);
		}
		return ptr;
		//return new JsonObject(name,JSON_OBJECT);
	}
	inline JsonObjectPtr JsonObject::NewArray(const JsonString& name)
	{
		//return new JsonObject(name,JSON_ARRAY);
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_ARRAY, true);
		}
		return ptr;
	}
	inline JsonObjectPtr JsonObject::NewString(const JsonString & str, const JsonString& name)
	{
		/*JsonObjectPtr p= new JsonObject(name,JSON_STRING);
		p->string_ = str;
		return p;*/
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_STRING, true);
			ptr->string_ = str;
		}
		return ptr;
	}
	inline JsonObjectPtr JsonObject::NewNumber(const JsonNumber & number, const JsonString& name)
	{
		/*JsonObjectPtr p = new JsonObject(name,JSON_NUMBER);
		p->number_ = number;
		return p;*/
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_NUMBER, true);
			ptr->number_ = number;
		}
		return ptr;
	}
	inline JsonObjectPtr JsonObject::NewBoolen(bool b, const JsonString& name)
	{
		/*JsonObjectPtr p = new JsonObject(name,JSON_BOOL);
		if(b)
			p->number_ = 1;
		else
			p->number_ = 0;
		return p;*/
		JsonObjectPtr ptr = JSON_NEW(JsonObject);
		if (ptr)
		{
			ptr->Reset();
			ptr->SetName(name);
			ptr->SetType(JSON_NUMBER, true);
			if (b)
				ptr->number_ = 1;
			else
				ptr->number_ = 0;
		}
		return ptr;
	}
	inline void JsonObject::Free(JsonObjectPtr ptr)
	{
		/*if (ptr)
			delete ptr;*/
		JSON_FREE(ptr);
	}

	inline JsonObjectPtr JsonObject::Copy(JsonObjectPtr src)
	{
		if (src)
			return src->Copy();
		return NULL;
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

	inline JsonString JsonObject::Print(bool f)
	{
		return PrintJson(f, 0);
	}

	inline JsonString JsonObject::PrintJson(bool f, unsigned w)
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
				//����
				snprintf(buff, buff_size, "%lld", static_cast<long long>(number_));

			}
			else
			{
				//������
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
					json += iter->ptr->PrintJson(f, w+1);
				}
				else
				{
					json +=","+ crlf +iter->ptr->PrintJson(f, w+1);
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
		//����
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
		return number_!=0.0;//��0Ϊtrue
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

	/*inline JsonObjectPtr JsonObject::operator[](const int index)
	{
		return childs_[index];
	}*/

	/*inline JsonObjectPtr JsonObject::operator[](const JsonString & name)
	{
		return childs_[name];
	}*/

	inline JsonObjectPtr JsonObject::FindByIndex(int index)
	{
		return childs_[index];
	}

	inline JsonObjectPtr JsonObject::FindByName(const JsonString & name)
	{
		return childs_[name];
	}

	inline JsonObjectPtr JsonObject::FindByPath(const JsonString & path, char spliter)
	{
		JsonString child_name = "";
		JsonObjectPtr now = this;
		size_t size = path.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (path[i] == spliter)
			{
				if (child_name.empty())
					return NULL;

				now = now->FindByName(child_name);
				if (NULL == now)
					return NULL;
				child_name = "";
			}
			else
			{
				child_name += path[i];
			}
		}
		if (!child_name.empty())
		{
			now = now->FindByName(child_name);
			if (NULL == now)
				return NULL;
		}
		return now == this?NULL:now;
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
			//�����ˣ��ǿյ�
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
		else if ((start >= '0'&&start <= '9') || start == '-' || start == '+')//��ֵ
		{
			type_ = JSON_NUMBER;
			return  ParserNumber(pdata, len, offset, number_);
		}
		else if (start == 'T' || start == 't' || start == 'F' || start == 'f')//��ֵ
		{
			type_ = JSON_BOOL;
			return  ParserBoolen(pdata, len, offset, number_);
		}
		else if (start == 'N' || start == 'n')//��ֵ
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
				++offset;
				if(false == isArrays)
					return true;
			}
			else if (pdata[offset] == ']')
			{
				++offset;
				if (true == isArrays)
					return true;
			}
			else
			{
				if (pdata[offset] == ',')
				{
					++offset;
				}
				JsonObjectPtr child = NewObject();
				childs_.Append(child);
				if (false == child->Parser(pdata, len, offset))
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
				//��Ҫ������
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
	
	//���л����ṹ�嵽JSON
	template<typename T>
	inline bool JsonObject::Serialize(T & t)
	{
#ifndef _SIM_DISABLE_JSON_SERIALIZE
		Reset();
		SetType(JSON_OBJECT, true);
		serialize::JsonSerialize ar(this);
		return ar.Serialize(t, true);
#else
		return false;
#endif
	}

	//�����л���JSON���ṹ��
	template<typename T>
	inline bool JsonObject::DeSerialize(T & t)
	{
#ifndef _SIM_DISABLE_JSON_SERIALIZE
		//OBJECT�ſ��Է����л����������ԡ�
		if (type_ != JSON_OBJECT)
			return false;

		serialize::JsonSerialize ar(this);
		return ar.Serialize(t, false);
#else
		return false;
#endif
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

	inline sim::AutoJsonObject::AutoJsonObject(JsonObjectPtr ptr)
		:ptr_(ptr)
	{
	}
	inline AutoJsonObject::~AutoJsonObject()
	{
		Reset(NULL);
	}
	inline JsonObjectPtr AutoJsonObject::operator->()
	{
		return ptr_;
	}
	inline JsonObject& AutoJsonObject::operator*()
	{
		return *ptr_;
	}
	inline void AutoJsonObject::Reset(JsonObjectPtr ptr)
	{
		//�ͷ�ԭ�е�
		if (ptr_)
			sim::JsonObject::Free(ptr_);
		ptr = ptr_;
	}
	inline JsonObjectPtr AutoJsonObject::Release()
	{
		JsonObjectPtr ptr = ptr_;
		ptr_ = NULL;
		return ptr;
	}
	inline void AutoJsonObject::Move(AutoJsonObject& other)
	{
		Reset(other.Release());
	}
	inline bool AutoJsonObject::IsNull()
	{
		return NULL == ptr_;
	}
}
#ifndef _SIM_DISABLE_JSON_SERIALIZE
template<typename T>
bool SerializeValueFormJson(sim::JsonObjectPtr pjson, T & t, bool isSerialize)
{
	if (isSerialize)
		pjson->SetType(sim::JSON_OBJECT, true);
	sim::serialize::JsonSerialize ar(pjson);
	return ar.Serialize(t, isSerialize);
}
template<typename archive, typename T>
bool JsonSerializeFunc(archive & ar, T & t, bool isSerialize)
{
	return t.JsonSerializeFunc(ar, isSerialize);
}

//ʵ�õ�JSON���л������
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
        bool ret=sim::serialize::SerializeValueFormJson(pjson, temp,sim::JSON_NUMBER, isSerialize);\
        if(!isSerialize) t=(type)(temp);\
        return ret;\
   SIM_DEF_JSON_SERIALIZE_TYPE_END(type)

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_NUM(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JSON_NUMBER)

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_BOOL(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JSON_BOOL)

#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_STR(type)\
   SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,sim::JSON_STRING)

//�������л����� �ڽṹ���ⲿ
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

//�������л����� �ڽṹ���ڲ�
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
#else
//�յĺ��ֹ ���������л����ܵ�ʱ������쳣
#define SIM_DEF_JSON_SERIALIZE_TYPE(type) 
#define SIM_DEF_JSON_SERIALIZE_TYPE_END(type) 
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS(type,jsontype)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_ENUM(type)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_NUM(type)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_BOOL(type)
#define SIM_DEF_JSON_SERIALIZE_TYPE_AS_STR(type)
#define SIM_DEF_JSON_SERIALIZE_STRUCT(type) 
#define SIM_JSON_SERIALIZE_VALUE(name,value,ismust)
#define SIM_JSON_SERIALIZE_VALUE_1(name,ismust)
#define SIM_JSON_SERIALIZE_VALUE_2(name)
#define SIM_DEF_JSON_SERIALIZE_STRUCT_END(type) 
#define SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT(name,value,ismust)
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_1(name,ismust) 
#define SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(name) 
#define SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END() 
#endif
#endif