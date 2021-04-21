#include "AsyncHttp.hpp"
int main(int argc, char* argv[])
{
	sim::AsyncHttp http;
	sim::AsyncHandle handle =  http.CreateSession();
	http.Connect(handle, "https://www.baidu.com/");
	return 0;
}