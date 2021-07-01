/*
*	��װƽ̨��ؽӿ�
*/
#ifndef SIM_SYSTEM_HPP_
#define SIM_SYSTEM_HPP_

//ƽ̨�����
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
	#define OS_WINDOWS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  
#endif
#include <Windows.h>
#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
	#define OS_LINUX
#endif
    #include <unistd.h>  
    #include <stdio.h>
	#include <string.h>
	#include <sys/types.h>
	#include <wait.h>
#else
	#error "��֧�ֵ�ƽ̨"
#endif
#include <stdlib.h>
namespace sim
{
	//��׼���
	//typedef void(SystemShellStdOut)(SystemShell*shell, const char*data, unsigned int len);
	//��׼�������
	//typedef void(SystemShellStdError)(SystemShell*shell, const char*data, unsigned int len);
	//���н���
	class SystemShell
	{
	public:
		SystemShell(): isStop_(true), exit_code_(-1)
		{
		}
		SystemShell(const char*username, const char*password) : isStop_(true), exit_code_(-1)
		{
			Start(username, password);
		}
		~SystemShell()
		{
			Stop(true);
		}

		bool Start(const char*username, const char*password
		,const char*shell=NULL, const char*command=NULL)
		{
			if (!IsStoped())
				return false;
			if (_Start(username, password, shell, command))
			{
				isStop_ = false;
				return true;
			}
			return false;
		}
		bool IsStoped()
		{
			return isStop_;
		}
		bool Stop(bool forced=false)
		{
			if (IsStoped())
				return false;
			if (_Stop(forced))
			{
				isStop_ = true;
				return true;
			}
			return false;
		}
		//��׼����
		bool StdIn(const char*data, unsigned int len)
		{
			if (IsStoped())
				return false;
			return _StdIn(data, len);
		}
		int ReadStdOut(char*buff, unsigned int buff_len)
		{
			if (IsStoped())
				return -1;
			return _ReadStdOut(buff, buff_len);
		}
		int ReadStdError(char*buff, unsigned int buff_len)
		{
			if (IsStoped())
				return -1;
			return _ReadStdError(buff, buff_len);
		}
		unsigned int exit_code()
		{
			return exit_code_;
		}
#if defined(OS_WINDOWS)
	private:
		bool _Start(const char*username, const char*password, const char*shell, const char*command)
		{
			//ͨ��
			//this->shell
			this_write_=shell_read_=shell_write_=this_read_ = err_shell_write_ = err_this_read_ =NULL;
			token_ = NULL;

			//��ȫ���ԵĶ���
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.lpSecurityDescriptor = 0;
			sa.bInheritHandle = TRUE;

			//�����ܵ�
			if (FALSE == CreatePipe(&shell_read_, &this_write_, &sa, 0)
				|| FALSE == CreatePipe(&this_read_, &shell_write_, &sa, 0)
				|| FALSE == CreatePipe(&err_this_read_, &err_shell_write_, &sa, 0))
			{
				
				//�ر��Ѿ��򿪵�ͨ��
				CloseAllPipe();
				return false;
			}

			//���Cmd FullPath
			char  strShellPath[MAX_PATH] = { 0 };
			if (NULL == shell)
			{
				GetSystemDirectoryA(strShellPath, MAX_PATH);  //C:\windows\system32
			   //C:\windows\system32\cmd.exe
				strcat(strShellPath, "\\cmd.exe");
				shell = strShellPath;
			}

			memset((void *)&pi, 0, sizeof(pi));

			STARTUPINFO          si = { 0 };
			memset((void *)&si, 0, sizeof(si));
			si.cb = sizeof(STARTUPINFO);  //��Ҫ
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			//��׼��������ʹ�������ض��򵽹ܵ�
			si.hStdInput = shell_read_;                         
			si.hStdOutput = shell_write_;
			si.hStdError = err_shell_write_;
			//���ش���
			si.wShowWindow = SW_HIDE;

			char*command_temp = NULL;
			if (command)
			{
				command_temp = new char[strlen(command) + 2];
				memset(command_temp, '\0', strlen(command) + 2);
				memcpy(command_temp, command, strlen(command));
			}

			if (username)
			{
				//��½�û�
				if (Logon(username, password, token_))
				{
					CloseAllPipe();
					delete[]command_temp;
					return false;
				}
				
				if (!CreateProcessAsUserA(token_, shell, command_temp, NULL,NULL, TRUE,
					NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
				{
					CloseAllPipe();
					delete[]command_temp;
					return false;
				}
			}
			else
			{
				//����Cmd����
				//3 �̳�
				if (!CreateProcess(shell, command_temp, NULL, NULL, TRUE,
					NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
				{
					CloseAllPipe();
					delete[]command_temp;
					return false;
				}
			}
			delete[]command_temp;
			return true;
		}
		bool _Stop(bool forced)
		{
			if (forced)
			{
				//ǿ�ƹر�
				if(pi.hProcess)
					TerminateProcess(pi.hProcess, 1);
				return _Stop(false);
			}
			else
			{
				if(pi.hProcess)
					WaitForSingleObject(pi.hProcess, INFINITE);
				//������
				if (pi.hProcess)
				{
					DWORD exitcode = 0;
					GetExitCodeProcess(pi.hProcess, &exitcode);
					exit_code_ = exitcode;
				}
				//�رվ��
				if(pi.hThread)
					CloseHandle(pi.hThread);
				if (pi.hProcess)
					CloseHandle(pi.hProcess);
				memset(&pi, 0, sizeof(pi));

				Logout(token_);
				CloseAllPipe();
				return true;
			}
		}
		//��׼����
		bool _StdIn(const char*data, unsigned int len)
		{
			unsigned long lBytesWrite=0;//��д������ű���
			return TRUE==WriteFile(this_write_, data, len, &lBytesWrite, NULL);//�������Լӵ�Async
		}
		int _ReadStdError(char*buff, unsigned int buff_len)
		{
			return _ReadPipe(err_this_read_,buff,buff_len);
		}
		int _ReadStdOut(char*buff, unsigned int buff_len)
		{
			return _ReadPipe(this_read_, buff, buff_len);
		}

		void ClosePipe(HANDLE &handle)
		{
			if (handle != NULL)
			{
				DisconnectNamedPipe(handle);
				CloseHandle(handle);
				handle = NULL;
				//return true;
			}
			//return false;
		}
		void CloseAllPipe()
		{
			ClosePipe(this_write_);
			ClosePipe(shell_read_);
			ClosePipe(shell_write_);
			ClosePipe(this_read_);
			ClosePipe(err_shell_write_);
			ClosePipe(err_this_read_);
		}

		static wchar_t *utf8_to_wchar(const char *utf8_str) 
		{
			int len;
			wchar_t *ret;

			len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
			ret = new wchar_t[len * 4];
			MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, ret, len);
			return ret;
		}
		static char *wchar_to_utf8(const wchar_t *ws) {
			int len;
			char *ret;

			len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
			ret = new char[len * 4];
			WideCharToMultiByte(CP_UTF8, 0, ws, -1, ret, len, NULL, NULL);
			return ret;
		}
		static bool Logon(const char*username, const char*password,HANDLE &token)
		{
			if (NULL == username|| NULL == password)
				return false;
			/*wchar_t * w_username = utf8_to_wchar(username);
			wchar_t * w_password = utf8_to_wchar(password);*/
			if (FALSE ==
				LogonUserA(username, NULL, password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &token))
			{
				/*delete[]w_username;
				delete[]w_password;*/
				return false;//��½ʧ��
			}
			/*delete[]w_username;
			delete[]w_password;*/
			return true;
		}
		static void Logout(HANDLE &token)
		{
			if (token)
				CloseHandle(token);
			token = NULL;
		}

		int _ReadPipe(HANDLE pipe,char*buff, unsigned int buff_len)
		{
			unsigned long lBytesRead = 0;
			PeekNamedPipe(pipe, buff, buff_len, &lBytesRead, 0, 0);//�ܵ��Ƿ������ݿɶ�
			if (lBytesRead != 0)
			{
				lBytesRead = 0;
				ReadFile(pipe, buff, buff_len, &lBytesRead, 0);//��ȡ�ܵ��������
			}
			return lBytesRead;//�޿ɶ�����
		}
	private:
		//ͨ��
		//this->shell
		HANDLE this_write_;
		HANDLE shell_read_;
		//shell->this
		HANDLE shell_write_;
		HANDLE this_read_;
		//error
		HANDLE err_shell_write_;
		HANDLE err_this_read_;

		HANDLE token_;

		PROCESS_INFORMATION  pi;
#elif defined(OS_LINUX)
	private:
		bool _Start(const char*username, const char*password, const char*shell, const char*command)
		{
			//��ʼ��
			memset(write_pipe_, 0, sizeof(write_pipe_));
			memset(read_pipe_, 0, sizeof(read_pipe_));
			//error_pipe_
			memset(error_pipe_, 0, sizeof(error_pipe_));
			pid_ = 0;

			if( pipe(write_pipe_) ==-1|| pipe(read_pipe_) == -1
				|| pipe(error_pipe_) == -1)
			{
				CloseAllPipe();
				return false;
			}
			//ʧ�ܷ���-1���ɹ����أ��� �����̷����ӽ��̵�ID(�Ǹ�) ���ӽ��̷��� 0
			
			pid_ = fork();
			if (pid_ == -1)
			{
				return false;
			}
			//�ӽ���
			else if (pid_ == 0)
			{	
				//���Cmd FullPath
				char  strShellPath[] = "/usr/bin/bash";
				if (NULL == shell)
				{
					shell = strShellPath;
				}
				printf("shell %s\n", shell);

				char  strCmd[] = "sh";
				if (NULL == command)
				{
					command = strCmd;
				}
				printf("command %s\n", command);

				/*char buff[100];
				scanf("%s", buff);
				printf("buff:%s\n", buff);*/

				//�ض�����
				dup2(write_pipe_[0], STDIN_FILENO);
				dup2(read_pipe_[1], STDOUT_FILENO);
				dup2(error_pipe_[1], STDERR_FILENO);
				
				close(write_pipe_[1]);
				close(read_pipe_[0]);
				close(error_pipe_[0]);
				write_pipe_[1] = 0;
				read_pipe_[0] = 0;
				error_pipe_[0] = 0;
				//printf("shell %s\n", shell);
				//popen
				//�ӽ��������ls����
				//dup2(fd,STDOUT_FILENO);//�ض���stdout��fdָ����ļ�

				char* argvv[] = { "", "-c","", NULL };
				argvv[0] = (char*)shell;
				argvv[2] = (char*)command;
				//printf("1 shell %s\n", shell);
				//execve(shell, argvv,NULL);
				//execvp(shell, argvv);
				execlp("sh","-c","ls -l", NULL);
				//printf("2 shell %s\n", shell);
				perror("execve");   
				//ֻ��execl����ִ��ʧ�ܵ�����²��л���ִ����������룬ִ�еĳɹ�������ȥ�޻��ˡ�
				exit(127);	
			}
			//������
			else
			{
				close(write_pipe_[0]);
				write_pipe_[0] = 0;
				close(read_pipe_[1]);
				read_pipe_[1] = 0;
				close(error_pipe_[1]);
				error_pipe_[1] = 0;
				
				return true;
			}
		}
		bool _Stop(bool forced)
		{
			if (forced)
			{
				////ǿ�ƹر�
				//if (pid_)
				//{
				//	StdIn("exit", strlen("exit"));//ָ���˳�
				//}
				return _Stop(false);
			}
			else
			{
				if (pid_)
				{
					StdIn("exit", strlen("exit"));//ָ���˳�
				}
				//������
				if (pid_)
				{
					int exitcode = 0;
					waitpid(pid_, &exitcode,0);
					exit_code_ = exitcode;
				}
				CloseAllPipe();
				return true;
			}
		}
		//��׼����
		bool _StdIn(const char*data, unsigned int len)
		{
			if (NULL == data || len == 0)
			{
				//�����쳣
				return false;
			}
			if (!WaitTimeOut(write_pipe_[1], 10, true))
			{
				//û�пɶ�������
				//return false;
			}
			return write(write_pipe_[1], data, len)!=-1;
		}
		int _ReadStdError(char*buff, unsigned int buff_len)
		{
			return _ReadPipe(error_pipe_[0], buff, buff_len);
		}
		int _ReadStdOut(char*buff, unsigned int buff_len)
		{
			return _ReadPipe(read_pipe_[0], buff, buff_len);
		}

		int _ReadPipe(int fd, char* buff, unsigned int buff_len)
		{
			if (fd <= 0 || NULL == buff || buff_len == 0)
			{
				//�����쳣
				return 0;
			}
			if (!WaitTimeOut(fd, 10, true))
			{
				//û�пɶ�������
				return 0;
			}
			return read(fd, buff, buff_len);
		}
		void ClosePipe(int pipe[2])
		{
			if (pipe[0] > 0)
				close(pipe[0]);
			pipe[0] = 0;
			if (pipe[1] > 0)
				close(pipe[1]);
			pipe[1] = 0;
		}
		void CloseAllPipe()
		{
			ClosePipe(write_pipe_);
			ClosePipe(read_pipe_);
			ClosePipe(error_pipe_);
		}

		//�ȴ� fd �Ƿ�������
		//�ο� Socket.hpp
		bool WaitTimeOut(int fd,int wait_ms,bool is_read)
		{
		
			struct timeval tv, * p = NULL;
			if (wait_ms >= 0)
			{
				tv.tv_sec = wait_ms / 1000;
				tv.tv_usec = (wait_ms % 1000) * 1000;
				p = &tv;
			}

			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			int ret = -1;
			if (is_read)
			{
				ret = select(fd + 1, &fds, NULL, NULL, p);
			}
			else
			{
				ret = select(fd + 1, NULL, &fds, NULL, p);
			}

			if (ret < 0)
			{
				return false;
			}
			else if (ret == 0)
			{
				return false;
			}


			if (FD_ISSET(fd, &fds))  //��������룬��stdin�л�ȡ�����ַ�  
			{
				//�¼����
				return true;
			}
			//��Ӧ��������
			return false;
		}
	private:
		//1->0
		int write_pipe_[2];
		int read_pipe_[2];
		int error_pipe_[2];

		//�ӽ���id
		pid_t pid_;
#endif
	private:
		bool isStop_;
		unsigned int exit_code_;
	};
}
#endif
