/*
* һ���򵥵�Ini������
*/
#ifndef SIM_INI_HPP_
#define SIM_INI_HPP_
#include <math.h>
#include <stdio.h> 
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>

namespace sim
{
	//����ǰ��
	struct IniSectionNode;
	class IniSection;
	class IniObject;

	//������
	typedef double IniNumber;
	typedef std::string IniString;
	typedef std::vector<IniString> IniStringVec;
	typedef std::vector<IniSectionNode> IniSectionNodeVec;
	typedef std::vector<IniSection> IniSectionVec;

	struct IniSectionNode
	{
		//ͷ����ע��
		IniStringVec comments;
		//�Ҳ��ע��
		IniString rcomment;

		IniString name;
		IniString value;
	};
	
	struct IniSection
	{
		//ͷ����ע��
		IniStringVec comments;
		//�Ҳ��ע��
		IniString rcomment;

		//comment
		IniString name;
		//�ڵ���ͷ
		IniSectionNodeVec nodes;

		void Clear()
		{
			comment.clear();
			rcomment = "";
			name = "";
			nodes.clear();
		}
	};

	class IniObject
	{
	public:
		IniObject();
		~IniObject();
	public:
		
		//����Ini�����ַ�����ʧ�ܷ���false,poffset���ؽ���ʧ�ܵĵط�
		bool Parser(const IniString&json,unsigned int *poffset=NULL);

		IniString Print();

		//���ļ��м���
		static IniObject ReadFile(const IniString&filename);
		bool SaveFile(const IniString & filename,bool f = true);

	public:
		IniString GetValue(const IniString&section, const IniString&name, const IniString&notfound="");
		bool SetValue(const IniString&section, const IniString&name, bool overwrite=true);
		
	private:
		//�����ӿ�
		bool Parser(const char* pdata, unsigned int len, unsigned int& offset);

		//�ҵ���β
		bool SkipStringEnd(const char* pdata, unsigned int len,
			unsigned int& offset, const IniString& ends);
		bool FindStringEnd(const char* pdata, unsigned int len, 
			unsigned int& offset, const IniString &ends, IniString& str);
	private:
		IniSectionVec sections_;
	};

	bool Parser(const char* pdata, unsigned int len, unsigned int& offset)
	{
		IniSection temp;
		IniString t_str;
		while(offset<len)
		{
			//# ; ע��
			//[Section Name]
			//Name=Value
			
			if (!SkipStringEnd(pdata, len, offset, "\t\r\n "))//�����ո�
				return false;
			if (offset >= len)
				return true;

			//�����ע��
			if ('#' == pdata[offset] || ';' == pdata[offset])
			{
				++offset;
				IniString t_str;
				if (false == FindStringEnd(pdata, len, offset, "\n", t_str))//ֱ���н���
					return false;
				temp.comments.push_back(t_str);
				++offset;
			}
			else if ('[' == pdata[offset])
			{
				++offset;
				if (!temp.name.empty())
					sections_.push_back(temp);
				temp.Clear();
				if (false == FindStringEnd(pdata, len, offset, "]", temp.name))//ֱ���н���
					return false;
				
			}

		}

		return false;
	}
	
}

#endif
