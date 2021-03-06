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
	#include <errno.h>
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
//��ʱ
#define SOCK_TIMEOUT (-(SOCK_ERR_BASE+2))
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

		//�Ƿ�Ϊ����
		bool is_non_block_;
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

		/*
			��Ҫ����Ϊ�Ƕ��������򷵻��쳣
		*/
		virtual SockRet ConnectTimeOut(const char* ipaddr, unsigned short port, int wait_ms = -1);

		virtual SockRet Bind(const char* ipaddr, unsigned short port);
        
		virtual SockRet Listen(int backlog);
        
		virtual SockRet Accept(Socket&client, int wait_ms = -1);
        
		virtual SockRet Accept(Socket& client,char* remote_ip, unsigned int ip_len,
			unsigned short* remote_port, int wait_ms = -1);
        
		virtual SockRet Send(const char* data, unsigned int data_len, int wait_ms = -1);
        
		virtual SockRet SendTo(const char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port, int wait_ms = -1);
        
		virtual SockRet Recv(char* data, unsigned int data_len, int wait_ms = -1);
        
		virtual SockRet Recvfrom(char* data, unsigned int data_len, \
			const char* ipaddr, unsigned short port, int wait_ms=-1);
        
		virtual SockRet Close();

		/***** 2021/03/05 ���� ��ʱ�ӿ� ******/

		//�ȴ�ʱ�� ,int wait_ms=-1
		enum WAIT_TYPE
		{
			WAIT_READ,
			WAIT_WRITE,
		};
		virtual SockRet WaitTimeOut(WAIT_TYPE type, int wait_ms);
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

		bool SetNonBlock(bool is_non_block);

		bool SetReusePort(bool set);

		//�Ƿ�Ϊ��Ч�׽���
		bool IsVaild()
		{
			
			return INVALID_SOCKET!=sock_;
		}
    private:

    };

	//ʵ��
	inline Socket::Socket() :sock_(INVALID_SOCKET), is_non_block_(false)
	{
		Init();
	}
	inline Socket::Socket(SOCKET sock) : sock_(sock), is_non_block_(false)
	{
		Init();
	}
	inline Socket::Socket(SockType type) : sock_(INVALID_SOCKET), is_non_block_(false)
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
		:sock_(INVALID_SOCKET), is_non_block_(false)
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
		/*int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}*/

		//����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;
		//���׽��ֺ�IP���˿ڰ�
		return  ::connect(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	}
	inline SockRet Socket::ConnectTimeOut(const char * ipaddr, unsigned short port, int wait_ms)
	{
		if (wait_ms < 0)
			return Connect(ipaddr, port);
		bool old_is_non_block = is_non_block_;

		//���óɷǶ���
		if (!is_non_block_)
		{
			SetNonBlock(true);
		}
		int ret = Connect(ipaddr, port);
		if (ret == 0)
		{
			SetNonBlock(old_is_non_block);
			return 0;
		}

		int wait_ret = WaitTimeOut(WAIT_WRITE, wait_ms);
		SetNonBlock(old_is_non_block);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}

		int error = 0;
#ifdef OS_WINDOWS
		int length = sizeof(error);
#endif

#ifdef OS_LINUX
		socklen_t length = sizeof(error);
