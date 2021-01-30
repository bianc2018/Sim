/*
* ÈÕÖ¾Ä£¿édemo
*/
#include "Logger.hpp"
#include "TaskPool.hpp"
void* taskFunc(void*)
{
	SIM_FUNC(sim::LDebug);
	SIM_LDEBUG("taskFunc");
	SIM_LINFO("taskFunc");
	SIM_LWARN("taskFunc");
	SIM_LERROR("taskFunc");
	return NULL;
}
int main(int argc, char* argv[])
{
	sim::TaskPool pool(0);
	SIM_LOG_CONSOLE(sim::LDebug);
	SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, ".", "test");
	SIM_FUNC(sim::LDebug);
	for (int i = 0; i < 1000000; i++)
	{
		pool.Post(taskFunc, NULL, NULL);
	}
	getchar();
	return 0;
}