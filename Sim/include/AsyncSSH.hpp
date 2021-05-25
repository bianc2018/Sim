/*
	ʵ��ssh2.0Э��
*/

#ifndef SIM_USE_OPENSSL
#define SIM_SSH_CTX_HPP_
#endif //! SIM_USE_OPENSSL

#ifndef SIM_SSH_CTX_HPP_
#define SIM_SSH_CTX_HPP_

//����
#include <stdint.h>

#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

//����������
#include "BaseParser.hpp"
#pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll
#ifndef SIM_SSH_TRANS_PAESER_TYPE
#define SIM_SSH_TRANS_PAESER_TYPE 3
#endif

//���߳����������
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

//����ص�
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
		string signature of H �ܳ���+�㷨������+�㷨��+ǩ�����ݳ���+ǩ��ֵ��
	*/
	struct SSHKexDHReply
	{
		Str K_S;
		Str f;
		Str S;
	};
	
	//ssh�㷨������
	struct SSHAlgorithmsCtx
	{
		//�㷨Э�̽׶ξ���
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

		//��Կ���½׶�Э�����
		EVP_CIPHER_CTX *evp_ctx_encrypt;
		EVP_CIPHER_CTX *evp_ctx_decrypt;

		//
		std::uint8_t mac_len;

		//�Ƿ��Ѿ����ͻ��߽��ձ��� new key ��ʶ�㷨���á�
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

		//hostkey �㷨
		RSA *rsa;
		DSA *dsa;

		SSHAlgorithmsCtx()
			:evp_ctx_encrypt(NULL)
			, evp_ctx_decrypt(NULL)
			, mac_len(0)
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
		//����״̬
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

	//�ص�
	typedef  void (*SSH_TRANS_HANDLER)(SshTransport*parser,
		std::uint8_t message_code,
		const char*payload_data,std::uint32_t payload_data_len, void*pdata);

	enum SshPointType
	{
		SshClient,
		SshServer
	};

	//Public Key Algorithms ssh��Կ�㷨
	enum SshHostKeyType
	{
		SshRsa,
		SshDsa
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
			//��Ϣ����
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
		//��������
		virtual bool Parser(const char*data, unsigned int len)
		{
			//�������
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
						//�ص�
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
					//�ָ��
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

		//���ļ��м���˽Կ
		bool LoadPriKey(SshHostKeyType type, const char*filename)
		{
			if (NULL == filename)
			{
				return false;
			}
			//��
			BIO*pIn = BIO_new_file(filename, "r");
			if (NULL == pIn)
			{
				//��ʧ��
				return false;
			}

			if (type == SshRsa)
			{
				//�ͷ��Ѿ����ڵ�
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
				//�ͷ��Ѿ����ڵ�
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
		//��������˽Կ filename !=NULL д����Ӧ���ļ�
		bool GeneratePriKey(SshHostKeyType type, const char*filename = NULL)
		{
			BIO* pOut = NULL;
			if (filename)
			{
				//��
				pOut = BIO_new_file(filename, "w");
				if (NULL == pOut)
				{
					//��ʧ��
					return false;
				}
			}
			
			if (type == SshRsa)
			{
				unsigned long  e = RSA_3;
				//�ͷ��Ѿ����ڵ�
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
				//�ͷ��Ѿ����ڵ�
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
				/* ������Կ */
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
	public:
		//Protocol Version Exchange
		//SSH-protoversion-softwareversion SP comments CR LF
		Str PrintProtocolVersion()
		{
			//���汨��
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
				return "";//����Ϊ��
			}
			else
			{
				res += ver.protoversion;
			}
			if (ver.comments.size() != 0)
			{
				res += " " + ver.comments;
			}
			
			//���汨��
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

		//��ӡĬ�ϵ��㷨�б�
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
				return false;//���Ȳ��� 

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
				return false;//���Ȳ��� 
			kex_init.first_kex_packet_follows = payload_data[offset++];

			if (payload_data_len < offset + 4)
				return false;//���Ȳ��� 
			kex_init.reserved =*(std::uint32_t*) payload_data +offset;
			return true;
		}

		//SSHKexDHInit
		Str PrintKexDHInit()
		{
			//��������ھʹ���
			if (NULL == algo_ctx_.e)
				algo_ctx_.e = BN_new();
			
			if (NULL == algo_ctx_.x)
				algo_ctx_.x = BN_new();

			if (!GenerateEF(algo_ctx_.x,algo_ctx_.e))
			{
				//����ʧ��
				return "";
			}
			SSHKexDHInit dh_init;
			dh_init.e = Mpint(algo_ctx_.e);
			if (dh_init.e.size() == 0)
				return "";
			
			/*BIGNUM *e1 = BigNum(dh_init.e);
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
		bool ParserKexDHInit(const char*payload_data, std::uint32_t payload_data_len, SSHKexDHInit&dh_init)
		{
			std::uint32_t offset = 0;
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
		bool ParserKexDHReply(const char*payload_data, std::uint32_t payload_data_len, SSHKexDHReply&dh_reply)
		{
			std::uint32_t offset = 0;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.K_S))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.f))
				return false;
			if (!ParserNameList(payload_data, payload_data_len, offset, dh_reply.S))
				return false;
			return true;
		}

		Str PrintPacket(std::uint8_t message_code,const char*payload_data, std::uint32_t payload_data_len)
		{
			//������
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
				//���汨��
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
			//ѹ��
			if (!Compress(raw_payload, raw_payload_len, tmp + 5, payload_len))
			{
				delete[]raw_payload;
				return "";
			}
			delete[]raw_payload;

			/*
			Note that the length of the concatenation of ��packet_length��,
			padding_length��, ��payload��, and ��random padding�� MUST be a multiple
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
			if (!OpensslGenerateRandArray(padding, padding_len))
			{
				return "";
			}

			//��䳤��
			//padding feild��1��+ payload_len + padding_len
			std::uint32_t packet_lenght = 1 + payload_len + padding_len;
			packet_lenght = htonl(packet_lenght);
			memcpy(tmp, &packet_lenght, 4);
			memcpy(tmp +4, &padding_len, 1);

			if (algo_ctx_.evp_ctx_encrypt)
			{
				//����
				//��Ҫ���ܵı��ĳ���
				char msg[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
				std::uint32_t msg_len = 0;
				std::uint32_t encrypt_size = 4 + 1 + payload_len + padding_len;
				std::uint32_t out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
				for (int i = 0; i < encrypt_size; i += block_size)//��֤һ�����
				{
					out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
					if (!Encrypt(tmp + i, block_size, msg + msg_len, out_len))
						return "";
					msg_len += out_len;
				}
				out_len = SSH_TRANS_PACKET_MAX_SIZE - msg_len;
				//����mac
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

	public:
		//�汾Э��
		bool VersionExchange(const SSHVersion&ver)
		{
			if (ver.protoversion == "2.0")
				return true;
			else
				return false;
		}

		//�㷨Э��
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

		//��ԿЭ��
		//�����
		//dh_init ���յ���dh ��ʼ������
		//dh_reply ���͵Ļظ�����
		bool KeyExchange(const SSHKexDHInit&dh_init, SSHKexDHReply &dh_reply)
		{
			/*
				������Ҳ���ѡȡһ������ y (0 < y < q)������ f = g^y mod p ��
				Ȼ���ÿͻ��˵� e ������ K = e^y mod p ��
				�����ϣֵ H �������ϣ����������� V_C, V_S, I_C, I_S, K_S, e, f, K ��
				Ȼ���÷�����������Կ��˽Կ�������ϣֵ������ǩ����ǩ��ֵ s ���� K_S, f, s ���͸��ͻ��ˡ�
			*/

			//��������ھʹ���
			if (NULL == algo_ctx_.f)
				algo_ctx_.f = BN_new();

			if (NULL == algo_ctx_.y)
				algo_ctx_.y = BN_new();

			if (!GenerateEF(algo_ctx_.y, algo_ctx_.f))
			{
				//����ʧ��
				return false;
			}

			algo_ctx_.e = BigNum(dh_init.e, algo_ctx_.e);
			if (NULL == algo_ctx_.e)
			{
				return false;
			}
			//���� K = e^y mod p 
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
			
			printf("server p=0x%s\n", BN_bn2hex(p));
			printf("server e=0x%s\n", BN_bn2hex(algo_ctx_.e));
			printf("server f=0x%s\n", BN_bn2hex(algo_ctx_.f));
			printf("server k=0x%s\n", BN_bn2hex(algo_ctx_.K));
			
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

			//ǩ��
			if (!SignDHReply(algo_ctx_.H, dh_reply))
			{
				return false;
			}
			dh_reply.f = Mpint(algo_ctx_.f);
			
			dh_reply.K_S = algo_ctx_.K_S;
			return true;
		}
		//�ͻ���
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
			algo_ctx_.f = BigNum(dh_reply.f, algo_ctx_.f);
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
			
			//��֤ǩ��
			//verify(Str &H, Str &S);
			if (!VerifyDHReply(algo_ctx_.H, dh_reply))
				return false;
			return true;
		}

		//������Կ�����ر���
		bool NewKeys();

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
			//ʣ����ֽ���
			std::uint32_t has_bytes = payload_len - offset;
			//���������ֽڳ���
			if (has_bytes < 4)
				return false;
			std::uint32_t list_len = ntohl(*(unsigned long*)(payload + offset));
			//���Ȳ���
			if (has_bytes < list_len+4)
				return false;
			list = name_list(payload + offset + 4, list_len);
			offset += list_len + 4;
			return true;
		}

		//����ת��Ϊ�ַ���
		Str Mpint(const BIGNUM *n)
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

		//�Ӵ�ת��Ϊ����
		BIGNUM * BigNum(Str m, BIGNUM *n=NULL)
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
							offset -= block_size;//����
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
			return true;
		}

		void OnHandler(std::uint8_t message_code,
			const char*payload_data, std::uint32_t payload_data_len)
		{
			if (SSH_MSG_VERSION == message_code)
			{
				//ȥ��\r\nĩβ
				Str ver;
				for(int i=0;i< payload_data_len;++i)
				{
					if (payload_data[i] == '\r' || payload_data[i] == '\n')
						break;
					ver += payload_data[i];
				}
				//���汨�� �ͻ��˽��յ����Ƿ���˱���
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
				//���汨�� �ͻ��˽��յ����Ƿ���˱���
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
			//���
			std::uint8_t padding_length = *((std::uint8_t*)t_msg_.c_str()+4);//�ĸ��ֽ�
			//�غɴ�С
			std::uint32_t payload_length = packet_lenght_ - padding_length - 1;
			const char *payload = t_msg_.c_str() + 4 + 1;

			//У��
			if (algo_ctx_.mac_len)
			{
				const char *mac = t_msg_.c_str() + packet_lenght_ + 4;
				if (!CheckMac(recv_sequence_number_, t_msg_.c_str(), packet_lenght_ + 4))
				{
					return false;
				}
			}

			//��ѹ��֮��Ļ���
			char payload_uncompress[SSH_TRANS_PACKET_MAX_SIZE] = { 0 };
			std::uint32_t payload_uncompress_length = SSH_TRANS_PACKET_MAX_SIZE;
			if (!UnCompress(payload, payload_length, payload_uncompress, payload_uncompress_length))
			{
				//��ѹʧ��
				return false;
			}
			std::uint8_t message_code = *(std::uint8_t*)(payload_uncompress);

			//�ص� payload_data
			OnHandler(message_code, payload_uncompress+1, payload_uncompress_length-1);

			//clear����״̬
			t_msg_ = "";
			packet_lenght_ = 0;
			
			return true;
		}
		
	private:
		//��ȡĬ�ϵ��㷨
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
		
		//�㷨Э��none ���߿շ���""
		Str AgreeAlgorithms(name_list server, name_list client)
		{
			Str client_algo, server_algo;
			for (int i = 0; i < client.size(); ++i)
			{
				if (client[i] != ',')
					client_algo += client[i];

				if (client[i] == ','||i==client.size()-1)
				{
					//������һ��client�ڵ�
					for (int j = 0; j < server.size(); ++j)
					{
						if (server[j] != ',')
							server_algo += server[j];

						if (server[j] == ',' || j == server.size() - 1)
						{
							//����ڵ� Э�����
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

		Str Sha1(const Str&data)
		{
			//sha-1 ����
			EVP_MD_CTX *ctx = EVP_MD_CTX_new();
			EVP_DigestInit(ctx, EVP_sha1());
			EVP_DigestUpdate(ctx, data.c_str(), data.size());
			unsigned char temp[EVP_MAX_MD_SIZE] = { 0 };
			unsigned int len = EVP_MAX_MD_SIZE;
			//�����ɹ�����1�����򷵻�0��
			int ret = EVP_DigestFinal(ctx, temp, &len);
			if (ret == 0)
				return "";
			return Str((char*)temp, len);
		}

		//ѹ�� out_len ����Ҳ����� ������󻺴� ����������
		bool Compress(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
		{
			//��ѹ��
			bool bc = false;
			if (bc)
			{
				return false;
			}
			else
			{
				if (out_len < in_len)
					return false;//���治��
				//��ѹ��
				memcpy(out, in, in_len);
				out_len = in_len;
				return true;
			}
		}

		bool UnCompress(const char*in, std::uint32_t in_len, char*out, std::uint32_t &out_len)
		{
			//��ѹ��
			bool bc = false;
			if (bc)
			{
				return false;
			}
			else
			{
				if (out_len < in_len)
					return false;//���治��
				//��ѹ��
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

		//����Э���㷨��ȡpg
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
		//��Ⱥ
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
		
		//����e��f
		//���ȿͻ������ѡȡһ������ x ����(1 < x < q)������ e = g^x mod p ���� e ������������
		//������Ҳ���ѡȡһ������ y (0 < y < q)������ f = g^y mod p
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
			//���ѡȡһ������ x ����(1 < x < q)
			BN_rand(xy, group_order * 8 - 1, 0, -1);
			//e = g^x mod p
			BN_CTX *ctx = BN_CTX_new();
			//int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p,const BIGNUM *m, BN_CTX *ctx); 
			//r = pow(a, p) % M
			BN_mod_exp(ef, g, xy, p,ctx);

			//�ͷ���Դ
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

		//����H
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
				//������Ϊ��
				return "";
			}
			Str buff;
			buff += PrintNameList(algo_ctx_.V_C);
			buff += PrintNameList(algo_ctx_.V_S);
			buff += PrintNameList(algo_ctx_.I_C);
			buff += PrintNameList(algo_ctx_.I_S);
			buff += PrintNameList(algo_ctx_.K_S);
			
			Str value_e = Mpint(algo_ctx_.e);
			//printf("e %02x %u\n", value_e.c_str(), value_e.size());
			buff += PrintNameList(value_e);

			Str value_f = Mpint(algo_ctx_.f);
			//printf("f %02x %u\n", value_f.c_str(), value_f.size());
			buff += PrintNameList(value_f);

			Str value_k = Mpint(algo_ctx_.K);
			//printf("k %02x %u\n", value_k.c_str(), value_k.size());
			buff += PrintNameList(value_k);

			/*buff += PrintNameList(Mpint(algo_ctx_.f));
			buff += PrintNameList(Mpint(algo_ctx_.K));*/
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
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(Mpint(RSA_get0_e(algo_ctx_.rsa)));
				host_key += PrintNameList(Mpint(RSA_get0_n(algo_ctx_.rsa)));
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
				host_key = PrintNameList(algo_ctx_.server_host_key_algorithms);
				host_key += PrintNameList(Mpint(DSA_get0_p(algo_ctx_.dsa)));
				host_key += PrintNameList(Mpint(DSA_get0_q(algo_ctx_.dsa)));
				host_key += PrintNameList(Mpint(DSA_get0_g(algo_ctx_.dsa)));
				host_key += PrintNameList(Mpint(DSA_get0_pub_key(algo_ctx_.dsa)));
			}
			return host_key;
		}

		//��ʼ�� ����˷������Ĺ�Կ
		bool InitPubKey(const SSHKexDHReply &dh_reply)
		{
			std::uint32_t offset = 0;
			Str ks_name;
			if (!ParserNameList(dh_reply.K_S.c_str(), dh_reply.K_S.size(), offset, ks_name))
				return false;
			if (ks_name != algo_ctx_.server_host_key_algorithms)
				return false;//��Э�̵��㷨��һ��
			//if (offset >= dh_reply.K_S.size())
			//	return false;//��
			/*
			The value for ��dss_signature_blob�� is encoded as a string containing
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
				BIGNUM *e = BigNum(str_e);
				BIGNUM *n = BigNum(str_n);
				if (algo_ctx_.rsa)
					RSA_free(algo_ctx_.rsa);
				algo_ctx_.rsa = RSA_new();
				RSA_set0_key(algo_ctx_.rsa, n, e,NULL);
				RSA_set0_factors(algo_ctx_.rsa, NULL, NULL);
				RSA_set0_crt_params(algo_ctx_.rsa, NULL, NULL, NULL);
				RSA_check_key(algo_ctx_.rsa);
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
				BIGNUM *p = BigNum(str_p);
				BIGNUM *q = BigNum(str_q);
				BIGNUM *g = BigNum(str_g);
				BIGNUM *y = BigNum(str_y);
				if (algo_ctx_.dsa)
					DSA_free(algo_ctx_.dsa);
				algo_ctx_.dsa = DSA_new();
				DSA_set0_pqg(algo_ctx_.dsa, p, q, g);
				DSA_set0_key(algo_ctx_.dsa, y,NULL);
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
			//�㷨������+�㷨��+ǩ�����ݳ���+ǩ��ֵ��
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
			//�㷨������+�㷨��+ǩ�����ݳ���+ǩ��ֵ��
			std::uint32_t offset = 0;
			Str ks_name, sign;
			if (!ParserNameList(dh_reply.S.c_str(), dh_reply.S.size(), offset, ks_name))
				return false;
			if (!ParserNameList(dh_reply.S.c_str(), dh_reply.S.size(), offset, sign))
				return false;

			if (ks_name != algo_ctx_.server_host_key_algorithms)
				return false;//��Э�̵��㷨��һ��
			if (sign.size() == 0)
				return false;//Ϊ��

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
		std::uint32_t packet_lenght_;
		
		SshTransportStatus status_;

		Str temp_;
		//��Ϣ����
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