#include <stdio.h>
#include "System.hpp"

int main(int argc, char *argv[])
{
	//signal(SIGPIPE, SIG_IGN);

	/*char* argvv[] = { "/usr/bin/bash", "-c","", NULL };
	execvp("/usr/bin/bash", argvv);
	getchar();*/
	sim::SystemShell shell;
	shell.Start(NULL, NULL,NULL,"ls -l");
	const int buff_size = 4 * 1024;
	char buff[buff_size] = { 0 };
	while (true)
	{
#ifdef OS_WINDOWS
		Sleep(1000);
#endif
		int read_len = shell.ReadStdOut(buff, buff_size);
		if(read_len<=0)
			read_len =shell.ReadStdError(buff, buff_size);
		if (read_len > 0)
		{
			if(read_len< buff_size)
				buff[read_len] = '\0';
			printf("%s", buff);
		}
		else
		{
			memset(buff, 0, buff_size);
			for(int i=0;i<buff_size;++i)
			{
				char c = getchar();
				//printf("%c\n", c);
				//channel->Send(input + "\n");
				if (c == '\n' || c == '\t')
				{
					
					buff[i] = '\0';
					break;
				}
				buff[i] = c;
			}
			if (0 == strcmp("stop", buff))
				break;
			shell.StdIn(buff, strlen(buff));
			shell.StdInEnd();
		}
	}

	shell.Stop(true);
	return 0;
}