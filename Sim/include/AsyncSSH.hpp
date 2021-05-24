/*
	实现ssh2.0协议
*/

#ifndef SIM_USE_OPENSSL
#define SIM_SSH_CTX_HPP_
#endif //! SIM_USE_OPENSSL

#ifndef SIM_SSH_CTX_HPP_
#define SIM_SSH_CTX_HPP_

//类型
#include <stdint.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

//基础解析器
#include "BaseParser.hpp"

#ifndef SIM_SSH_TRANS_PAESER_TYPE
#define SIM_SSH_TRANS_PAESER_TYPE 3
#endif

//多线程情况下运行
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

#define SIM_SSH_VER "SSH-2.0-SIM.SSH.1.0"

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

#ifndef SSH_TRANS_PACKET_MAX_SIZE
//35000
#define SSH_TRANS_PACKET_MAX_SIZE 35000
#endif
namespace sim
{
	class SshTransport;

	//The Secure Shell (SSH) Transport Layer Protocol

	//SSH-protoversion-softwareversion SP comments CR LF
	struct SSHVersion
	{
		Str protoversion;//2.0
		Str softwareversion;
		Str comments;
	};

	typedef Str name_list;
	typedef Str mpint;

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
		std::uint8_t cookie[16];
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
		std::uint32_t reserved;
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
		string signature of H
	*/
	struct SSHKexDHReply
	{
		Str K_S;
		Str f;
		Str H;
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

		//
		std::uint8_t mac_len;

		//是否已经发送或者接收报文 new key 标识算法可用。
		bool is_newkeys;

		//V_C, V_S, I_C, I_S, K_S
		Str V_C, V_S, I_C, I_S, K_S;
		/*
			mpint e, exchange value sent by the client
			mpint f, exchange value sent by the server
			mpint K, the shared secret
		*/
		BIGNUM* e, *f, *K;

		SSHAlgorithmsCtx()
			:evp_ctx_encrypt(NULL)
			, evp_ctx_decrypt(NULL)
			, mac_len(0)
			, is_newkeys(false)
			,e(NULL)
			,f(NULL)
			,K(NULL)
		{

		}
		//重置状态
		void ReSet()
		{
			if (evp_ctx_encrypt)
			{
				EVP_CIPHER_CTX_cleanup(evp_ctx_encrypt);
				evp_ctx_encrypt = NULL;
			}
			if (evp_ctx_decrypt)
			{
				EVP_CIPHER_CTX_cleanup(evp_ctx_decrypt);
				evp_ctx_decrypt = NULL;
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
			ReleaseBIGNUM(&e);
			ReleaseBIGNUM(&f);
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
		}
		void ReleaseBIGNUM(BIGNUM** pn)
		{
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
		std::uint8_t message_code,
		const char*payload_data,std::uint32_t payload_data_len, void*pdata);

	enum SshPointType
	{
		SshClient,
		SshServer
	};
	//DH_generate_parameters_ex

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
			, handler_(NULL)
			, handler_data_(NULL)
			, packet_lenght_(0)
			, send_sequence_number_(0)
			, recv_sequence_number_(0)
		{

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

		}
		void SetHandler(SSH_TRANS_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			handler_data_ = pdata;
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
		}
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
		bool ParserVersion(const char*payload_data, std::uint32_t payload_data_len, SSHVersion&ver)
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
			std::uint32_t offset = 0;

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
		bool ParserKexInit(const char*payload_data, std::uint32_t payload_data_len, SSHKexInit&kex_init)
		{
			std::uint32_t offset = 0;
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
			kex_init.reserved =*(std::uint32_t*) payload_data +offset;
			return true;
		}

		//SSHKexDHInit
		Str PrintKexDHInit()
		{
			//如果不存在就创建
			if (NULL == algo_ctx_.e)
				algo_ctx_.e = BN_new();

			if (!GenerateE(algo_ctx_.e))
			{
				//生成失败
				return false;
			}
			SSHKexDHInit dh_init;
			dh_init.e = Mpint(algo_ctx_.e);
			if (dh_init.e.size() == 0)
				return false;
			return PrintKexDHInit(dh_init);
		}
		Str PrintKexDHInit(const SSHKexDHInit&dh_init);
		bool ParserKexDHInit(const char*payload_data, std::uint32_t payload_data_len, SSHKexDHInit&dh_init)
		{
			std::uint32_t offset = 0;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_init.e))
				return false;
			return true;
		}

