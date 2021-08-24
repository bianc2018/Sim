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
	typedef IniSectionNode* IniSectionNodePtr;
	typedef IniObject* IniObjectPtr;

	struct IniSectionNode
	{
		//头顶的注释
		IniString comment;
		//右侧的注释
		IniString rcomment;

		IniString name;
		IniString value;
		IniSectionNodePtr next;

		IniSectionNode():next(NULL)
		{

		}
	};


	struct IniSection
	{
		//头顶的注释
		IniString comment;
		//右侧的注释
		IniString rcomment;

		//comment
		IniString name;
		//节点起头
		IniSectionNodePtr node_head;

		IniSection() :node_head(NULL)
		{

		}
	};

	class IniObject
	{
		IniObject();
	public:
		~IniObject();
	public:
		//API
		static void Free(IniObject ptr);
		
		//解析Ini数据字符串，失败返回空
		static IniObject Parser(const IniString&json);

		IniString Print();

		//从文件中加载
		static IniObject ReadFile(const IniString&filename);
		bool SaveFile(const IniString & filename,bool f = true);

	public:
		IniString GetValue(const IniString&section, const IniString&name, const IniString&notfound="");
		bool SetValue(const IniString&section, const IniString&name, bool overwrite=true);
	};

	
}

#endif
