#include "Queue.hpp"
#include <stdio.h>
int main(int argc, char* argv[])
{
	sim::Queue<int> intq;
	for(unsigned long int i=0;i<1000000;++i)
		intq.PushBack(i);
	intq.PushBack(10);
	intq.PushBack(11);
	int i=0;
	intq.PopFront(&i);
	printf("i=%d\n", i);
	intq.clear();
	return 0;
}