		Str PrintPacket(std::uint8_t message_code,const char*payload_data, std::uint32_t payload_data_len)
		{
			//包过大
			if (payload_data_len + 1 +1 + 4+algo_ctx_.mac_len > SSH_TRANS_PACKET_MAX_SIZE)
				return "";
			
			char tmp[SSH_TRANS_PACKET_MAX_SIZE] = {0};

			//playload
			std::uint32_t raw_payload_len = payload_data_len + 1;
			char *raw_payload = new char [raw_payload_len] ;
			memcpy(raw_payload, &message_code, 1);
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
			std::uint32_t payload_len = SSH_TRANS_PACKET_MAX_SIZE-5;
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
			std::uint32_t total = 4+1+ payload_len;
			std::uint8_t block_size = 8;
			if (algo_ctx_.evp_ctx_encrypt)
			{
				block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_encrypt);
			}
			std::uint8_t padding_len = block_size - total % block_size;
			if (padding_len < 4)
				padding_len += block_size;
			if (4+1+ payload_len + padding_len + algo_ctx_.mac_len > SSH_TRANS_PACKET_MAX_SIZE)
				return "";
			unsigned char *padding = (unsigned char *)tmp + 1 + 4 + payload_len;
			if (!OpensslGenerateRandArray(padding, sizeof(padding_len)))
			{
				return "";
			}

			//填充长度
			//padding feild（1）+ payload_len + padding_len
			std::uint32_t packet_lenght = 1 + payload_len + padding_len;
			packet_lenght = htonl(packet_lenght);
			memcpy(tmp, &packet_lenght, 4);
			memcpy(tmp +4, &padding_len, 1);

			if (algo_ctx_.evp_ctx_encrypt)
			{
				//加密
				//需要加密的报文长度
				char msg[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
				std::uint32_t msg_len = 0;
				std::uint32_t encrypt_size = 4 + 1 + payload_len + padding_len;
				std::uint32_t out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
				for (int i = 0; i < encrypt_size; i += block_size)//保证一次完成
				{
					out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
					if (!Encrypt(tmp + i, block_size, msg + msg_len, out_len))
						return "";
					msg_len += out_len;
				}
				out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
				//计算mac
				if (!Mac(send_sequence_number_, tmp, encrypt_size, msg + msg_len, out_len))
				{
					return "";
				}
				msg_len += out_len;
				++send_sequence_number_;
				return Str(msg, msg_len);
			}
			else
			{
				++send_sequence_number_;
				return Str(tmp, 4 + 1 + payload_len + padding_len);
			}
		}

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
		
	private:
		Str PrintNameList(name_list list)
		{
			std::uint32_t size = htonl(list.size());

			Str res = Str((const char*)&size, 4);
			if (list.size() != 0)
				res += list;
			return res;
		}
		
		bool ParserNameList(const char*payload, std::uint32_t payload_len, std::uint32_t &offset, name_list&list)
		{
			//剩余的字节数
			std::uint32_t has_bytes = payload_len - offset;
			//必须有四字节长度
			if (has_bytes < 4)
				return false;
			std::uint32_t list_len = ntohl(*(unsigned long*)(payload + offset));
			//长度不足
			if (has_bytes < list_len+4)
				return false;
			list = name_list(payload + offset + 4, list_len);
			offset += list_len + 4;
			return true;
		}

		//大数转换为字符串
		Str Mpint(BIGNUM *n)
		{
			int bit_num = BN_num_bytes(n);
		}

