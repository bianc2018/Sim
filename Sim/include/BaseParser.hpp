/*
	����������
*/
#ifndef SIM_BASE_PARSER_HPP_
#define SIM_BASE_PARSER_HPP_
#include <stdio.h>
#include <string>
#ifndef SIM_PARSER_BASE_TYPE
#define SIM_PARSER_BASE_TYPE 0
#endif
namespace sim
{
	typedef std::string Str;

	//׷��ģʽ
	enum KvMapAppendMode
	{
		//����
		HM_COVER,
		//�����ڲ��������Ѵ��ں��Ե�
		HM_ADD_IF_NO_EXIST,
		//׷��һ����
		HM_APPEND,
	};
	typedef bool(*KV_MAP_TRA_FUNC)(const Str& key, const Str& val, void*pdata);
	struct KvMap
	{
		struct KvMapNode
		{
			Str Key, Value;
			KvMapNode*next;
			KvMapNode() :next(NULL) {}
		};
	public:
		KvMapNode*pHead;
	public:
		KvMap() :pHead(NULL)
		{}
		KvMap(const KvMap&other) :pHead(NULL)
		{
			operator=(other);
		}
		KvMap&operator=(const KvMap&other)
		{
			if (this != &other)
			{
				Release();
				KvMapNode*pn = other.pHead;
				while (pn)
				{
					Append(pn->Key, pn->Value);
					pn = pn->next;
				}
			}
			return (*this);
		}

		~KvMap()
		{
			Release();
		}

		Str Get(const Str& key, const Str &notfound)
		{
			if (pHead == NULL)
			{
				return notfound;
			}
			KvMapNode* pn = pHead;
			while (pn)
			{
				if (pn->Key == key)
					return pn->Value;
				pn = pn->next;
			}
			return notfound;
		}
		//������key��Сд
		Str GetCase(const Str& key, const Str &notfound);

		void Append(const Str& key, const Str& val, KvMapAppendMode mode = HM_ADD_IF_NO_EXIST)
		{
			if (pHead == NULL)
			{
				pHead = new KvMapNode;
				pHead->Key = key;
				pHead->Value = val;
				return;
			}
			KvMapNode* pn = pHead;
			while (pn != NULL)
			{
				//���������ͬ��ֵ����
				if (pn->Key == key)
				{
					if (HM_ADD_IF_NO_EXIST == mode)
					{
						return;//������
					}
					else if (HM_COVER == mode)
					{
						//����
						pn->Value = val;
						return;
					}
				}
				
				if (pn->next == NULL)
				{
					//׷��
					pn->next = new KvMapNode;;
					pn->next->Key = key;
					pn->next->Value = val;
					return;
				}
				pn = pn->next;
			}
			return;
		}

		void AppendMap(const KvMap&other, KvMapAppendMode mode = HM_ADD_IF_NO_EXIST)
		{
			KvMapNode*pn = other.pHead;
			while (pn)
			{
				Append(pn->Key, pn->Value, mode);
				pn = pn->next;
			}
		}

		int Count(const Str& key)
		{
			int count = 0;
			KvMapNode* pn = pHead;
			while (pn)
			{
				if (pn->Key == key)
					++count;
				pn = pn->next;
			}
			return count;
		}

		int Count()
		{
			int count = 0;
			KvMapNode* pn = pHead;
			while (pn)
			{
				++count;
				pn = pn->next;
			}
			return count;
		}

		void Release()
		{
			//�ͷ��ڴ�
			while (pHead)
			{
				KvMapNode* pn = pHead->next;
				delete pHead;
				pHead = pn;
			}
			pHead = NULL;
		}

		//����
		void Traverse(KV_MAP_TRA_FUNC func, void*pdata)const
		{
			if (NULL == func)
				return;

			KvMapNode* pn = pHead;
			while (pn)
			{
				if (false == func(pn->Key, pn->Value, pdata))
					break;
				pn = pn->next;
			}
			return;
		}
	};
	
	//������
	class BaseParser
	{
	public:
		BaseParser(int type) :type_(type){};
		~BaseParser() {};

		virtual bool Parser(const char*data, unsigned int len) = 0;

		virtual int Type() { return type_; };
	public:
		//ȥ��ǰ��Ŀո�
		static Str Trim(const Str&s)
		{
			int size = s.size();
			if (size <= 0)
				return "";
			int start = 0, end = size - 1;
			for (int i = 0; i < size; ++i)
			{
				if (s[i] != ' ')
				{
					start = i;
					break;
				}
			}
			for (int i = size - 1; i >= 0; --i)
			{
				if (s[i] != ' ')
				{
					end = i;
					break;
				}
			}
			Str res;
			for (int i = start; i <= end; ++i)
			{
				res += s[i];
			}
			return  res;
		}
		//ת��Ϊ��Сд
		static Str ToLower(const Str &s)
		{
			Str result = s;
			unsigned int size = s.size();
			for (int i = 0; i < size; ++i)
				if (s[i] >= 'A'&&s[i] <= 'Z')
					result[i] = 'a' + s[i] - 'A';
			return result;
		}
		static Str ToUpper(const Str &s)
		{
			Str result = s;
			unsigned int size = s.size();
			for (int i = 0; i < size; ++i)
				if (s[i] >= 'a'&&s[i] <= 'z')
					result[i] = 'A' + s[i] - 'a';
			return result;
		}
		static int StrToInt(const Str&s, int fail = -1)
		{
			return StrToNum<int>(s, fail);
		}
		template <typename T>
		static T StrToNum(const Str&s, T fail = -1)
		{
			int size = s.size();
			if (size == 0)
				return fail;

			int i = 0;
			T num = 0;
			bool is_neg = false;
			if (s[0] == '-')
			{
				is_neg = true;
				i = 1;
			}

			for (; i < size; ++i)
			{
				if (s[i] >= '0'&& s[i] <= '9')
					num = num * 10 + (s[i] - '0');
				else
					return fail;
			}
			return is_neg ? -num : num;
		}
		static Str IntToStr(const int&s, Str fail = "")
		{
			const int temp_buff_size = 256;
			char temp_buff[temp_buff_size] = { 0 };
			snprintf(temp_buff, temp_buff_size, "%d", s);
			return temp_buff;
		}
		template <typename T>
		static Str NumToStr(const T&s, const Str& d = "%d")
		{
			const int temp_buff_size = 256;
			char temp_buff[temp_buff_size] = { 0 };
			snprintf(temp_buff, temp_buff_size, d.c_str(), s);
			return temp_buff;
		}

	private:
		int type_;
	};

	Str KvMap::GetCase(const Str & key, const Str & notfound)
	{
		if (pHead == NULL)
		{
			return notfound;
		}
		KvMapNode* pn = pHead;
		while (pn)
		{
			if (BaseParser::ToLower(pn->Key) == BaseParser::ToLower(key))
				return pn->Value;
			pn = pn->next;
		}
		return notfound;
	}
}
#endif