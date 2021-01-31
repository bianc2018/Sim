#include "Socket.hpp"
#include "Thread.hpp"

#if _WIN32
HANDLE completionPort = NULL;
ThRet GetCompletionPort(LPVOID p)
{
	DWORD NumberOfBytesTransferred = 0;
	ULONG pkey=NULL;
	LPOVERLAPPED* lpOverlapped = NULL;
	while (true)
	{
		BOOL ret = GetQueuedCompletionStatus(completionPort,
			&NumberOfBytesTransferred, &pkey, lpOverlapped, 10);
	}
	return 0;
}
int main(int argc, char* argv[])
{
	sim::Socket::Init();
	//WSAID_CONNECTEX;
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	sim::Thread th(GetCompletionPort, NULL);
	//WSAIoctl()
	sim::Socket s(sim::TCP);
	s.SetNonBlock(true);

	getchar();
	if (th.JoinAble())
		th.Join();
	return 0;
}
#else
int main(int argc, char* argv[])
{
	return 1;
}
#endif