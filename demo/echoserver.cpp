/*
	echo server
*/

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include "Logger.hpp"
#include "Array.hpp"
#include "Socket.hpp"
#include "Thread.hpp"
#include "Mutex.hpp"

unsigned int port = 80;
unsigned int buffsize = 1024*4;
unsigned int accept_size = 8;
bool is_print = false;

sim::Mutex active_connect_lock;
unsigned int active_connect = 0;
unsigned int all_connect = 0;

ThRet DoConnect(void*content)
{
	{
		sim::AutoMutex lk(active_connect_lock);
		++active_connect;
		++all_connect;
		printf("do connect %u/%u\n", active_connect, all_connect);
	}


	sim::Socket *client = (sim::Socket *)content;
	char *buff = new char[buffsize];
	
	while (true)
	{
		sim::SockRet ret = client->Recv(buff, buffsize, 100);
		if (ret <=0)
		{
			if (ret == -2)
				continue;
			printf("client end ret=%d\n", ret);
			break;
		}
		else
		{
			buff[ret] = '\0';
			if (is_print)
			{
				printf("echo %d %s\n", ret, buff);
			}
			client->Send(buff, ret, 1000);
		}
	}

	delete client;
	delete[]buff;

	{
		sim::AutoMutex lk(active_connect_lock);
		--active_connect;
		printf("end do connect %u/%u\n", active_connect, all_connect);
	}
	return NULL;
}
ThRet AcceptConnect(void*content)
{
	sim::Socket *server = (sim::Socket *)content;
	while (true)
	{
		sim::Socket * client = new sim::Socket(sim::TCP);
		sim::SockRet ret = server->Accept(*client,1000);
		if (ret == SOCK_SUCCESS)
		{
			printf("accept one client %d on thread %d active %u\n",
				client->GetSocket(), sim::Thread::GetThisThreadId()
			, active_connect);

			sim::Thread on_connect((sim::ThreadProc)DoConnect, client);
			on_connect.Detach();
		}
		else if (ret == SOCK_TIMEOUT)
		{
			continue;
		}
		else
		{
			printf("Accept fail ret=%d\n", ret);
			break;
		}
	}
	return NULL;
}


template<typename T1, typename T2>
T2 SimTo(const T1 &t)
{
	std::stringstream oss;
	oss << t;
	T2 t2;
	oss >> t2;
	return t2;
}

void print_help()
{
	printf("use as:echoserver  port buff_size  accept_size +/-p\n");
}

int main(int argc, char* argv[])
{
	SIM_LOG_CONSOLE(sim::LDebug);
#if 1
	if (argc == 1)
	{
		port = 8080;
	}
	else
#endif
	if (argc <= 1)
	{
		print_help();
		return -1;
	}

	if (argc >= 2)
	{
		port = SimTo<std::string,unsigned int>(argv[1]);
	}
	if (argc >= 3)
	{
		buffsize = SimTo<std::string, unsigned int>(argv[2]);
	}
	if (argc >= 4)
	{
		accept_size = SimTo<std::string, unsigned int>(argv[3]);
	}
	if (argc >= 5)
	{
		if (argv[4] == "p")
		{
			is_print = true;
		}
	}

	sim::Socket server(sim::TCP);
	server.SetReusePort(true);

	int ret = server.Bind(NULL, port);
	if (ret != SOCK_SUCCESS)
	{
		printf("Bind %u fail ret=%d\n", port, ret);
		return ret;
	}
	ret = server.Listen(1024);
	if (ret != SOCK_SUCCESS)
	{
		printf("Listen 1024 ret=%d\n",  ret);
		return ret;
	}
	sim::Thread* th_array = new sim::Thread[accept_size];

	for (int i = 0; i < accept_size; ++i)
	{
		sim::Thread accept_th((sim::ThreadProc)AcceptConnect, &server);
		th_array[i].Swap(accept_th);
	}
	
	getchar();
	server.Close();
	for (int i = 0; i < accept_size; ++i)
	{
		if (th_array[i].JoinAble())
			th_array[i].Join();
	}
	delete[]th_array;
	return 0;
}