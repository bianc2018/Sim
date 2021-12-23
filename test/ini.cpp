#include "Test.hpp"
#include "File/Ini.hpp"

using namespace sim;
//解析
SIM_TEST(IniParser)
{
	IniString ini = "";
	sim::IniObject ini_obj;

	SIM_TEST_IS_FALSE(ini_obj.Parser(ini));

	ini =	"   #key comments 1 \n"
			"# key comments 2\n"
			"\t[key] #key rcomment\n"
			"#name comments 1\n"
			"#name comments 2\n"
			" name\t= value #name rcomment";
	SIM_ASSERT_IS_TRUE(ini_obj.Parser(ini));
	SIM_TEST_IS_EQUAL("value", ini_obj.GetValue("key", "name", ""));
	IniStringVec key_comments = ini_obj.GetSecComments("key");
	SIM_ASSERT_IS_EQUAL(2, key_comments.size());
	SIM_TEST_IS_EQUAL("key comments 1 ", key_comments[0]);
	SIM_TEST_IS_EQUAL(" key comments 2", key_comments[1]);
	SIM_TEST_IS_EQUAL("key rcomment", ini_obj.GetSecRComment("key"));

	IniStringVec name_comments = ini_obj.GetComments("key","name");
	SIM_ASSERT_IS_EQUAL(2, name_comments.size());
	SIM_TEST_IS_EQUAL("name comments 1", name_comments[0]);
	SIM_TEST_IS_EQUAL("name comments 2", name_comments[1]);
	SIM_TEST_IS_EQUAL("name rcomment", ini_obj.GetRComment("key", "name"));

	ini = "   #key comments 1 \n"
		"# key comments 2\n"
		"\t[key#] #key rcomment\n"
		"#name comments 1\n"
		"#name comments 2\n"
		" name\t#= value #name rcomment";
	SIM_ASSERT_IS_FALSE(ini_obj.Parser(ini));
}

SIM_TEST(IniOperator)
{
	sim::IniObject ini_obj;
	SIM_TEST_IS_TRUE(ini_obj.SetValue("key", "name1", "100"));
	SIM_TEST_IS_TRUE(ini_obj.SetValue("key", "name2", "101"));
	SIM_TEST_IS_TRUE(ini_obj.SetValue("ke2", "name0", "102"));

	SIM_TEST_IS_EQUAL("100", ini_obj.GetValue("key", "name1"));
	SIM_TEST_IS_EQUAL("101", ini_obj.GetValue("key", "name2"));
	SIM_TEST_IS_EQUAL("102", ini_obj.GetValue("ke2", "name0"));

	SIM_TEST_IS_FALSE(ini_obj.SetValue("ke2", "name0", "103",false));
	SIM_TEST_IS_EQUAL("102", ini_obj.GetValue("ke2", "name0"));

	IniStringVec key_comments,result;
	key_comments.push_back("aaa aaa");
	key_comments.push_back("aaa点对点");
	SIM_TEST_IS_TRUE(ini_obj.SetSecComments("key", key_comments));
	SIM_TEST_IS_TRUE(ini_obj.SetSecRComment("key", "rcc "));
	result = ini_obj.GetSecComments("key");
	SIM_ASSERT_IS_EQUAL(2, result.size());
	SIM_TEST_IS_EQUAL(key_comments[0], result[0]);
	SIM_TEST_IS_EQUAL(key_comments[1], result[1]);
	SIM_TEST_IS_EQUAL("rcc ", ini_obj.GetSecRComment("key"));

	result.clear();

	IniStringVec name1_comments;
	name1_comments.push_back("adadsadx aaa");
	name1_comments.push_back("aaazczdasdqwwc");
	SIM_TEST_IS_TRUE(ini_obj.SetComments("key","name1", name1_comments));
	SIM_TEST_IS_TRUE(ini_obj.SetRComment("key", "name1", "adasdd "));
	result = ini_obj.GetComments("key", "name1");
	SIM_ASSERT_IS_EQUAL(2, result.size());
	SIM_TEST_IS_EQUAL(name1_comments[0], result[0]);
	SIM_TEST_IS_EQUAL(name1_comments[1], result[1]);
	SIM_TEST_IS_EQUAL("adasdd ", ini_obj.GetRComment("key", "name1"));

}
SIM_TEST(IniFile)
{
	//初始化
	sim::IniObject ini_obj;
	SIM_TEST_IS_TRUE(ini_obj.SetValue("key", "name1", "100"));
	SIM_TEST_IS_TRUE(ini_obj.SetValue("key", "name2", "101"));
	SIM_TEST_IS_TRUE(ini_obj.SetValue("ke2", "name0", "102"));
	IniStringVec key_comments;
	key_comments.push_back("adsad");
	key_comments.push_back("adasdas");
	SIM_TEST_IS_TRUE(ini_obj.SetSecComments("key", key_comments));
	SIM_TEST_IS_TRUE(ini_obj.SetSecRComment("key", "adasda"));
	IniStringVec name1_comments;
	name1_comments.push_back("adadsadx aaa");
	name1_comments.push_back("aaazczdasdqwwc");
	SIM_TEST_IS_TRUE(ini_obj.SetComments("key", "name1", name1_comments));
	SIM_TEST_IS_TRUE(ini_obj.SetRComment("key", "name1", "adasdd "));

	//保存文件
	SIM_ASSERT_IS_TRUE(ini_obj.SaveFile("test_ini.ini"));
	ini_obj.Clear();
	
	//读取
	sim::IniObject ini_obj2;
	SIM_ASSERT_IS_TRUE(ini_obj2.ReadFile("test_ini.ini"));

	SIM_TEST_IS_EQUAL("100", ini_obj2.GetValue("key", "name1"));
	SIM_TEST_IS_EQUAL("101", ini_obj2.GetValue("key", "name2"));
	SIM_TEST_IS_EQUAL("102", ini_obj2.GetValue("ke2", "name0"));

	IniStringVec result;
	result = ini_obj2.GetSecComments("key");
	SIM_ASSERT_IS_EQUAL(2, result.size());
	SIM_TEST_IS_EQUAL(key_comments[0], result[0]);
	SIM_TEST_IS_EQUAL(key_comments[1], result[1]);
	SIM_TEST_IS_EQUAL("adasda", ini_obj2.GetSecRComment("key"));

	result.clear();

	result = ini_obj2.GetComments("key", "name1");
	SIM_ASSERT_IS_EQUAL(2, result.size());
	SIM_TEST_IS_EQUAL(name1_comments[0], result[0]);
	SIM_TEST_IS_EQUAL(name1_comments[1], result[1]);
	SIM_TEST_IS_EQUAL("adasdd ", ini_obj2.GetRComment("key", "name1"));

}
SIM_TEST_MAIN(sim::noisy)