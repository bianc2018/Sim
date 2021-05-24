#ifndef SIM_USE_OPENSSL
int main(int argc, char*argv[])
{
	printf("not define SIM_USE_OPENSSL \n");
	return -1;
}
#else
#include <string.h>
#include <openssl/evp.h>
int    main()
{
	int ret, which = 1;
	EVP_CIPHER_CTX  * ctx = EVP_CIPHER_CTX_new();
	const EVP_CIPHER      *cipher;
	unsigned char        key[24], iv[8], in[80], out[108], de[100];
	int i, len, inl, outl, total = 0;

	for (i = 0; i < 24; i++)
	{
		memset(&key[i], i, 1);
	}

	for (i = 0; i < 8; i++)
	{
		memset(&iv[i], i, 1);
	}

	/*for (i = 0; i < 100; i++)
	{
		memset(&in[i], i, 1);
	}*/
	memset(&in, 'A', 79);
	in[79] = '\0';

	EVP_CIPHER_CTX_init(ctx);

	/*printf("please select :\n");

	printf("1: EVP_des_ede3_ofb\n");

	printf("2: EVP_des_ede3_cbc\n");*/

	/*scanf("%d", &which);

	if (which == 1)
		cipher = EVP_des_ede3_ofb();
	else
		cipher = EVP_des_ede3_cbc();*/
	cipher = EVP_des_ede3_cbc();

	ret = EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv);
	if (ret != 1)
	{
		printf("EVP_EncryptInit_ex err1!\n");
		return -1;
	}

	inl = 80;
	len = 0;
	EVP_EncryptUpdate(ctx, out + len, &outl, in, inl);
	len += outl;
	EVP_EncryptFinal_ex(ctx, out + len, &outl);
	len += outl;
	printf("加密结果长度：%d\n", len);

	/* 解密 */
	EVP_CIPHER_CTX_cleanup(ctx);
	EVP_CIPHER_CTX_init(ctx);
	ret = EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv);
	if (ret != 1)
	{
		printf("EVP_DecryptInit_ex err1!\n");
		return -1;
	}
	
	int block_size = EVP_CIPHER_CTX_block_size(ctx);
	printf("block_size %d\n", block_size);
	total = 0;
	for (int i = 0; i < len; i += block_size)
	{
		//total = 0;
		EVP_DecryptUpdate(ctx, de + total, &outl, out+i, block_size);
		printf("outl %d\n", outl);
		total += outl;
	}

	/*total = 0;
	EVP_DecryptUpdate(ctx, de + total, &outl, out, 44);
	total += outl;
	EVP_DecryptUpdate(ctx, de + total, &outl, out + 44, len - 44);
	total += outl;
	ret = EVP_DecryptFinal_ex(ctx, de + total, &outl);
	total += outl;*/
	ret = EVP_DecryptFinal_ex(ctx, de + total, &outl);
	total += outl;
	if (ret != 1)
	{
		EVP_CIPHER_CTX_cleanup(ctx);
		printf("EVP_DecryptFinal_ex err\n");
		return -1;
	}

	if ((total != 100) || (memcmp(de, in, 100)))
	{
		printf("err!\n");
		return -1;
	}

	EVP_CIPHER_CTX_cleanup(ctx);

	EVP_CIPHER_CTX_free(ctx);

	printf("test ok!\n");
	return 0;
}
#endif //! SIM_USE_OPENSSL