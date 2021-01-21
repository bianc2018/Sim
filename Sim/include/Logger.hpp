/*
* 日志输出组件
*/
#ifndef SIM_LOGGER_HPP_
#define SIM_LOGGER_HPP_
#include <stdio.h>
#include <sstream>

//日志宏
#ifndef SIM_NO_LOGGER
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
	{
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
		printf("%s.%s.%u: %s\n", LvInfo[lv], func, line, msg);
		//printf("%s\n", msg);
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