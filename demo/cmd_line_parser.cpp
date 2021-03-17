#include "CmdLineParser.hpp"
int main(int argc, char *argv[])
{
	int fargc=5;
	char *fargv[] = { "./cmd","-ip","127.0.0.1","-port","8080" };
	sim::CmdLineParser cmd(fargc, fargv);
	printf("ip:%s\n", cmd.GetCmdLineParams("ip", "notfind").c_str());
	printf("port:%d\n", cmd.GetCmdLineParams("port", -1));
	getchar();
	return 0;
}