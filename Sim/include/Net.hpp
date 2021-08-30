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
#ifndef NET_IOCP
    #define NET_IOCP
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
#ifndef NET_EPOLL
   #define NET_EPOLL
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

	//�¼�����
	enum NetType
	{
		//���ӽ����¼�
		NetTypeConnect,

		//���������¼�
		NetTypeAccept,

		//����TCP�����¼�
		NetTypeRecv,

		//����UDP�����¼������˵�ַ��Ϣ��
		NetTypeRecvFrom,

		//���ݷ����¼�
		NetTypeSend,

		//���ӹر��¼�
		NetTypeClose,
	};

	//�����ַ
	class NetAddress
	{
		NetString ip;
		unsigned port;
	};

	//�Ự����
	class NetSession :public sim::EnableRefFormThis<NetSession>
	{
		friend class NetManager;

		NetSession(WeakObject<NetManager> net_manager);
	public:
		virtual NetSessionPtr NewSession(WeakObject<NetManager> net_manager) = 0;
		virtual ~NetSession() {};
	public:
		//�ص�
		//�������ӻص�
		virtual void OnAccept(NetSock client) = 0;

		//������ɻص�
		virtual void OnConnect() = 0;

		//tcp���ݽ��ջص�
		virtual void OnRecvData(const NetBuffer&buff) = 0;

		//udp���ݽ��ջص�
		virtual void OnRecvDataFrom(const NetBuffer& buff,
			const NetAddress&address) = 0;

		//���ݷ�����ɻص�
		virtual void OnSendComplete(const NetBuffer& buff) = 0;

		//���ӹر�
		virtual void OnClose() = 0;
	public:
		//�ӿ�
		virtual bool StartConnect(const NetAddress&address) = 0;

		virtual bool StartAccept(const NetAddress& address, int backlog = 1024) = 0;

		virtual bool StopAccept() = 0;

		virtual bool StartRecv(NetBuffer buff = NULL) = 0;

		virtual bool StartRecvForm(NetBuffer buff = NULL) = 0;

		virtual bool StopRecv() = 0;

		virtual bool Send(const NetBuffer& buff) = 0;

		virtual bool SendTo(const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close() = 0;

	private:

	private:
		NetSock sock_;
		bool bind_flag_, connect_flag_, accept_flag_, recv_flag_;
		WeakObject<NetManager> net_manager_;
	};

	//����������
	class NetManager :public sim::EnableRefFormThis<NetManager>
	{
		friend class NetSession;

		NetManager();
	public:
		virtual ~NetManager() {};

		virtual NetManagerPtr NewManager() = 0;

		//�¼�ѭ�� wait_ms���ȴ�ʱ��
		virtual int Poll(unsigned wait_ms) = 0;

	protected:
		//���幦��ʵ��
		virtual bool StartConnect(NetSessionPtr ss, const NetAddress&address) = 0;

		virtual bool Accept(NetSessionPtr ss, const NetAddress& address, int backlog = 1024) = 0;

		virtual bool Recv(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool RecvForm(NetSessionPtr ss, NetBuffer buff = NULL) = 0;

		virtual bool Send(NetSessionPtr ss, const NetBuffer& buff) = 0;

		virtual bool SendTo(NetSessionPtr ss, const NetBuffer& buff, const NetAddress& address) = 0;

		virtual bool Close(NetSessionPtr ss) = 0;
	private:

	};

#ifdef NET_IOCP

	//wsa��չ�������ض���
	class WsaExFunction
	{
	public:
		WsaExFunction()
		{
			static NetWsInit init;

			//����һ���յ�socket
			SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

			//������չ����
			__AcceptEx = (LPFN_ACCEPTEX)_GetExFunctnion(socket, WSAID_ACCEPTEX);
			__ConnectEx = (LPFN_CONNECTEX)_GetExFunctnion(socket, WSAID_CONNECTEX);
			__AcceptExScokAddrs = (LPFN_GETACCEPTEXSOCKADDRS)_GetExFunctnion(socket, WSAID_GETACCEPTEXSOCKADDRS);
			__DisconnectionEx = (LPFN_DISCONNECTEX)_GetExFunctnion(socket, WSAID_DISCONNECTEX);

			//�ر�����
			closesocket(socket);
		}
	private:
		//������չ����
		static void* _GetExFunctnion(const SOCKET &socket, const GUID& which)
		{
			void* func = nullptr;
			DWORD bytes = 0;
			WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&which,
				sizeof(which), &func, sizeof(func), &bytes, NULL, NULL);

			return func;
		}
	public:
		LPFN_ACCEPTEX                __AcceptEx;
		LPFN_CONNECTEX               __ConnectEx;
		LPFN_GETACCEPTEXSOCKADDRS    __AcceptExScokAddrs;
		LPFN_DISCONNECTEX            __DisconnectionEx;
	};

	//�ص�����
	struct IocpOverlapped
	{
		//IO�ص�����
		OVERLAPPED  overlapped;

		NetType type;

		//�����Ӵ��� acceptexʱ������
		NetSock accepted;

		//����
		NetBuffer buff;
		//buff����������
		WSABUF wsa_buf;

		//������ֽ���Ŀ
		DWORD bytes_transfered;

		//��ַ ����SendTo or Recvfrom
		struct sockaddr_in temp_addr;
		int temp_addr_len;

		//�Ự����
		NetSessionPtr ss;
	};

	typedef NetSession IocpNetSession;

	class IocpNetManager :public NetManager
	{
		IocpNetManager();
	public:
		virtual ~IocpNetManager();

		virtual NetManagerPtr NewManager();

		//�¼�ѭ�� wait_ms���ȴ�ʱ��
		virtual int Poll(unsigned wait_ms);

	protected:
		//���幦��ʵ��
		virtual bool StartConnect(NetSessionPtr ss, const NetAddress&address);

		virtual bool Accept(NetSessionPtr ss, const NetAddress& address, int backlog = 1024);

		virtual bool Recv(NetSessionPtr ss, NetBuffer buff = NULL);

		virtual bool RecvForm(NetSessionPtr ss, NetBuffer buff = NULL);

		virtual bool Send(NetSessionPtr ss, const NetBuffer& buff);

		virtual bool SendTo(NetSessionPtr ss, const NetBuffer& buff, const NetAddress& address);

		virtual bool Close(NetSessionPtr ss);
	protected:
		//������չ����
		WsaExFunction &LoadWsaExFunction()
		{
			//����wsa��չ��������ֻ֤����һ��
			static WsaExFunction ex_func;
			return ex_func;
		}

	private:
		HANDLE iocp_handler_;
	};

	typedef IocpNetSession SimNetSession;
	typedef IocpNetManager SimNetManager;
#endif
}
#endif // SIM_NET_HPP_
