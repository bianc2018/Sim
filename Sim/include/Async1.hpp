/*
	�첽�ӿ� 1
*/
#ifndef SIM_ASYNC_IO_HPP_
#define SIM_ASYNC_IO_HPP_
//SIM_ASYNC_IO_NEW
//#ifdef SIM_ASYNC_IO_NEW
#include <new>
//#endif

//ƽ̨���
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
		#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
	#endif
	#include <stdio.h>
	#include <WinSock2.h>
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (SOCKET)(~0)
	#endif
	#pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll
	namespace sim
	{
		//��ʼ������
		class AsyncInit
		{
		public:
			AsyncInit()
			{
				WSADATA wsaData;
				WSAStartup(MAKEWORD(2, 2), &wsaData);
			}
			~AsyncInit()
			{
				//��ֹ DLL ��ʹ��
				WSACleanup();
			}
		};
	}
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif  
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	typedef int SOCKET;
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET -1
	#endif
#else
	#error "��֧�ֵ�ƽ̨"
#endif

//������־
#ifdef USING_SIM_LOGGER
#include "Logger.hpp"
#else
#define SIM_FUNC(lv)
#define SIM_FUNC_DEBUG() 
#define SIM_LOG_CONFIG(max_lv,handler,userdata)
#define SIM_LOG(lv,x) 
#define SIM_LDEBUG(x) 
#define SIM_LINFO(x) 
#define SIM_LWARN(x) 
#define SIM_LERROR(x) 
#endif // USING_SIM_LOGGER

//������
#define ASYNC_SUCCESS SOCK_SUCCESS
#define ASYNC_FAILURE SOCK_FAILURE
#define ASYNC_ERR_BASE (SOCK_ERR_BASE+100)
//�������
#define ASYNC_IDLE			1
//����Ϊ��
#define ASYNC_ERR_SERVICE			(-(ASYNC_ERR_BASE+1))
//������ʱ
#define ASYNC_ERR_TIMEOUT			(-(ASYNC_ERR_BASE+2))
//Ŀ���׽�����Ч
#define ASYNC_ERR_SOCKET_INVAILD	(-(ASYNC_ERR_BASE+3))
//�յĻ���
#define ASYNC_ERR_EMPTY_BUFF		(-(ASYNC_ERR_BASE+4))

//�첽��ʶ
//����
#define ASYNC_FLAG_CONNECT			(1<<0)
//��������
#define ASYNC_FLAG_ACCEPT			(1<<1)
//��������
#define ASYNC_FLAG_RECV				(1<<2)
//����
#define ASYNC_FLAG_SEND				(1<<3)
//�Ͽ�����
#define ASYNC_FLAG_DISCONNECT		(1<<4)
//������
#define ASYNC_FLAG_ERROR		(1<<5)
//�ͷ��ڴ�
#define ASYNC_FLAG_RELEASE		(1<<6)

namespace sim
{
	//�����ַ
	struct NetAddr
	{
		//�Ƿ���Ч false Ϊ��Ч��ַ
		bool isusef;
		//0��β
		char ipaddr[64];
		unsigned port;
		enum IpType
		{
			//Ŀǰֻ֧��IPv4
			IPV4,
		};
		IpType type;
	};

	//�첽����
	struct AsyncBuffer
	{
		char*ptr;
		unsigned int size;
	};

	struct Event
	{
		//iocp 
#ifdef OS_WINDOWS
		OVERLAPPED  overlapped;
		WSABUF      wsa_buf;
#endif
		//����sock
		SOCKET link_sock;

		//���λ
		unsigned int event_flag;

		//����
		AsyncBuffer cache;

		//��������ݴ�С
		unsigned long long               bytes_transfered;

		//����
		int error;

		//accept�¼����淵�صĿͻ�������
		SOCKET accept_client;

		//�û���������
		void* pdata;

		//������ַ ���ӻ��߰� ���߽������ӵ�ʱ����Ч
		NetAddr net_addr;
#ifdef USING_SIM_LOGGER
	public:
		//���Ժ��������ʶ��ӡ ����������־���
		std::string FormatFlag();
#endif
	};

	//�¼��ص�
	typedef void(*AsyncEventHandler)(Event*e, void*pdata);

	//�ڴ���亯��
	typedef void* (*AsyncEventMalloc)(size_t size);
	typedef void(*AsyncEventFree)(void*);

	//�첽ϵͳ
	class AsyncEventService
	{
	public:
		AsyncEventService()
			:run_flag_(true) , malloc_(::malloc),free_(::free){};
		virtual ~AsyncEventService() {};
		
		//��һ��sock
		virtual SockRet AddSocket(SOCKET socket)=0;

		//�����
		virtual SockRet RemoveSocket(SOCKET socket)=0;

		//�����¼�
		virtual SockRet ProcessEvent(unsigned int wait_ms)=0;
		
		//�˳�
		virtual SockRet Exit()=0;

		//�����ڴ����뺯��
		virtual bool SetAlloc(AsyncEventMalloc m, AsyncEventFree f);

	protected:

	protected:
		virtual SockRet DoEvent(Event* e);

		virtual void* Malloc(size_t size);
		virtual void Free(void* p);
		//����һ���¼�
		virtual Event* MallocEvent();
		virtual void FreeEvent(Event* pe);
	protected:
		bool run_flag_;
		AsyncEventMalloc malloc_;
		AsyncEventFree free_;
		//�ص����
		AsyncEventHandler handler_; 
		void* pdata_;
	};

}