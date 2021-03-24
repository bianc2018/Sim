/*
* 日志输出组件
*/
#ifndef SIM_LOGGER_HPP_
#define SIM_LOGGER_HPP_
#include <stdio.h>
#include <sstream>
#include <string>
#include <iomanip>
//平台相关
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
	#include <io.h>
	#include <direct.h>
	#include <Windows.h>
	#ifndef SIM_ACCESS 
	#define SIM_ACCESS _access
	#endif
	//_mkdir
	#ifndef SIM_MKDIR
	#define SIM_MKDIR _mkdir
	#endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#ifndef OS_LINUX
		#define OS_LINUX
	#endif 
	#include <pthread.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/time.h>
	#include <unistd.h>
	#include <sys/stat.h>
	typedef pthread_mutex_t CRITICAL_SECTION;

	#define NONE         "\033[m"
	#define RED          "\033[0;32;31m"
	#define LIGHT_RED    "\033[1;31m"
	#define GREEN        "\033[0;32;32m"
	#define LIGHT_GREEN  "\033[1;32m"
	#define BLUE         "\033[0;32;34m"
	#define LIGHT_BLUE   "\033[1;34m"
	#define DARY_GRAY    "\033[1;30m"
	#define CYAN         "\033[0;36m"
	#define LIGHT_CYAN   "\033[1;36m"
	#define PURPLE       "\033[0;35m"
	#define LIGHT_PURPLE "\033[1;35m"
	#define BROWN        "\033[0;33m"
	#define YELLOW       "\033[1;33m"
	#define LIGHT_GRAY   "\033[0;37m"
	#define WHITE        "\033[1;37m"
	#ifndef SIM_ACCESS 
	#define SIM_ACCESS access
	#endif
	#ifndef SIM_MKDIR
	#define SIM_MKDIR(path) mkdir(path,S_IRWXU)
	#endif
	#include <sys/syscall.h> /*此头必须带上*/
	namespace sim
	{
		pid_t gettid()
		{
			return syscall(SYS_gettid);
			/*这才是内涵*/
		}
	}
#else
	#error "不支持的平台"
#endif