#endif
		
		if (getsockopt(sock_, SOL_SOCKET, SO_ERROR, (char*)&error, &length) < 0)
		{
			SIM_LERROR("get socket option failed");
			return SOCK_FAILURE;
		}

		if (error != 0)
		{
			SIM_LERROR("connection failed after select with the error:"<< error);
			return SOCK_FAILURE;
		}

		return SOCK_SUCCESS;
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
	inline SockRet Socket::Accept(Socket&s, int wait_ms)
	{
		/*if (NULL == s)
			return SOCK_FAILURE;*/
		
		int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}

		SOCKET accept_cli = ::accept(sock_, NULL, 0);
		s = accept_cli;
		return SOCK_SUCCESS;
	}
	inline SockRet Socket::Accept(Socket& s,
		char * remote_ip, unsigned int ip_len, unsigned short * remote_port, int wait_ms)
	{
		/*if (NULL == s)
			return SOCK_FAILURE;*/

		int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}

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
		s = accept_cli;
		return SOCK_SUCCESS;
	}
	inline SockRet Socket::Send(const char * data, unsigned int data_len, int wait_ms)
	{
		int wait_ret = WaitTimeOut(WAIT_WRITE, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}
		return ::send(sock_, data, data_len, 0);
	}
	inline SockRet Socket::SendTo(const char * data, unsigned int data_len, const char * ipaddr, unsigned short port, int wait_ms)
	{
		int wait_ret = WaitTimeOut(WAIT_WRITE, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}
		//����sockaddr_in�ṹ�����
	   //����sockaddr_in�ṹ�����
		struct sockaddr_in serv_addr;
		if (!IpToAddressV4(ipaddr, port, &serv_addr))
			return -1;
		//���׽��ֺ�IP���˿ڰ�
		return ::sendto(sock_, data, data_len, 0,
			(struct sockaddr*)&serv_addr, sizeof(serv_addr));

	}
	inline SockRet Socket::Recv(char * data, unsigned int data_len, int wait_ms)
	{
		int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}
		return ::recv(sock_, data, data_len, 0);
	}
	inline SockRet Socket::Recvfrom(char * data, unsigned int data_len, const char * ipaddr, unsigned short port, int wait_ms)
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

		int wait_ret = WaitTimeOut(WAIT_READ, wait_ms);
		if (wait_ret != SOCK_SUCCESS)
		{
			return wait_ret;
		}

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

	inline SockRet Socket::WaitTimeOut(WAIT_TYPE type, int wait_ms)
	{
		/*
		�������select�Ĳ�����
		int maxfdp��һ������ֵ����ָ�����������ļ��������ķ�Χ���������ļ������������ֵ��1�����ܴ�
			��Windows�����������ֵ����ν���������ò���ȷ��
		fd_set *readfds��ָ��fd_set�ṹ��ָ�룬���������Ӧ�ð����ļ���������������Ҫ������Щ�ļ��������Ķ��仯�ģ�
			�����ǹ����Ƿ���Դ���Щ�ļ��� ��ȡ�����ˣ���������������һ���ļ��ɶ���select�ͻ᷵��һ������0��ֵ��
			��ʾ���ļ��ɶ������û�пɶ����ļ��������timeout�������ж� �Ƿ�ʱ��������timeout��ʱ�䣬select����0��
			���������󷵻ظ�ֵ�����Դ���NULLֵ����ʾ�������κ��ļ��Ķ��仯��
		fd_set *writefds��ָ��fd_set�ṹ��ָ�룬���������Ӧ�ð����ļ���������������Ҫ������Щ�ļ���������д�仯�ģ�
			�����ǹ����Ƿ��������Щ�ļ� ��д�������ˣ���������������һ���ļ���д��select�ͻ᷵��һ������0��ֵ��
			��ʾ���ļ���д�����û�п�д���ļ��������timeout�������� ���Ƿ�ʱ��������timeout��ʱ�䣬select����0��
			���������󷵻ظ�ֵ�����Դ���NULLֵ����ʾ�������κ��ļ���д�仯��
		fd_set *errorfdsͬ����������������ͼ�����������ļ������쳣��
		struct timeval* timeout��select�ĳ�ʱʱ�䣬�������������Ҫ��������ʹselect��������״̬����һ������NULL���βδ��룬
		��������ʱ��ṹ������ ��select��������״̬��һ���ȵ������ļ�������������ĳ���ļ������������仯Ϊֹ���ڶ���
		����ʱ��ֵ��Ϊ0��0���룬�ͱ��һ������ķ����������� �����ļ��������Ƿ��б仯�������̷��ؼ���ִ�У��ļ��ޱ仯����0��
		�б仯����һ����ֵ��������timeout��ֵ����0������ǵȴ��ĳ�ʱʱ�䣬�� select��timeoutʱ����������
		��ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ�����أ�����ֵͬ������
		*/
		struct timeval tv,*p=NULL;
		if (wait_ms >= 0)
		{
			tv.tv_sec = wait_ms / 1000;
			tv.tv_usec = (wait_ms % 1000) * 1000;
			p = &tv;
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(sock_, &fds);

		int ret = -1;
		if (WAIT_READ == type)
		{
			ret = select(sock_+1, &fds, NULL, NULL, p);
		}
		else if (WAIT_WRITE == type)
		{
			ret = select(sock_+1, NULL, &fds, NULL, p);
		}

		if (ret < 0)
		{
			SIM_LERROR("select error ret=" << ret << " errno=" << errno);
			return SOCK_FAILURE;
		}
		else if (ret == 0)
		{
			SIM_LERROR("select timeout ");
			return SOCK_TIMEOUT;
		}
		

		if (FD_ISSET(sock_, &fds))  //��������룬��stdin�л�ȡ�����ַ�  
		{
			//�¼����
			return SOCK_SUCCESS;
		}
		//��Ӧ��������
		SIM_LERROR("select error ret=" << ret << " errno=" << errno);
		return SOCK_FAILURE;
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
	inline bool Socket::SetNonBlock(bool is_non_block)
	{
		
#ifdef OS_WINDOWS
		unsigned long ul = is_non_block;
		int ret = ioctlsocket(sock_, FIONBIO, (unsigned long *)&ul);    //���óɷ�����ģʽ
		if (ret == SOCKET_ERROR)   //����ʧ��
		{
			return false;
		}
		is_non_block_ = is_non_block;
		return true;
#endif
#ifdef OS_LINUX
		int old_option = fcntl(sock_, F_GETFL);
		int new_option = old_option | (is_non_block ? O_NONBLOCK : (~O_NONBLOCK));
		if (fcntl(sock_, F_SETFL, new_option) < 0)
		{
			return false;
		}
		is_non_block_ = is_non_block;
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