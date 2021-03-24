/*
* ��־������
*/
#ifndef SIM_LOGGER_HPP_
#define SIM_LOGGER_HPP_
#include <stdio.h>
#include <sstream>
#include <string>
#include <iomanip>
//ƽ̨���
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
	#include <sys/syscall.h> /*��ͷ�������*/
	namespace sim
	{
		pid_t gettid()
		{
			return syscall(SYS_gettid);
			/*������ں�*/
		}
	}
#else
	#error "��֧�ֵ�ƽ̨"
#endif

//��־��
#ifndef SIM_NO_LOGGER
	//��ʽ
	#define SIM_FORMAT_NUM(num,base,w,f)	std::setbase(base) << std::setw(w) << std::setfill(f) << num
	#define SIM_FORMAT_STR(str,w,f)			std::setw(2) << std::setfill(f) << str
	#define SIM_HEX(num) SIM_FORMAT_NUM(num,16,8,'0')
	#define SIM_FORMAT_STR0(str,w) SIM_FORMAT_STR(str,w,' ')

	//��ֹ����
	#define SIM_FUNC(lv)\
		sim::LoggerFunction LoggerFunction1263637187321356273517631263(lv,__FUNCTION__,__LINE__)
	#define SIM_FUNC_DEBUG() SIM_FUNC(sim::LDebug)
	
	//���������
	#define SIM_LOG_ADD(Stream,...) sim::Logger::GetLog().AddStream(new Stream(__VA_ARGS__))
	//����������
	#define SIM_LOG_HANDLER(max_lv,handler,userdata) SIM_LOG_ADD(sim::LogHandlerStream,handler,userdata,max_lv)

	//���ÿ���̨���
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
	//��ʽ
	#define SIM_FORMAT_NUM(num,base,w,f)	
	#define SIM_FORMAT_STR(str,w,f)			
	#define SIM_HEX(num) 
	#define SIM_FORMAT_STR0(str,w) 

	//��ֹ����
	#define SIM_FUNC(lv)
	#define SIM_FUNC_DEBUG() 

	//���������
	#define SIM_LOG_ADD(Stream,...) 
	//����������
	#define SIM_LOG_HANDLER(max_lv,handler,userdata) 

	//���ÿ���̨���
	#define SIM_LOG_CONSOLE(max_lv)

	#define SIM_LOG(lv,x)
	#define SIM_LDEBUG(x) 
	#define SIM_LINFO(x) 
	#define SIM_LWARN(x) 
	#define SIM_LERROR(x)
#endif // !SIM_NO_LOGGER

namespace sim
{
	//��־����
	enum LogLevel
	{
		LNone,
		LError,
		LWarn,
		LInfo,
		LDebug,
	};

	//��־������
	typedef void(*LoggerHandler)(LogLevel lv,
		const char* func, 
		unsigned int line,
		const char* msg,
		void* userdata);

	//��־�����
	class LogStream
	{
		//�������ƿ���
		LogStream(const LogStream &other) {};
		LogStream& operator=(const LogStream &other) {};
	public:
		LogStream(LogLevel max_level = LInfo) :max_level_(max_level)
		{}
		//����
		virtual void Input(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg)=0;
		virtual bool isVaild() { return true; };
	protected:
		//��ȡ��ǰʱ����ַ���
		virtual std::string get_time_str(const std::string& fmt = "%Y-%m-%d %H:%M:%S")
		{
			time_t t;  //��ʱ��  
			tm * local; //����ʱ��   
			char buf[128] = { 0 };
			t = time(NULL); //��ȡĿǰ��ʱ��  
			local = localtime(&t); //תΪ����ʱ��  
			strftime(buf, 64, fmt.c_str(), local);
			return buf;
		}
		//��ȡ�߳�id
		unsigned int get_this_thread_id()
		{
#ifdef OS_WINDOWS
			return::GetCurrentThreadId();
#else
			//int i = gettid();
			return  static_cast<unsigned int>( gettid());
#endif
		}
		//��ȡ������
		short get_now_milliseconds()
		{
#ifdef OS_WINDOWS
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);  //��ȡ��ǰʱ�� �ɾ�ȷ��ms
			return st.wMilliseconds;
#else
			struct timeval time;

			/* ��ȡʱ�䣬���۵�us */
			gettimeofday(&time, NULL);
			return time.tv_usec / 1000;
#endif
		}

		//��ȡ��־��������
		const char* get_lv_str(LogLevel lv)
		{
			static char LvInfo[5][8] = { "None","Error","Warning","Info","Debug" };
			return LvInfo[lv];
		}
	protected:
		//�����־���� ��������Ĵ��������־�����
		LogLevel max_level_;
	};

	//������������
	class LogHandlerStream :public LogStream
	{
	public:
		LogHandlerStream(LoggerHandler handler,
			void* userdata=NULL,LogLevel max_level = LInfo) 
			:LogStream(max_level)
			, handler_(NULL), userdata_(NULL)
		{}
		//����
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

	// console ����̨���
	class LogConsoleStream :public LogStream
	{
	public:
		LogConsoleStream(LogLevel max_level = LInfo) :LogStream(max_level)
		{
#ifdef OS_WINDOWS
			std_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
		}

		//����
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
		///033[1;31;40m �����ɫ�ַ� /033[0m
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

	//��־�ļ����(���ļ���)
	class LogFileStream :public LogStream
	{
	public:
		LogFileStream(LogLevel max_level, const std::string& dir,
			const std::string& name,
			const std::string& ext = "log",//��׺
			long max_byte = 1024 * 1024 * 1024,//����С 1G
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
				//ˢ��
				fflush(fp_);
				fclose(fp_);
			}
		}
		//����
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
		// ȥ��ǰ��ո�
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
		//�����ļ���
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
					//_access()�ж��ļ��Ƿ���ڣ����ж��ļ��Ƿ��д
					//int _access(const char *pathname, int mode);
					//pathname: �ļ�·����Ŀ¼·��;  mode: ����Ȩ�ޣ��ڲ�ͬϵͳ�п����ò��ܵĺ궨�����¶��壩
					//��pathnameΪ�ļ�ʱ��_access�����ж��ļ��Ƿ���ڣ����ж��ļ��Ƿ������modeֵָ����ģʽ���з��ʡ�
					//��pathnameΪĿ¼ʱ��_accessֻ�ж�ָ��Ŀ¼�Ƿ���ڣ���Windows NT��Windows 2000�У����е�Ŀ¼��ֻ�ж�дȨ��
					//0����>ֻ����ļ��Ƿ����
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
		//��ȡ�ļ����
		FILE* get_file_handle()
		{
			if (NULL == fp_)
			{
				create_dir(dir_);
				//��
				std::string filename = dir_ + "/" + name_ + get_time_str("_%Y%m%d%H%M%S.") + ext_;
				fp_ = fopen(filename.c_str(), "w");
			}
			else
			{
				//����������С ���´�
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
		//�ļ����
		FILE* fp_;
		//�Ƿ�ÿһ��д���ʱ������ˢ��
		bool fflush_;
	};

	class Logger
	{
		//�ڵ�
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

		//ȫ��
		Logger();
		Logger(const Logger&) {};
		Logger& operator=(const Logger&) {}
	public:
		//ȫ��
		static Logger& GetLog();
		//��־���
		void Log(LogLevel lv,
			const char* func,
			unsigned int line,
			const char* msg);
		//��ʼ��
		bool AddStream(LogStream* pStream);
		~Logger();
	private:
		CRITICAL_SECTION lock_;
		LogStreamNode* phead_;
	};

	//�����ӿ�����
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