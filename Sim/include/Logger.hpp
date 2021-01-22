/*
* 日志输出组件
*/
#ifndef SIM_LOGGER_HPP_
#define SIM_LOGGER_HPP_
#include <stdio.h>
#include <sstream>
#include <iomanip>

//平台相关
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
	#include <Windows.h>

#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif 
	#define NONE         "/033[m"
	#define RED          "/033[0;32;31m"
	#define LIGHT_RED    "/033[1;31m"
	#define GREEN        "/033[0;32;32m"
	#define LIGHT_GREEN  "/033[1;32m"
	#define BLUE         "/033[0;32;34m"
	#define LIGHT_BLUE   "/033[1;34m"
	#define DARY_GRAY    "/033[1;30m"
	#define CYAN         "/033[0;36m"
	#define LIGHT_CYAN   "/033[1;36m"
	#define PURPLE       "/033[0;35m"
	#define LIGHT_PURPLE "/033[1;35m"
	#define BROWN        "/033[0;33m"
	#define YELLOW       "/033[1;33m"
	#define LIGHT_GRAY   "/033[0;37m"
	#define WHITE        "/033[1;37m"
#else
	#error "不支持的平台"
#endif

//日志宏
#ifndef SIM_NO_LOGGER
	//格式
	#define SIM_FORMAT_NUM(num,base,w,f)	std::setbase(base) << std::setw(w) << std::setfill(f) << num
	#define SIM_FORMAT_STR(str,w,f)			std::setw(2) << std::setfill(f) << str
	#define SIM_HEX(num) SIM_FORMAT_NUM(num,16,8,'0');
	#define SIM_FORMAT_STR0(str,w) SIM_FORMAT_STR(str,w,' ')

	//防止重名
	#define SIM_FUNC(lv)\
		sim::LoggerFunction LoggerFunction1263637187321356273517631263(lv,__FUNCTION__,__LINE__)
	#define SIM_FUNC_DEBUG() SIM_FUNC(sim::LDebug)
	//配置
	#define SIM_LOG_CONFIG(max_lv,handler,userdata)\
	{\
		sim::Logger::GetLog().SetMaxLogLevel(max_lv);\
		sim::Logger::GetLog().SetHandler(handler,userdata);\
	}
	#define SIM_LOG(lv,x)\
	{\
		std::ostringstream oss;\
		oss<<x;\
		sim::Logger::GetLog().Log(lv,__FUNCTION__,__LINE__,oss.str().c_str());\
	}
	#define SIM_LDEBUG(x) SIM_LOG(sim::LDebug,x)
	#define SIM_LINFO(x) SIM_LOG(sim::LInfo,x)
	#define SIM_LWARN(x) SIM_LOG(sim::LWarn,x)
	#define SIM_LERROR(x) SIM_LOG(sim::LError,x)
#else
	#define SIM_FORMAT_NUM(num,base,w,f)	
	#define SIM_FORMAT_STR(str,w,f)			
	#define SIM_HEX(num) 
	#define SIM_FORMAT_STR0(str,w) 

	#define SIM_FUNC(lv)
	#define SIM_FUNC_DEBUG() SIM_FUNC(sim::LDebug)
	#define SIM_LOG_CONFIG(max_lv,handler,userdata)
	#define SIM_LOG(lv,x) 
	#define SIM_LDEBUG(x) SIM_LOG(sim::LDebug,x)
	#define SIM_LINFO(x) SIM_LOG(sim::LInfo,x)
	#define SIM_LWARN(x) SIM_LOG(sim::LWarn,x)
	#define SIM_LERROR(x) SIM_LOG(sim::LError,x)
#endif // !SIM_NO_LOGGER

namespace sim
{
	//日志级别
	enum LogLevel
	{
		LNone,
		LError,
		LWarn,
		LInfo,
		LDebug,
	};

