/*
	实现ssh2.0协议
*/

#ifndef SIM_USE_OPENSSL
#define SIM_SSH_CTX_HPP_
#endif //! SIM_USE_OPENSSL

#ifndef SIM_SSHV2_HPP_
#define SIM_SSHV2_HPP_

//类型
//#include <stdint.h>
#include <cstring>

#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && \
    !defined(LIBRESSL_VERSION_NUMBER)
# define HAVE_OPAQUE_STRUCTS 1
#endif

//基础解析器
#include "BaseParser.hpp"
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#ifndef SIM_SSH_TRANS_PAESER_TYPE
#define SIM_SSH_TRANS_PAESER_TYPE 3
#endif

//多线程情况下运行
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

#define SIM_SSH_VER "SSH-2.0-SIM.SSH.1.1"

/*
 Message ID							Value	Reference
 -----------						-----	---------
 SSH_MSG_DISCONNECT					1		[SSH-TRANS]
 SSH_MSG_IGNORE						2		[SSH-TRANS]
 SSH_MSG_UNIMPLEMENTED				3		[SSH-TRANS]
 SSH_MSG_DEBUG						4		[SSH-TRANS]
 SSH_MSG_SERVICE_REQUEST			5		[SSH-TRANS]
 SSH_MSG_SERVICE_ACCEPT				6		[SSH-TRANS]
 SSH_MSG_KEXINIT					20		[SSH-TRANS]
 SSH_MSG_NEWKEYS					21		[SSH-TRANS]

 SSH_MSG_USERAUTH_REQUEST			50		[SSH-USERAUTH]
 SSH_MSG_USERAUTH_FAILURE			51		[SSH-USERAUTH]
 SSH_MSG_USERAUTH_SUCCESS			52		[SSH-USERAUTH]
 SSH_MSG_USERAUTH_BANNER			53		[SSH-USERAUTH]

 SSH_MSG_GLOBAL_REQUEST				80		[SSH-CONNECT]
 SSH_MSG_REQUEST_SUCCESS			81		[SSH-CONNECT]
 SSH_MSG_REQUEST_FAILURE			82		[SSH-CONNECT]
 SSH_MSG_CHANNEL_OPEN				90		[SSH-CONNECT]
 SSH_MSG_CHANNEL_OPEN_CONFIRMATION	91		[SSH-CONNECT]
 SSH_MSG_CHANNEL_OPEN_FAILURE		92		[SSH-CONNECT]
 SSH_MSG_CHANNEL_WINDOW_ADJUST		93		[SSH-CONNECT]
 SSH_MSG_CHANNEL_DATA				94		[SSH-CONNECT]
 SSH_MSG_CHANNEL_EXTENDED_DATA		95		[SSH-CONNECT]
 SSH_MSG_CHANNEL_EOF				96		[SSH-CONNECT]
 SSH_MSG_CHANNEL_CLOSE				97		[SSH-CONNECT]
 SSH_MSG_CHANNEL_REQUEST			98		[SSH-CONNECT]
 SSH_MSG_CHANNEL_SUCCESS			99		[SSH-CONNECT]
 SSH_MSG_CHANNEL_FAILURE			100		[SSH-CONNECT]
*/

//代替回调
#define SSH_MSG_VERSION						0
#define SSH_MSG_DISCONNECT					1
#define SSH_MSG_IGNORE						2
#define SSH_MSG_UNIMPLEMENTED				3
#define SSH_MSG_DEBUG						4
#define SSH_MSG_SERVICE_REQUEST				5
#define SSH_MSG_SERVICE_ACCEPT				6
#define SSH_MSG_KEXINIT						20
#define SSH_MSG_NEWKEYS						21

#define SSH_MSG_KEXDH_INIT					30
#define SSH_MSG_KEXDH_REPLY					31

#define SSH_MSG_USERAUTH_REQUEST			50
#define SSH_MSG_USERAUTH_FAILURE			51
#define SSH_MSG_USERAUTH_SUCCESS			52
#define SSH_MSG_USERAUTH_BANNER				53

#define SSH_MSG_GLOBAL_REQUEST				80
#define SSH_MSG_REQUEST_SUCCESS				81
#define SSH_MSG_REQUEST_FAILURE				82
#define SSH_MSG_CHANNEL_OPEN				90
#define SSH_MSG_CHANNEL_OPEN_CONFIRMATION	91
#define SSH_MSG_CHANNEL_OPEN_FAILURE		92
#define SSH_MSG_CHANNEL_WINDOW_ADJUST		93
#define SSH_MSG_CHANNEL_DATA				94
#define SSH_MSG_CHANNEL_EXTENDED_DATA		95 
#define SSH_MSG_CHANNEL_EOF					96 
#define SSH_MSG_CHANNEL_CLOSE				97
#define SSH_MSG_CHANNEL_REQUEST				98
#define SH_MSG_CHANNEL_SUCCESS				99
#define SSH_MSG_CHANNEL_FAILURE				100

//1-256
//SSH packets have message numbers in the range 1 to 255
//最大msg消息
#define SSH_MSG_MAX				256
/*
Local extensions:
	192 to 255 Local extensions
255 作为错误回调
*/
#define SSH_MSG_ERR				SSH_MSG_MAX-1

/*
Symbolic Name											reason code
 -------------											-----------
 SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT				1
 SSH_DISCONNECT_PROTOCOL_ERROR							2
 SSH_DISCONNECT_KEY_EXCHANGE_FAILED						3
 SSH_DISCONNECT_RESERVED								4
 SSH_DISCONNECT_MAC_ERROR								5
 SSH_DISCONNECT_COMPRESSION_ERROR						6
 SSH_DISCONNECT_SERVICE_NOT_AVAILABLE					7
 SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED			8
 SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE					9
 SSH_DISCONNECT_CONNECTION_LOST							10
 SSH_DISCONNECT_BY_APPLICATION							11
 SSH_DISCONNECT_TOO_MANY_CONNECTIONS					12
 SSH_DISCONNECT_AUTH_CANCELLED_BY_USER					13
 SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE			14
 SSH_DISCONNECT_ILLEGAL_USER_NAME						15
*/

//	The following ’method name’ values are defined.
//		"publickey" REQUIRED
//		"password" OPTIONAL
//		"hostbased" OPTIONAL
//		"none" NOT RECOMMENDED

#define SSH_AUTH_PUB_KEY	"publickey"
#define SSH_AUTH_PASSWORD	"password"
#define SSH_AUTH_HOST_BASED "hostbased"
#define SSH_AUTH_NONE		"none"

//需要释放内存
char* DumpHex(const unsigned char*hex, unsigned int hex_len)
{
	unsigned int buff_len = hex_len * 8;
	char *buff = new char[buff_len];
	memset(buff, 0, buff_len);
	const int count = 2;
	for (int i = 0; i < hex_len; i += count)
		snprintf(buff + i, buff_len - i * count, "%02X", hex[i]);
	return buff;
}

#ifndef SSH_TRANS_PACKET_MAX_SIZE
//35000
#define SSH_TRANS_PACKET_MAX_SIZE 35000
#endif

namespace sim
{
	class SshTransport;
	
	typedef unsigned char		uint8_t;
	typedef unsigned short		uint16_t;
	typedef unsigned long		uint32_t;
	typedef unsigned long long	uint64_t;

	typedef Str name_list;
	typedef Str mpint;

	//The Secure Shell (SSH) Transport Layer Protocol

	//SSH-protoversion-softwareversion SP comments CR LF
	struct SSHVersion
	{
		Str protoversion;//2.0
		Str softwareversion;
		Str comments;
	};

	/*
		byte SSH_MSG_KEXINIT
		byte[16] cookie (random bytes)
		name-list kex_algorithms
		name-list server_host_key_algorithms
		name-list encryption_algorithms_client_to_server
		name-list encryption_algorithms_server_to_client
		name-list mac_algorithms_client_to_server
		name-list mac_algorithms_server_to_client
		name-list compression_algorithms_client_to_server
		name-list compression_algorithms_server_to_client
		name-list languages_client_to_server
		name-list languages_server_to_client
		boolean first_kex_packet_follows
		uint32 0 (reserved for future extension)
	*/
	struct SSHKexInit
	{
		uint8_t cookie[16];
		name_list kex_algorithms;
		name_list server_host_key_algorithms;
		name_list encryption_algorithms_client_to_server;
		name_list encryption_algorithms_server_to_client;
		name_list mac_algorithms_client_to_server;
		name_list mac_algorithms_server_to_client;
		name_list compression_algorithms_client_to_server;
		name_list compression_algorithms_server_to_client;
		name_list languages_client_to_server;
		name_list languages_server_to_client;
		bool first_kex_packet_follows;
		uint32_t reserved;
	};

	//#define SSH_MSG_KEXDH_INIT					30
	//mpint e
	struct SSHKexDHInit
	{
		Str e;
	};

	//#define SSH_MSG_KEXDH_REPLY					31
	/*
		string server public host key and certificates (K_S)
		mpint f
		string signature of H 总长度+算法名长度+算法名+签名数据长度+签名值。
	*/
	struct SSHKexDHReply
	{
		Str K_S;
		Str f;
		Str S;
	};
	

	//hmac算法 基类
	class SshHMac
	{
	public:
		SshHMac(const Str& name,
		uint8_t mac_len,
		uint8_t key_len,
		const Str& key,
		const EVP_MD*md)
			:name_(name)
			, mac_len_(mac_len)
			, key_len_(key_len)
			, key_(key)
			, md_(md)
		{}
		virtual uint8_t len()const
		{
			return mac_len_;
		}
		virtual uint8_t keylen()const
		{
			return key_len_;
		}
		virtual Str key()const
		{
			return key_;
		}
		virtual Str name()const
		{
			return name_;
		}
		virtual bool set_key(const Str&key)
		{
			if (key.size() != key_len_)
				return false;
			key_ = key;
			return true;
		}
		//计算mac 失败返回""
		virtual bool mac(uint32_t sequence_number, const char*in, uint32_t in_len,
			char*out, uint32_t &out_len)=0;
	protected:
		Str name_;
		uint8_t mac_len_;
		uint8_t key_len_;
		Str key_;
		const EVP_MD*md_;

	};
	//return "hmac-sha1-96,hmac-sha1";
	class SshHMacSha1:public SshHMac
	{
	public:
		SshHMacSha1()
			:SshHMac("hmac-sha1",20,20,"", EVP_sha1())
		{}
		virtual bool mac(uint32_t sequence_number, const char*in, uint32_t in_len,
			char*out, uint32_t &out_len)
		{
			if (out_len < mac_len_)
				return false;
			if (NULL == md_)
				return false;
#ifdef HAVE_OPAQUE_STRUCTS
			HMAC_CTX *ctx = HMAC_CTX_new();
#else
			HMAC_CTX *ctx = new HMAC_CTX();
			HMAC_CTX_init(ctx);
#endif
			HMAC_Init(ctx, key_.c_str(), key_.size(), md_);
			sequence_number = htonl(sequence_number);
			HMAC_Update(ctx, (const unsigned char *)&sequence_number, 4);
			HMAC_Update(ctx, (const unsigned char *)in, in_len);
			HMAC_Final(ctx, (unsigned char *)out, (unsigned int*)&out_len);
#ifdef HAVE_OPAQUE_STRUCTS
			HMAC_CTX_free(ctx);
#else
			HMAC_cleanup(ctx);
			delete ctx;
#endif
			return true;
		}
	};
	class SshHMacSha1_96 :public SshHMacSha1
	{
	public:
		SshHMacSha1_96()
		{
			name_ = "hmac-sha1-96";
			mac_len_ = 96 / 8;
		}
		virtual bool mac(uint32_t sequence_number, const char*in, uint32_t in_len,
			char*out, uint32_t &out_len)
		{
			if (out_len < mac_len_)
				return false;
			if (NULL == md_)
				return false;
			uint32_t len = 20;
			char mac_temp[20] = { 0 };
			if(!SshHMacSha1::mac(sequence_number,in, in_len, mac_temp, len))
				return false;
			out_len = mac_len_;
			memcpy(out, mac_temp, out_len);
			return true;
		}
	};

