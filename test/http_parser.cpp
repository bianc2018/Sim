#include "Test.hpp"
#include "HttpParser.hpp"
SIM_TEST(HttpUrlParser)
{
	sim::HttpUrl out;
	SIM_TEST_IS_FALSE(sim::HttpParser::ParserUrl("", out));
	SIM_TEST_IS_TRUE(sim::HttpParser::ParserUrl("www.baidu.com", out));
	SIM_TEST_IS_EQUAL("http", out.scheme);
	SIM_TEST_IS_EQUAL("www.baidu.com", out.host);
	SIM_TEST_IS_EQUAL(80, out.port);
	SIM_TEST_IS_EQUAL("/", out.path);

	SIM_TEST_IS_TRUE(sim::HttpParser::ParserUrl("www.baidu.com:90", out));
	SIM_TEST_IS_EQUAL("http", out.scheme);
	SIM_TEST_IS_EQUAL("www.baidu.com", out.host);
	SIM_TEST_IS_EQUAL(90, out.port);
	SIM_TEST_IS_EQUAL("/", out.path);

	SIM_TEST_IS_TRUE(sim::HttpParser::ParserUrl("www.baidu.com/aaa", out));
	SIM_TEST_IS_EQUAL("http", out.scheme);
	SIM_TEST_IS_EQUAL("www.baidu.com", out.host);
	SIM_TEST_IS_EQUAL(80, out.port);
	SIM_TEST_IS_EQUAL("/aaa", out.path);

	SIM_TEST_IS_TRUE(sim::HttpParser::ParserUrl("test://www.baidu.com:999/aaa", out));
	SIM_TEST_IS_EQUAL("test", out.scheme);
	SIM_TEST_IS_EQUAL("www.baidu.com", out.host);
	SIM_TEST_IS_EQUAL(999, out.port);
	SIM_TEST_IS_EQUAL("/aaa", out.path);
}
SIM_TEST_MAIN(sim::noisy)