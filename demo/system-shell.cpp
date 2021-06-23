#include <stdio.h>
#include "System.hpp"

int main(int argc, char *argv[])
{
	sim::SystemShell shell;
	shell.Start(NULL, NULL/*, NULL, "/c ipconfig"*/);
	const int buff_size = 4 * 1024;
	char buff[buff_size] = { 0 };
	while (true)
	{
		Sleep(1000);
		//shell.StdIn("\r\n", strlen("\r\n"));
		int read_len = shell.ReadStdOut(buff, buff_size);
		if (read_len > 0)
		{
			if(read_len< buff_size)
				buff[read_len] = '\0';
			printf("%s", buff);
		}
		else
		{
			memset(buff, 0, buff_size);
			scanf("%s", buff);
			if (0 == strcmp("stop", buff))
				break;
			shell.StdIn(buff, strlen(buff));
			shell.StdIn("\r\n", strlen("\r\n"));
		}
	}

	shell.Stop(true);
	return 0;
}