	//ssh算法上下文
	struct SSHAlgorithmsCtx
	{
		//算法协商阶段决定
		Str kex_algorithms;
		Str server_host_key_algorithms;
		Str	encryption_algorithms_client_to_server;
		Str encryption_algorithms_server_to_client;
		Str	mac_algorithms_client_to_server;
		Str mac_algorithms_server_to_client;
		Str compression_algorithms_client_to_server;
		Str compression_algorithms_server_to_client;
		Str languages_client_to_server;
		Str languages_server_to_client;

		//密钥更新阶段协商完成
		EVP_CIPHER_CTX *evp_ctx_encrypt;
		EVP_CIPHER_CTX *evp_ctx_decrypt;

		//发送的时候
		SshHMac *mac,*check_mac;

		//是否已经发送或者接收报文 new key 标识算法可用。
		bool is_newkeys;

		//V_C, V_S, I_C, I_S, K_S
		Str V_C, V_S, I_C, I_S, K_S;
		Str H, session_id;
		/*
			mpint e, exchange value sent by the client
			mpint f, exchange value sent by the server
			mpint K, the shared secret
		*/
		BIGNUM* e,*x, *f,*y, *K;

		//hostkey 算法
		RSA *rsa;
		DSA *dsa;

		SSHAlgorithmsCtx()
			:evp_ctx_encrypt(NULL)
			, evp_ctx_decrypt(NULL)
			, mac(NULL)
			, check_mac(NULL)
			, is_newkeys(false)
			,e(NULL)
			, x(NULL)
			,f(NULL)
			, y(NULL)
			,K(NULL)
			,rsa(NULL)
			,dsa(NULL)
		{

		}
		//重置状态
		void ReSet()
		{
			if (evp_ctx_encrypt)
			{
				EVP_CIPHER_CTX_cleanup(evp_ctx_encrypt);
				EVP_CIPHER_CTX_free(evp_ctx_encrypt);
				evp_ctx_encrypt = NULL;
			}
			if (evp_ctx_decrypt)
			{
				EVP_CIPHER_CTX_cleanup(evp_ctx_decrypt);
				EVP_CIPHER_CTX_free(evp_ctx_decrypt);
				evp_ctx_decrypt = NULL;
			}
			if (mac)
			{
				delete mac;
				mac = NULL;
			}
			if (check_mac)
			{
				delete check_mac;
				check_mac = NULL;
			}
			kex_algorithms = "";
			server_host_key_algorithms = "";
			encryption_algorithms_client_to_server = "";
			encryption_algorithms_server_to_client = "";
			mac_algorithms_client_to_server = "";
			mac_algorithms_server_to_client = "";
			compression_algorithms_client_to_server = "";
			compression_algorithms_server_to_client = "";
			languages_client_to_server = "";
			languages_server_to_client = "";

			V_C=V_S=I_C=I_S=K_S="";
			H = session_id = "";
			ReleaseBIGNUM(&e);
			ReleaseBIGNUM(&x);
			ReleaseBIGNUM(&f);
			ReleaseBIGNUM(&y);
			ReleaseBIGNUM(&K);
			/*if (e)
			{
				BN_clear_free(e);
				e = NULL;
			}
			if (f)
			{
				BN_clear_free(f);
				f = NULL;
			}
			if (K)
			{
				BN_clear_free(K);
				K = NULL;
			}*/

			if (rsa)
			{
				RSA_free(rsa);
				rsa = NULL;
			}
			if (dsa == NULL)
			{
				DSA_free(dsa);
				dsa = NULL;
			}
		}
		void ReleaseBIGNUM(BIGNUM** pn)
		{
			//EVP_PKEY
			//EVP_PKEY
			if (pn&&*pn)
			{
				BN_clear_free(*pn);
				*pn = NULL;
			}
		}
		~SSHAlgorithmsCtx()
		{
			ReSet();
		}
	};

	//回调
	typedef  void (*SSH_TRANS_HANDLER)(SshTransport*parser,
		const char*payload_data,uint32_t payload_data_len, void*pdata);
	//句柄
	struct SshTransportHandler
	{
		SSH_TRANS_HANDLER func;
		void*pdata;
	};
	enum SshPointType
	{
		SshClient,
		SshServer
	};

	//Public Key Algorithms ssh公钥算法
	enum SshHostKeyType
	{
		SshRsa,
		SshDsa
	};
	
	////io base
	//io数据基类,需要定制这个基类
	//class SshIOOuput
	//{
	//public:
	//	SshIOOuput() {};
	//	virtual ~SshIOOuput()=0;
	//	//数据发送接口，失败返回 false
	//	virtual bool Send(const char*data, uint32_t len)=0;
	//	//关闭io
	//	virtual bool Close() = 0;
	//private:
	//};

	class SshTransport:protected BaseParser
	{
		enum SshTransportStatus
		{
			SshTransportStatus_VersionLF,
			SshTransportStatus_Packet,
			SshTransportStatus_Mac,
		};
		
	public:
		SshTransport(SshPointType sp_type)
			:BaseParser(SIM_SSH_TRANS_PAESER_TYPE)
			, sp_type_(sp_type)
			, status_(SshTransportStatus_VersionLF)
			, packet_lenght_(0)
			, send_sequence_number_(0)
			, recv_sequence_number_(0)
		{
			memset(handlers_, 0, sizeof(handlers_));
		}
		
		void ReSet()
		{
			packet_lenght_ =0;
			status_= SshTransportStatus_VersionLF;

			temp_="";
			//消息缓存
			t_msg_="";

			algo_ctx_.ReSet();

			send_sequence_number_ = 0;
			recv_sequence_number_ = 0;
			//回调也初始化
			//memset(handlers_, 0, sizeof(handlers_));
		}
		
		bool SetHandler(uint8_t msg_code,SSH_TRANS_HANDLER handler, void*pdata)
		{
			if (msg_code >= SSH_MSG_MAX)
				return false;
			handlers_[msg_code].func = handler;
			handlers_[msg_code].pdata = pdata;
			return true;
		}
		
		//数据输入
		virtual bool Parser(const char*data, unsigned int len)
		{
			//并行情况
#ifdef SIM_PARSER_MULTI_THREAD
			sim::AutoMutex lk(parser_lock_);
#endif
			unsigned int offset = 0;
			while (offset < len)
			{
				if (status_ == SshTransportStatus_VersionLF)
				{
					//LF
					if (FindChar(data, len, offset,'\n',temp_))
					{
						temp_ += "\n";
						//回调
						OnHandler(SSH_MSG_VERSION, temp_.c_str(), temp_.size());
						status_ = SshTransportStatus_Packet;
						temp_ = "";
					}
				}
				else if (status_ == SshTransportStatus_Packet|| status_ == SshTransportStatus_Mac)
				{
					/*bool ret = ReadPacket(data, len, offset);
					if (false == ret)
						return ret;*/
					temp_ += Str(data + offset, len - offset);
					offset = len;
					//分割报文
					bool ret = ReadPacket();
					if (false == ret)
						return ret;
				}
				/*else if (status_ == SshTransportStatus_Mac)
				{
					bool has_mac = false;
					if (has_mac)
					{
						if (temp_.size() >= packet_lenght_ + 4 + 20)
						{
							bool ret = OnMessage();
							if (false == ret)
								return ret;
						}
						temp_ += data[offset++];
					}
					else
					{
						bool ret = OnMessage();
						if (false == ret)
							return ret;
					}
				}*/
				else
				{
					return false;
				}
			}
			return true;
		}

		//从文件中加载私钥
		bool LoadPriKey(SshHostKeyType type, const char*filename)
		{
			if (NULL == filename)
			{
				return false;
			}
			//打开
			BIO*pIn = BIO_new_file(filename, "r");
			if (NULL == pIn)
			{
				//打开失败
				return false;
			}

			if (type == SshRsa)
			{
				//释放已经存在的
				if (algo_ctx_.rsa)
				{
					RSA_free(algo_ctx_.rsa);
					algo_ctx_.rsa = NULL;
				}
				algo_ctx_.rsa = PEM_read_bio_RSAPrivateKey(pIn, NULL, NULL, NULL);
				BIO_free_all(pIn);
				//RSA_print_fp(stdout, algo_ctx_.rsa, 0);
				if (NULL == algo_ctx_.rsa)
				{
					return false;
				}
				return true;
			}
			else if (type == SshDsa)
			{
				//释放已经存在的
				if (algo_ctx_.dsa)
				{
					DSA_free(algo_ctx_.dsa);
					algo_ctx_.dsa = NULL;
				}
				algo_ctx_.dsa = PEM_read_bio_DSAPrivateKey(pIn, NULL, NULL, NULL);
				BIO_free_all(pIn);
				//DSA_print_fp(stdout, algo_ctx_.dsa, 0);
				if (NULL == algo_ctx_.dsa)
				{
					return false;
				}
				return true;
			}
			else
			{
				BIO_free_all(pIn);
				return false;
			}
		}
		
		//或者生成私钥 filename !=NULL 写到对应的文件
		bool GeneratePriKey(SshHostKeyType type, const char*filename = NULL)
		{
			BIO* pOut = NULL;
			if (filename)
			{
				//打开
				pOut = BIO_new_file(filename, "w");
				if (NULL == pOut)
				{
					//打开失败
					return false;
				}
			}
			
			if (type == SshRsa)
			{
				unsigned long  e = RSA_3;
				//释放已经存在的
				if (algo_ctx_.rsa)
				{
					RSA_free(algo_ctx_.rsa);
					algo_ctx_.rsa = NULL;
				}
				algo_ctx_.rsa = RSA_generate_key(1024, e, NULL, NULL);
				if (NULL == algo_ctx_.rsa)
				{
					if(pOut)
						BIO_free_all(pOut);
					return false;
				}
				if (pOut)
				{
					PEM_write_bio_RSAPrivateKey(pOut, algo_ctx_.rsa, NULL, NULL, 0, NULL, NULL);
					BIO_free_all(pOut);
				}
				return true;
			}
			else if (type == SshDsa)
			{
				//释放已经存在的
				if (algo_ctx_.dsa)
				{
					DSA_free(algo_ctx_.dsa);
					algo_ctx_.dsa = NULL;
				}
				
				algo_ctx_.dsa = DSA_new();
				//char seed[20];
				int ret = DSA_generate_parameters_ex(algo_ctx_.dsa, 1024  , NULL, 0, NULL, NULL, NULL);
				if (ret != 1)
				{
					if (pOut)
						BIO_free_all(pOut);
					DSA_free(algo_ctx_.dsa);
					return false;

				}
				/* 生成密钥 */
				ret = DSA_generate_key(algo_ctx_.dsa);
				if (ret != 1)
				{
					if (pOut)
						BIO_free_all(pOut);
					DSA_free(algo_ctx_.dsa);
					return false;
				}

				if (pOut)
				{
					PEM_write_bio_DSAPrivateKey(pOut, algo_ctx_.dsa, NULL, NULL, 0, NULL, NULL);
					BIO_free_all(pOut);
				}
				return true;
			}
			else
			{
				if (pOut)
					BIO_free_all(pOut);
				return false;
			}
		}

