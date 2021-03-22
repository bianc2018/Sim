#include <iostream>
#include "Timer.hpp"
#include "TaskPool.hpp"

sim::TimerMgr mgr;
sim::TaskPool pool(8);

void TIMER_OUT_HANDLER(sim::timer_id timer_id, void *userdata)
{
	printf("%u TIMER_OUT_HANDLER %llu time %d \n",sim::Thread::GetThisThreadId(), timer_id, time(NULL));
	mgr.RemoveTimer(timer_id);
	//sim::Time::Sleep(1000);
}
void done()
{
	while (true)
	{
		mgr.Poll();
		sim::Time::Sleep(100);
	}
}
void* TaskFunc(void*)
{
	done();
	return NULL;
}
int main(int argc, char*argv[])
{
	/*mgr.AddTimer(100, TIMER_OUT_HANDLER, NULL, false);
	mgr.AddTimer(1000, TIMER_OUT_HANDLER, NULL, false);
	mgr.AddTimer(10000, TIMER_OUT_HANDLER, NULL, false);*/
	const unsigned int size = 10000;
	for(int i=0;i<size;++i)
		mgr.AddTimer(1000/*+i*/, TIMER_OUT_HANDLER, NULL, false);
	//done();
	for(int i=0;i<1;++i)
		pool.Post(TaskFunc, NULL,NULL);
	getchar();
	return 0;
}