/*
* socket �ӿڷ�װ
*/
#ifndef SIM_SOCKET_HPP_
#define SIM_SOCKET_HPP_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #ifndef OS_WINDOWS
        #define OS_WINDOWS
    #endif
    #include <stdio.h>
    #include <winsock2.h>
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
    ��
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #ifndef OS_LINUX
        #define OS_LINUX
    #endif  
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
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

#endif
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
        //Socket(SockType type);
        Socket(int af, int type, int protocol);
        ~Socket();
    public:
        //��װ�ĺ��� ͬ���ӿ�
        SockRet connect(const char* ipaddr, unsigned short port);
        SockRet bind(const char* ipaddr, unsigned short port);
        SockRet listen(int backlog);
        Socket accept();
        Socket accept(char* remote_ip, unsigned int ip_len, unsigned short* remote_port);
        SockRet send(const char* data, unsigned int data_len);
        SockRet sendto(const char* data, unsigned int data_len, \
            const char* ipaddr, unsigned short port);
        SockRet recv(char* data, unsigned int data_len);
        SockRet recvfrom(char* data, unsigned int data_len, \
            const char* ipaddr, unsigned short port);
        SockRet close();
    public:
        //��չ
        SOCKET get_socket();//��ȡԭʼsocket

        SOCKET release();//�ͷ�socket����Ȩ

        static void Init()
        {
#ifdef OS_WINDOWS
            //��ʼ������
            static WsInit g_init;
#endif
        }

        //�ṹ��ת��
        static bool IpToAddressV4(const char* ipaddr, unsigned short port
            , struct sockaddr_in*out_addr)
        {
            if (NULL == out_addr)
                return false;
            memset(&out_addr, 0, sizeof(out_addr));  //ÿ���ֽڶ���0���
            out_addr.sin_family = AF_INET;  //ʹ��IPv4��ַ
            if (ipaddr)
                out_addr.sin_addr.s_addr = inet_addr(ipaddr);  //�����IP��ַ
            else
                out_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //����ip
            out_addr.sin_port = htons(port);  //�˿�
            return true;
        }
        static bool AddressToIpV4(const struct sockaddr_in* addr,
            char* ipaddr, unsigned int ipaddr_len, unsigned short* port
        )
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


    private:

    };

}
#endif