		//启动握手流程
		//bool StartHandShake(SshIOOuput*output);
	public:
		//Protocol Version Exchange
		//SSH-protoversion-softwareversion SP comments CR LF
		Str PrintProtocolVersion()
		{
			//缓存报文
			if (sp_type_ == SshClient)
			{
				algo_ctx_.V_C = SIM_SSH_VER;
			}
			else
			{
				algo_ctx_.V_S = SIM_SSH_VER;
			}

			return SIM_SSH_VER "\r\n";
		}

		//SSH-protoversion-softwareversion SP comments CR LF
		Str PrintVersion(const SSHVersion&ver)
		{
			Str res = "SSH-";
			if (ver.protoversion.size() == 0)
			{
				res+="2.0-";
			}
			else
			{
				res += ver.protoversion+"-";
			}
			if (ver.softwareversion.size() == 0)
			{
				return "";//不可为空
			}
			else
			{
				res += ver.protoversion;
			}
			if (ver.comments.size() != 0)
			{
				res += " " + ver.comments;
			}
			
			//缓存报文
			if (sp_type_ == SshClient)
			{
				algo_ctx_.V_C = res;
			}
			else
			{
				algo_ctx_.V_S = res;
			}

			return res + "\r\n";
		}
		bool ParserVersion(const char*payload_data, uint32_t payload_data_len, SSHVersion&ver)
		{
			
			enum ParserVerSt
			{
				ParserVerSt_SSH,
				ParserVerSt_protoversion,
				ParserVerSt_softwareversion,
				ParserVerSt_comments,
				ParserVerSt_end,
			};
			ParserVerSt st = ParserVerSt_SSH;
			uint32_t offset = 0;

			while (offset < payload_data_len)
			{
				if (ParserVerSt_SSH == st)
				{
					if (payload_data[offset] == '-')
						st = ParserVerSt_protoversion;
					++offset;
				}
				else if (ParserVerSt_protoversion == st)
				{
					if (payload_data[offset] == '-')
					{
						st = ParserVerSt_softwareversion;
						++offset;
					}
					else
					{
						ver.protoversion += payload_data[offset++];
					}
				}
				else if (ParserVerSt_softwareversion == st)
				{
					if (payload_data[offset] == '\r' || payload_data[offset] == '\n')
					{
						st = ParserVerSt_end;
						break;
					}
					else if (payload_data[offset] == ' ')
					{
						st = ParserVerSt_comments;
						++offset;
					}
					else
					{
						ver.softwareversion += payload_data[offset++];
					}
				}
				else if (ParserVerSt_comments == st)
				{
					if (payload_data[offset] == '\r' || payload_data[offset] == '\n')
					{
						st = ParserVerSt_end;
						break;
					}
					else
					{
						ver.comments += payload_data[offset++];
					}
				}
				else
				{
					return false;
				}
			}
			if (st != ParserVerSt_end)
				return false;
			return true;
		}

