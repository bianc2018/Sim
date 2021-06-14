/*
	异步ssh 实现
*/
#ifndef SIM_ASYNC_SSH_HPP_
#define SIM_ASYNC_SSH_HPP_

#include "Async.hpp"

#ifndef SIM_PARSER_MULTI_THREAD
#define SIM_PARSER_MULTI_THREAD
#endif

#include "SSHv2.hpp"
#define DEFAULT_INIT_WINDOWS_SIZE	4*1024*1024
#define DEFAULT_MAX_PACK_SIZE		4*1024
namespace sim
{
	//前置声明
	class SshChannel;
	class SshSession;
	class AsyncSsh;

	class SshChannelHandler
	{
	public:
		//打开通道
		virtual void OnConfirmation(SshChannel* channel) { return; }

		virtual void OnClosed(SshChannel* channel) { return; }

		virtual void OnData(SshChannel* channel,const Str&data) { return; }

		virtual void OnExtData(SshChannel* channel, uint32_t data_type_code, const Str& data) { return; }
	};

	//channel
	class SshChannel
	{
		friend class SshSession;
		friend class AsyncSsh;

		struct ChnData
		{
			uint32_t channel_id;
			uint32_t initial_window_size;
			uint32_t maximum_packet_size;
			ChnData()
				:channel_id(0), initial_window_size(0), maximum_packet_size(0)
			{}
		};
	public:
		SshChannel(SshSession& session, RefObject<SshChannelHandler> phandlers=NULL)
			:session_(session), phandlers_(phandlers)
		{

		}
		void SetHandlers(RefObject<SshChannelHandler> phandlers)
		{
			phandlers_ = phandlers;
		}
		bool Open(const Str& channel_type,
			uint32_t initial_window_size = DEFAULT_INIT_WINDOWS_SIZE,
			uint32_t maximum_packet_size = DEFAULT_MAX_PACK_SIZE);
		bool Close();
		bool Send(const Str& data);
		bool SendExt(uint32_t data_type_code,const Str& data);
		bool SendEof();
		bool WindowAdjust(uint32_t bytes_to_add);

	public:
		uint32_t GetId()
		{
			return local_channel_.channel_id;
		}
	private:
		ChnData remote_channel_;
		ChnData local_channel_;
		Str channel_type_;
		RefObject<SshChannelHandler> phandlers_;
		SshSession& session_;
	};

	class SshSessionHandler
	{
	public:
		
		////连接建立
		virtual void OnConnectEstablished(SshSession*session) { return; }

		//连接会话关闭
		virtual void OnConnectClosed(SshSession*session) { return; };

		//客户端验证结果
		virtual void OnAuthResponse(SshSession*session, bool success) { return; };

		//服务端验证鉴权请求
		//res_status -1 fail
		//res_status 0  retry SSH_MSG_USERAUTH_PK_OK or SSH_MSG_USERAUTH_PASSWD_CHANGEREQ 
		//res_status 1  success
		virtual void OnAuthRequest(SshSession*session, const SshAuthRequest &req, int& res_status) { return; };

		//密钥变更
		virtual void OnPassWordChanged(SshSession*session, Str&new_password) { return; };

	};

	enum SSH_SESSION_STATUS
	{
		SSH_INIT,
		SSH_CONN,
		SSH_ACCEPT,
		//handshake
		SSH_HANDSHAKE,
		SSH_AUTH,
		SSH_SESSION,
		SSH_CLOSE,
	};
	class SshSession
	{
		friend class AsyncSsh;
		friend class SshChannel;
	public:
		//AsyncSsh&async_;
		//AsyncHandle handle_;
		SshSession(AsyncSsh&async, AsyncHandle handle)
			:async_(async), handle_(handle), status_(SSH_INIT)
			, channel_id_(0)
		{

		}
		~SshSession()
		{
			Close();
		}

		AsyncHandle GetHandle() { return handle_; }
		AsyncSsh& GetAsync() { return async_; }
		
		//设置句柄
		virtual bool SetHandler(RefObject<SshSessionHandler> phandlers)
		{
			phandlers_ = phandlers;
			return true;
		}

