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

		void Clear()
		{
			comments.clear();
			rcomment = "";
			name = "";
			value="";
		}
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
			comments.clear();
			rcomment = "";
			name = "";
			nodes.clear();
		}
	};

	class IniObject
	{
	public:
		IniObject(const IniString &comments_char = "#") :comments_char_(comments_char)
		{
			if (comments_char_.empty())
				comments_char_ = "#";
		}
		~IniObject() {};
	public:
		
		//解析Ini数据字符串，失败返回false,poffset返回解析失败的地方
		bool Parser(const IniString&json,unsigned int *poffset=NULL);

		IniString Print();

		//从文件中加载
		bool ReadFile(const IniString&filename);
		bool SaveFile(const IniString & filename,bool f = true);

	public:
		IniString GetValue(const IniString&section, const IniString&name, const IniString&notfound="");
		bool SetValue(const IniString&section, const IniString&name, const IniString&value, bool overwrite=true);
		
		IniStringVec GetComments(const IniString&section, const IniString&name);
		bool SetComments(const IniString&section, const IniString&name, const IniStringVec &comments, bool overwrite = true);

		IniString GetRComment(const IniString&section, const IniString&name);
		bool SetRComment(const IniString&section, const IniString&name, const IniString &rcomment, bool overwrite = true);

		IniStringVec GetSecComments(const IniString&section);
		bool SetSecComments(const IniString&section, const IniStringVec &comments, bool overwrite = true);

		IniString GetSecRComment(const IniString&section);
		bool SetSecRComment(const IniString&section, const IniString &rcomment, bool overwrite = true);

		bool DelNode(const IniString&section, const IniString&name);
		bool DelSection(const IniString&section);
	private:
		IniSection*GetSection(const IniString&section);
		IniSectionNode*GetNode(IniSection* sec, const IniString&name);
		IniSectionNode*GetNode1(const IniString&section, const IniString&name);
	private:
		//解析接口
		bool Parser(const char* pdata, unsigned int len, unsigned int& offset);

		//# .......
		bool ParserComment(const char* pdata, unsigned int len, unsigned int& offset, IniString &comment_temp);
		//......]
		bool ParserSectionHead(const char* pdata, unsigned int len, unsigned int& offset, IniSection &sec_temp);
		//name=value  #
		bool ParserNode(const char* pdata, unsigned int len, unsigned int& offset, IniSectionNode &node_temp);

		//找到结尾
		bool SkipStringEnd(const char* pdata, unsigned int len,
			unsigned int& offset, const IniString& ends);
		bool FindStringEnd(const char* pdata, unsigned int len, 
			unsigned int& offset, const IniString &ends, IniString& str);

		//vec有等于 c的字符返回 true 否则返回 false
		bool CheckChar(const IniString &vec, char c);
	private:
		IniSectionVec sections_;

		//注释字符
		IniString comments_char_;
	};

	inline bool IniObject::Parser(const IniString & str, unsigned int * poffset)
	{
		sections_.clear();
		unsigned int offset = 0;
		if (false == Parser(str.c_str(), str.size(), offset))
		{
			if (poffset)
				*poffset = offset;
			return false;
		}
		return true;
	}

	inline IniString IniObject::Print()
	{
		IniString data;
		unsigned int size = sections_.size();
		for (int i = 0; i < size; ++i)
		{
			//打印注释
			for (int j = 0; j < sections_[i].comments.size(); ++j)
			{
				if (!sections_[i].comments[j].empty())
					data += comments_char_[0] + sections_[i].comments[j] + "\n";
			}
			data += "[" + sections_[i].name + "]";
			if (sections_[i].rcomment.empty())
				data += "\n";
			else
				data += comments_char_[0] + sections_[i].rcomment + "\n";

			//子节点
			for (int k = 0; k < sections_[i].nodes.size(); ++k)
			{
				for (int j = 0; j < sections_[i].nodes[k].comments.size(); ++j)
				{
					if (!sections_[i].nodes[k].comments[j].empty())
						data += comments_char_[0] + sections_[i].nodes[k].comments[j] + "\n";
				}
				data += sections_[i].nodes[k].name +"="+ sections_[i].nodes[k].value;
				if (sections_[i].nodes[k].rcomment.empty())
					data += "\n";
				else
					data += comments_char_[0] + sections_[i].nodes[k].rcomment + "\n";
			}
		}
		return data;
	}

	inline bool IniObject::ReadFile(const IniString & filename)
	{

		FILE *file = fopen(filename.c_str(), "r");
		if (NULL == file)
			return NULL;

		IniString  ini;
		const size_t buff_size = 1024;
		char buff[buff_size] = { 0 };

		while (true)
		{
			size_t readed = fread(buff, sizeof(char), buff_size, file);
			if (readed == 0)
				break;
			ini += IniString(buff, readed);
		}
		fclose(file);
		return Parser(ini);
	}

	inline bool IniObject::SaveFile(const IniString & filename, bool f)
	{
		FILE *file = fopen(filename.c_str(), "w+");
		if (NULL == file)
			return false;

		IniString  ini = Print();
		if (ini.empty())
		{
			fclose(file);
			return false;
		}
		fwrite(ini.c_str(), sizeof(char), ini.size(), file);
		fclose(file);
		return true;
	}

	inline IniString IniObject::GetValue(const IniString & section, const IniString & name, const IniString & notfound)
	{
		IniSectionNode * node = GetNode1(section, name);
		if (NULL == node)
			return notfound;
		return node->value;
	}

	inline bool IniObject::SetValue(const IniString & section, const IniString & name, const IniString & value, bool overwrite)
	{
		IniSection*sec = GetSection(section);
		if (NULL == sec)
		{
			IniSection new_sec;
			new_sec.name = section;
			sections_.push_back(new_sec);
			sec = GetSection(section);
		}

		IniSectionNode * node = GetNode(sec, name);
		if (NULL == node)
		{
			IniSectionNode new_node;
			new_node.name = name;
			new_node.value = value;
			sec->nodes.push_back(new_node);
			return true;
		}
		else
		{
			if (overwrite)
			{
				node->value = value;
			}
			else
			{
				return false;
			}
		}
	}

	inline IniStringVec IniObject::GetComments(const IniString & section, const IniString & name)
	{
		IniSectionNode * node = GetNode1(section, name);
		if (NULL == node)
			return IniStringVec();
		return node->comments;
	}

	inline bool IniObject::SetComments(const IniString & section, const IniString & name, const IniStringVec & comments, bool overwrite)
	{
		IniSectionNode * node = GetNode1(section, name);
		if (NULL == node)
			return false;
		if (node->comments.empty() || overwrite)
		{
			node->comments = comments;
			return true;
		}
		return false;

	}

	inline IniString IniObject::GetRComment(const IniString & section, const IniString & name)
	{
		IniSectionNode * node = GetNode1(section, name);
		if (NULL == node)
			return IniString();
		return node->rcomment;
	}

	inline bool IniObject::SetRComment(const IniString & section, const IniString & name, const IniString & rcomment, bool overwrite)
	{
		IniSectionNode * node = GetNode1(section, name);
		if (NULL == node)
			return false;
		if (node->rcomment.empty() || overwrite)
		{
			node->rcomment = rcomment;
			return true;
		}
		return false;
	}

	inline IniStringVec IniObject::GetSecComments(const IniString & section)
	{
		IniSection * sec = GetSection(section);
		if (NULL == sec)
			return IniStringVec();
		return sec->comments;
	}

	inline bool IniObject::SetSecComments(const IniString & section, const IniStringVec & comments, bool overwrite)
	{
		IniSection * sec = GetSection(section);
		if (NULL == sec)
			return false;
		if (sec->comments.empty() || overwrite)
		{
			sec->comments = comments;
			return true;
		}
		return false;
	}

	inline IniString IniObject::GetSecRComment(const IniString & section)
	{
		IniSection * sec = GetSection(section);
		if (NULL == sec)
			return IniString();
		return sec->rcomment;
	}

	inline bool IniObject::SetSecRComment(const IniString & section, const IniString & rcomment, bool overwrite)
	{
		IniSection * sec = GetSection(section);
		if (NULL == sec)
			return false;
		if (sec->rcomment.empty() || overwrite)
		{
			sec->rcomment = rcomment;
			return true;
		}
		return false;
	}

	inline bool IniObject::DelNode(const IniString & section, const IniString & name)
	{
		IniSection * sec = GetSection(section);
		if (NULL == sec)
			return false;
		for (IniSectionNodeVec::iterator iter = sec->nodes.begin(); iter != sec->nodes.end(); ++iter)
			if (iter->name == name)
			{
				sec->nodes.erase(iter);
				return true;
			}
		return false;
	}

	inline bool IniObject::DelSection(const IniString & section)
	{
		for(IniSectionVec::iterator iter = sections_.begin();iter!=sections_.end();++iter)
			if (iter->name == section)
			{
				sections_.erase(iter);
				return true;
			}
		return false;
	}

	inline IniSection * IniObject::GetSection(const IniString & section)
	{
		for (int i = 0; i < sections_.size(); ++i)
		{
			if (sections_[i].name == section)
				return &sections_[i];
		}
		return NULL;
	}

	inline IniSectionNode * IniObject::GetNode(IniSection * sec, const IniString & name)
	{
		if(NULL == sec)
			return NULL;

		for (int i = 0; i < sec->nodes.size(); ++i)
		{
			if (sec->nodes[i].name == name)
				return &sec->nodes[i];
		}
		return NULL;
	}

	inline IniSectionNode * IniObject::GetNode1(const IniString & section, const IniString & name)
	{
		return GetNode(GetSection(section),name);
	}


	inline bool IniObject::Parser(const char* pdata, unsigned int len, unsigned int& offset)
	{
		if (NULL == pdata || 0 == len || offset >= len)
			return false;

		IniStringVec comments_temp;
		IniSection sec_temp;

		while (offset < len)
		{
			//去除空格
			if (false == SkipStringEnd(pdata, len, offset, "\n\t\r "))
				return false;
			if (offset >= len)
				break;
			if (CheckChar(comments_char_, pdata[offset]))
			{
				++offset;
				IniString comment_temp;
				if (false == ParserComment(pdata, len, offset, comment_temp))
					return false;
				comments_temp.push_back(comment_temp);
			}
			else if ('[' == pdata[offset])
			{
				++offset;
				if (!sec_temp.name.empty())
					sections_.push_back(sec_temp);

				sec_temp.Clear();
				sec_temp.comments = comments_temp;
				comments_temp.clear();

				if (false == ParserSectionHead(pdata, len, offset, sec_temp))
					return false;

			}
			else
			{
				IniSectionNode node_temp;
				node_temp.comments = comments_temp;
				comments_temp.clear();

				if (false == ParserNode(pdata, len, offset, node_temp))
					return false;
				sec_temp.nodes.push_back(node_temp);
			}
		}

		if (!sec_temp.name.empty())
			sections_.push_back(sec_temp);
		return true;
	}

	inline bool IniObject::ParserComment(const char * pdata, unsigned int len, unsigned int & offset, IniString & comment_temp)
	{
		FindStringEnd(pdata,len,offset,"\n", comment_temp);
		return true;
	}

	inline bool IniObject::ParserSectionHead(const char * pdata, unsigned int len, unsigned int & offset, IniSection & sec_temp)
	{
		if(false == FindStringEnd(pdata, len, offset, "]", sec_temp.name))
			return false;
		if (sec_temp.name.empty())
			return false;
		++offset;//skip ]
		for (; offset < len; ++offset)
		{
			if (CheckChar(comments_char_, pdata[offset]))
			{
				++offset;
				return ParserComment(pdata, len, offset, sec_temp.rcomment);//右边注释
			}
			else if (pdata[offset] == '\n')
			{
				++offset;
				return true;
			}
			else
			{
				//空格跳过
				if (CheckChar("\t ", pdata[offset]))
					continue;
				return false;
			}
		}
		return true;
	}
	inline bool IniObject::ParserNode(const char * pdata, unsigned int len, unsigned int & offset, IniSectionNode & node_temp)
	{
		//name = value # \n
		if (false == FindStringEnd(pdata, len, offset, " \t=", node_temp.name))//找name 遇到空格或者=结束
			return false;
		if (node_temp.name.empty())
			return false;
		
		if (false == SkipStringEnd(pdata, len, offset, " \t"))//跳过 空格
			return false;
		if (offset >= len)
			return false;
		if (pdata[offset] != '=')//判断是否存在 =
			return false;
		++offset;
		if (false == SkipStringEnd(pdata, len, offset, " \t"))//跳过 空格
			return false;
		if (offset >= len)
			return false;

		if (false == FindStringEnd(pdata, len, offset, " \t\n\r"+comments_char_, node_temp.value))//找value 遇到空格或者注释符或者换行结束
			return false;

		if (false == SkipStringEnd(pdata, len, offset, " \t"))//跳过空格
			return false;
		if (offset >= len)
			return false;

		//看一下第一个字符是什么
		if (CheckChar(comments_char_, pdata[offset]))
		{
			++offset;
			return ParserComment(pdata, len, offset, node_temp.rcomment);//右边注释
		}
		else if (pdata[offset] == '\n')
		{
			++offset;
			return true;
		}
		else
		{
			return false;
		}
		return true;
	}

	inline bool IniObject::SkipStringEnd(const char * pdata, unsigned int len, unsigned int & offset, const IniString & ends)
	{
		for (; offset<len; ++offset)
		{
			if (!CheckChar(ends, pdata[offset]))
				break;
		}
		return true;
	}

	inline bool IniObject::FindStringEnd(const char * pdata, unsigned int len, unsigned int & offset, 
		const IniString & ends, IniString & str)
	{
		for (; offset < len; ++offset)
		{
			if (CheckChar(ends, pdata[offset]))
				return true;
			str += pdata[offset];
		}
		return false;
	}

	inline bool IniObject::CheckChar(const IniString & vec, char c)
	{
		for (int i = 0; i < vec.size(); ++i)
			if (vec[i] == c)
				return true;
		return false;
	}
	
}

#endif
