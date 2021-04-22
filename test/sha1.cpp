#include "Test.hpp"
#include "Sha1.hpp"
SIM_TEST(Sha1)
{
	const int buff_size = 100;
	char buff[buff_size] = {0};
	const char input1[] = "hello world";
	const char result1[] = "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed";
	sim::Sha1::SHA1(input1, ::strlen(input1), buff, buff_size);
	SIM_TEST_IS_EQUAL(buff, result1);
}
SIM_TEST_MAIN(sim::noisy)