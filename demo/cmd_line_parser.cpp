#include "CmdLineParser.hpp"
int main(int argc, char *argv[])
{
	
	for (int i = 0; i < argc; ++i)
	{
		printf("%d %s\n", i, argv[i]);
	}
	getchar();
	return 0;
}