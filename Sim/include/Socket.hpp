/*
* socket 接口封装
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
    #pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
    namespace sim
    {
        //初始化函数
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
                //终止 DLL 的使用
                WSACleanup();
            }
        };
    ｝
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
    #error "不支持的平台"
#endif

#endif
namespace sim
{
    //类型
    enum SockType
    {
        TCP,
        UDP
    };

    //返回的错误吗
    typedef int SockRet;
    //INVALID_SOCKET
    class  Socket
    {
    private:
        /* data */
        //原始套接字
        SOCKET sock_;
    public:
        //构造函数
        Socket();
        Socket(SOCKET sock);
        // SOCK_STREAM tcp SOCK_DGRAM udp
        //Socket(SockType type);
        Socket(int af, int type, int protocol);
        ~Socket();
    public:
        //封装的函数 同步接口
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
        //拓展
        SOCKET get_socket();//获取原始socket

        SOCKET release();//释放socket所有权

        static void Init()
        {
#ifdef OS_WINDOWS
            //初始化函数
            static WsInit g_init;
#endif
        }

        //结构体转换
        static bool IpToAddressV4(const char* ipaddr, unsigned short port
            , struct sockaddr_in*out_addr)
        {
            if (NULL == out_addr)
                return false;
            memset(&out_addr, 0, sizeof(out_addr));  //每个字节都用0填充
            out_addr.sin_family = AF_INET;  //使用IPv4地址
            if (ipaddr)
                out_addr.sin_addr.s_addr = inet_addr(ipaddr);  //具体的IP地址
            else
                out_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //所有ip
            out_addr.sin_port = htons(port);  //端口
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