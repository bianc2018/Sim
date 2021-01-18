#include "TaskPool.hpp"
#include "stdio.h"
static unsigned int i = 0;
void* TaskFunc1(void*)
{
	//ThreadSleep(1000);
	printf("task 1 %ld\n",sim::Thread::GetThisThreadId());
	++i;
	//self->Post(t2);
	return NULL;
}
void TaskComplete1(void* pUserData, void* ret)
{
	printf("TaskComplete1 1 %ld\n", sim::Thread::GetThisThreadId());
	//self->Post(t1);
}
void* TaskFunc2(void*)
{
	//ThreadSleep(1000);
	printf("task 2 %ld\n", sim::Thread::GetThisThreadId());
	//self->Post(t1);
	return NULL;
}
void TaskComplete2(void* pUserData, void* ret)
{
	printf("TaskComplete2 2 %ld\n", sim::Thread::GetThisThreadId());
	//self->Post(t2);
}
int main(int argc, char* argv[])
{
	sim::TaskPool worker(0);
	
	for (int i = 0; i < 10000; ++i)
	{
		worker.Post(TaskFunc1,NULL, NULL);
	}
	
	getchar();
	printf("i=%u\n", i);
	return 0;
}