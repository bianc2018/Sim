#include "Mutex.hpp"
#include "Thread.hpp"
#include <ctime>
#include <cstdio>
#include <vector>
static unsigned int i = 0;
static sim::Mutex lock;

ThRet inc(LPVOID p)
{
	printf("p=%d\n", ((long)p));
	char buff[1024];
	for (int j = 0; j < 1000000; ++j)
	{
		sim::AutoMutex lk(lock);
		//auto p = lk;
		//i++;
		++i;
		
		//std::lock_guard<std::mutex> lk(lock1);
	}
	return (ThRet)i;
}

ThRet detch(LPVOID p)
{
	while (true)
	{
		printf("1\n");
	}
}
int main(int argc, char*argv[])
{
	const int th_size = 100;
	sim::Thread ths[th_size];
	clock_t s = ::clock();
	for (int t = 0; t < th_size; ++t)
	{
		clock_t s1 = ::clock();
		sim::Thread st((sim::ThreadProc)inc, (LPVOID)t);
		ths[t].Swap(st);
		/*if (ths[t].JoinAble())
			ths[t].Join();
		printf("t=%d usg: %ld ms\n", t, ::clock() - s1);*/
	}
	for (int t = 0; t < th_size; ++t)
	{
		if (ths[t].JoinAble())
			ths[t].Join();
	}
	printf("i=%d usg: %ld ms\n", i,::clock()-s);
	//getchar();
	sim::Thread st((sim::ThreadProc)detch, (LPVOID)NULL);
	st.Detach();
	getchar();
	return 0;
}