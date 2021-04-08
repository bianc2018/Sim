/*
	ssl ��װ
*/

//#define SIM_USE_OPENSSL

#ifndef SIM_USE_OPENSSL
#define SIM_SSL_CTX_HPP_
#endif //! SIM_USE_OPENSSL

#ifndef SIM_SSL_CTX_HPP_
#define SIM_SSL_CTX_HPP_

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#define SSL_CTX_IO_RECV 0
#define SSL_CTX_IO_SEND 1
#define SSL_CTX_IO_NUM 2
namespace sim
{
	class SSLSession
	{
		friend class SSLCtx;
		//�������ƿ���
		SSLSession(const SSLSession &other) {};
		SSLSession& operator=(const SSLSession &other) {};
		SSLSession(int fd, SSL_CTX*ssl_ctx)
			:is_do_handshake_(false), fd_(fd)
		{
			/*
			BIO_new
			*/
			bio_[SSL_CTX_IO_RECV] = BIO_new(BIO_s_mem());
			bio_[SSL_CTX_IO_SEND] = BIO_new(BIO_s_mem());

			ssl_ = SSL_new(ssl_ctx);
		}
	public:
		
		~SSLSession()
		{
			if (ssl_)
			{
				SSL_free(ssl_);
			}

			/*for (int i = 0; i < SSL_CTX_IO_NUM; ++i)
			{
				if (bio_[i] != NULL)
				{
					BIO_free(bio_[i]);
					bio_[i] = NULL;
				}
			}*/
		}

		////���� �������Ļص�����
		//bool Encrypt(char *data, unsigned int datalen, SSL_DATA_FUNC func, void*pd)
		//{
		//	if (!(HandShake()))
		//	{
		//		return false;
		//	}
		//	//д�� �Ὣ���ܵ�����д��wbio 
		//	int len = SSL_write(ssl_, data, datalen);
		//	//wbio �����ȡ ����֮�������
		//	len = BIO_read(bio_[SSL_CTX_IO_SEND], data, datalen);
		//	//�ص�
		//	if (func)
		//		func(data, len, pd);
		//	return true;
		//}
		////���� �������Ļص�����
		//bool Decrypt(char *data, unsigned int datalen, SSL_DATA_FUNC func, void*pd)
		//{
		//	if (!(HandShake()))
		//	{
		//		return false;
		//	}
		//	int len = -1;
		//	//������д�� rbio
		//	len = BIO_write(bio_[SSL_CTX_IO_RECV], data, datalen);
		//	//ssl��rbio��ȡ���ݲ�����
		//	len = SSL_read(ssl_, data, datalen);
		//	//�ص�
		//	if (func)
		//		func(data, len, pd);
		//}

		//���� �������Ļص����� ��������ĳ���
		int Encrypt(const char *in, unsigned int inlen, char *out, unsigned int outlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			//д�� �Ὣ���ܵ�����д��wbio 
			int len = SSL_write(ssl_, in, inlen);
			//wbio �����ȡ ����֮�������
			len = BIO_read(bio_[SSL_CTX_IO_SEND], out, outlen);
			return len;
		}
		//���� �������Ļص�����
		int Decrypt(const char *in, unsigned int inlen, char *out, unsigned int outlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			int len = -1;
			//������д�� rbio
			len = BIO_write(bio_[SSL_CTX_IO_RECV], in, inlen);
			//ssl��rbio��ȡ���ݲ�����
			len = SSL_read(ssl_, out, outlen);

			return len;
		}

		//������������
		int InEncrypt(const char *in, unsigned int inlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			return SSL_write(ssl_, in, inlen);;
		}
		//�������
		int OutEncrypt(char *out, unsigned int outlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			return BIO_read(bio_[SSL_CTX_IO_SEND], out, outlen);
		}

		//��������
		int InDecrypt(const char *in, unsigned int inlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			return  BIO_write(bio_[SSL_CTX_IO_RECV], in, inlen);
		}
		//�������
		int OutDecrypt(char *out, unsigned int outlen)
		{
			if (!is_do_handshake_)
			{
				return -1;
			}
			return SSL_read(ssl_, out, outlen);
		}

