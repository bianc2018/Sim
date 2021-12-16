#include "MultiThread.hpp"
#include "Logger.hpp"
#include <ctime>
#include <cstdio>
#include <vector>
sim::AtomicNumber n(0);
sim::Cond cond;

void WaitThreadProc1(sim::Thread& self, void* lpParam)
{
	while (n == 0)
	{
		cond.Wait();
		SIM_LERROR("WaitThreadProc1!");
	}
	SIM_LERROR("WaitThreadProc1 END"<< n);
}
void WaitThreadProc2(sim::Thread& self, void* lpParam)
{
	while (n == 0)
	{
		cond.Wait();
		SIM_LERROR("WaitThreadProc2!");
	}
	SIM_LERROR("WaitThreadProc2 END" << n);
}
int main(int argc, char*argv[])
{
	SIM_LOG_CONSOLE(sim::LDebug);
	SIM_LERROR("Start:" << n);
	sim::Thread t1(WaitThreadProc1, NULL);
	sim::Thread t2(WaitThreadProc2, NULL);
	sim::Thread::Sleep(5000);
	n.Add(1);
	cond.Signal();
	SIM_LERROR("Signal:" << n);
	cond.Signal();
	SIM_LERROR("Signal:" << n);
	//getchar();
	sim::Thread::Sleep(1000);
	return 0;
}