#ifndef SIM_USE_OPENSSL
int main(int argc, char*argv[])
{
	printf("not define SIM_USE_OPENSSL \n");
	return -1;
}
#else

#include <openssl/bn.h>
#include <string.h>
#include <openssl/bio.h>

int main(int argc, char*argv[])
{

	BIGNUM*bn;
	BIO        *b;
	char a[20];
	int           ret;

	bn = BN_new();

	strcpy(a, "32");

	ret = BN_hex2bn(&bn, a);
	BN_generate_prime(bn, 10, 1, NULL, NULL,NULL,NULL);

	b = BIO_new(BIO_s_file());

	ret = BIO_set_fp(b, stdout, BIO_NOCLOSE);
	char *bndec = BN_bn2dec(bn);
	printf("bn:%s\n", bndec);
	
	OPENSSL_free(bndec);

	BN_free(bn);

	return 0;

}
#endif //! SIM_USE_OPENSSL