		//子串转换为大数
		BIGNUM * BigNum(Str m);

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
				std::uint8_t block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_decrypt);
				return false;
			}
		}*/

		bool SplitPacket(const char*data, unsigned int len, unsigned int &offset)
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
							packet_lenght_ = ntohl(*((std::uint32_t*)(t_msg_.c_str())));
							break;
						}
						t_msg_ += data[offset++];
					}
				}
				else
				{
					std::uint32_t need_bytes = (packet_lenght_ + 4) - t_msg_.size();
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
						std::uint32_t has_bytes = len - offset;
						std::uint32_t copy_bytes = need_bytes>has_bytes? has_bytes: need_bytes;
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
				if (algo_ctx_.mac_len == 0)
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
			std::uint32_t offset = 0;
			while (offset < temp_.size())
			{
				if (status_ == SshTransportStatus_Packet)
				{
					if (algo_ctx_.evp_ctx_decrypt)
					{
						std::uint8_t block_size = EVP_CIPHER_CTX_block_size(algo_ctx_.evp_ctx_decrypt);
						const std::uint32_t decrypt_buff_size = 1024;
						char decrypt_buff[decrypt_buff_size] = { 0 };
						for (; offset < temp_.size(); offset += block_size)
						{
							std::uint32_t out_len = decrypt_buff_size;
							if (!Decrypt(temp_.c_str() + offset, block_size, decrypt_buff, out_len))
							{
								return false;
							}
							if (out_len > 0)
							{
								std::uint32_t s_offset = 0;
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
					if (algo_ctx_.mac_len)
					{
						if (t_msg_.size() >= packet_lenght_ + 4 + algo_ctx_.mac_len)
						{
							bool ret = OnMessage();
							if (false == ret)
								return ret;
						}
						t_msg_ += temp_[offset++];
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
		}

		void OnHandler(std::uint8_t message_code,
			const char*payload_data, std::uint32_t payload_data_len)
		{
			if (SSH_MSG_VERSION == message_code)
			{
				//去除\r\n末尾
				std::uint32_t len = payload_data_len-1;
				while (len > 0)
				{
					if (payload_data[len] != '\r' || payload_data[len] != '\n')
						break;
					--len;
				}
				len += 1;
				//缓存报文 客户端接收到的是服务端报文
				if (sp_type_ == SshClient)
				{
					algo_ctx_.V_S = Str(payload_data, len);
				}
				else
				{
					algo_ctx_.V_C = Str(payload_data, len);
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
			++recv_sequence_number_;
			if (handler_)
			{
				handler_(this, message_code, payload_data, payload_data_len, handler_data_);
			}
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
			std::uint8_t padding_length = *((std::uint8_t*)t_msg_.c_str()+4);//四个字节
			//载荷大小
			std::uint32_t payload_length = packet_lenght_ - padding_length - 1;
			const char *payload = t_msg_.c_str() + 4 + 1;

			//校验
			if (algo_ctx_.mac_len)
			{
				const char *mac = t_msg_.c_str() + packet_lenght_ + 4;
				if (!CheckMac(recv_sequence_number_, t_msg_.c_str(), packet_lenght_ + 4))
				{
					return false;
				}
			}

			//解压缩之后的缓存
			char payload_uncompress[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
			std::uint32_t payload_uncompress_length = SSH_TRANS_PACKET_MAX_SIZE;
			if (!UnCompress(payload, payload_length, payload_uncompress, payload_uncompress_length))
			{
				//解压失败
				return false;
			}
			std::uint8_t message_code = *(std::uint8_t*)(payload_uncompress);

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
			return "diffie-hellman-group1-sha1,diffie-hellman-group14-sha1";
		}
		
		name_list GetLocalServerHostKeyAlgorithms()
		{
			return "ssh-rsa,ssh-dss";
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

		//压缩 out_len 输入也是输出 输入最大缓存 输出结果缓存
		bool Compress(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
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

		bool UnCompress(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
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

		bool CheckMac(std::uint32_t sequence_number, const char*in, std::uint32_t in_len)
		{
			return false;
		}
		bool Mac(std::uint32_t sequence_number,const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
		{
			return false;
		}
		bool Encrypt(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
		{
			return false;
		}
		bool Decrypt(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
		{
			return false;
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
			else if (kex_algorithm == "diffie-hellman-group1-sha1")
			{
				BN_set_word(g, g_group14_value);
				BN_bin2bn(p_group1_value, sizeof(p_group14_value), p);
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
			else if (kex_algorithm == "diffie-hellman-group1-sha1")
			{
				return 256;
			}
			else
			{
				return -1;
			}
		}
		//生成e
		//首先客户端随机选取一个整数 x 满足(1 < x < q)，计算 e = g^x mod p 。把 e 发给服务器。
		bool GenerateE(BIGNUM *e)
		{
			if (NULL == e)
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
			BIGNUM *x = BN_new();
			//随机选取一个整数 x 满足(1 < x < q)
			BN_rand(x, group_order * 8 - 1, 0, -1);
			//e = g^x mod p
			BN_CTX *ctx = BN_CTX_new();
			//int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p,const BIGNUM *m, BN_CTX *ctx); 
			//r = pow(a, p) % M
			BN_mod_exp(e, g, x, p,ctx);

			//释放资源
			algo_ctx_.ReleaseBIGNUM(&p);
			algo_ctx_.ReleaseBIGNUM(&g);
			algo_ctx_.ReleaseBIGNUM(&x);
			BN_CTX_free(ctx);
			ctx = NULL;
			return true;
		}

		static bool OpensslGenerateRandArray(unsigned char *arrays, unsigned short size)
		{
			while (1)
			{
				int ret = RAND_status();
				if (ret == 1)
				{
					break;
				}
				else
				{
					//RAND_add()
					RAND_poll();
				}

			}
			int ret = RAND_bytes(arrays, size);
			if (ret != 1)
			{
				return false;
			}
			return true;
		}
	private:
		
#ifdef SIM_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
		std::uint32_t packet_lenght_;
		
		SshTransportStatus status_;

		Str temp_;
		//消息缓存
		Str t_msg_;

		SSHAlgorithmsCtx algo_ctx_;

		SSH_TRANS_HANDLER handler_;
		void*handler_data_;

		std::uint32_t send_sequence_number_, recv_sequence_number_;

		SshPointType sp_type_;
	};

	//The Secure Shell (SSH) Authentication Protocol
	class SshAuthentication :public SshTransport
	{
	public:

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