		//打印默认的算法列表
		Str PrintKexInit()
		{
			SSHKexInit kex_init;
			if (!OpensslGenerateRandArray(kex_init.cookie, sizeof(kex_init.cookie)))
			{
				return "";
			}
			kex_init.first_kex_packet_follows = false;
			kex_init.reserved = 0;
			kex_init.kex_algorithms = GetLocalKexAlgorithms();
			kex_init.server_host_key_algorithms = GetLocalServerHostKeyAlgorithms();
			kex_init.encryption_algorithms_client_to_server = GetLocalEncryptionAlgorithmsClientToServer();
			kex_init.encryption_algorithms_server_to_client = GetLocalEncryptionAlgorithmsServerToClient();
			kex_init.compression_algorithms_client_to_server = GetLocalCompressionAlgorithmsClientToServer();
			kex_init.compression_algorithms_server_to_client = GetLocalCompressionAlgorithmsServerToClient();
			kex_init.mac_algorithms_server_to_client = GetLocalMacAlgorithmsServerToClient();
			kex_init.mac_algorithms_client_to_server = GetLocalMacAlgorithmsClientToServer();
			kex_init.languages_client_to_server = GetLocalLanguagesClientToServer();
			kex_init.languages_server_to_client = GetLocalLanguagesServerToClient();
		
			return PrintKexInit(kex_init);
		}
		Str PrintKexInit(const SSHKexInit&kex_init)
		{
			Str payload_data= Str((const char*)kex_init.cookie,16);
			/*
			name-list kex_algorithms
			name-list server_host_key_algorithms
			name-list encryption_algorithms_client_to_server
			name-list encryption_algorithms_server_to_client
			name-list mac_algorithms_client_to_server
			name-list mac_algorithms_server_to_client
			name-list compression_algorithms_client_to_server
			name-list compression_algorithms_server_to_client
			name-list languages_client_to_server
			name-list languages_server_to_client
			*/
			payload_data += PrintNameList(kex_init.kex_algorithms);
			payload_data += PrintNameList(kex_init.server_host_key_algorithms);
			payload_data += PrintNameList(kex_init.encryption_algorithms_client_to_server);
			payload_data += PrintNameList(kex_init.encryption_algorithms_server_to_client);
			payload_data += PrintNameList(kex_init.mac_algorithms_client_to_server);
			payload_data += PrintNameList(kex_init.mac_algorithms_server_to_client);
			payload_data += PrintNameList(kex_init.compression_algorithms_client_to_server);
			payload_data += PrintNameList(kex_init.compression_algorithms_server_to_client);
			payload_data += PrintNameList(kex_init.languages_client_to_server);
			payload_data += PrintNameList(kex_init.languages_server_to_client);
			/*
			boolean first_kex_packet_follows
			uint32 0 (reserved for future extension)
			*/
			payload_data += (const char)kex_init.first_kex_packet_follows;
			payload_data += Str((const char*)&kex_init.reserved,4);

			return PrintPacket(SSH_MSG_KEXINIT, payload_data.c_str(), payload_data.size());
		}
		bool ParserKexInit(const char*payload_data, uint32_t payload_data_len, SSHKexInit&kex_init)
		{
			uint32_t offset = 0;
			if (payload_data_len < 16)
				return false;//长度不够 

			memcpy(kex_init.cookie, payload_data, 16);
			offset += 16;

			/*
			name-list kex_algorithms
			name-list server_host_key_algorithms
			name-list encryption_algorithms_client_to_server
			name-list encryption_algorithms_server_to_client
			name-list mac_algorithms_client_to_server
			name-list mac_algorithms_server_to_client
			name-list compression_algorithms_client_to_server
			name-list compression_algorithms_server_to_client
			name-list languages_client_to_server
			name-list languages_server_to_client
			*/
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.kex_algorithms))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.server_host_key_algorithms))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.encryption_algorithms_client_to_server))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.encryption_algorithms_server_to_client))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.mac_algorithms_client_to_server))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.mac_algorithms_server_to_client))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.compression_algorithms_client_to_server))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.compression_algorithms_server_to_client))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.languages_client_to_server))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, kex_init.languages_server_to_client))
				return false;
			/*
			boolean first_kex_packet_follows
			uint32 0 (reserved for future extension)
			*/
			if (payload_data_len < offset+1)
				return false;//长度不够 
			kex_init.first_kex_packet_follows = payload_data[offset++];

			if (payload_data_len < offset + 4)
				return false;//长度不够 
			kex_init.reserved =*(uint32_t*) payload_data +offset;
			return true;
		}

		//SSHKexDHInit
		Str PrintKexDHInit()
		{
			//如果不存在就创建
			if (NULL == algo_ctx_.e)
				algo_ctx_.e = BN_new();
			
			if (NULL == algo_ctx_.x)
				algo_ctx_.x = BN_new();

			if (!GenerateEF(algo_ctx_.x,algo_ctx_.e))
			{
				//生成失败
				return "";
			}
			SSHKexDHInit dh_init;
			dh_init.e = BN2Str(algo_ctx_.e);
			if (dh_init.e.size() == 0)
				return "";
			
			/*BIGNUM *e1 = Str2BN(dh_init.e);
			int ret = BN_cmp(algo_ctx_.e, e1);
			printf("e 0x%s\ne1 0x%s\n", BN_bn2hex(algo_ctx_.e), BN_bn2hex(e1));*/
			return PrintKexDHInit(dh_init);
		}
		Str PrintKexDHInit(const SSHKexDHInit&dh_init)
		{
			Str payload_data = PrintNameList(dh_init.e);
			if (payload_data.size() == 0)
				return "";
			return PrintPacket(SSH_MSG_KEXDH_INIT, payload_data.c_str(), payload_data.size());
		}
		bool ParserKexDHInit(const char*payload_data, uint32_t payload_data_len, SSHKexDHInit&dh_init)
		{
			uint32_t offset = 0;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_init.e))
				return false;
			return true;
		}

		//SSHKexDHReply
		Str PrintKexDHReply(const SSHKexDHReply&dh_reply)
		{
			/*
			string server public host key and certificates (K_S)
			mpint f
			string signature of S
			*/
			Str payload_data = PrintNameList(dh_reply.K_S);
			payload_data += PrintNameList(dh_reply.f);
			payload_data += PrintNameList(dh_reply.S);
			return PrintPacket(SSH_MSG_KEXDH_REPLY, payload_data.c_str(), payload_data.size());
		}
		bool ParserKexDHReply(const char*payload_data, uint32_t payload_data_len, SSHKexDHReply&dh_reply)
		{
			uint32_t offset = 0;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.K_S))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.f))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.S))
				return false;
			return true;
		}

		Str PrintNewKeys()
		{
			return PrintPacket(SSH_MSG_NEWKEYS, NULL, 0);
		}

		Str PrintPacket(uint8_t message_code,const char*payload_data, uint32_t payload_data_len)
		{
			//包过大
			if (payload_data_len + 1 +1 + 4> SSH_TRANS_PACKET_MAX_SIZE)
				return "";
			
			char tmp[SSH_TRANS_PACKET_MAX_SIZE] = {0};

			//playload
			uint32_t raw_payload_len = payload_data_len + 1;
			char *raw_payload = new char [raw_payload_len] ;
			memcpy(raw_payload, &message_code, 1);
			if(payload_data&&payload_data_len)
				memcpy(raw_payload +1, payload_data, payload_data_len);
			if (SSH_MSG_KEXINIT == message_code)
			{
				//缓存报文
				if (sp_type_ == SshClient)
				{
					algo_ctx_.I_C = Str(raw_payload, raw_payload_len);
				}
				else
				{
					algo_ctx_.I_S = Str(raw_payload, raw_payload_len);
				}
			}
			uint32_t payload_len = SSH_TRANS_PACKET_MAX_SIZE-5;
			//压缩
			if (!Compress(raw_payload, raw_payload_len, tmp + 5, payload_len))
			{
				delete[]raw_payload;
				return "";
			}
			delete[]raw_payload;

			/*
			Note that the length of the concatenation of ’packet_length’,
			padding_length’, ’payload’, and ’random padding’ MUST be a multiple
			of the cipher block size or 8, whichever is larger.
			*/
			/* Plain math: (4 + 1 + packet_length + padding_length) % blocksize == 0 */
			// packagelen + padding_length+message_code+playloaddata
			uint32_t total = 4+1+ payload_len;
			uint8_t block_size = 8;
			if (algo_ctx_.evp_ctx_encrypt)
			{
				//block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_encrypt);
				block_size = EVP_CIPHER_CTX_iv_length(algo_ctx_.evp_ctx_encrypt);
			}
			uint8_t padding_len = block_size - total % block_size;
			if (padding_len < 4)
				padding_len += block_size;
			if (4+1+ payload_len + padding_len > SSH_TRANS_PACKET_MAX_SIZE)
				return "";
			unsigned char *padding = (unsigned char *)tmp + 1 + 4 + payload_len;
			if (!OpensslGenerateRandArray(padding, padding_len))
			{
				return "";
			}

			//填充长度
			//padding feild（1）+ payload_len + padding_len
			uint32_t packet_lenght = 1 + payload_len + padding_len;
			packet_lenght = htonl(packet_lenght);
			memcpy(tmp, &packet_lenght, 4);
			memcpy(tmp +4, &padding_len, 1);

			if (algo_ctx_.evp_ctx_encrypt)
			{
				//加密
				//需要加密的报文长度
				char msg[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
				uint32_t msg_len = 0;
				uint32_t encrypt_size = 4 + 1 + payload_len + padding_len;
				uint32_t out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
				for (int i = 0; i < encrypt_size; i += block_size)//保证一次完成
				{
					out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
					if (!Encrypt(tmp + i, block_size, msg + msg_len, out_len))
						return "";
					msg_len += out_len;
				}
				if (algo_ctx_.mac)
				{
					out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
					//计算mac
					if (!Mac(send_sequence_number_, tmp, encrypt_size, msg + msg_len, out_len))
					{
						return "";
					}
					msg_len += out_len;
				}
				++send_sequence_number_;
				return Str(msg, msg_len);
			}
			else
			{
				++send_sequence_number_;
				return Str(tmp, 4 + 1 + payload_len + padding_len);
			}
		}

		//SERVICE_REQUEST
		Str PrintServiceRequest(const Str &service)
		{
			/*
			byte SSH_MSG_SERVICE_REQUEST
			string service name
			*/
			Str payload_data = PrintNameList(service);
			return PrintPacket(SSH_MSG_SERVICE_REQUEST, payload_data.c_str(), payload_data.size());
		}
		Str PrintServiceAccept(const Str &service)
		{
			/*
			byte SSH_MSG_SERVICE_ACCEPT
			string service name
			*/
			Str payload_data = PrintNameList(service);
			return PrintPacket(SSH_MSG_SERVICE_ACCEPT, payload_data.c_str(), payload_data.size());
		}
		bool ParserServiceRequest(const char*payload_data, uint32_t payload_data_len, Str &service)
		{
			uint32_t offset = 0;
			if (!ParserNameList(payload_data, payload_data_len, offset, service))
				return false;
			return true;
		}
		bool ParserServiceAccept(const char*payload_data, uint32_t payload_data_len, Str &service)
		{
			return ParserServiceRequest(payload_data, payload_data_len, service);
		}
	public:
		//版本协商
		bool VersionExchange(const SSHVersion&ver)
		{
			if (ver.protoversion == "2.0")
				return true;
			else
				return false;
		}

		//算法协商
		bool KexInit(const SSHKexInit&other_kex_init)
		{
			if (!AgreeKexAlgorithms(other_kex_init.kex_algorithms))
				return false;
			if (!AgreeServerHostKeyAlgorithms(other_kex_init.server_host_key_algorithms))
				return false;
			if (!AgreeEncryptionAlgorithmsClientToServerAlgorithms(other_kex_init.encryption_algorithms_client_to_server))
				return false;
			if (!AgreeEncryptionAlgorithmsServerToClientAlgorithms(other_kex_init.encryption_algorithms_server_to_client))
				return false;
			if (!AgreeMacAlgorithmsClientToServerAlgorithms(other_kex_init.mac_algorithms_client_to_server))
				return false;
			if (!AgreeMacAlgorithmsServerToClientAlgorithms(other_kex_init.mac_algorithms_server_to_client))
				return false;
			if (!AgreeCompressionAlgorithmsClientToServerAlgorithms(other_kex_init.compression_algorithms_client_to_server))
				return false;
			if (!AgreeCompressionAlgorithmsServerToClientAlgorithms(other_kex_init.compression_algorithms_server_to_client))
				return false;
			if (!AgreeLanguagesClientToServerAlgorithms(other_kex_init.languages_client_to_server))
				return false;
			if (!AgreeLanguagesServerToClientAlgorithms(other_kex_init.languages_server_to_client))
				return false;
			return true;
		}

		//密钥协商
		//服务端
		//dh_init 接收到的dh 初始化报文
		//dh_reply 发送的回复报文
		bool KeyExchange(const SSHKexDHInit&dh_init, SSHKexDHReply &dh_reply)
		{
			/*
				服务器也随机选取一个整数 y (0 < y < q)，计算 f = g^y mod p 。
				然后获得客户端的 e ，计算 K = e^y mod p 。
				计算哈希值 H ，参与哈希运算的内容有 V_C, V_S, I_C, I_S, K_S, e, f, K 。
				然后用服务器主机密钥的私钥对这个哈希值做数字签名，签名值 s 。把 K_S, f, s 发送给客户端。
			*/

			//如果不存在就创建
			if (NULL == algo_ctx_.f)
				algo_ctx_.f = BN_new();

			if (NULL == algo_ctx_.y)
				algo_ctx_.y = BN_new();

			if (!GenerateEF(algo_ctx_.y, algo_ctx_.f))
			{
				//生成失败
				return false;
			}

			algo_ctx_.e = Str2BN(dh_init.e, algo_ctx_.e);
			if (NULL == algo_ctx_.e)
			{
				return false;
			}
			//计算 K = e^y mod p 
			BIGNUM *p = BN_new();
			BIGNUM *g = BN_new();
			if (!GetKexPG(algo_ctx_.kex_algorithms, p, g))
			{
				algo_ctx_.ReleaseBIGNUM(&p);
				algo_ctx_.ReleaseBIGNUM(&g);
				return false;
			}
			if (NULL == algo_ctx_.K)
				algo_ctx_.K = BN_new();
			BN_CTX *ctx = BN_CTX_new();
			//int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p,const BIGNUM *m, BN_CTX *ctx); 
			//r = pow(a, p) % M
			BN_mod_exp(algo_ctx_.K, algo_ctx_.e, algo_ctx_.y, p, ctx);
			
			/*printf("server p=0x%s\n", BN_bn2hex(p));
			printf("server e=0x%s\n", BN_bn2hex(algo_ctx_.e));
			printf("server f=0x%s\n", BN_bn2hex(algo_ctx_.f));
			printf("server k=0x%s\n", BN_bn2hex(algo_ctx_.K));*/
			
			algo_ctx_.ReleaseBIGNUM(&p);
			algo_ctx_.ReleaseBIGNUM(&g);
			//algo_ctx_.ReleaseBIGNUM(&x);
			BN_CTX_free(ctx);

			algo_ctx_.K_S = MakeHostKey();
			if (algo_ctx_.K_S.size() == 0)
			{
				return false;
			}

			//H = hash(V_C || V_S || I_C || I_S || K_S|| e || f || K)
			algo_ctx_.H = MakeH();
			if (algo_ctx_.H.size() == 0)
				return false;
			if (algo_ctx_.session_id.size() == 0)
				algo_ctx_.session_id = algo_ctx_.H;

			//签名
			if (!SignDHReply(algo_ctx_.H, dh_reply))
			{
				return false;
			}
			dh_reply.f = BN2Str(algo_ctx_.f);
			
			dh_reply.K_S = algo_ctx_.K_S;
			return true;
		}
		//客户端
		bool KeyExchange(const SSHKexDHReply &dh_reply)
		{
			if (!InitPubKey(dh_reply))
			{
				return false;
			}

			//K = f ^ x mod p
			BIGNUM *p = BN_new();
			BIGNUM *g = BN_new();
			if (!GetKexPG(algo_ctx_.kex_algorithms, p, g))
			{
				algo_ctx_.ReleaseBIGNUM(&p);
				algo_ctx_.ReleaseBIGNUM(&g);
				return false;
			}
			if (NULL == algo_ctx_.K)
				algo_ctx_.K = BN_new();
			algo_ctx_.f = Str2BN(dh_reply.f, algo_ctx_.f);
			BN_CTX *ctx = BN_CTX_new();
			//int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p,const BIGNUM *m, BN_CTX *ctx); 
			//r = pow(a, p) % M
			BN_mod_exp(algo_ctx_.K, algo_ctx_.f, algo_ctx_.x, p, ctx);
			algo_ctx_.ReleaseBIGNUM(&p);
			algo_ctx_.ReleaseBIGNUM(&g);
			//algo_ctx_.ReleaseBIGNUM(&x);
			BN_CTX_free(ctx);

			//H = hash(V_C || V_S || I_C || I_S || K_S|| e || f || K)
			algo_ctx_.H = MakeH();
			if (algo_ctx_.H.size() == 0)
				return false;
			if (algo_ctx_.session_id.size() == 0)
				algo_ctx_.session_id = algo_ctx_.H;
			
			//验证签名
			//verify(Str &H, Str &S);
			if (!VerifyDHReply(algo_ctx_.H, dh_reply))
				return false;
			return true;
		}

		//计算密钥，返回报文
		bool NewKeys()
		{
			//检查数据
			if (NULL == algo_ctx_.K||0==algo_ctx_.H.size()|| 0 == algo_ctx_.session_id.size())
				return false;
			Str s_k = PrintNameList(BN2Str(algo_ctx_.K));
			const EVP_CIPHER *e_c2s = GetEncryptionCipher(algo_ctx_.encryption_algorithms_client_to_server);
			const EVP_CIPHER *e_s2c = GetEncryptionCipher(algo_ctx_.encryption_algorithms_server_to_client);
			SshHMac*mac_c2s = NewHMac(algo_ctx_.mac_algorithms_client_to_server);
			SshHMac*mac_s2c = NewHMac(algo_ctx_.mac_algorithms_server_to_client);

			if (NULL == e_c2s || NULL == e_s2c || NULL == mac_c2s || NULL == mac_s2c)
			{
				if (mac_c2s)
					delete mac_c2s;
				if (mac_s2c)
					delete mac_s2c;
				return false;
			}
			
			/*
			 Initial IV client to server: HASH(K || H || "A" || session_id)
			(Here K is encoded as mpint and "A" as byte and session_id as raw
			data. "A" means the single character A, ASCII 65).
			o Initial IV server to client: HASH(K || H || "B" || session_id)
			o Encryption key client to server: HASH(K || H || "C" || session_id)
			o Encryption key server to client: HASH(K || H || "D" || session_id)
			o Integrity key client to server: HASH(K || H || "E" || session_id)
			o Integrity key server to client: HASH(K || H || "F" || session_id)
			*/
			//计算密钥
			Str iv_c2s = MakeKey(s_k, algo_ctx_.H, 'A', algo_ctx_.session_id, EVP_CIPHER_iv_length(e_c2s));
			Str iv_s2c = MakeKey(s_k, algo_ctx_.H, 'B', algo_ctx_.session_id, EVP_CIPHER_iv_length(e_s2c));
			
			Str key_c2s = MakeKey(s_k, algo_ctx_.H, 'C', algo_ctx_.session_id, EVP_CIPHER_key_length(e_c2s));
			Str key_s2c = MakeKey(s_k, algo_ctx_.H, 'D', algo_ctx_.session_id, EVP_CIPHER_key_length(e_s2c));

			Str mac_key_c2s = MakeKey(s_k, algo_ctx_.H, 'E', algo_ctx_.session_id, mac_c2s->keylen());
			
			Str mac_key_s2c = MakeKey(s_k, algo_ctx_.H, 'F', algo_ctx_.session_id, mac_c2s->keylen());

			/*printf("client iv A:%s\n", DumpHex((const unsigned char*)iv_c2s.c_str(), iv_c2s.size()));
			printf("server iv B:%s\n", DumpHex((const unsigned char*)iv_s2c.c_str(), iv_s2c.size()));
			printf("client key C:%s\n", DumpHex((const unsigned char*)key_c2s.c_str(), key_c2s.size()));
			printf("server key D:%s\n", DumpHex((const unsigned char*)key_s2c.c_str(), key_s2c.size()));
			printf("client mac E:%s\n", DumpHex((const unsigned char*)mac_key_c2s.c_str(), mac_key_c2s.size()));
			printf("server mac F:%s\n", DumpHex((const unsigned char*)mac_key_s2c.c_str(), mac_key_s2c.size()));*/

			if (0 == iv_c2s.size() || 0 == iv_s2c.size()
				|| 0 == key_c2s.size() || 0 == key_s2c.size()
				|| 0 == mac_key_c2s.size() || 0 == mac_key_s2c.size())
			{
				if (mac_c2s)
					delete mac_c2s;
				if (mac_s2c)
					delete mac_s2c;
				return false;
			}

			mac_c2s->set_key(mac_key_c2s);
			mac_s2c->set_key(mac_key_s2c);

			//初始化
			if (SshClient == sp_type_)
			{
				//client to server 加密 反之 解密
				//已存在的释放
				if (algo_ctx_.evp_ctx_encrypt)
				{
					EVP_CIPHER_CTX_free(algo_ctx_.evp_ctx_encrypt);
					algo_ctx_.evp_ctx_encrypt = NULL;
				}
				algo_ctx_.evp_ctx_encrypt = EVP_CIPHER_CTX_new();
				EVP_CIPHER_CTX_cleanup(algo_ctx_.evp_ctx_encrypt);
				EVP_CipherInit(algo_ctx_.evp_ctx_encrypt, e_c2s,
					(const unsigned char*)key_c2s.c_str(), (const unsigned char*)iv_c2s.c_str(), 1);

				if (algo_ctx_.evp_ctx_decrypt)
				{
					EVP_CIPHER_CTX_free(algo_ctx_.evp_ctx_decrypt);
					algo_ctx_.evp_ctx_decrypt = NULL;
				}
				algo_ctx_.evp_ctx_decrypt = EVP_CIPHER_CTX_new();
				EVP_CIPHER_CTX_cleanup(algo_ctx_.evp_ctx_decrypt);
				EVP_CipherInit(algo_ctx_.evp_ctx_decrypt, e_s2c,
					(const unsigned char*)key_s2c.c_str(), (const unsigned char*)iv_s2c.c_str(), 0);

				if (algo_ctx_.mac)
					delete algo_ctx_.mac;
				algo_ctx_.mac = mac_c2s;

				if (algo_ctx_.check_mac)
					delete algo_ctx_.check_mac;
				algo_ctx_.check_mac = mac_s2c;
				return true;

			}
			else if (SshServer == sp_type_)
			{
				// server to server 加密 反之 解密
				//已存在的释放
				if (algo_ctx_.evp_ctx_encrypt)
				{
					EVP_CIPHER_CTX_free(algo_ctx_.evp_ctx_encrypt);
					algo_ctx_.evp_ctx_encrypt = NULL;
				}
				algo_ctx_.evp_ctx_encrypt = EVP_CIPHER_CTX_new();
				EVP_CIPHER_CTX_cleanup(algo_ctx_.evp_ctx_encrypt);
				EVP_CipherInit(algo_ctx_.evp_ctx_encrypt, e_s2c,
					(const unsigned char*)key_s2c.c_str(), (const unsigned char*)iv_s2c.c_str(), 1);

				if (algo_ctx_.evp_ctx_decrypt)
				{
					EVP_CIPHER_CTX_free(algo_ctx_.evp_ctx_decrypt);
					algo_ctx_.evp_ctx_decrypt = NULL;
				}
				algo_ctx_.evp_ctx_decrypt = EVP_CIPHER_CTX_new();
				EVP_CIPHER_CTX_cleanup(algo_ctx_.evp_ctx_decrypt);
				EVP_CipherInit(algo_ctx_.evp_ctx_decrypt, e_c2s,
					(const unsigned char*)key_c2s.c_str(), (const unsigned char*)iv_c2s.c_str(), 0);

				if (algo_ctx_.mac)
					delete algo_ctx_.mac;
				algo_ctx_.mac = mac_s2c;

				if (algo_ctx_.check_mac)
					delete algo_ctx_.check_mac;
				algo_ctx_.check_mac = mac_c2s;
				return true;
			}
			else
			{
				if (mac_c2s)
					delete mac_c2s;
				if (mac_s2c)
					delete mac_s2c;
				return false;
			}
		}

	public:
		Str PrintNameList(const name_list& list)
		{
			uint32_t size = htonl(list.size());

			Str res = Str((const char*)&size, 4);
			if (list.size() != 0)
				res += list;
			return res;
		}
		
		bool ParserNameList(const char*payload, uint32_t payload_len, uint32_t &offset, name_list&list)
		{
			//剩余的字节数
			uint32_t has_bytes = payload_len - offset;
			//必须有四字节长度
			if (has_bytes < 4)
				return false;
			uint32_t list_len = ntohl(*(unsigned long*)(payload + offset));
			//长度不足
			if (has_bytes < list_len+4)
				return false;
			list = name_list(payload + offset + 4, list_len);
			offset += list_len + 4;
			return true;
		}

		//大数转换为字符串
		Str BN2Str(const BIGNUM *n)
		{
			if (NULL == n)
				return "";

			/*int bytes_num = BN_num_bytes(n);
			unsigned char*buffer = new unsigned char[bytes_num];
			int ret = BN_bn2bin(n, buffer);
			if (ret <0)
			{
				printf("BN_bn2bin fail!ret=%d\n", ret);
				return "";
			}
			Str res((char*)buffer, bytes_num);
			delete[]buffer;
			return res;*/
			int value_len = BN_num_bytes(n)+1;
			if (BN_num_bits(n) % 8) {
				/* don't need leading 00 */
				value_len--;
			}
			unsigned char*value = new unsigned char[value_len];
			
			if (BN_num_bits(n) % 8) {
				BN_bn2bin(n, value);
			}
			else {
				value[0] = 0;
				BN_bn2bin(n, value + 1);;
			}
			Str res((char*)value, value_len);
			delete[]value;
			return res;
		}

		//子串转换为大数
		BIGNUM * Str2BN(Str m, BIGNUM *n=NULL)
		{
			if (m.size() == 0)
				return NULL;
			if(NULL == n)
				n = BN_new();
			if (NULL == n)
				return NULL;
			BN_bin2bn((const unsigned char*)m.c_str(), m.size(), n);
			return n;
		}

	protected:

		/*bool ReadPacket(const char*data, unsigned int len, unsigned int &offset)
		{
			if (!algo_ctx_.evp_ctx_decrypt)
			{
				while (offset < len)
				{
					bool ret = SplitPacket(data + offset, 1);
					if (false == ret|| status_ == SshTransportStatus_Mac)
						return ret;
					++offset;
				}

			}
			else
			{
				uint8_t block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_decrypt);
				return false;
			}
		}*/

		bool SplitPacket(const char*data, unsigned int len, uint32_t &offset)
		{
			while (offset < len)
			{
				if (t_msg_.size() > SSH_TRANS_PACKET_MAX_SIZE)
					return false;

				if (packet_lenght_ == 0)
				{
					while (offset < len)
					{
						if (t_msg_.size() >= 4)
						{
							packet_lenght_ = ntohl(*((uint32_t*)(t_msg_.c_str())));
							break;
						}
						t_msg_ += data[offset++];
					}
				}
				else
				{
					uint32_t need_bytes = (packet_lenght_ + 4) - t_msg_.size();
					if (need_bytes == 0)
					{
						/*bool ret = OnMessage();
						if (false == ret)
							return false;*/
						status_ = SshTransportStatus_Mac;
						break;
					}
					else
					{
						uint32_t has_bytes = len - offset;
						uint32_t copy_bytes = need_bytes>has_bytes? has_bytes: need_bytes;
						t_msg_ += Str(data + offset, copy_bytes);
						offset += copy_bytes;
						need_bytes = (packet_lenght_ + 4) - t_msg_.size();
						if (need_bytes == 0)
						{
							/*bool ret = OnMessage();
							if (false == ret)
								return false;*/
							status_ = SshTransportStatus_Mac;
							break;
						}
					}
				}
			}
			if (status_ == SshTransportStatus_Mac)
			{
				if (algo_ctx_.check_mac == NULL)
				{
					bool ret = OnMessage();
					if (false == ret)
						return ret;
					status_ = SshTransportStatus_Packet;
				}
			}
			/*if(offset>=len)
				return true;
			return false;*/
			return true;
		}

		bool ReadPacket()
		{
			uint32_t offset = 0;
			while (offset < temp_.size())
			{
				if (status_ == SshTransportStatus_Packet)
				{
					if (algo_ctx_.evp_ctx_decrypt)
					{
						//uint8_t block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_decrypt);
						//EVP_CIPHER_CTX_iv_length
						uint8_t block_size = EVP_CIPHER_CTX_iv_length(algo_ctx_.evp_ctx_decrypt);
						const uint32_t decrypt_buff_size = 1024;
						char decrypt_buff[decrypt_buff_size] = { 0 };
						for (; offset < temp_.size(); offset += block_size)
						{
							uint32_t out_len = decrypt_buff_size;
							if (!Decrypt(temp_.c_str() + offset, block_size, decrypt_buff, out_len))
							{
								return false;
							}
							if (out_len > 0)
							{
								uint32_t s_offset = 0;
								bool ret = SplitPacket(decrypt_buff, out_len, s_offset);
								if (false == ret)
								{
									return false;
								}
								if (s_offset != out_len)
								{
									return false;
								}
								if (status_ == SshTransportStatus_Mac)
								{
									offset += block_size;
									break;
								}
							}
						}
						if (offset > temp_.size())
							offset -= block_size;//多了
					}
					else
					{
						bool ret = SplitPacket(temp_.c_str(), temp_.size(), offset);
						if (false == ret)
							return false;
					}
				}
				else if (status_ == SshTransportStatus_Mac)
				{
					if (algo_ctx_.check_mac)
					{
						t_msg_ += temp_[offset++];
						if (t_msg_.size() >= packet_lenght_ + 4 + algo_ctx_.check_mac->len())
						{
							bool ret = OnMessage();
							if (false == ret)
								return ret;
						}
						
					}
					else
					{
						bool ret = OnMessage();
						if (false == ret)
							return ret;
					}
				}
				else
				{
					return false;
				}
			}
			if (offset >= temp_.size())
				temp_ = "";
			else
				temp_ = temp_.substr(offset);
			return true;
		}

		void OnHandler(uint8_t message_code,
			const char*payload_data, uint32_t payload_data_len)
		{
			if (SSH_MSG_VERSION == message_code)
			{
				//去除\r\n末尾
				Str ver;
				for(int i=0;i< payload_data_len;++i)
				{
					if (payload_data[i] == '\r' || payload_data[i] == '\n')
						break;
					ver += payload_data[i];
				}
				//缓存报文 客户端接收到的是服务端报文
				if (sp_type_ == SshClient)
				{
					algo_ctx_.V_S = ver;
				}
				else
				{
					algo_ctx_.V_C = ver;
				}
			}
			else if (SSH_MSG_KEXINIT == message_code)
			{
				//缓存报文 客户端接收到的是服务端报文
				if (sp_type_ == SshClient)
				{
					algo_ctx_.I_S =(char)message_code+Str(payload_data, payload_data_len);
				}
				else
				{
					algo_ctx_.I_C = (char)message_code + Str(payload_data, payload_data_len);
				}
			}
			if (SSH_MSG_VERSION != message_code)
				++recv_sequence_number_;

			if (message_code >= SSH_MSG_MAX)
			{
				return;
			}
			if (handlers_[message_code].func)
				handlers_[message_code].func(this, payload_data, payload_data_len, handlers_[message_code].pdata);
			/*if (handler_)
			{
				handler_(this, message_code, payload_data, payload_data_len, handler_data_);
			}*/
		}

		bool OnMessage()
		{
			/*
				uint32 packet_length
				byte padding_length											|
				byte[n1] payload; n1 = packet_length - padding_length - 1	|	packet_length
				byte[n2] random padding; n2 = padding_length				|
				byte[m] mac (Message Authentication Code - MAC); m = mac_length
			*/
			//检查
			uint8_t padding_length = *((uint8_t*)t_msg_.c_str()+4);//四个字节
			//载荷大小
			uint32_t payload_length = packet_lenght_ - padding_length - 1;
			const char *payload = t_msg_.c_str() + 4 + 1;

			//校验
			if (algo_ctx_.check_mac)
			{
				//the length fields, ’payload’ and ’random padding’
				const char *mac = t_msg_.c_str() + packet_lenght_ + 4;
				if (!CheckMac(recv_sequence_number_, t_msg_.c_str(), packet_lenght_ + 4,mac, t_msg_.size()- packet_lenght_ - 4))
				{
					return false;
				}
			}

			//解压缩之后的缓存
			char payload_uncompress[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
			uint32_t payload_uncompress_length = SSH_TRANS_PACKET_MAX_SIZE;
			if (!UnCompress(payload, payload_length, payload_uncompress, payload_uncompress_length))
			{
				//解压失败
				return false;
			}
			uint8_t message_code = *(uint8_t*)(payload_uncompress);

			//回调 payload_data
			OnHandler(message_code, payload_uncompress+1, payload_uncompress_length-1);

			//clear清理状态
			t_msg_ = "";
			packet_lenght_ = 0;
			
			return true;
		}
		
	private:
		//获取默认的算法
		name_list GetLocalKexAlgorithms()
		{
			return "diffie-hellman-group14-sha1,diffie-hellman-group1-sha1";
		}
		
		name_list GetLocalServerHostKeyAlgorithms()
		{
			//#include <openssl/rsa.h>
			/*PEM_read_RSAPrivateKey()
				PEM_read_DSAPrivateKey*/
			/*PEM_write_RSAPrivateKey*/
			name_list list = "";
			if (sp_type_ == SshServer)
			{
				
				if (algo_ctx_.rsa)
					list = "ssh-rsa";
				if (algo_ctx_.dsa)
				{
					if (list.size() == 0)
						list = "ssh-dsa";
					else
						list += ",ssh-dsa";
				}
			}
			else
			{
				list = "ssh-rsa,ssh-dsa";
			}
			return list;
		}
		
		name_list GetLocalEncryptionAlgorithmsClientToServer()
		{
			return "aes128-cbc,3des-cbc,aes256-ctr,aes128-ctr,aes192-ctr";
		}
		
		name_list GetLocalEncryptionAlgorithmsServerToClient()
		{
			return "aes128-cbc,3des-cbc,aes256-ctr,aes128-ctr,aes192-ctr";
		}
		
		name_list GetLocalMacAlgorithmsClientToServer()
		{
			return "hmac-sha1-96,hmac-sha1";
		}
		
		name_list GetLocalMacAlgorithmsServerToClient()
		{
			return "hmac-sha1-96,hmac-sha1";
		}
		
		name_list GetLocalCompressionAlgorithmsClientToServer()
		{
			return "none";
		}
		
		name_list GetLocalCompressionAlgorithmsServerToClient()
		{
			return "none";
		}
		
		name_list GetLocalLanguagesClientToServer()
		{
			return "";
		}
		
		name_list GetLocalLanguagesServerToClient()
		{
			return "";
		}

		bool AgreeKexAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.kex_algorithms = AgreeAlgorithms(list, GetLocalKexAlgorithms());
			else
				algo_ctx_.kex_algorithms = AgreeAlgorithms(GetLocalKexAlgorithms(),list);

			if (algo_ctx_.kex_algorithms == ""|| algo_ctx_.kex_algorithms == "none")
				return false;
			return true;
		}

		bool AgreeServerHostKeyAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.server_host_key_algorithms = AgreeAlgorithms(list, GetLocalServerHostKeyAlgorithms());
			else
				algo_ctx_.server_host_key_algorithms = AgreeAlgorithms(GetLocalServerHostKeyAlgorithms(), list);

			if (algo_ctx_.server_host_key_algorithms == "" || algo_ctx_.server_host_key_algorithms == "none")
				return false;
			return true;
		}

		bool AgreeEncryptionAlgorithmsClientToServerAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.encryption_algorithms_client_to_server = AgreeAlgorithms(list, GetLocalEncryptionAlgorithmsClientToServer());
			else
				algo_ctx_.encryption_algorithms_client_to_server = AgreeAlgorithms(GetLocalEncryptionAlgorithmsClientToServer(), list);

			if (algo_ctx_.encryption_algorithms_client_to_server == "" || algo_ctx_.encryption_algorithms_client_to_server == "none")
				return false;
			return true;
		}

		bool AgreeEncryptionAlgorithmsServerToClientAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.encryption_algorithms_server_to_client = AgreeAlgorithms(list, GetLocalEncryptionAlgorithmsServerToClient());
			else
				algo_ctx_.encryption_algorithms_server_to_client = AgreeAlgorithms(GetLocalEncryptionAlgorithmsServerToClient(), list);

			if (algo_ctx_.encryption_algorithms_server_to_client == "" || algo_ctx_.encryption_algorithms_server_to_client == "none")
				return false;
			return true;
		}

		bool AgreeMacAlgorithmsClientToServerAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.mac_algorithms_client_to_server = AgreeAlgorithms(list, GetLocalMacAlgorithmsClientToServer());
			else
				algo_ctx_.mac_algorithms_client_to_server = AgreeAlgorithms(GetLocalMacAlgorithmsClientToServer(), list);

			if (algo_ctx_.mac_algorithms_client_to_server == "" || algo_ctx_.mac_algorithms_client_to_server == "none")
				return false;
			return true;
		}

		bool AgreeMacAlgorithmsServerToClientAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.mac_algorithms_server_to_client = AgreeAlgorithms(list, GetLocalMacAlgorithmsServerToClient());
			else
				algo_ctx_.mac_algorithms_server_to_client = AgreeAlgorithms(GetLocalMacAlgorithmsServerToClient(), list);

			if (algo_ctx_.mac_algorithms_server_to_client == "" || algo_ctx_.mac_algorithms_server_to_client == "none")
				return false;
			return true;
		}

		bool AgreeCompressionAlgorithmsClientToServerAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.compression_algorithms_client_to_server = AgreeAlgorithms(list, GetLocalCompressionAlgorithmsClientToServer());
			else
				algo_ctx_.compression_algorithms_client_to_server = AgreeAlgorithms(GetLocalCompressionAlgorithmsClientToServer(), list);

			if (algo_ctx_.compression_algorithms_client_to_server == "")
				return false;
			return true;
		}

		bool AgreeCompressionAlgorithmsServerToClientAlgorithms(name_list list)
		{
			if (sp_type_ == SshClient)
				algo_ctx_.compression_algorithms_server_to_client = AgreeAlgorithms(list, GetLocalCompressionAlgorithmsServerToClient());
			else
				algo_ctx_.compression_algorithms_server_to_client = AgreeAlgorithms(GetLocalCompressionAlgorithmsServerToClient(), list);

			if (algo_ctx_.compression_algorithms_server_to_client == "")
				return false;
			return true;
		}

		bool AgreeLanguagesClientToServerAlgorithms(name_list list)
		{
			return true;
		}

		bool AgreeLanguagesServerToClientAlgorithms(name_list list)
		{
			return true;
		}
		
		//算法协商none 或者空返回""
		Str AgreeAlgorithms(name_list server, name_list client)
		{
			Str client_algo, server_algo;
			for (int i = 0; i < client.size(); ++i)
			{
				if (client[i] != ',')
					client_algo += client[i];

				if (client[i] == ','||i==client.size()-1)
				{
					//解析到一个client节点
					for (int j = 0; j < server.size(); ++j)
					{
						if (server[j] != ',')
							server_algo += server[j];

						if (server[j] == ',' || j == server.size() - 1)
						{
							//服务节点 协商完成
							if (client_algo == server_algo)
								return server_algo;
							server_algo = "";
						}
					}
					client_algo = "";
				}
			}
		
			return "";
		}

		//根据名称获取加密方法
		const EVP_CIPHER      *GetEncryptionCipher(const Str &name)
		{
			//return EVP_get_cipherbyname(name.c_str());
			//"aes128-cbc,3des-cbc,aes256-ctr,aes128-ctr,aes192-ctr";
			if ("aes128-cbc" == name)
				return EVP_aes_128_cbc();
			else if ("3des-cbc" == name)
				return EVP_des_ede3_cbc();
			else if ("aes256-ctr" == name)
				return EVP_aes_256_ctr();
			else if ("aes128-ctr" == name)
				return EVP_aes_128_ctr();
			else if ("aes192-ctr" == name)
				return EVP_aes_192_ctr();
			return NULL;
		}

		//根据算法名称创建 hmac
		SshHMac * NewHMac(const Str &name)
		{
			//"hmac-sha1-96,hmac-sha1"
			if ("hmac-sha1")
				return new SshHMacSha1();
			if ("hmac-sha1-96")
				return new SshHMacSha1_96();
			return NULL;
		}

		Str Sha1(const Str&data)
		{
			//sha-1 运算
#ifdef HAVE_OPAQUE_STRUCTS
			EVP_MD_CTX *ctx = EVP_MD_CTX_new();
#else
			EVP_MD_CTX *ctx = EVP_MD_CTX_create();
			EVP_MD_CTX_init(ctx);
#endif
			
			EVP_DigestInit(ctx, EVP_sha1());
			EVP_DigestUpdate(ctx, data.c_str(), data.size());
			unsigned char temp[EVP_MAX_MD_SIZE] = { 0 };
			unsigned int len = EVP_MAX_MD_SIZE;
			//操作成功返回1，否则返回0。
			int ret = EVP_DigestFinal(ctx, temp, &len);

#ifdef HAVE_OPAQUE_STRUCTS
			EVP_MD_CTX_free(ctx);
			
#else
			EVP_MD_CTX_cleanup(ctx);
			EVP_MD_CTX_destroy(ctx);
#endif
			if (ret == 0)
				return "";
			return Str((char*)temp, len);
		}

		//压缩 out_len 输入也是输出 输入最大缓存 输出结果缓存
		bool Compress(const char*in, uint32_t in_len, char*out, uint32_t &out_len)
		{
			//不压缩
			bool bc = false;
			if (bc)
			{
				return false;
			}
			else
			{
				if (out_len < in_len)
					return false;//缓存不够
				//不压缩
				memcpy(out, in, in_len);
				out_len = in_len;
				return true;
			}
		}

		bool UnCompress(const char*in, uint32_t in_len, char*out, uint32_t &out_len)
		{
			//不压缩
			bool bc = false;
			if (bc)
			{
				return false;
			}
			else
			{
				if (out_len < in_len)
					return false;//缓存不够
				//不压缩
				memcpy(out, in, in_len);
				out_len = in_len;
				return true;
			}
		}

		bool CheckMac(uint32_t sequence_number, const char*in, uint32_t in_len,const char*mac, uint32_t mac_len)
		{
			if(NULL == algo_ctx_.check_mac)
				return false;
			if (mac_len != algo_ctx_.check_mac->len())
				return false;
			uint32_t check_mac_len= algo_ctx_.check_mac->len();
			char *check_mac = new char[check_mac_len];
			if (!algo_ctx_.check_mac->mac(sequence_number, in, in_len, check_mac, check_mac_len))
			{
				delete []check_mac;
				return false;
			}
			if (check_mac_len != mac_len || memcmp(mac, check_mac, mac_len) != 0)
			{
				delete[]check_mac;
				return false;
			}
			delete[]check_mac;
			return true;
		}
		bool Mac(uint32_t sequence_number,const char*in, uint32_t in_len, char*out, uint32_t &out_len)
		{
			if (NULL == algo_ctx_.mac)
				return false;
			if (!algo_ctx_.mac->mac(sequence_number, in, in_len, out, out_len))
			{
				return false;
			}
			/*fprintf(stderr, "mac :");
			for (int i = 0; i < out_len; i++) {
				fprintf(stderr, "%02X ", (unsigned char)(out)[i]);
			}
			fprintf(stderr, "\n");*/
			return true;
		}
		bool Encrypt(const char*in, uint32_t in_len, char*out, uint32_t &out_len)
		{
			if (NULL == algo_ctx_.evp_ctx_encrypt)
			{
				return false;
			}
			if (out_len < EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_encrypt))
				return false;

			int outl = 0;
			EVP_CipherUpdate(algo_ctx_.evp_ctx_encrypt, (unsigned char*)out, &outl, (const unsigned char*)in, in_len);
			out_len = outl;
			return true;
		}
		bool Decrypt(const char*in, uint32_t in_len, char*out, uint32_t &out_len)
		{
			if (NULL == algo_ctx_.evp_ctx_decrypt)
			{
				return false;
			}
			if (out_len < EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_decrypt))
				return false;

			int outl = 0;
			EVP_CipherUpdate(algo_ctx_.evp_ctx_decrypt, (unsigned char*)out, &outl, (const unsigned char*)in, in_len);
			out_len = outl;
			return true;
		}

		//根据协商算法获取pg
		bool GetKexPG(Str kex_algorithm, BIGNUM *p, BIGNUM *g)
		{
			static const unsigned char p_group1_value[128] = {
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
				0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
				0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
				0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
				0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
				0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
				0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
				0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
				0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
				0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
				0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
				0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
				0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
				0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
			};
			static int g_group1_value = 2;

			static const unsigned char p_group14_value[256] = {
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
				0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
				0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
				0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
				0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
				0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
				0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
				0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
				0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
				0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
				0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
				0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
				0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
				0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
				0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
				0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A,
				0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
				0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96,
				0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
				0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
				0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
				0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C,
				0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
				0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03,
				0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
				0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
				0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
				0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5,
				0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
				0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
			};
			static int g_group14_value = 2;

			if (NULL == p || NULL == g)
			{
				return false;
			}
			//diffie-hellman-group1-sha1,diffie-hellman-group14-sha1
			if (kex_algorithm == "diffie-hellman-group1-sha1")
			{
				BN_set_word(g, g_group1_value);
				BN_bin2bn(p_group1_value, sizeof(p_group1_value), p);
				return true;
			}
			else if (kex_algorithm == "diffie-hellman-group14-sha1")
			{
				BN_set_word(g, g_group14_value);
				BN_bin2bn(p_group14_value, sizeof(p_group14_value), p);
				return true;
			}
			else
			{
				return false;
			}

		}
		//子群
		int GetKexGroupOrder(Str kex_algorithm)
		{
			if (kex_algorithm == "diffie-hellman-group1-sha1")
			{
				return 128;
			}
			else if (kex_algorithm == "diffie-hellman-group14-sha1")
			{
				return 256;
			}
			else
			{
				return -1;
			}
		}
		
		//生成e或f
		//首先客户端随机选取一个整数 x 满足(1 < x < q)，计算 e = g^x mod p 。把 e 发给服务器。
		//服务器也随机选取一个整数 y (0 < y < q)，计算 f = g^y mod p
		bool GenerateEF(BIGNUM *xy,BIGNUM *ef)
		{
			if (NULL == xy||NULL == ef)
				return false;

			int group_order = GetKexGroupOrder(algo_ctx_.kex_algorithms);
			if (-1 == group_order)
				return false;

			BIGNUM *p = BN_new();
			BIGNUM *g = BN_new();
			if (!GetKexPG(algo_ctx_.kex_algorithms, p, g))
			{
				algo_ctx_.ReleaseBIGNUM(&p);
				algo_ctx_.ReleaseBIGNUM(&g);
				return false;
			}
			//BIGNUM *x = BN_new();
			//随机选取一个整数 x 满足(1 < x < q)
			BN_rand(xy, group_order * 8 - 1, 0, -1);
			//e = g^x mod p
			BN_CTX *ctx = BN_CTX_new();
			//int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p,const BIGNUM *m, BN_CTX *ctx); 
			//r = pow(a, p) % M
			BN_mod_exp(ef, g, xy, p,ctx);

			//释放资源
			algo_ctx_.ReleaseBIGNUM(&p);
			algo_ctx_.ReleaseBIGNUM(&g);
			//algo_ctx_.ReleaseBIGNUM(&x);
			BN_CTX_free(ctx);
			ctx = NULL;
			return true;
		}

		static bool OpensslGenerateRandArray(unsigned char *arrays, unsigned short size)
		{
			return BaseParser::GenerateRandArray(arrays, size);
			//while (1)
			//{
			//	int ret = RAND_status();
			//	if (ret == 1)
			//	{
			//		break;
			//	}
			//	else
			//	{
			//		//RAND_add()
			//		RAND_poll();
			//	}

			//}
			//int ret = RAND_bytes(arrays, size);
			//if (ret != 1)
			//{
			//	return false;
			//}
			//return true;
		}

		//计算H
		//H = hash(V_C || V_S || I_C || I_S || K_S || e || f || K)
		Str MakeH()
		{
			if (algo_ctx_.V_C.size() == 0
				|| algo_ctx_.V_S.size() == 0
				|| algo_ctx_.I_C.size() == 0
				|| algo_ctx_.I_S.size() == 0
				|| algo_ctx_.K_S.size() == 0
				|| NULL == algo_ctx_.e
				|| NULL == algo_ctx_.f
				|| NULL == algo_ctx_.K)
			{
				//有数据为空
				return "";
			}
			Str buff;
			buff += PrintNameList(algo_ctx_.V_C);
			buff += PrintNameList(algo_ctx_.V_S);
			buff += PrintNameList(algo_ctx_.I_C);
			buff += PrintNameList(algo_ctx_.I_S);
			buff += PrintNameList(algo_ctx_.K_S);
			
			Str value_e = BN2Str(algo_ctx_.e);
			//printf("e %02x %u\n", value_e.c_str(), value_e.size());
			buff += PrintNameList(value_e);

			Str value_f = BN2Str(algo_ctx_.f);
			//printf("f %02x %u\n", value_f.c_str(), value_f.size());
			buff += PrintNameList(value_f);

			Str value_k = BN2Str(algo_ctx_.K);
			//printf("k %02x %u\n", value_k.c_str(), value_k.size());
			buff += PrintNameList(value_k);

			/*buff += PrintNameList(BN2Str(algo_ctx_.f));
			buff += PrintNameList(BN2Str(algo_ctx_.K));*/
			//printf("k %x %u\n", exchange_state->k_value, exchange_state->k_value_len);
			return Sha1(buff);
		}

		//
		Str MakeHostKey()
		{
			Str host_key;

			if ("ssh-rsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.rsa)
			{
				/*
					The "ssh-rsa" key format has the following specific encoding:
					string "ssh-rsa"
					mpint e
					mpint n
				*/
#ifdef HAVE_OPAQUE_STRUCTS
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(BN2Str(RSA_get0_e(algo_ctx_.rsa)));
				host_key += PrintNameList(BN2Str(RSA_get0_n(algo_ctx_.rsa)));
#else
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(BN2Str((algo_ctx_.rsa)->e));
				host_key += PrintNameList(BN2Str((algo_ctx_.rsa)->n));
#endif
			}
			else if ("ssh-dsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.dsa)
			{
				/*
					string "ssh-dss"
					mpint p
					mpint q
					mpint g
					mpint y
				*/
#ifdef HAVE_OPAQUE_STRUCTS
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(BN2Str(DSA_get0_p(algo_ctx_.dsa)));
				host_key += PrintNameList(BN2Str(DSA_get0_q(algo_ctx_.dsa)));
				host_key += PrintNameList(BN2Str(DSA_get0_g(algo_ctx_.dsa)));
				host_key += PrintNameList(BN2Str(DSA_get0_pub_key(algo_ctx_.dsa)));
#else
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(BN2Str((algo_ctx_.dsa)->p));
				host_key += PrintNameList(BN2Str((algo_ctx_.dsa)->q));
				host_key += PrintNameList(BN2Str((algo_ctx_.dsa)->g));
				host_key += PrintNameList(BN2Str((algo_ctx_.dsa)->pub_key));
#endif
			}
			return host_key;
		}

		/*
		创建密钥
		K1 = HASH(K || H || X || session_id) (X is e.g., "A")
		K2 = HASH(K || H || K1)
		K3 = HASH(K || H || K1 || K2)
		...
		key = K1 || K2 || K3 || ...
		Here K is encoded as mpint and "A" as byte and session_id as raw
		data. "A" means the single character A, ASCII 65
		*/
		Str MakeKey(mpint K,Str H,char X,Str session_id,uint32_t len)
		{
			//K1 = HASH(K || H || X || session_id) (X is e.g., "A")
			Str  data = K + H + X + session_id;
			Str Key = Sha1(data);
			while (Key.size() < len)
			{
				//K2 = HASH(K || H || K1)
				data = K + H + Key;
				Key += Sha1(data);
			}
			return Key.substr(0, len);
		}

		//初始化 服务端发过来的公钥
		bool InitPubKey(const SSHKexDHReply &dh_reply)
		{
			uint32_t offset = 0;
			Str ks_name;
			if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, ks_name))
				return false;
			if (ks_name != algo_ctx_.server_host_key_algorithms)
				return false;//与协商的算法不一致
			//if (offset >= dh_reply.K_S.size())
			//	return false;//空
			/*
			The value for ’dss_signature_blob’ is encoded as a string containing
			r, followed by s (which are 160-bit integers, without lengths or
			padding, unsigned, and in network byte order).
			*/
			algo_ctx_.K_S = dh_reply.K_S;//Str(dh_reply.K_S.c_str() + offset, dh_reply.K_S.size() - offset);
			
			if ("ssh-rsa" == ks_name)
			{
				/*
					string "ssh-rsa"
					mpint e
					mpint n
				*/

				Str str_e, str_n;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_e))
					return false;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_n))
					return false;
				BIGNUM *e = Str2BN(str_e);
				BIGNUM *n = Str2BN(str_n);
				if (algo_ctx_.rsa)
					RSA_free(algo_ctx_.rsa);
				algo_ctx_.rsa = RSA_new();
