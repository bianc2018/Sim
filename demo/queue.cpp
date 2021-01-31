#include "Queue.hpp"
#include <stdio.h>
struct fakedata
{
	char d[1024 * 4];
};
static unsigned long long memory_size = 0;
void* queueMalloc(size_t size)
{
	++memory_size ;
	return ::malloc(size);
}
void queueFree(void*p)
{
	--memory_size;
	return free(p);
}
int main(int argc, char* argv[])
{
	sim::Queue<fakedata> q;
	q.SetAlloc(queueMalloc, queueFree);
	for (int j = 0; j < 10000; ++j)
	{
		printf("size=%ld\n", memory_size);
		for (unsigned long int i = 0; i < 100000; ++i)
		{
			if(j%2==0)
				q.PushBack(fakedata());
			else
				q.PopFront(NULL);
		}
	}
	printf("end size=%ld\n", memory_size);
	getchar();
	return 0;
}