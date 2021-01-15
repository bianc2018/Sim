#include "TaskPool.hpp"
#include "stdio.h"
sim::Task t1, t2;
void* TaskFunc1(sim::TaskWorker *self,void*)
{
	//ThreadSleep(1000);
	printf("task 1 %ld\n",self->GetTaskSize());
	//self->Post(t2);
	return NULL;
}
void TaskComplete1(sim::TaskWorker* self, void* pUserData, void* ret)
{
	printf("task 1 ret=%p %ld\n",ret, self->GetTaskSize());
	//self->Post(t1);
}
void* TaskFunc2(sim::TaskWorker* self,void*)
{
	//ThreadSleep(1000);
	printf("task 2 %ld\n", self->GetTaskSize());
	//self->Post(t1);
	return NULL;
}
void TaskComplete2(sim::TaskWorker* self,void* pUserData, void* ret)
{
	printf("task 2 ret=%p %ld\n",ret, self->GetTaskSize());
	//self->Post(t2);
}
int main(int argc, char* argv[])
{
	sim::TaskWorker worker;
	
	t1.pFunc = TaskFunc1;
	t2.pFunc = TaskFunc2;
	t1.pComplete = TaskComplete1;
	t2.pComplete = TaskComplete2;
	t1.pUserData = t2.pUserData=NULL;

	for (int i = 0; i < 1000; ++i)
	{
		worker.Post(t1);
		worker.Post(t2);
		printf("status %d\n", worker.GetStatus());
	}
	worker.Pause();
	getchar();
	worker.ReSume();
	getchar();
	return 0;
}