/*
* 一个简单的Ini解析器
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
	//声明前置
	struct IniSectionNode;
	class IniSection;
	class IniObject;

	//重命名
	typedef double IniNumber;
	typedef std::string IniString;
	typedef std::vector<IniString> IniStringVec;
	typedef std::vector<IniSectionNode> IniSectionNodeVec;
	typedef std::vector<IniSection> IniSectionVec;

	struct IniSectionNode
	{
		//头顶的注释
		IniStringVec comments;
		//右侧的注释
		IniString rcomment;

		IniString name;
		IniString value;
	};
	
	struct IniSection
	{
		//头顶的注释
		IniStringVec comments;
		//右侧的注释
		IniString rcomment;

		//comment
		IniString name;
		//节点起头
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
		
		//解析Ini数据字符串，失败返回false,poffset返回解析失败的地方
		bool Parser(const IniString&json,unsigned int *poffset=NULL);

		IniString Print();

		//从文件中加载
		static IniObject ReadFile(const IniString&filename);
		bool SaveFile(const IniString & filename,bool f = true);

	public:
		IniString GetValue(const IniString&section, const IniString&name, const IniString&notfound="");
		bool SetValue(const IniString&section, const IniString&name, bool overwrite=true);
		
	private:
		//解析接口
		bool Parser(const char* pdata, unsigned int len, unsigned int& offset);

		//找到结尾
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
			//# ; 注释
			//[Section Name]
			//Name=Value
			
			if (!SkipStringEnd(pdata, len, offset, "\t\r\n "))//跳过空格
				return false;
			if (offset >= len)
				return true;

			//这个是注释
			if ('#' == pdata[offset] || ';' == pdata[offset])
			{
				++offset;
				IniString t_str;
				if (false == FindStringEnd(pdata, len, offset, "\n", t_str))//直到行结束
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
				if (false == FindStringEnd(pdata, len, offset, "]", temp.name))//直到行结束
					return false;
				
			}

		}

		return false;
	}
	
}

#endif