//日志宏
#ifndef SIM_NO_LOGGER
	//格式
	#define SIM_FORMAT_NUM(num,base,w,f)	std::setbase(base) << std::setw(w) << std::setfill(f) << num
	#define SIM_FORMAT_STR(str,w,f)			std::setw(2) << std::setfill(f) << str
	#define SIM_HEX(num) SIM_FORMAT_NUM(num,16,8,'0')
	#define SIM_FORMAT_STR0(str,w) SIM_FORMAT_STR(str,w,' ')

	//防止重名
	#define SIM_FUNC(lv)\
		sim::LoggerFunction LoggerFunction1263637187321356273517631263(lv,__FUNCTION__,__LINE__)
	#define SIM_FUNC_DEBUG() SIM_FUNC(sim::LDebug)
	
	//新增输出流
	#define SIM_LOG_ADD(Stream,...) sim::Logger::GetLog().AddStream(new Stream(__VA_ARGS__))
	//配置输出句柄
	#define SIM_LOG_HANDLER(max_lv,handler,userdata) SIM_LOG_ADD(sim::LogHandlerStream,handler,userdata,max_lv)

	//配置控制台输出
	#define SIM_LOG_CONSOLE(max_lv)\
		SIM_LOG_ADD(sim::LogConsoleStream,max_lv)

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
	//格式
	#define SIM_FORMAT_NUM(num,base,w,f)	
	#define SIM_FORMAT_STR(str,w,f)			
	#define SIM_HEX(num) 
	#define SIM_FORMAT_STR0(str,w) 

	//防止重名
	#define SIM_FUNC(lv)
	#define SIM_FUNC_DEBUG() 

	//新增输出流
	#define SIM_LOG_ADD(Stream,...) 
	//配置输出句柄
	#define SIM_LOG_HANDLER(max_lv,handler,userdata) 

	//配置控制台输出
	#define SIM_LOG_CONSOLE(max_lv)

	#define SIM_LOG(lv,x)
	#define SIM_LDEBUG(x) 
	#define SIM_LINFO(x) 
	#define SIM_LWARN(x) 
	#define SIM_LERROR(x)
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

	//日志输出流
	class LogStream
	{
		//不允许复制拷贝
		LogStream(const LogStream &other) {};
		LogStream& operator=(const LogStream &other) {};
	public:
		LogStream(LogLevel max_level = LInfo) :max_level_(max_level)
		{}
		//输入
		virtual void Input(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg)=0;
		virtual bool isVaild() { return true; };
	protected:
		//获取当前时间的字符串
		virtual std::string get_time_str(const std::string& fmt = "%Y-%m-%d %H:%M:%S")
		{
			time_t t;  //秒时间  
			tm * local; //本地时间   
			char buf[128] = { 0 };
			t = time(NULL); //获取目前秒时间  
			local = localtime(&t); //转为本地时间  
			strftime(buf, 64, fmt.c_str(), local);
			return buf;
		}
		//获取线程id
		unsigned int get_this_thread_id()
		{
#ifdef OS_WINDOWS
			return::GetCurrentThreadId();
#else
			//int i = gettid();
			return  static_cast<unsigned int>( gettid());
#endif
		}
		//获取毫秒数
		short get_now_milliseconds()
		{
#ifdef OS_WINDOWS
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);  //获取当前时间 可精确到ms
			return st.wMilliseconds;
#else
			struct timeval time;

			/* 获取时间，理论到us */
			gettimeofday(&time, NULL);
			return time.tv_usec / 1000;
#endif
		}

		//获取日志级别描述
		const char* get_lv_str(LogLevel lv)
		{
			static char LvInfo[5][8] = { "None","Error","Warning","Info","Debug" };
			return LvInfo[lv];
		}
	protected:
		//最大日志级别 大于这个的大于这个日志不输出
		LogLevel max_level_;
	};

	//设置输出句柄的
	class LogHandlerStream :public LogStream
	{
	public:
		LogHandlerStream(LoggerHandler handler,
			void* userdata=NULL,LogLevel max_level = LInfo) 
			:LogStream(max_level)
			, handler_(NULL), userdata_(NULL)
		{}
		//输入
		virtual void Input(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg)
		{
			if (lv > max_level_)
				return;
			handler_(lv, func, line, msg, userdata_);
		}
		virtual bool isVaild()
		{
			return handler_ != NULL;
		}
	private:
		LoggerHandler handler_;
		void* userdata_;
	};

	// console 控制台输出
	class LogConsoleStream :public LogStream
	{
	public:
		LogConsoleStream(LogLevel max_level = LInfo) :LogStream(max_level)
		{
#ifdef OS_WINDOWS
			std_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		}

		//输入
		virtual void Input(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg)
		{
			if (lv > max_level_)
				return;

			//printf("%-8s %-30s %4u:%s\n", LvInfo[lv], func, line, msg);
#ifdef OS_WINDOWS
			if (lv == LError)
			{
				SetConsoleTextAttribute(std_handle_, FOREGROUND_INTENSITY | FOREGROUND_RED);
				printf("%s.%03d %6u %s.%s.%u: %s\n", get_time_str().c_str(), get_now_milliseconds(),
					get_this_thread_id(),
					get_lv_str(lv), func, line, msg);
				SetConsoleTextAttribute(std_handle_,
					FOREGROUND_RED |
					FOREGROUND_GREEN |
					FOREGROUND_BLUE);
			}
			else
			{
				printf("%s.%03d %6u %s.%s.%u: %s\n", get_time_str().c_str(),
					 get_now_milliseconds(),
					get_this_thread_id(),
					get_lv_str(lv), func, line, msg);
			}
#else
		///033[1;31;40m 输出红色字符 /033[0m
			if (lv == LError)
				//printf("/033[1;31;40m  %s.%s.%u: %s /033[0m\n", LvInfo[lv], func, line, msg);
				printf(RED"%s.%03d %6u %s.%s.%u: %s"NONE"\n", get_time_str().c_str(),
					get_now_milliseconds(),
				get_this_thread_id(),
					get_lv_str(lv), func, line, msg);
			else
				printf("%s.%03d %6u %s.%s.%u: %s\n", get_time_str().c_str(),
					get_now_milliseconds(),
					get_this_thread_id(),
					get_lv_str(lv), func, line, msg);
#endif
		}
		virtual bool isVaild()
		{
			return true;
		}
	private:
#ifdef OS_WINDOWS
		HANDLE std_handle_;
#endif
	};

	//日志文件输出(单文件的)
	class LogFileStream :public LogStream
	{
	public:
		LogFileStream(LogLevel max_level, const std::string& dir,
			const std::string& name,
			const std::string& ext = "log",//后缀
			long max_byte = 1024 * 1024 * 1024,//最大大小 1G
			bool f = true
		)
			:LogStream(max_level),
			fp_(NULL),
			dir_(wipe_space(dir)),
			name_(name),
			ext_(ext),
			max_byte_(max_byte),
			fflush_(f)
		{

		}
		~LogFileStream()
		{
			if (fp_)
			{
				//刷新
				fflush(fp_);
				fclose(fp_);
			}
		}
		//输入
		virtual void Input(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg)
		{
			if (lv > max_level_)
				return;
			FILE* pfile = get_file_handle();
			if (NULL == pfile)
			{
				return;
			}
			fprintf(pfile,"%s.%03d %6u %s.%s.%u: %s\n", get_time_str().c_str(),
				get_now_milliseconds(),
				get_this_thread_id(),
				get_lv_str(lv), func, line, msg);

			if(fflush_)
				fflush(pfile);
		}
	private:
		// 去除前后空格
		std::string wipe_space(const std::string&path)
		{
			int start = 0, end = path.size();
			for (int i = 0; i < path.size(); ++i)
			{
				if (path[i] == ' ' || path[i] == '\t' || path[i] == '\r' || path[i] == '\n')
				{
					++start;
				}
				else
				{
					break;
				}
			}
			for (int i = path.size(); i>=0; --i)
			{
				if (path[i] == ' ' || path[i] == '\t' || path[i] == '\r' || path[i] == '\n')
				{
					--end;
				}
				else
				{
					break;
				}
			}
			return path.substr(start, end - start - 1);
		}
		//创建文件夹
		bool create_dir(const std::string &dir)
		{
			if (SIM_ACCESS(dir.c_str(), 0) ==0)
			{
				return true;
			}

			int len = dir.length();
			char tmpDirPath[256] = { 0 };
			for (int i = 0; i < len; i++)
			{
				tmpDirPath[i] = dir[i];
				if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/')
				{
					//_access()判断文件是否存在，并判断文件是否可写
					//int _access(const char *pathname, int mode);
					//pathname: 文件路径或目录路径;  mode: 访问权限（在不同系统中可能用不能的宏定义重新定义）
					//当pathname为文件时，_access函数判断文件是否存在，并判断文件是否可以用mode值指定的模式进行访问。
					//当pathname为目录时，_access只判断指定目录是否存在，在Windows NT和Windows 2000中，所有的目录都只有读写权限
					//0——>只检查文件是否存在
					if (SIM_ACCESS(tmpDirPath, 0) == -1)
					{
						int ret = SIM_MKDIR(tmpDirPath);
						if (ret == -1)
							return false;
						return true;
					}
					else
					{
						continue;
					}
				}
			}

			if (SIM_ACCESS(tmpDirPath, 0) == -1)
			{
				int ret = SIM_MKDIR(tmpDirPath);
				if (ret == -1)
					return false;
				return true;
			}
			else
			{
				return true;
			}
		}
		//获取文件句柄
		FILE* get_file_handle()
		{
			if (NULL == fp_)
			{
				create_dir(dir_);
				//打开
				std::string filename = dir_ + "/" + name_ + get_time_str("_%Y%m%d%H%M%S.") + ext_;
				fp_ = fopen(filename.c_str(), "w");
			}
			else
			{
				//超过了最大大小 重新打开
			/*	long len = ftell(fp_);
				printf("len %ld\n", len);*/
				if (ftell(fp_) >= max_byte_)
				{
					fflush(fp_);
					fclose(fp_);
					fp_ = NULL;
					return get_file_handle();
				}
			}
			return fp_;
		}
	private:
		std::string dir_;
		std::string name_;
		std::string ext_;
		long max_byte_;
		//文件句柄
		FILE* fp_;
		//是否每一次写入的时候总是刷新
		bool fflush_;
	};

	class Logger
	{
		//节点
		class LogStreamNode
		{
		public:
			LogStream *pStream;
			LogStreamNode* pNext;
			LogStreamNode(LogStream* pS)
				:pStream(pS), pNext(NULL)
			{

			}
		};

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
		//初始化
		bool AddStream(LogStream* pStream);
		~Logger();
	private:
		CRITICAL_SECTION lock_;
		LogStreamNode* phead_;
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
		:phead_(NULL)
	{
#ifdef OS_WINDOWS
		InitializeCriticalSection(&lock_);
#else
		pthread_mutex_init(&lock_, NULL);
#endif
	}
	Logger& Logger::GetLog()
	{
		static Logger glog;
		return glog;
	}
	inline void Logger::Log(LogLevel lv, const char* func,
		unsigned int line, const char* msg)
	{
#ifdef OS_WINDOWS
		EnterCriticalSection(&lock_);
#else
		pthread_mutex_lock(&lock_);
#endif
		LogStreamNode* pTemp = phead_;
		while (pTemp)
		{
			pTemp->pStream->Input(lv, func, line, msg);
			pTemp = pTemp->pNext;
		}
#ifdef OS_WINDOWS
		LeaveCriticalSection(&lock_);
#else
		pthread_mutex_unlock(&lock_);
#endif
	}
	inline bool Logger::AddStream(LogStream* pStream)
	{

		if (pStream && pStream->isVaild())
		{
#ifdef OS_WINDOWS
			EnterCriticalSection(&lock_);
#else
			pthread_mutex_lock(&lock_);
#endif
			if (NULL == phead_)
			{
				phead_ = new LogStreamNode(pStream);
#ifdef OS_WINDOWS
				LeaveCriticalSection(&lock_);
#else
				pthread_mutex_unlock(&lock_);
#endif
				return true;
			}

			LogStreamNode* pTemp = phead_;
			while (pTemp->pNext)
			{
				pTemp = pTemp->pNext;
			}
			pTemp->pNext = new LogStreamNode(pStream);
#ifdef OS_WINDOWS
			LeaveCriticalSection(&lock_);
#else
			pthread_mutex_unlock(&lock_);
#endif
			return true;
		}
		return false;
	}
	inline Logger::~Logger()
	{
#ifdef OS_WINDOWS
		EnterCriticalSection(&lock_);
#else
		pthread_mutex_lock(&lock_);
#endif
		LogStreamNode* pTemp = phead_;
		while (pTemp)
		{
			LogStreamNode* pn = pTemp->pNext;
			delete pTemp->pStream;
			delete pTemp;
			pTemp = pn;
		}
#ifdef OS_WINDOWS
		LeaveCriticalSection(&lock_);
#else
		pthread_mutex_unlock(&lock_);
#endif
#ifdef OS_WINDOWS
		DeleteCriticalSection(&lock_);
#else
		pthread_mutex_destroy(&lock_);
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