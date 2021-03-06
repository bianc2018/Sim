/*
* µ÷ÊÔarray
*/
#include "Array.hpp"
#include <string>
void arr_print(sim::Array<int>&a)
{
	std::string temp;
	for (int i = 0; i < a.Size(); ++i)
	{
		if (i == 0)
			temp += "[ " + std::to_string(a[i]);
		else
			temp+="," + std::to_string(a[i]);
	}
	if (temp.empty())
		temp = "[ ]\n";
	else
		temp += " ]\n";
	printf(temp.c_str());
}
int main(int argc, char* argv[])
{
	sim::Array<int>a1;
	for (int i = 0; i < 5; ++i)
	{
		a1.Assign(i);
		arr_print(a1);
	}
	a1.Erase(0);
	arr_print(a1);
	a1.Erase(1);
	arr_print(a1);
	a1.Erase(a1.Size()-1);
	arr_print(a1);

	for (int i = 0; i < 100000; ++i)
	{
		a1.Assign(i);
		//arr_print(a1);
	}
	while (a1.Size() != 0)
		a1.Erase(0);
	for (int i = 0; i < 100000; ++i)
	{
		a1.Assign(i);
		//arr_print(a1);
	}
	return 0;
}