		bool HandShake()
		{
			if (!is_do_handshake_)
			{
				int ret = SSL_set_fd(ssl_, fd_);
				printf("SSL HandShake:start\n");
				while (true)
				{
					if (SSL_is_server(ssl_))
						ret = SSL_accept(ssl_);
					else
						ret = SSL_connect(ssl_);
					/*X509 *cert;
					char *line;

					cert = SSL_get_peer_certificate(ssl_);
					if (cert != NULL) {
						printf("����֤����Ϣ:\n");
						line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
						printf("֤��: %s\n", line);
						free(line);
						line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
						printf("�䷢��: %s\n", line);
						free(line);
						X509_free(cert);
					}
					else
						printf("��֤����Ϣ��\n");*/
					if (ret != 1)
					{
						int e = SSL_get_error(ssl_, -1);
						if (e == SSL_ERROR_WANT_READ)//�ȴ�
						{
							//printf("HandShake:SSL_ERROR_WANT_READ\n");
							continue;
						}

						char buff[4 * 1024] = { 0 };
						//����
						printf("SSL_do_handshake fail ret=%d SSL_get_error=%d %s\n", ret, e, ERR_error_string(e, buff));
						ERR_print_errors_fp(stderr);
						return false;
					}
					else
					{
						break;
					}
				}
				printf("SSL��HandShake:ok\n");
				SSL_set_bio(ssl_, bio_[SSL_CTX_IO_RECV], bio_[SSL_CTX_IO_SEND]);
				is_do_handshake_ = true;
			}
			return true;
		}
	private:
		SSL *ssl_; // SSL structure used by OpenSSL

		BIO *bio_[SSL_CTX_IO_NUM]; // memory BIO used by OpenSSL

		//�Ƿ��Ѿ�������
		bool is_do_handshake_;

		int fd_;
	};

	class SSLCtx
	{
		//�������ƿ���
		SSLCtx(const SSLCtx &other) {};
		SSLCtx& operator=(const SSLCtx &other) {};
	public:
		SSLCtx(const SSL_METHOD *meth)
		{
			SSL_load_error_strings();
			SSLeay_add_ssl_algorithms();
			ssl_ctx_ = SSL_CTX_new(meth);
		}
		~SSLCtx()
		{
			if (ssl_ctx_ != (SSL_CTX*)0)
				SSL_CTX_free(ssl_ctx_);
		}

		bool SetCipher(char * cipher)
		{
			if (cipher != (char*)0)
			{
				if (SSL_CTX_set_cipher_list(ssl_ctx_, cipher))
				{
					return true;
				}
				ERR_print_errors_fp(stderr);
			}
			return false;
		}

		bool SetKeyFile(const char *pub_key_file, const char*pri_key_file)
		{
			if (pub_key_file&&pri_key_file)
			{
				/* �����û�������֤�飬 ��֤���������͸��ͻ��ˡ� ֤��������й�Կ */
				if (SSL_CTX_use_certificate_file(ssl_ctx_, pub_key_file, SSL_FILETYPE_PEM) <= 0) {
					ERR_print_errors_fp(stdout);
					return false;
				}
				/* �����û�˽Կ */
				if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, pri_key_file, SSL_FILETYPE_PEM) <= 0) {
					ERR_print_errors_fp(stdout);
					return false;
				}
				/* ����û�˽Կ�Ƿ���ȷ */
				if (!SSL_CTX_check_private_key(ssl_ctx_)) {
					ERR_print_errors_fp(stdout);
					return false;
				}
				return true;
			}
			return false;
		}

		SSLSession*NewSession(int fd)
		{
			return new SSLSession(fd, ssl_ctx_);
		}
		void DelSession(SSLSession*s)
		{
			delete s;
		}
	private:
		SSL_CTX*ssl_ctx_;
	};
}
#endif // !SIM_SSL_CTX_HPP_