		//设置通道默认句柄
		virtual bool SetChannelHandler(RefObject<SshChannelHandler> phandlers)
		{
			channel_phandlers_ = phandlers;
			return true;
		}

		//客户端
		virtual bool Connect(const Str& host, unsigned port = 22);
		virtual bool LoginPassWord(const Str& username, const Str& password)
		{
			if (status_ != SSH_AUTH)
				return false;

			ssh_conn_->SetHandler(SSH_MSG_USERAUTH_PASSWD_CHANGEREQ, MsgUserAuthPassWdChangeReq, this);

			Str service_name = "ssh-connection";
			Str req = ssh_conn_->AuthPassWord(username, password, service_name);
			if (req.size() == 0)
				return false;
			
			auth_req_.method = SSH_AUTH_PASSWORD;
			auth_req_.user_name = username;
			auth_req_.service_name = service_name;
			auth_req_.method_fields.password.flag = false;
			auth_req_.method_fields.password.password = password;

			return SendRawStr(req);
		}
		virtual bool LoginPublicKey(const Str& username, const Str& key_file)
		{
			if (status_ != SSH_AUTH)
				return false;
			
			ssh_conn_->SetHandler(SSH_MSG_USERAUTH_PK_OK, MsgUserAuthPKok, this);

			Str service_name = "ssh-connection";
			Str req = ssh_conn_->AuthPublicKey(username, key_file,false, service_name);
			if (req.size() == 0)
				return false;

			login_key_file_ = key_file;
			auth_req_.method = SSH_AUTH_PUB_KEY;
			auth_req_.user_name = username;
			auth_req_.service_name = service_name;
			auth_req_.method_fields.publickey.flag = false;

			return SendRawStr(req);
		}

		virtual RefObject<SshChannel> CreateChannel(RefObject<SshChannelHandler> channel_phandlers = NULL)
		{
			return NewSshChannel(channel_phandlers);
		}
		virtual RefObject<SshChannel> GetChannel(uint32_t channel_id)
		{
			RefObject<SshChannel> chn;
			{
				AutoMutex lk(channel_map_lock_);
				channel_map_.Find(channel_id,&chn);
			}
			return chn;
		}
		//服务端
		//加载私钥
		virtual bool LoadHostPrivateKey(const Str& file)
		{
			if (NULL == ssh_conn_ || ssh_conn_->PointType() != SshServer)
			{
				ssh_conn_ = NewSshConnection(SshServer);
			}
			if (!ssh_conn_->LoadPriKey(file.c_str()))
			{
				return false;
			}
			return true;
		}
		virtual bool Accept(unsigned port = 22);

		virtual bool Close();

		//拷贝句柄
		virtual bool DupFrom(SshSession*other)
		{
			if (NULL == other)
				return false;

			if (other->ssh_conn_)
			{
				if (ssh_conn_)
					ssh_conn_->ReSet();
				else
					ssh_conn_ = NewSshConnection(other->ssh_conn_->PointType());
				if (!ssh_conn_->DupFrom(other->ssh_conn_.get()))
					return false;
				ResetHandlers(ssh_conn_);
			}
			phandlers_ = other->phandlers_;
			channel_phandlers_ = other->channel_phandlers_;
			return true;
		}

	private:
		//发送底层数据
		bool SendRawData(const char*buff, unsigned int buff_len);
		
		bool SendRawStr(const Str& data)
		{
			return SendRawData(data.c_str(), data.size());
		}

		//加载验证公钥
		bool LoadAuthorizedKeys(const Str&usename, const Str&file);

		bool OnNetStart()
		{
			if (ssh_conn_)
			{
				sim::Str ver = ssh_conn_->PrintProtocolVersion();
				if (ver.empty())
				{
					printf("PrintProtocolVersion falt\n");
					Close();
					return false;
				}
				SendRawData(ver.c_str(), ver.size());
				status_ = SSH_HANDSHAKE;
				return true;
			}
			return false;
		}
	private:
		//网络回调接口
		void OnNetClose(AsyncCloseReason reason, int error)
		{
			if (phandlers_)
				phandlers_->OnConnectClosed(this);
		}
		
