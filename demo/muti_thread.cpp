#include "MultiThread.hpp"
#include <ctime>
#include <cstdio>
#include <vector>

int main(int argc, char*argv[])
{
	sim::AtomicNumber n(100);
	n.Add(1000);
	printf("n:%d\n", long(n));
}