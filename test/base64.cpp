#include "Test.hpp"
#include "Crypto/Base64.hpp"
SIM_TEST(Base64)
{
	const int buff_size = 100;
	char buff[buff_size] = { 0 };
	const char str[] = "hello world";
	const char base64[] = "aGVsbG8gd29ybGQ=";
	unsigned len=sim::Base64::encode((const unsigned char*)str, ::strlen(str), buff, buff_size);
	SIM_TEST_IS_NOT_EQUAL(0, len);
	buff[len] = 0;
	SIM_TEST_IS_EQUAL(buff, base64);
	
	len=sim::Base64::decode(base64, ::strlen(base64), (unsigned char*)buff, buff_size);
	SIM_TEST_IS_NOT_EQUAL(0, len);
	buff[len] = 0;
	SIM_TEST_IS_EQUAL(buff, str);

	SIM_TEST_IS_EQUAL(0, sim::Base64::encode((const unsigned char*)str, ::strlen(str), buff, 1));
}
SIM_TEST_MAIN(sim::noisy)