		void OnNetAccept(RefObject<SshSession> cli_ref);

		void OnNetConnect()
		{
			OnNetStart();
		}

		void OnNetRecvData(char* buff, unsigned int buff_len)
		{
			if (ssh_conn_)
			{
				if (!ssh_conn_->Parser(buff, buff_len))
				{
					SIM_LERROR(handle_ << " Parser error");
					Close();
				}
			}
			else
			{
				SIM_LERROR(handle_ << " ssh_conn_ is NULL");
				Close();
			}
		}

		void OnNetSendComplete(char* buff, unsigned int buff_len) {}
	private:
		//回调
		void OnMsgVersion(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			SSHVersion ver;
			if (!ssh_conn_->ParserVersion(payload_data, payload_data_len, ver))
			{
				SIM_LERROR(handle_<<" ParserVersion error ");
				Close();
				return;
			}

			if (!ssh_conn_->VersionExchange(ver))
			{
				SIM_LERROR(handle_ << " VersionExchange error ");
				Close();
				return;
			}

			Str data = ssh_conn_->PrintKexInit();
			if(data.size() == 0)
			{
				SIM_LERROR(handle_ << " PrintKexInit error ");
				Close();
				return;
			}
			SendRawData(data.c_str(), data.size());
			return;
		}
		static void MsgVersion(sim::SshTransport* parser,
			const char* payload_data, sim::uint32_t payload_data_len, void* pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgVersion(payload_data, payload_data_len);
			}
		}

