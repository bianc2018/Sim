/*��������ģ��*/
#ifndef SIM_NET_HPP_
#define SIM_NET_HPP_

#include <stdio.h>
#include "RefObject.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
    #define OS_WINDOWS
#endif
//windowsƽ̨ʹ��iocp
#ifndef ASYNC_IOCP
    #define ASYNC_IOCP
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
    #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif


#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN  
#endif
#include <WinSock2.h>
//ʹ����չ�ӿ�
#include <winsock2.h>
#include <MSWSock.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll
namespace sim
{
    //��ʼ������
    class NetWsInit
    {
    public:
        NetWsInit()
        {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }
        ~NetWsInit()
        {
            //��ֹ DLL ��ʹ��
            WSACleanup();
            }
        };
}

typedef SOCKET NetSock;

#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
#define OS_LINUX
#endif  
//����linuxƽ̨�ӿ�
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
//linuxʹ��epoll
#ifndef ASYNC_EPOLL
   #define ASYNC_EPOLL
#endif
typedef int NetSock;
#ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
#endif
#else
#error "��֧�ֵ�ƽ̨"
#endif

namespace sim
{
	//ǰ������
	class NetSession;
	class NetManager;

	//��������
	typedef sim::RefBuff NetBuffer;
	typedef sim::RefBuff NetString;
	typedef RefObject<NetSession> NetSessionPtr;
	typedef RefObject<NetManager> NetManagerPtr;
	//�����ַ
	class NetAddress
	{
		NetString ip;
		unsigned port;
	};

	//�Ự����
	class NetSession:public sim::EnableRefFormThis<NetSession>
	{
		friend class NetManager;

		NetSession(WeakObject<NetManager> net_manager);
	public:
		virtual NetSessionPtr NewSession(WeakObject<NetManager> net_manager)=0;
		virtual ~NetSession();
    public:
        //�ص�
		//�������ӻص�
		virtual void OnAccept(NetSock client)=0;

		//������ɻص�
		virtual void OnConnect()=0;

		//tcp���ݽ��ջص�
		virtual void OnRecvData(const NetBuffer&buff)=0;

		//udp���ݽ��ջص�
		virtual void OnRecvDataFrom(const NetBuffer& buff,
			const NetAddress&address)=0;

		//���ݷ�����ɻص�
		virtual void OnSendComplete(const NetBuffer& buff) = 0;
		
		//���ӹر�
		virtual void OnClose()=0;
    public:
        //�ӿ�
		virtual bool StartConnect(const NetAddress&address) = 0;

		virtual bool StartAccept(const NetAddress& address, int backlog=1024) = 0;

		virtual bool StopAccept() = 0;

		virtual bool StartRecv(NetBuffer buff=NULL) = 0;

		virtual bool StartRecvForm(NetBuffer buff = NULL) = 0;

		virtual bool StopRecv() = 0;

		virtual bool Send(const NetBuffer& buff) = 0;

		virtual bool SendTo(const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close()=0;

	private:

	private:
		NetSock sock_;
		bool connect_flag_, accept_flag_, recv_flag_;
		WeakObject<NetManager> net_manager_;
	};

	//����������
	class NetManager :public sim::EnableRefFormThis<NetManager>
	{
		friend class NetSession;

		NetManager();
	public:
		virtual ~NetManager(){];

		virtual NetManagerPtr NewManager()=0;

		//�¼�ѭ�� wait_ms���ȴ�ʱ��
		virtual int Poll(unsigned wait_ms)=0;

	protected:
		//���幦��ʵ��
		virtual bool StartConnect(NetSessionPtr ss,const NetAddress&address) = 0;

		virtual bool Accept(NetSessionPtr ss, const NetAddress& address, int backlog = 1024) = 0;

		virtual bool Recv(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool RecvForm(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool Send(NetSessionPtr ss, const NetBuffer& buff) = 0;

		virtual bool SendTo(NetSessionPtr ss, const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close(NetSessionPtr ss) = 0;
	private:

	};
}

#endif // SIM_NET_HPP_