	//日志输出句柄
	typedef void(*LoggerHandler)(LogLevel lv,
		const char* func, 
		unsigned int line,
		const char* msg,
		void* userdata);

	class Logger
	{
		//全局
		Logger();
		Logger(const Logger&) {};
		Logger& operator=(const Logger&) {}
	public:
		//全局
		static Logger& GetLog();
		//日志输出
		void Log(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg);
		//设置句柄
		bool SetHandler(LoggerHandler handler, void* userdata);
		//设置最大日志级别
		bool SetMaxLogLevel(LogLevel max_level);
	private:
		//默认打印
		void DefaultLog(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg,
			void*userdata);
	private:
		//日志参数
		//最大日志级别 大于这个的大于这个日志不输出
		LogLevel max_level_;
		//日志输出句柄
		LoggerHandler handler_;
		//用户数据
		void* userdata_;
#ifdef OS_WINDOWS
		HANDLE std_handle_;
#endif
	};

	//函数接口输入
	class LoggerFunction
	{
	public:
		LoggerFunction(LogLevel lv, const char* func,unsigned int line);
		~LoggerFunction();
	private:
		LogLevel lv_;
		const char* func_;
		unsigned int line_;
	};

	inline Logger::Logger()
		:max_level_(LError)
		, handler_(NULL)
		, userdata_(NULL)
		, std_handle_ (NULL)
	{
		std_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	Logger& sim::Logger::GetLog()
	{
		static Logger glog;
		return glog;
	}
	inline void Logger::Log(LogLevel lv, const char* func,
		unsigned int line, const char* msg)
	{
		if (lv > max_level_)
			return;
		if (handler_)
		{
			handler_(lv, func, line, msg, userdata_);
		}
		else
		{
			DefaultLog(lv, func, line, msg, userdata_);
		}
		return ;
	}

	inline bool Logger::SetHandler(LoggerHandler handler, void* userdata)
	{
		handler_ = handler;
		userdata_ = userdata;
		return true;
	}
	inline bool Logger::SetMaxLogLevel(LogLevel max_level)
	{
		max_level_ = max_level;
		return true;
	}
	inline void Logger::DefaultLog(LogLevel lv, 
		const char* func,
		unsigned int line,
		const char* msg,void* userdata)
	{
		static char LvInfo[5][8] = { "None","Error","Warning","Info","Debug"};
		//printf("%-8s %-30s %4u:%s\n", LvInfo[lv], func, line, msg);
#ifdef OS_WINDOWS
		if (lv == LError)
		{
			SetConsoleTextAttribute(std_handle_, FOREGROUND_INTENSITY | FOREGROUND_RED);
			printf("%s.%s.%u: %s\n", LvInfo[lv], func, line, msg);
			SetConsoleTextAttribute(std_handle_,
				FOREGROUND_RED |
				FOREGROUND_GREEN |
				FOREGROUND_BLUE);
		}
		else
		{
			printf("%s.%s.%u: %s\n", LvInfo[lv], func, line, msg);
		}
#else
		///033[1;31;40m 输出红色字符 /033[0m
		if (lv == LError)
			printf("/033[1;31;40m  %s.%s.%u: %s /033[0m\n", LvInfo[lv], func, line, msg);
		else
			printf("%s.%s.%u: %s\n", LvInfo[lv], func, line, msg);
#endif
	}

	sim::LoggerFunction::LoggerFunction(LogLevel lv, 
		const char* func, unsigned int line)
		:lv_(lv),func_(func),line_(line)
	{
		std::ostringstream oss; 
		oss << "Enter Function:" << func_;
		sim::Logger::GetLog().Log(lv_, func_, line_, oss.str().c_str());
	}
	sim::LoggerFunction::~LoggerFunction()		
	{
		std::ostringstream oss;
		oss << "Lever Function:" << func_;
		sim::Logger::GetLog().Log(lv_, func_, line_, oss.str().c_str());
	}
}
#endif // !SIM_LOGGER_HPP_