		void OnMsgKexInit(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();

			SSHKexInit kex_init;
			if (!ssh_conn_->ParserKexInit(payload_data, payload_data_len, kex_init))
			{
				SIM_LERROR(handle_ << " ParserKexInit error ");
				Close();
				return;
			}
			if (!ssh_conn_->KexInit(kex_init))
			{
				SIM_LERROR(handle_ << " KexInit error ");
				Close();
				return;
			}

			if (ssh_conn_->PointType() == SshClient)
			{
				Str data = ssh_conn_->PrintKexDHInit();
				if (data.size() == 0)
				{
					SIM_LERROR(handle_ << " PrintKexDHInit error ");
					Close();
					return;
				}
				SendRawData(data.c_str(), data.size());
			}
			return;
		}
		static void MsgKexInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgKexInit(payload_data, payload_data_len);
			}
		}

		//KEXDH_INIT
		void OnMsgKexDHInit(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			sim::SSHKexDHInit dh_init;
			if (!ssh_conn_->ParserKexDHInit(payload_data, payload_data_len, dh_init))
			{
				SIM_LERROR(handle_ << " ParserKexDHInit error ");
				Close();
				return;
			}
			
			sim::SSHKexDHReply dh_reply;
			if (!ssh_conn_->KeyExchange(dh_init, dh_reply))
			{
				SIM_LERROR(handle_ << " KeyExchange error ");
				Close();
				return;
			}
			
			sim::Str reply = ssh_conn_->PrintKexDHReply(dh_reply);
			if (reply.empty())
			{
				SIM_LERROR(handle_ << " PrintKexDHReply error ");
				Close();
				return;
			}
			SendRawStr(reply);

			sim::Str newkey = ssh_conn_->PrintNewKeys();
			if (newkey.empty())
			{
				SIM_LERROR(handle_ << " PrintNewKeys error ");
				Close();
				return;
			}
			SendRawStr(newkey);
		}
		static void MsgKexDHInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgKexDHInit(payload_data, payload_data_len);
			}
		}

		//KEXDH_REPLY
		void OnMsgKexDHReply(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			sim::SSHKexDHReply reply;
			if (!ssh_conn_->ParserKexDHReply(payload_data, payload_data_len, reply))
			{
				SIM_LERROR(handle_ << " ParserKexDHReply error ");
				Close();
				return;
			}
			
			if (!ssh_conn_->KeyExchange(reply))
			{
				SIM_LERROR(handle_ << " KeyExchange error ");
				Close();
				return;
			}
			
			sim::Str newkey = ssh_conn_->PrintNewKeys();
			if (newkey.empty())
			{
				SIM_LERROR(handle_ << " PrintNewKeys error ");
				Close();
				return;
			}
			SendRawStr(newkey);
		}
		static void MsgKexDHReply(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgKexDHReply(payload_data, payload_data_len);
			}
		}

		//NEWKEYS
		void OnMsgNewKeys(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			if (!ssh_conn_->NewKeys())
			{
				SIM_LERROR(handle_ << " NewKeys error ");
				Close();
				return;
			}
			
			if (ssh_conn_->PointType() == SshClient)
			{
				sim::Str req = ssh_conn_->PrintServiceRequest("ssh-userauth");
				if (req.empty())
				{
					return;
				}
				SendRawStr(req);
			}
		}
		static void MsgNewKeys(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgNewKeys(payload_data, payload_data_len);
			}
		}

		//SERVICE_REQUEST
		void OnMsgServiceRequest(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			Str service;
			if (!ssh_conn_->ParserServiceRequest(payload_data, payload_data_len, service))
			{
				SIM_LERROR(handle_ << " ParserServiceRequest error ");
				Close();
				return;
			}
			
			if ("ssh-userauth" == service)
			{
				//握手完成
				status_ = SSH_AUTH;
				if (phandlers_)
					phandlers_->OnConnectEstablished(this);

				sim::Str response = ssh_conn_->PrintServiceAccept(service);
				if (response.empty())
				{
					SIM_LERROR(handle_ << " PrintServiceAccept error service=" << service);
					Close();
					return;
				}
				//Get().Send(handle, newkey.c_str(), newkey.size());
				SendRawStr(response);
			}
			else
			{
				SIM_LERROR(handle_ << " nuknow error service="<< service);
				Close();
				return;
			}
		}
		static void MsgServiceRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgServiceRequest(payload_data, payload_data_len);
			}
		}

		void OnMsgServiceAccept(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			Str service;
			if (!ssh_conn_->ParserServiceAccept(payload_data, payload_data_len, service))
			{
				SIM_LERROR(handle_ << " ParserServiceAccept error ");
				Close();
				return;
			}
			if ("ssh-userauth" == service)
			{
				//握手完成
				status_ = SSH_AUTH;
				if (phandlers_)
					phandlers_->OnConnectEstablished(this);
			}
			else
			{
				SIM_LERROR(handle_ << " nuknow error service=" << service);
				Close();
				return;
			}
		}
		static void MsgServiceAccept(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgServiceAccept(payload_data, payload_data_len);
			}
		}

		//USERAUTH_REQUEST
		void OnMsgUserAuthRequest(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			sim::SshAuthRequest req;
			if (!ssh_conn_->ParserAuthRequset(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserAuthRequset error ");
				Close();
				return;
			}

			//验证签名
			if (req.method == SSH_AUTH_PUB_KEY && req.method_fields.publickey.flag)
			{
				if (!ssh_conn_->VerifyAuthRequest(req))
				{
					SIM_LERROR(handle_ << " VerifyAuthRequest error ");
					Close();
					return;
				}
			}

			int res_status = -1;// fail
			if (phandlers_)
				phandlers_->OnAuthRequest(this, req, res_status);

			//res_status -1 fail
			//res_status 0  retry SSH_MSG_USERAUTH_PK_OK or SSH_MSG_USERAUTH_PASSWD_CHANGEREQ 
			//res_status 1  success
			Str response;
			if (res_status == -1)
			{
				response = ssh_conn_->PrintAuthResponseFailure(SSH_AUTH_PASSWORD + Str(",") + SSH_AUTH_PUB_KEY, false);
			}
			else if (res_status == 0)
			{
				if (req.method == SSH_AUTH_PASSWORD)
					response = ssh_conn_->PrintPkOK(req.method_fields.publickey.key_algorithm_name, req.method_fields.publickey.key_blob);
				else if (req.method == SSH_AUTH_PASSWORD)
					response = ssh_conn_->PrintPassWDChangeReq("password is changed!", "UTF-8");
				else
					response = ssh_conn_->PrintAuthResponseFailure(SSH_AUTH_PASSWORD + Str(",") + SSH_AUTH_PUB_KEY, false);
			}
			else if (res_status == 1)
			{
				response = ssh_conn_->PrintAuthResponseSuccess();
			}

			if (response.size() == 0)
			{
				SIM_LERROR(handle_ << " response is empty ");
				Close();
				return;
			}
			SendRawStr(response);
		}
		static void MsgUserAuthRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthRequest(payload_data, payload_data_len);
			}
		}
		
		//USERAUTH_PK_OK
		void OnMsgUserAuthPKok(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			Str key_name,key;
			if (!ssh_conn_->ParserPkOK(payload_data, payload_data_len, key_name, key))
			{
				SIM_LERROR(handle_ << " ParserPkOK error ");
				Close();
				return;
			}
			
			if (!ssh_conn_->CheckPkOK(key_name, key, login_key_file_))
			{
				SIM_LERROR(handle_ << " CheckPkOK error ");
				Close();
				return;
			}
			
			Str response = ssh_conn_->AuthPublicKey(auth_req_.user_name, login_key_file_, true, auth_req_.service_name);
			if (response.size() == 0)
			{
				SIM_LERROR(handle_ << " AuthPublicKey error ");
				Close();
				return;
			}
			SendRawStr(response);
		}
		static void MsgUserAuthPKok(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthPKok(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_USERAUTH_FAILURE
		void OnMsgUserAuthFailure(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			if (phandlers_)
				phandlers_->OnAuthResponse(this, false);
		}
		static void MsgUserAuthFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthFailure(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_USERAUTH_SUCCESS
		void OnMsgUserAuthSucccess(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			status_ = SSH_SESSION;
			if (phandlers_)
				phandlers_->OnAuthResponse(this, true);
		}
		static void MsgUserAuthSucccess(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthSucccess(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_USERAUTH_BANNER
		void OnMsgUserAuthBanner(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			Str msg, lag;
			if (!ssh_conn_->ParserBannerMessage(payload_data, payload_data_len, msg, lag))
			{
				SIM_LERROR(handle_ << " ParserBannerMessage error");
				return;
			}
			SIM_LINFO(handle_ << " Banner[" << lag << "]:" << msg);
		}
		static void MsgUserAuthBanner(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthBanner(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_USERAUTH_PASSWD_CHANGEREQ
		void OnMsgUserAuthPassWdChangeReq(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			Str prompt, lag;
			if (!ssh_conn_->ParserPassWDChangeReq(payload_data, payload_data_len, prompt, lag))
			{
				SIM_LERROR(handle_ << " ParserPassWDChangeReq error");
				Close();
				return;
			}
			Str new_password;
			if (phandlers_)
				phandlers_->OnPassWordChanged(this, new_password);
			if (new_password.size() == 0)
			{
				SIM_LERROR(handle_ << " new_password is empty");
				Close();
				return;
			}
			Str response = ssh_conn_->AuthPassWordChangeReq(auth_req_.user_name,
				auth_req_.method_fields.password.password,
				new_password,
				auth_req_.service_name);
			if (response.size() == 0)
			{
				SIM_LERROR(handle_ << " AuthPassWordChangeReq error");
				Close();
				return;
			}
			SendRawStr(response);
			auth_req_.method_fields.password.old_password = auth_req_.method_fields.password.password;
			auth_req_.method_fields.password.password = new_password;
		}
		static void MsgUserAuthPassWdChangeReq(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgUserAuthPassWdChangeReq(payload_data, payload_data_len);
			}
		}
	private:
		bool ResetHandlers(RefObject<SshConnection> ssh_conn)
		{
			if (NULL == ssh_conn)
				return false;

			ssh_conn->SetHandler(SSH_MSG_VERSION, MsgVersion, this);
			ssh_conn->SetHandler(SSH_MSG_KEXINIT, MsgKexInit, this);
			ssh_conn->SetHandler(SSH_MSG_KEXDH_INIT, MsgKexDHInit, this);
			ssh_conn->SetHandler(SSH_MSG_KEXDH_REPLY, MsgKexDHReply, this);
			ssh_conn->SetHandler(SSH_MSG_NEWKEYS, MsgNewKeys, this);
			ssh_conn->SetHandler(SSH_MSG_SERVICE_REQUEST, MsgServiceRequest, this);
			ssh_conn->SetHandler(SSH_MSG_SERVICE_ACCEPT, MsgServiceAccept, this);
			ssh_conn->SetHandler(SSH_MSG_USERAUTH_REQUEST, MsgUserAuthRequest, this);
			ssh_conn->SetHandler(SSH_MSG_USERAUTH_FAILURE, MsgUserAuthFailure, this);
			ssh_conn->SetHandler(SSH_MSG_USERAUTH_SUCCESS, MsgUserAuthSucccess, this);
			ssh_conn->SetHandler(SSH_MSG_USERAUTH_BANNER, MsgUserAuthBanner, this);
			return true;
		}

		RefObject<SshConnection> NewSshConnection(SshPointType type)
		{
			RefObject<SshConnection> ssh_conn(new SshConnection(type));
			if (!ResetHandlers(ssh_conn))
				return NULL;
			return ssh_conn;
		}

		RefObject<SshChannel> NewSshChannel(RefObject<SshChannelHandler> channel_phandlers)
		{
			RefObject<SshChannel> chn(new SshChannel(*this, channel_phandlers));
			if (NULL == chn)
			{
				SIM_LERROR("new SshChannel error");
				return NULL;
			}
			if (NULL == channel_phandlers)
				chn->SetHandlers(channel_phandlers_);//设置默认回调
			
			chn->local_channel_.channel_id = NextChannelId();
			{
				//加入环境
				AutoMutex lk(channel_map_lock_);
				//chn->local_channel_.channel_id
				channel_map_.Add(chn->local_channel_.channel_id, chn);
			}
			return chn;
		}

		uint32_t NextChannelId()
		{
			AutoMutex lk(channel_id_lock_);
			return ++channel_id_;
		}
	private:
		Str login_key_file_;
		SshAuthRequest auth_req_;

		Mutex channel_map_lock_;
		//local channel id->channel obj
		RbTree< RefObject<SshChannel> > channel_map_;

		AsyncSsh&async_;
		AsyncHandle handle_;

		//ssh协议解析器
		RefObject<SshConnection> ssh_conn_;

		RefObject<SshSessionHandler> phandlers_;

		RefObject<SshChannelHandler> channel_phandlers_;

		SSH_SESSION_STATUS status_;

		Mutex channel_id_lock_;
		uint32_t channel_id_;
	};

	class AsyncSsh :protected SimAsync
	{
		friend class SshSession;
	public:
		virtual int Poll(unsigned int wait_ms)
		{
			return SimAsync::Poll(wait_ms);
		}

		//生成一个会话
		virtual RefObject<SshSession> CreateSession()
		{
			AsyncHandle handle = CreateTcpHandle();
			if (handle == SOCK_FAILURE)
				return NULL;
			RefObject<SshSession> ref = NewSession(handle);
			if (NULL == ref)
			{
				Close(handle);
				return NULL;
			}
			return ref;
		}

		virtual RefObject<SshSession> GetSession(AsyncHandle handle)
		{
			RefObject<SshSession> ref;
			{
				//加到会话列表
				AutoMutex lk(session_map_lock_);
				session_map_.Find(handle, &ref);
			}
			return ref;
		}
		//设置回调
	private:
		//网络回调接口
		void OnNetClose(AsyncHandle handle, AsyncCloseReason reason, int error)
		{
			RefObject<SshSession> session = GetSession(handle);
			if (session)
			{
				session->OnNetClose(reason, error);
				session_map_.Del(handle);
			}
		}
		static void NetCloseHandler(AsyncHandle handle, AsyncCloseReason reason, int error, void*data)
		{
			AsyncSsh*ssh = (AsyncSsh*)data;
			if (ssh)
			{
				ssh->OnNetClose(handle, reason, error);
			}
		}

		void OnNetAccept(AsyncHandle handle, AsyncHandle client)
		{
			SIM_LERROR(handle << " OnNetAccept " << client);
			RefObject<SshSession> session = GetSession(handle);
			if (session)
			{
				session->OnNetAccept(NewSession(client));
			}
		}
		static void NetAcceptHandler(AsyncHandle handle, AsyncHandle client, void*data)
		{
			AsyncSsh*ssh = (AsyncSsh*)data;
			if (ssh)
			{
				ssh->OnNetAccept(handle, client);
			}
		}

		void OnNetConnect(AsyncHandle handle)
		{
			RefObject<SshSession> session = GetSession(handle);
			if (session)
			{
				session->OnNetConnect();
			}
		}
		static void NetConnectHandler(AsyncHandle handle, void*data)
		{
			AsyncSsh*ssh = (AsyncSsh*)data;
			if (ssh)
			{
				ssh->OnNetConnect(handle);
			}
		}

		void OnNetRecvData(AsyncHandle handle, char *buff, unsigned int buff_len)
		{
			RefObject<SshSession> session = GetSession(handle);
			if (session)
			{
				session->OnNetRecvData(buff,buff_len);
			}
		}
		static void NetRecvDataHandler(AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{
			AsyncSsh*ssh = (AsyncSsh*)data;
			if (ssh)
			{
				ssh->OnNetRecvData(handle, buff, buff_len);
			}
		}

		void OnNetSendComplete(AsyncHandle handle, char *buff, unsigned int buff_len)
		{
			//nodoing
		}
		static void NetSendCompleteHandler(AsyncHandle handle, char *buff, unsigned int buff_len, void*data)
		{

		}
	
	private:
		virtual RefObject<SshSession> NewSession(AsyncHandle handle)
		{
			if (!IsHas(handle))
				return NULL;

			RefObject<SshSession> ref(new SshSession(*this, handle));

			//设置回调
			SetConnectHandler(handle, NetConnectHandler, this);
			SetAcceptHandler(handle, NetAcceptHandler, this);
			SetCloseHandler(handle, NetCloseHandler, this);
			SetRecvDataHandler(handle, NetRecvDataHandler, this);
			SetSendCompleteHandler(handle, NetSendCompleteHandler, this);

			{
				//加到会话列表
				AutoMutex lk(session_map_lock_);
				session_map_.Add(handle, ref);
			}
			return ref;
		}
	private:
		Mutex session_map_lock_;
		RbTree< RefObject<SshSession> > session_map_;
	};

	//SshSession 实现
	//客户端
	bool SshSession::Connect(const Str& host, unsigned port)
	{
		if (status_ != SSH_INIT)
			return false;
		if (host.size() == 0)
			return false;
		
		//获取ip
		const int buff_size = 64;
		char ip[buff_size] = { 0 };
		if (!Socket::GetFirstIpByName(host.c_str(), ip, buff_size))
		{
			return false;
		}
		ssh_conn_ = NewSshConnection(SshClient);
		//连接
		if (SOCK_SUCCESS != async_.AddTcpConnect(handle_, ip, port))
			return false;
		status_ = SSH_CONN;
		return true;
	}
	
	bool SshSession::Accept(unsigned port)
	{
		if (status_ != SSH_INIT)
			return false;
		if (NULL == ssh_conn_)
		{
			ssh_conn_ = NewSshConnection(SshServer);
		}
		//连接
		if (SOCK_SUCCESS != async_.AddTcpServer(handle_, NULL, port))
			return false;
		status_ = SSH_CONN;
		return true;
	}

	bool SshSession::Close()
	{
		return SOCK_SUCCESS == async_.Close(handle_);
	}

	void SshSession::OnNetAccept(RefObject<SshSession> cli_ref)
	{
		//RefObject<SshSession> cli_ref = async_.GetSession(client);
		if (cli_ref)
		{
			cli_ref->DupFrom(this);
			/*if (phandlers_)
				phandlers_->OnConnectAccepted(this, cli_ref.get());*/
			cli_ref->OnNetStart();
		}
	}

	bool SshSession::SendRawData(const char* buff, unsigned int buff_len)
	{
		SIM_LERROR(handle_ << " SendRawData " << buff_len);
		async_.Send(handle_, buff, buff_len);
		return true;
	}
}
#endif