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
	typedef IniSectionNode* IniSectionNodePtr;
	typedef IniObject* IniObjectPtr;

	struct IniSectionNode
	{
		//ͷ����ע��
		IniString comment;
		//�Ҳ��ע��
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
		//ͷ����ע��
		IniString comment;
		//�Ҳ��ע��
		IniString rcomment;

		//comment
		IniString name;
		//�ڵ���ͷ
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
		
		//����Ini�����ַ�����ʧ�ܷ��ؿ�
		static IniObject Parser(const IniString&json);

		IniString Print();

		//���ļ��м���
		static IniObject ReadFile(const IniString&filename);
		bool SaveFile(const IniString & filename,bool f = true);

	public:
		IniString GetValue(const IniString&section, const IniString&name, const IniString&notfound="");
		bool SetValue(const IniString&section, const IniString&name, bool overwrite=true);
	};

	
}

#endif
