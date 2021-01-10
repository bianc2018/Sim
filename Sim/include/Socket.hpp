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
    #include <WinSock2.h>
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
        Socket() :sock_(INVALID_SOCKET)
        {

        }
        
        Socket(SOCKET sock) :sock_(sock)
        {

        }
        
        // SOCK_STREAM tcp SOCK_DGRAM udp
        Socket(SockType type) :sock_(INVALID_SOCKET)
        {
            if (type == TCP)
                sock_ = socket(AF_INET, SOCK_STREAM, 0);
            else if (type == UDP)
                sock_ = socket(AF_INET, SOCK_DGRAM, 0);
            else
                sock_ = INVALID_SOCKET;
        }
        
        Socket(int af, int type, int protocol)
            :sock_(INVALID_SOCKET)
        {
            sock_ = socket(af, type, protocol);
        }
        
        ~Socket()
        {
            Close();
        }
    public:
        //封装的函数 同步接口
        //返回 false 标识终止
        typedef bool(*GetHostByNameCallBack)(const char* ip, void* pdata);
        static SockRet GetHostByName(const char* szHost,
            GetHostByNameCallBack cb, void* pdata)
        {
            if (NULL == szHost||NULL == cb)
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

        SockRet Connect(const char* ipaddr, unsigned short port)
        {
            //创建sockaddr_in结构体变量
            struct sockaddr_in serv_addr;
            if(!IpToAddressV4(ipaddr, port, &serv_addr))
                return -1;
            //将套接字和IP、端口绑定
            return  ::connect(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        }
        
        SockRet Bind(const char* ipaddr, unsigned short port)
        {
            //创建sockaddr_in结构体变量
            struct sockaddr_in serv_addr;
            if (!IpToAddressV4(ipaddr, port, &serv_addr))
                return -1;

            return ::bind(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        }
        
        SockRet Listen(int backlog)
        {
            return ::listen(sock_, backlog);
        }
        
        Socket Accept()
        {
            SOCKET ret = ::accept(sock_, NULL, 0);
            return Socket((SOCKET)ret);
        }
        
        Socket Accept(char* remote_ip, unsigned int ip_len,
            unsigned short* remote_port)
        {
            //创建sockaddr_in结构体变量
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));  //每个字节都用0填充

#ifdef OS_WINDOWS
            int addr_len = sizeof(addr);
#endif

#ifdef OS_LINUX
            socklen_t addr_len = sizeof(addr);
#endif
            SOCKET ret = ::accept(sock_, (struct sockaddr*)&addr, &addr_len);
            if (ret == INVALID_SOCKET)
            {
                return -1;
            }
            if (!AddressToIpV4(&addr, remote_ip, ip_len, remote_port))
            {
                return -1;
            }

            return Socket((SOCKET)ret);
        }
        
        SockRet Send(const char* data, unsigned int data_len)
        {
            return ::send(sock_, data, data_len, 0);
        }
        
        SockRet SendTo(const char* data, unsigned int data_len, \
            const char* ipaddr, unsigned short port)
        {
            //创建sockaddr_in结构体变量
           //创建sockaddr_in结构体变量
            struct sockaddr_in serv_addr;
            if (!IpToAddressV4(ipaddr, port, &serv_addr))
                return -1;
            //将套接字和IP、端口绑定
            return ::sendto(sock_, data, data_len, 0,
                (struct sockaddr*)&serv_addr, sizeof(serv_addr));
           
        }
        
        SockRet Recv(char* data, unsigned int data_len)
        {
            return ::recv(sock_, data, data_len, 0);
        }
        
        SockRet Recvfrom(char* data, unsigned int data_len, \
            const char* ipaddr, unsigned short port)
        {
            //创建sockaddr_in结构体变量
            struct sockaddr_in serv_addr;
#ifdef OS_WINDOWS
            int add_len = sizeof(serv_addr);
#endif

#ifdef OS_LINUX
            socklen_t add_len = sizeof(serv_addr);
#endif
            if (!IpToAddressV4(ipaddr, port, &serv_addr))
                return -1;
            //将套接字和IP、端口绑定
            return ::recvfrom(sock_, data, data_len, 0,
                (struct sockaddr*)&serv_addr, &add_len);
        }
        
        SockRet Close()
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
    public:
        //获取原始socket
        SOCKET GetSocket()
        {
            return sock_;
        }

        //释放socket所有权
        SOCKET Release()
        {
            SOCKET temp = sock_;
            sock_ = INVALID_SOCKET;
            return temp;
        }

        static void Init()
        {
#ifdef OS_WINDOWS
            //初始化函数
            static WsInit g_init;
#endif
        }

        //结构体转换
        static bool IpToAddressV4(const char* ipaddr, unsigned short port
            , struct sockaddr_in* out_addr)
        {
            if (NULL == out_addr)
                return false;
            memset(out_addr, 0, sizeof(*out_addr));  //每个字节都用0填充
            out_addr->sin_family = AF_INET;  //使用IPv4地址
            if (ipaddr)
                out_addr->sin_addr.s_addr = inet_addr(ipaddr);  //具体的IP地址
            else
                out_addr->sin_addr.s_addr = htonl(INADDR_ANY);  //所有ip
            out_addr->sin_port = htons(port);  //端口
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