#ifdef HAVE_OPAQUE_STRUCTS
				RSA_set0_key(algo_ctx_.rsa, n, e,NULL);
				RSA_set0_factors(algo_ctx_.rsa, NULL, NULL);
				RSA_set0_crt_params(algo_ctx_.rsa, NULL, NULL, NULL);
#else
				(algo_ctx_.rsa)->e = e;
				(algo_ctx_.rsa)->n = n;
				(algo_ctx_.rsa)->d = NULL;

				(algo_ctx_.rsa)->p = NULL;
				(algo_ctx_.rsa)->q = NULL;

				(algo_ctx_.rsa)->dmp1 = NULL;
				(algo_ctx_.rsa)->dmq1 = NULL;
				(algo_ctx_.rsa)->iqmp = NULL;
#endif
				/*algo_ctx_.ReleaseBIGNUM(&e);
				algo_ctx_.ReleaseBIGNUM(&n);*/
				return true;
			}
			else if ("ssh-dsa" == ks_name)
			{
				/*
					string "ssh-dss"
					mpint p
					mpint q
					mpint g
					mpint y
				*/

				Str str_p, str_q, str_g, str_y;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_p))
					return false;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_q))
					return false;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_g))
					return false;
				if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, str_y))
					return false;
				BIGNUM *p = Str2BN(str_p);
				BIGNUM *q = Str2BN(str_q);
				BIGNUM *g = Str2BN(str_g);
				BIGNUM *y = Str2BN(str_y);
				if (algo_ctx_.dsa)
					DSA_free(algo_ctx_.dsa);
				algo_ctx_.dsa = DSA_new();
