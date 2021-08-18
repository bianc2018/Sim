/*
	测试函数
	源自 QUnit.hpp - a simple unit test framework for C++
*/
#ifndef SIM_TEST_HPP_
#define SIM_TEST_HPP_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#ifndef OS_WINDOWS
	#define OS_WINDOWS
#endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#ifndef OS_LINUX
	#define OS_LINUX
#endif  
#endif
#include <ctime>
#include <sstream>
#include <string>
#include <iostream>

#define SIM_TEST_COMPARE(compare,result,expr1,expr2,bassert) {                 \
        std::stringstream s1, s2;                                   \
        s1 << std::boolalpha << (expr1);                            \
        s2 << std::boolalpha << (expr2);                            \
		++test_num_;\
        if(!::sim::UnitTest::Instance().evaluate(this,                                             \
            compare, result, s1.str(), s2.str(), #expr1, #expr2,    \
            __FILE__, __LINE__))\
		{\
			this->is_ok_=false;\
			if(bassert) {\
				::sim::UnitTest::Instance().output()\
				<<"CASE "<<case_name_<<" ASSERT by test "<<test_num_<<"\n";\
				 return;\
				}\
		}\
    }

//不中断
#define SIM_TEST_IS_EQUAL(expr1,expr2)     SIM_TEST_COMPARE(true,true,expr1,expr2,false)
#define SIM_TEST_IS_NOT_EQUAL(expr1,expr2) SIM_TEST_COMPARE(true,false,expr1,expr2,false)
#define SIM_TEST_IS_TRUE(expr)             SIM_TEST_COMPARE(false,true,expr,true,false)
#define SIM_TEST_IS_FALSE(expr)            SIM_TEST_COMPARE(false,true,expr,false,false)
#define SIM_TEST_IS_NULL(expr)             SIM_TEST_IS_TRUE(NULL==expr)
#define SIM_TEST_IS_NOT_NULL(expr)         SIM_TEST_IS_FALSE(NULL==expr)
//ASSERT 中断
#define SIM_ASSERT_IS_EQUAL(expr1,expr2)     SIM_TEST_COMPARE(true,true,expr1,expr2,true)
#define SIM_ASSERT_IS_NOT_EQUAL(expr1,expr2) SIM_TEST_COMPARE(true,false,expr1,expr2,true)
#define SIM_ASSERT_IS_TRUE(expr)             SIM_TEST_COMPARE(false,true,expr,true,true)
#define SIM_ASSERT_IS_FALSE(expr)            SIM_TEST_COMPARE(false,true,expr,false,true)
#define SIM_ASSERT_IS_NULL(expr)             SIM_ASSERT_IS_TRUE(NULL==expr)
#define SIM_ASSERT_IS_NOT_NULL(expr)         SIM_ASSERT_IS_FALSE(NULL==expr)

#define SIM_TEST_CLASS(name) ___SIM_TEST_##name
#define SIM_ADD_TEST_CLASS(class_name) ::sim::UnitTest::Instance().add(new class_name())

#define SIM_TEST(name) \
class SIM_TEST_CLASS(name):public ::sim::UnitTestCase\
{\
public:\
	SIM_TEST_CLASS(name)()\
		: ::sim::UnitTestCase(#name)\
	{}\
	virtual void TestBody();\
private:\
	static bool add_method;\
\
};\
bool SIM_TEST_CLASS(name)::add_method = SIM_ADD_TEST_CLASS(SIM_TEST_CLASS(name));\
void SIM_TEST_CLASS(name)::TestBody()

#define SIM_TEST_LEVEL(lv) ::sim::UnitTest::Instance().verboseLevel(lv)
#define SIM_TEST_RUN_ALL(c,v) ::sim::UnitTest::Instance().run(c,v)
#define SIM_TEST_MAIN(lv) int main(int argc,char*argv[]){\
		SIM_TEST_LEVEL(lv);\
		return SIM_TEST_RUN_ALL(argc,argv);\
    }

namespace sim
{
	enum { silent, quiet, normal, verbose, noisy };
	
	class UnitTest;

	//用例基础
	class UnitTestCase
	{
		friend class UnitTest;
	public:
		UnitTestCase(std::string case_name) :
			case_name_(case_name), test_num_(0), is_ok_(true)
		{
		};
		virtual ~UnitTestCase() {};
		//环境初始化
		virtual void SetUp() {}

		//测试体
		virtual void TestBody() = 0;

		//环境释放
		virtual void TearDown() {}
	protected:
		std::string case_name_;
		unsigned int test_num_;
		bool is_ok_;
	};

	//用例列表
	struct TestCaseNode
	{
		UnitTestCase*pCase;
		TestCaseNode *pNext;

		TestCaseNode(UnitTestCase*p):pCase(p), pNext(NULL)
		{}
	};

	class UnitTest {
		UnitTest(std::ostream & out, int verboseLevel);
	public:
		virtual int run(int argc, char *argv[]);

		virtual bool add(UnitTestCase*tcase);
	public:
		static UnitTest&Instance();

		virtual ~UnitTest();

		void verboseLevel(int level);
		int  verboseLevel();

		void printStatus();

		int  errors() const;

		bool evaluate(UnitTestCase* tcase,bool,bool,
			std::string, std::string, std::string, std::string,
			const char *, int);

