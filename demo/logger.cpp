/*
* ÈÕÖ¾Ä£¿édemo
*/
#include "Logger.hpp"
#include "TaskPool.hpp"
std::string msg = std::string(10000, 'i');
void* taskFunc(void*)
{
	SIM_FUNC(sim::LDebug);
	SIM_LDEBUG("taskFunc");
	SIM_LINFO("taskFunc");
	SIM_LWARN("taskFunc");
	SIM_LERROR("taskFunc");
	SIM_LERROR("msg="<<msg);
	ThreadSleep(100);
	return NULL;
}
int main(int argc, char* argv[])
{
	sim::TaskPool pool(0);
	SIM_LOG_CONSOLE(sim::LDebug);
	SIM_LOG_ADD(sim::LogFileStream, sim::LDebug, ".", "test","txt",32);
	SIM_FUNC(sim::LDebug);
	for (int i = 0; i < 10000000; i++)
	{
		pool.Post(taskFunc, NULL, NULL);
	}
	getchar();
	return 0;
}