#ifdef HAVE_OPAQUE_STRUCTS
				DSA_set0_pqg(algo_ctx_.dsa, p, q, g);
				DSA_set0_key(algo_ctx_.dsa, y,NULL);
#else
				(algo_ctx_.dsa)->p = p;
				(algo_ctx_.dsa)->g = g;
				(algo_ctx_.dsa)->q = q;
				(algo_ctx_.dsa)->pub_key = y;
				(algo_ctx_.dsa)->priv_key = NULL;
#endif
				/*algo_ctx_.ReleaseBIGNUM(&p);
				algo_ctx_.ReleaseBIGNUM(&q);
				algo_ctx_.ReleaseBIGNUM(&g);
				algo_ctx_.ReleaseBIGNUM(&y);*/
				return true;
			}
			else
			{
				return false;
			}

		}

		bool SignDHReply(const Str &data, SSHKexDHReply &dh_reply)
		{
			//算法名长度+算法名+签名数据长度+签名值。
			dh_reply.S = PrintNameList(algo_ctx_.server_host_key_algorithms);
			Str sign;
			if (!Sign(data, sign))
			{
				return false;
			}
			dh_reply.S+= PrintNameList(sign);
			return true;
		}

		bool VerifyDHReply(const Str &data, const SSHKexDHReply &dh_reply)
		{
			//算法名长度+算法名+签名数据长度+签名值。
			uint32_t offset = 0;
			Str ks_name, sign;
			if (!ParserNameList(dh_reply.S.c_str(), dh_reply.S.size(), offset, ks_name))
				return false;
			if (!ParserNameList(dh_reply.S.c_str(), dh_reply.S.size(), offset, sign))
				return false;

			if (ks_name != algo_ctx_.server_host_key_algorithms)
				return false;//与协商的算法不一致
			if (sign.size() == 0)
				return false;//为空

			return Verify(data, sign);
		}

		bool Verify(const Str &data, const Str &sign)
		{
			if ("ssh-rsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.rsa)
			{
				//hash
				Str hash = Sha1(data);

				int ret = RSA_verify(NID_sha1, (unsigned char*)hash.c_str(), hash.size()
					,(unsigned char*)sign.c_str(), sign.size(), algo_ctx_.rsa);
				if (ret != 1)
				{
					return false;
				}
				return true;
			}
			else if ("ssh-dsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.dsa)
			{
				//hash
				Str hash = Sha1(data);

				int ret = DSA_verify(NID_sha1, (unsigned char*)hash.c_str(), hash.size()
					, (unsigned char*)sign.c_str(), sign.size(), algo_ctx_.dsa);
				if (ret != 1)
				{
					return false;
				}
				return true;
			}
			else
			{
				return false;
			}
			
		}

		bool Sign(const Str &data, Str &sign)
		{
			if ("ssh-rsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.rsa)
			{
				//hash
				Str hash = Sha1(data);
				unsigned int siglen = RSA_size(algo_ctx_.rsa) * 2+1;
				unsigned char *sigret = new unsigned char[siglen];
				
				int ret = RSA_sign(NID_sha1, (unsigned char*)hash.c_str(), hash.size()
					, sigret, &siglen, algo_ctx_.rsa);
				if (ret != 1)
				{
					delete[]sigret;
					return false;
				}
				sign = Str((char*)sigret, siglen);
				delete[]sigret;
				return true;
			}
			else if ("ssh-dsa" == algo_ctx_.server_host_key_algorithms&&algo_ctx_.dsa)
			{
				//hash
				Str hash = Sha1(data);
				unsigned int siglen = DSA_size(algo_ctx_.dsa) * 2 + 1;
				unsigned char *sigret = new unsigned char[siglen];

				int ret = DSA_sign(NID_sha1, (unsigned char*)hash.c_str(), hash.size()
					, sigret, &siglen, algo_ctx_.dsa);
				if (ret != 1)
				{
					delete[]sigret;
					return false;
				}
				sign = Str((char*)sigret, siglen);
				delete[]sigret;
				return true;
			}
			else
			{
				return false;
			}

		}
	private:
		
#ifdef SIM_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
		uint32_t packet_lenght_;
		
		SshTransportStatus status_;

		Str temp_;
		//消息缓存
		Str t_msg_;

		SSHAlgorithmsCtx algo_ctx_;

		SshTransportHandler handlers_[SSH_MSG_MAX];
		/*SSH_TRANS_HANDLER handler_;
		void*handler_data_;*/

		uint32_t send_sequence_number_, recv_sequence_number_;

		SshPointType sp_type_;
	};
	
	/*
	*	Authentication Requests
	* 
		byte SSH_MSG_USERAUTH_REQUEST
		string user name in ISO-10646 UTF-8 encoding [RFC3629]
		string service name in US-ASCII
		string method name in US-ASCII
		.... method specific fields
	*/
	struct SshAuthRequest
	{
		Str user_name;
		Str service_name;
		Str method;
	};

	/*
	*  
		Responses to Authentication Requests
		If the server rejects the authentication request, it MUST respond
		with the following:
			byte SSH_MSG_USERAUTH_FAILURE
			name-list authentications that can continue
			boolean partial success
	*/
	/*struct SshAuthResponse
	{
		Str authentications;
	};*/

	//The Secure Shell (SSH) Authentication Protocol
	class SshAuthentication :public SshTransport
	{
	public:
		SshAuthentication(SshPointType sp_type)
			:SshTransport(sp_type)
		{

		}
		Str PrintAuthRequset(const SshAuthRequest& req)
		{
			return "";
		}
		bool ParserAuthRequset(const char* payload_data, uint32_t payload_data_len, SshAuthRequest& req)
		{
			return false;
		}
		/*
		* name-list authentications that can continue
boolean partial success FAILURE
		*/
		bool ParserAuthResponseFailure(const char* payload_data, uint32_t payload_data_len, Str& authentications, bool& partial);
		Str PrintAuthResponseFailure(const Str& authentications, bool partial);
		Str PrintAuthResponseSuccess();

		/*
		*	Banner Message
			In some jurisdictions, sending a warning message before
			authentication may be relevant for getting legal protection. Many
			UNIX machines, for example, normally display text from /etc/issue,
			use TCP wrappers, or similar software to display a banner before
			issuing a login prompt.
			The SSH server may send an SSH_MSG_USERAUTH_BANNER message at any
			time after this authentication protocol starts and before
			authentication is successful. This message contains text to be
			displayed to the client user before authentication is attempted. The
			format is as follows:
				byte SSH_MSG_USERAUTH_BANNER
				string message in ISO-10646 UTF-8 encoding [RFC3629]
				string language tag [RFC3066]
			By default, the client SHOULD display the ’message’ on the screen.
			However, since the ’message’ is likely to be sent for every login
			attempt, and since some client software will need to open a separate
			window for this warning, the client software may allow the user to
			explicitly disable the display of banners from the server. The
			’message’ may consist of multiple lines, with line breaks indicated
			by CRLF pairs.
		*/
		bool ParserBannerMessage(const char* payload_data, uint32_t payload_data_len, Str& message, Str& language_tag);
		Str PrintAuthResponseFailure(const Str& message, const Str& language_tag);
	private:

	};

	//The Secure Shell (SSH) Connection Protocol
	class SshConnection :public SshAuthentication
	{
	public:

	private:

	};
}
#endif