		std::ostream & output() { return out_; }

		//获取当前的毫秒时间
		unsigned long long get_current_ms()
		{
#ifdef OS_WINDOWS
			
			SYSTEMTIME wtm;
			GetLocalTime(&wtm);
			
			return ::time(NULL)*1000+wtm.wMilliseconds;
#endif
#ifdef OS_LINUX
			struct timeval tp;
			::memset(&tp, 0, sizeof(tp));
			gettimeofday(&tp, NULL);
			return (unsigned long long)tp.tv_sec * 1000 + 
				(double)tp.tv_usec/1000;
#endif
			return 0;
		}
	private:
		int verboseLevel_;
		int errors_;
		int tests_;
		std::ostream & out_;
		TestCaseNode *pStart;
		unsigned int test_case_size_;
		//使用的时间
		unsigned long long used_time_ms_;
	};
	
	inline UnitTest::UnitTest(std::ostream & out, int verboseLevel)
		: verboseLevel_(verboseLevel), errors_(0), tests_(0), out_(out),
		pStart(NULL), used_time_ms_(0), test_case_size_(0)
	{
	}

	inline int UnitTest::run(int argc, char * argv[])
	{
		TestCaseNode *pt = pStart;
		int test_case_size = 1;
		while (pt)
		{
			if (pt->pCase)
			{
				unsigned long long use_time_ms = get_current_ms();
				if (verboseLevel_ > quiet)
				{
					out_ << "-----------------------------------------\n";
					out_ << "START(" << float(test_case_size / float(test_case_size_)) * 100 << "% "
						<< test_case_size << "/" << test_case_size_ << ") "
						<< pt->pCase->case_name_ << "\n";
				}
				
				pt->pCase->SetUp();
				pt->pCase->TestBody();
				pt->pCase->TearDown();
				use_time_ms = get_current_ms() - use_time_ms;

				if (verboseLevel_ > quiet)
				{
					out_ << "END   " << pt->pCase->case_name_ << "[" << (pt->pCase->is_ok_ ? "OK" : "FAILED") << "]"
						<< " use " << use_time_ms << " ms\n";
					out_ << "-----------------------------------------\n";
				}
				used_time_ms_ += use_time_ms;
				++test_case_size;

			}
			pt = pt->pNext;
		}
		if (verboseLevel_ >= quiet)
			printStatus();
#if _DEBUG
#if _WIN32
		getchar();
#endif
#endif
		return errors();
	}

	inline bool UnitTest::add(UnitTestCase * tcase)
	{
		if (tcase)
		{
			++test_case_size_;
			TestCaseNode *pt = pStart;
			if (NULL == pt)
			{
				pStart = new TestCaseNode(tcase);
				return true;
			}
			while (pt->pNext)
			{
				pt = pt->pNext;
			}
			pt->pNext = new TestCaseNode(tcase);
			return true;
		}
		else
		{
			out_ << "add test case fail,is NULL";
			return false;
		}
	}

	inline UnitTest & UnitTest::Instance()
	{
		static UnitTest gtest(std::cerr, noisy);
		return gtest;
		// TODO: 在此处插入 return 语句
	}

	inline UnitTest::~UnitTest() {
		TestCaseNode *pt = pStart;
		pStart = NULL;
		while (pt)
		{
			if (pt->pCase)
			{
				delete pt->pCase;
			}
			TestCaseNode *ptt = pt;
			pt = pt->pNext;
			delete ptt;
		}
	}

	inline void UnitTest::verboseLevel(int level) {
		verboseLevel_ = level;
	}

	inline int UnitTest::verboseLevel() {
		return verboseLevel_;
	}

	inline void UnitTest::printStatus() {
		out_ << "Testing " << (errors_ ? "FAILED" : "OK") << " ("
			<< tests_ << " tests, " << (tests_ - errors_) << " ok, "
			<< errors_ << " failed)" 
			<<" used "<<used_time_ms_<<" ms"<< std::endl;
	}

	inline int UnitTest::errors() const {
		return errors_;
	}

	inline bool UnitTest::evaluate(UnitTestCase* tcase,
		bool compare , bool result,
		std::string val1, std::string val2,
		std::string str1, std::string str2,
		const char * file, int line) {

		bool ok = result ? (val1 == val2) : (val1 != val2);
		tests_ += 1;
		errors_ += ok ? 0 : 1;

		if ((ok && !(verboseLevel_ > normal)) || verboseLevel_ == silent)
		{
			return ok;
		}

		out_ << "\tCASE "<< tcase->case_name_<<" "<<tcase->test_num_<<" L("<< line<<")"
			<<":"<<(ok ? "PASS " : "FAILED " );
		//out_ << file << (ok ? ";" : ":") << line << ": ";
		if(!ok)
		if (compare) {
			const std::string cmp = (result ? "==" : "!=");
			out_ <<"\n\t\t at file "<< file <<"["<< line <<"]"
				<<" compare {" << str1 << "} " << cmp << " {" << str2 << "} "
				<< "got {\"" << val1 << "\"} " << cmp << " {\"" << val2 << "\"}";
		}
		else {
			out_ << "\n\t\t at file " << file << "[" << line << "]"
				 << " evaluate {" << str1 << "} = " << val1;
		}
		out_ << std::endl;
		return ok;
	}
}
#endif