/*
	array Êý×é²âÊÔ
*/

#include "Test.hpp"
#include "Array.hpp"

SIM_TEST(ArraySetAlloc)
{
	sim::Array<int>a;
	SIM_TEST_IS_TRUE(a.SetAlloc(NULL,NULL, NULL));
}

SIM_TEST(ArrayEmpty)
{
	sim::Array<int> a;
	SIM_TEST_IS_EQUAL(0, a.Size());
	SIM_TEST_IS_EQUAL(0, a.Capacity());
}

SIM_TEST(ArraySize)
{
	sim::Array<int> a;
	unsigned int size = 10000;

	SIM_TEST_IS_EQUAL(0, a.Size());

	for (int i = 0; i < size; ++i)
	{
		a.Assign(i);
	}
	SIM_TEST_IS_EQUAL(size, a.Size());
	
	a.Clear();

	SIM_TEST_IS_EQUAL(0, a.Size());
}

SIM_TEST(ArrayAssignAndErase)
{
	sim::Array<int> a;
	SIM_TEST_IS_TRUE(a.Assign(0));
	SIM_TEST_IS_EQUAL(1, a.Size());

	SIM_TEST_IS_EQUAL(0, a[0]);
	SIM_TEST_IS_EQUAL(0, a.At(0));
	SIM_TEST_IS_EQUAL(0, *(a.Begin()));
	SIM_TEST_IS_EQUAL(0, *(a.End() - 1));

	SIM_TEST_IS_TRUE(a.Erase(0));

	SIM_TEST_IS_EQUAL(0, a.Size());
}

SIM_TEST(ArrayAssignList)
{
	sim::Array<int> a;
	const int size = 100;
	int arr[size];
	for (int i = 0; i < 100; ++i)
	{
		arr[i] = i;
	}

	SIM_TEST_IS_TRUE(a.Assign(arr,size));
	SIM_TEST_IS_EQUAL(size, a.Size());

	SIM_TEST_IS_TRUE(a.Assign(arr, size));
	SIM_TEST_IS_EQUAL(size*2, a.Size());
	SIM_TEST_IS_EQUAL(0 , a[size]);
}

SIM_TEST(ArrayModify)
{
	sim::Array<int> a;
	for (int i = 0; i < 100000; ++i)
	{
		a.Assign(i);
	}
	int index = 800;
	SIM_TEST_IS_EQUAL(index, a[index]);
	SIM_TEST_IS_EQUAL(index, a.At(index));
	a[index]++;
	SIM_TEST_IS_EQUAL(index+1, a[index]);
	SIM_TEST_IS_EQUAL(index+1, a.At(index));

	++a[index];
	SIM_TEST_IS_EQUAL(index + 2, a[index]);
	SIM_TEST_IS_EQUAL(index + 2, a.At(index));

	a[index]+=100;
	SIM_TEST_IS_EQUAL(index + 102, a[index]);
	SIM_TEST_IS_EQUAL(index + 102, a.At(index));
}

SIM_TEST(ArrayFor)
{
	sim::Array<int> a;
	int size = 5;
	for (int i = 0; i < size; ++i)
	{
		a.Assign(i);
	}

	int index = 0;
	for (int *p = a.Begin(); p != a.End(); ++p)
	{
		SIM_TEST_IS_EQUAL(index, *p);
		++index;
	}
	
	for (int i=0; i<a.Size(); ++i)
	{
		SIM_TEST_IS_EQUAL(i, a[i]);
	}
}

SIM_TEST(ArrayAssignment)
{
	sim::Array<int> a;
	int size = 5;
	for (int i = 0; i < size; ++i)
	{
		a.Assign(i);
	}
	sim::Array<int> b(a);
	sim::Array<int> c;
	c = a;

	SIM_TEST_IS_EQUAL(a.Size(), b.Size());
	SIM_TEST_IS_EQUAL(a.Size(), c.Size());

	for (int i = 0; i < a.Size(); ++i)
	{
		SIM_TEST_IS_EQUAL(a[i], b[i]);
		SIM_TEST_IS_EQUAL(a[i], c[i]);
	}
}

SIM_TEST(ArrayForErase)
{
	sim::Array<int> a;
	int size = 5;

	for (int i = 0; i < size; ++i)
	{
		a.Assign(i);
	}
	for (int *p = a.Begin(); p != a.End(); )
	{
		p = a.ErasePtr(p);
	}
	SIM_TEST_IS_EQUAL(0, a.Size());

	for (int i = 0; i < size; ++i)
	{
		a.Assign(i);
	}
	for (int i = 0; i < size; ++i)
	{
		a.Erase(0);
	}
	SIM_TEST_IS_EQUAL(0,a.Size());
}

struct ArrayAssignAndEraseStruct
{
	int i;
	double j;
	std::string str;
	ArrayAssignAndEraseStruct() :
		i(0), j(0.0), str("")
	{

	}
};
SIM_TEST(ArrayAssignAndEraseStruct)
{
	sim::Array<ArrayAssignAndEraseStruct> a;
	ArrayAssignAndEraseStruct data;
	data.i = 1;
	data.j = 1.123;
	data.str = "abcdef";

	SIM_ASSERT_IS_TRUE(a.Assign(data));

	SIM_TEST_IS_EQUAL(data.i, a[0].i);
	SIM_TEST_IS_EQUAL(data.j, a[0].j);
	SIM_TEST_IS_EQUAL(data.str, a[0].str);
}

SIM_TEST_MAIN(sim::noisy)