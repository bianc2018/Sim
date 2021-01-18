#ifndef SIM_THREAD_HPP_
#define SIM_THREAD_HPP_

#ifdef WIN32
#include <Windows.h>
#include <process.h>
typedef unsigned      ThRet;
#else
#include <pthread.h>
typedef void*       ThRet;
typedef void*       LPVOID;
typedef pthread_t HANDLE;
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif
#endif
namespace sim
{
	class Thread;
	typedef ThRet(*ThreadProc)(LPVOID lpParam);

//#ifdef WIN32
//	struct ThreadData
//	{
//		ThreadProc Proc;
//		LPVOID lpParam;
//	};
//	//windows 版本的需要一个中间函数退出
//	ThRet ThreadProcAndExit(LPVOID lpParam)
//	{
//		ThRet ret = 0;
//		if (lpParam )
//		{
//			ThreadData*pData = (ThreadData*)lpParam;
//			if (pData->Proc)
//			{
//				ret = pData->Proc(pData->lpParam);
//				delete pData;
//				_endthreadex(ret);
//				return ret;
//			}
//			delete pData;
//		}
//		_endthread();
//		return ret;
//	}
//#endif
//	
	
	class Thread
	{

		/*ThreadProc proc_;
		LPVOID lpparam_;*/

		HANDLE pth_;
		unsigned int th_id_;
		LPVOID lpParam_;
	protected:
		Thread(const Thread &other) {};
		Thread operator=(const Thread &other) {};
		template<typename T>
		inline void ThSwap(T &t1, T&t2)
		{
			T temp = t1;
			t1 = t2;
			t2 = temp;
		}
	public:
		Thread() 
			:pth_(INVALID_HANDLE_VALUE)
			, th_id_(-1)
			, lpParam_(NULL)
		{

		}
		Thread(ThreadProc Proc, LPVOID lpParam)
		{
			SetParam(lpParam);

#ifdef WIN32
			//pth_ = CreateThread(NULL, 0, Proc, lpParam, 0, NULL);
#if 1
			pth_ = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Proc, lpParam, 0, &th_id_);
#else
			ThreadData *p = new ThreadData();
			p->Proc = Proc;
			p->lpParam = lpParam;
			pth_ = (HANDLE)_beginthreadex(NULL, 0, ThreadProcAndExit, (void*)p, 0, &th_id_);
#endif
#else
			int err = pthread_create(&pth_, NULL, Proc, lpParam);
			if (err != 0)
			{
				pth_ = INVALID_HANDLE_VALUE;
			}
			else
			{
				th_id_ = pth_;
			}
#endif
		}
		
		virtual ~Thread()
		{
#ifdef WIN32
			CloseHandle(pth_);
#endif
			pth_ = INVALID_HANDLE_VALUE;
		}

		bool Join(ThRet*ret_val=NULL)
		{
			if (!JoinAble())
				return false;
#ifdef WIN32
			//pth = CreateThread(NULL, 0, Proc, lpParam, 0, NULL);
			WaitForSingleObject(pth_, INFINITE);
			if (ret_val)
			{
				DWORD ret = 0;
				GetExitCodeThread(pth_, (LPDWORD)&ret);
				*ret_val = (ThRet)ret;
			}
			//GetExitCodeThread
			CloseHandle(pth_);
			pth_ = INVALID_HANDLE_VALUE;
			return true;
#else
			int err = pthread_join(pth_, ret_val);
			pth_ = INVALID_HANDLE_VALUE;
			if (err != 0)
				return false;
			return true;
#endif
		}

		//GetExitCodeThread
		bool JoinAble()
		{
			if (INVALID_HANDLE_VALUE == pth_)
				return false;
			return true;
		}

		bool Detach()
		{
			if (INVALID_HANDLE_VALUE == pth_)
				return false;
#ifdef WIN32
			CloseHandle(pth_);
			pth_ = INVALID_HANDLE_VALUE;
			return true;
#else
			int err = pthread_detach(pth_);
			pth_ = INVALID_HANDLE_VALUE;
			if (err != 0)
				return false;
			return true;
#endif
		}
		
		void Swap(Thread &other)
		{
			ThSwap(pth_, other.pth_);
			ThSwap(th_id_, other.th_id_);
		}

		unsigned int GetId()
		{
			return th_id_;
		}

		static unsigned GetThisThreadId();

		void SetParam(LPVOID p)
		{
			lpParam_ = p;
		}
		LPVOID GetParam()
		{
			return lpParam_;
		}
	};
	unsigned int sim::Thread::GetThisThreadId()
	{
#ifdef WIN32
		return::GetCurrentThreadId();
#else
		return pthread_self();
#endif
	}
}
#endif