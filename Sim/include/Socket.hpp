/*
* socket �ӿڷ�װ
*/
#ifndef SIM_SOCKET_HPP_
#define SIM_SOCKET_HPP_

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
        class WsInit
        {
        public:
            WsInit()
            {
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
            }
            ~WsInit()
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
//�ɹ�
#define SOCK_SUCCESS 0
#define SOCK_ERR_BASE SOCK_SUCCESS
#define SOCK_FAILURE (-(SOCK_ERR_BASE+1))
namespace sim
{
    //����
    enum SockType
    {
        TCP,
        UDP
    };

    //���صĴ�����
    typedef int SockRet;
    //INVALID_SOCKET
    class  Socket
    {
    private:
        /* data */
        //ԭʼ�׽���
        SOCKET sock_;

    public:
        //���캯��
		Socket();
        
		Socket(SOCKET sock);
        
        // SOCK_STREAM tcp SOCK_DGRAM udp
		Socket(SockType type);
        
		Socket(int af, int type, int protocol);
        
		virtual ~Socket();
    public:
        //��װ�ĺ��� ͬ���ӿ�
        //���� false ��ʶ��ֹ
        typedef bool(*GetHostByNameCallBack)(const char* ip, void* pdata);
		static SockRet GetHostByName(const char* szHost,
			GetHostByNameCallBack cb, void* pdata);

		virtual SockRet Connect(const char* ipaddr, unsigned short port);
        
		virtual SockRet Bind(const char* ipaddr, unsigned short port);
        
		virtual SockRet Listen(int backlog);
        
		virtual SockRet Accept(Socket*client);
        
		virtual SockRet Accept(Socket* client,char* remote_ip, unsigned int ip_len,
			unsigned short* remote_port);
        
		virtual SockRet Send(const char* data, unsigned int data_len);
        
		virtual SockRet SendTo(const char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port);
        
		virtual SockRet Recv(char* data, unsigned int data_len);
        
		virtual SockRet Recvfrom(char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port);
        
		virtual SockRet Close();

		
    public:
		//����
		void SetSocket(SOCKET s);
        //��ȡԭʼsocket
		SOCKET GetSocket();

        //�ͷ�socket����Ȩ
		SOCKET Release();

		static void Init();

        //�ṹ��ת��
		static bool IpToAddressV4(const char* ipaddr, unsigned short port
			, struct sockaddr_in* out_addr);
		static bool AddressToIpV4(const struct sockaddr_in* addr,
			char* ipaddr, unsigned int ipaddr_len, unsigned short* port
		);

		bool SetNonBlock(bool block);

		bool SetReusePort(bool set);

		//�Ƿ�Ϊ��Ч�׽���
		bool IsVaild()
		{
			
			return INVALID_SOCKET!=sock_;
		}
    private:

    };

	//ʵ��
	inline Socket::Socket() :sock_(INVALID_SOCKET)
	{
		Init();
	}
	inline Socket::Socket(SOCKET sock) : sock_(sock)
	{
		Init();
	}
	inline Socket::Socket(SockType type) : sock_(INVALID_SOCKET)
	{
		Init();
		if (type == TCP)
			sock_ = socket(AF_INET, SOCK_STREAM, 0);
		else if (type == UDP)
			sock_ = socket(AF_INET, SOCK_DGRAM, 0);
		else
			sock_ = INVALID_SOCKET;
	}
	inline Socket::Socket(int af, int type, int protocol)
		:sock_(INVALID_SOCKET)
	{
		Init();
		sock_ = socket(af, type, protocol);
	}
	inline Socket::~Socket()
	{
		//ȡ���Զ��ر�
		//Close();
	}
	inline SockRet Socket::GetHostByName(const char * szHost, GetHostByNameCallBack cb, void * pdata)
	{
		if (NULL == szHost || NULL == cb)
		{
			return -1;//
		}
		hostent* pHost = gethostbyname(szHost);
		if (NULL == pHost)
		{
			return -1;
		}
		int i;
		in_addr addr;
		for (i = 0;; i++)
		{
			char* p = pHost->h_addr_list[i];
			if (p == NULL)
			{
				break;
			}
#ifdef OS_WINDOWS
			memcpy(&addr.S_un.S_addr, p, pHost->h_length);
			const char* strIp = ::inet_ntoa(addr);
#endif // OS_WINDOWS
#ifdef OS_LINUX
			const socklen_t buff_size = 128;
			char buff[buff_size] = { 0 };
			const char* strIp = inet_ntop(pHost->h_addrtype,
				p, buff, buff_size);
#endif 
			if (!cb(strIp, pdata))
				return 0;
		}
		return 0;
	}
	inline SockRet Socket::Connect(const char * ipaddr, unsigned short port)
	{
		//����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;
		//���׽��ֺ�IP���˿ڰ�
		return  ::connect(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	}
	inline SockRet Socket::Bind(const char * ipaddr, unsigned short port)
	{
		//����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;

		return ::bind(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	}
	inline SockRet Socket::Listen(int backlog)
	{
		return ::listen(sock_, backlog);
	}
	inline SockRet Socket::Accept(Socket*s)
	{
		if (NULL == s)
			return SOCK_FAILURE;
		SOCKET accept_cli = ::accept(sock_, NULL, 0);
		*s = accept_cli;
		return SOCK_SUCCESS;
	}
	inline SockRet Socket::Accept(Socket* s,
		char * remote_ip, unsigned int ip_len, unsigned short * remote_port)
	{
		if (NULL == s)
			return SOCK_FAILURE;

		//����sockaddr_in�ṹ�����
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));  //ÿ���ֽڶ���0���

#ifdef OS_WINDOWS
		int addr_len = sizeof(addr);
#endif

#ifdef OS_LINUX
		socklen_t addr_len = sizeof(addr);
#endif
		SOCKET accept_cli = ::accept(sock_, (struct sockaddr*)&addr, &addr_len);
		if (accept_cli == INVALID_SOCKET)
		{
			return SOCK_FAILURE;
		}
		if (!AddressToIpV4(&addr, remote_ip, ip_len, remote_port))
		{
			return SOCK_FAILURE;
		}
		*s = accept_cli;
		return SOCK_SUCCESS;
	}
	inline SockRet Socket::Send(const char * data, unsigned int data_len)
	{
		return ::send(sock_, data, data_len, 0);
	}
	inline SockRet Socket::SendTo(const char * data, unsigned int data_len, const char * ipaddr, unsigned short port)
	{
		//����sockaddr_in�ṹ�����
	   //����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;
		//���׽��ֺ�IP���˿ڰ�
		return ::sendto(sock_, data, data_len, 0,
			(struct sockaddr*)&serv_addr, sizeof(serv_addr));

	}
	inline SockRet Socket::Recv(char * data, unsigned int data_len)
	{
		return ::recv(sock_, data, data_len, 0);
	}
	inline SockRet Socket::Recvfrom(char * data, unsigned int data_len, const char * ipaddr, unsigned short port)
	{
		//����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
#ifdef OS_WINDOWS
		int add_len = sizeof(serv_addr);
#endif

#ifdef OS_LINUX
		socklen_t add_len = sizeof(serv_addr);
#endif
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;
		//���׽��ֺ�IP���˿ڰ�
		return ::recvfrom(sock_, data, data_len, 0,
			(struct sockaddr*)&serv_addr, &add_len);
	}
	inline SockRet Socket::Close()
	{
		SOCKET temp = sock_;
		sock_ = INVALID_SOCKET;

#ifdef OS_WINDOWS
		return ::closesocket(temp);
#endif

#ifdef OS_LINUX
		return ::close(temp);
#endif
	}
	inline void Socket::SetSocket(SOCKET s)
	{
		sock_ = s;
	}
	inline SOCKET Socket::GetSocket()
	{
		return sock_;
	}
	inline SOCKET Socket::Release()
	{
		SOCKET temp = sock_;
		sock_ = INVALID_SOCKET;
		return temp;
	}
	inline void Socket::Init()
	{
#ifdef OS_WINDOWS
		//��ʼ������
		static WsInit g_init;
#endif
	}
	inline bool Socket::IpToAddressV4(const char * ipaddr, unsigned short port, sockaddr_in * out_addr)
	{
		if (NULL == out_addr)
			return false;
		memset(out_addr, 0, sizeof(*out_addr));  //ÿ���ֽڶ���0���
		out_addr->sin_family = AF_INET;  //ʹ��IPv4��ַ
		if (ipaddr)
			out_addr->sin_addr.s_addr = inet_addr(ipaddr);  //�����IP��ַ
		else
			out_addr->sin_addr.s_addr = htonl(INADDR_ANY);  //����ip
		out_addr->sin_port = htons(port);  //�˿�
		return true;
	}
	inline bool Socket::AddressToIpV4(const sockaddr_in * addr, char * ipaddr, unsigned int ipaddr_len, unsigned short * port)
	{
		if (NULL == addr)
			return false;

		if (ipaddr && ipaddr_len >= 0)
		{
			snprintf(ipaddr, ipaddr_len, "%s", inet_ntoa(addr->sin_addr));
		}
		if (port)
		{
			*port = ntohs(addr->sin_port);
		}
		return true;
	}
	inline bool Socket::SetNonBlock(bool block)
	{
#ifdef OS_WINDOWS
		unsigned long ul = block;
		int ret = ioctlsocket(sock_, FIONBIO, (unsigned long *)&ul);    //���óɷ�����ģʽ
		if (ret == SOCKET_ERROR)   //����ʧ��
		{
			return false;
		}
		return true;
#endif
#ifdef OS_LINUX
		int old_option = fcntl(sock_, F_GETFL);
		int new_option = old_option | (block ? O_NONBLOCK : (~O_NONBLOCK));
		if (fcntl(sock_, F_SETFL, new_option) < 0)
		{
			return false;
		}
		return true;
#endif
		return false;
	}
	inline bool Socket::SetReusePort(bool set)
	{
#ifdef OS_WINDOWS
		int opt = set ? 1 : 0;
		int ret = setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, \
			(const char*)&opt, sizeof(opt));
		if (ret == SOCKET_ERROR)   //����ʧ��
		{
			return false;
		}
		return true;
#endif
#ifdef OS_LINUX
		int opt = set ? 1 : 0;
		int ret = setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT,
			&opt, static_cast<socklen_t>(sizeof(opt)));
		return ret >= 0;
#endif
		return false;
	}
}
#endif