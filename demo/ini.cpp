#include "Ini.hpp"

int main(int argc, char *argv[])
{
	sim::IniObject ini;
	if (false == ini.ReadFile("test.ini"))
		return 1;
	ini.DelNode("key", "name2");
	ini.DelSection("key1");
	ini.SetValue("≤‚ ‘", "kkkk", "");

	ini.SaveFile("test.ini");
	return 0;
}