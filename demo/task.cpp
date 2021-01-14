#include "TaskPool.hpp"
#include "stdio.h"
void* TaskFunc1(void*)
{
	ThreadSleep(1000);
	printf("task 1\n");
	return NULL;
}
void TaskComplete1(void* pUserData, void* ret)
{
	printf("task 1 ret=%p\n",ret);
}
void* TaskFunc2(void*)
{
	ThreadSleep(1000);
	printf("task 2\n");
	return NULL;
}
void TaskComplete2(void* pUserData, void* ret)
{
	printf("task 2 ret=%p\n", ret);
}
int main(int argc, char* argv[])
{
	sim::TaskWorker worker;
	sim::Task t1,t2;
	t1.pFunc = TaskFunc1;
	t2.pFunc = TaskFunc2;
	t1.pComplete = TaskComplete1;
	t2.pComplete = TaskComplete2;
	t1.pUserData = t2.pUserData=NULL;
	worker.Post(t1);
	worker.Post(t2);
	worker.ReSume();
	sim::TaskWorker &worker1=worker;
	worker1.Post(t2);
	worker1.Post(t1);
	getchar();
	return 0;
}