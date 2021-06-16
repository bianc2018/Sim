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

namespace sim
{
	//前置声明
	class SshChannel;
	class SshSession;
	class AsyncSsh;

	class SshChannelHandler
	{
	public:
		//打开通道 主动
		virtual void OnOpenConfirmation(SshChannel* channel) { return; }

		virtual void OnOpenFailure(SshChannel* channel,const SshOpenChannelFailure&failure) { return; }

		virtual void OnClosed(SshChannel* channel) { return; }

		virtual void OnData(SshChannel* channel,const Str&data) { return; }

		virtual void OnExtData(SshChannel* channel, uint32_t data_type_code, const Str& data) { return; }

		virtual void OnRequest(SshChannel* channel, const SshChannelRequest&req) { return; }

		virtual void OnResponse(SshChannel* channel, bool success) { return; }
	};
	
	enum SSH_CHANNEL_STATUS
	{
		SSH_CHANNEL_INIT,
		SSH_CHANNEL_OPENING,
		SSH_CHANNEL_OPENED,
		SSH_CHANNEL_EOF,
		SSH_CHANNEL_CLOSEING,
		SSH_CHANNEL_CLOSED,
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
	private:
		SshChannel(SshSession& session, RefObject<SshChannelHandler> phandlers=NULL)
			:session_(session), phandlers_(phandlers), status_(SSH_CHANNEL_INIT)
		{

		}
	public:
		~SshChannel();
		void SetHandlers(RefObject<SshChannelHandler> phandlers);
		bool Open(const Str& channel_type,
			uint32_t initial_window_size = SSH_CHANNEL_WINDOW_DEFAULT,
			uint32_t maximum_packet_size = SSH_CHANNEL_PACKET_DEFAULT);
		bool Close(bool forced = false);
		bool Send(const Str& data);
		bool SendExt(uint32_t data_type_code, const Str& data);
		bool SendEof();
		bool WindowAdjust(uint32_t bytes_to_add);

		//channel request
		bool SendRequest(const SshChannelRequest&req);
		bool SendResponse(bool success);
		
		//基于SendRequest
		bool Shell(bool want_reply = false);
		bool Env(const Str &name, const Str &value, bool want_reply = false);
		bool Exec(const Str &cmd, bool want_reply = false);
		bool PtyReq(const Str &term, bool want_reply = false,const Str &modes="",
			int width= SSH_TERM_WIDTH, int height= SSH_TERM_HEIGHT,
			int width_px= SSH_TERM_WIDTH_PX, int height_px= SSH_TERM_HEIGHT_PX);

		inline SSH_CHANNEL_STATUS Status() { return status_; }

		RefObject<SshChannel> GetRef();
	public:
		uint32_t GetId();
		SshSession&GetSession();
	private:
		void OnRelease();
	private:
		void OnOpenConfirmation(const SshOpenChannelConfirmation &req);
		void OnOpenFailure(const SshOpenChannelFailure &req);
		void OnData(const Str &data);
		void OnExtData(uint32_t data_type_code, const Str& data);
		void OnClose();
		void OnEof();
		void OnRequest(const SshChannelRequest&req);
		void OnReqResponse(bool success);
		void OnWindowAdjust(const uint32_t &bytes_to_add);
	private:
		ChnData remote_channel_;
		ChnData local_channel_;
		Str channel_type_;
		RefObject<SshChannelHandler> phandlers_;
		SshSession& session_;
		SSH_CHANNEL_STATUS status_;
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

		//请求打开通道//accept=false 拒绝
		virtual void OnChannelOpenRequest(SshSession*session, 
			RefObject<SshChannel> channel,
			bool &accept,
			uint32_t &local_initial_window_size,
			uint32_t &local_maximum_packet_size) { return; };

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
		SSH_CLOSED,
	};
	
	class SshSession
	{
		friend class AsyncSsh;
		friend class SshChannel;
	
	private:
		//AsyncSsh&async_;
		//AsyncHandle handle_;
		SshSession(AsyncSsh&async, AsyncHandle handle)
			:async_(async), handle_(handle), status_(SSH_INIT)
			, channel_id_(0)
		{

		}
	public:
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
		//bool LoadAuthorizedKeys(const Str&usename, const Str&file);

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

		//获取解析句柄
		RefObject<SshConnection> GetSSHv2()
		{
			return ssh_conn_;
		}

		static bool CloseChannelTreeTraverseFunc(sim::RbTreeNode<RefObject<SshChannel> >* Now, void*pdata);
	private:
		//网络回调接口
		void OnNetClose(AsyncCloseReason reason, int error)
		{
			Close();

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

		//SSH_MSG_GLOBAL_REQUEST
		void OnMsgGlobalRequest(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			SshChannelGlobalRequest req;
			if (!ssh_conn_->ParserChannelGlobalRequest(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserChannelGlobalRequest error");
				Close();
				return;
			}
			SIM_LINFO("GlobalRequest:" << req.request_name);
			if (req.want_reply)
			{
				//no support
				Str response = ssh_conn_->PrintChannelGlobalReqFailure();
				SendRawStr(response);
			}
		}
		static void MsgGlobalRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgGlobalRequest(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_REQUEST_SUCCESS
		void OnMsgGlobalRequestSuccess(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			//not do
		}
		static void MsgGlobalRequestSuccess(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgGlobalRequestSuccess(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_REQUEST_FAILURE
		void OnMsgGlobalRequestFailure(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SIM_FUNC_DEBUG();
			//not do
		}
		static void MsgGlobalRequestFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgGlobalRequestFailure(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_OPEN
		void OnMsgChannelOpen(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshOpenChannelRequest req;
			if (!ssh_conn_->ParserOpenChannelRequest(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserOpenChannelRequest error");
				Close();
				return;
			}
			if (req.channel_type != SSH_CHANNEL_TYPE_SESSION)
			{
				SIM_LERROR(handle_ << " open channel error,no support type="<<req.channel_type);
				//SSH_OPEN_UNKNOWN_CHANNEL_TYPE
				SshOpenChannelFailure fail;
				fail.recipient_channel = req.sender_channel;
				fail.reason_code = SSH_OPEN_UNKNOWN_CHANNEL_TYPE;
				fail.description = "unknow channel type:" + req.channel_type;
				Str data = ssh_conn_->PrintOpenChannelFailure(fail);
				SendRawStr(data);
				return ;
			}

			//创建通道
			RefObject<SshChannel> channel = NewSshChannel(channel_phandlers_);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " open channel error,NewSshChannel failure");
				//SSH_OPEN_UNKNOWN_CHANNEL_TYPE
				SshOpenChannelFailure fail;
				fail.recipient_channel = req.sender_channel;
				fail.reason_code = SSH_OPEN_RESOURCE_SHORTAGE;
				fail.description = "NewSshChannel failure";
				Str data = ssh_conn_->PrintOpenChannelFailure(fail);
				SendRawStr(data);
				return;
			}
			
			channel->channel_type_ = req.channel_type;
			channel->remote_channel_.channel_id = req.sender_channel;
			channel->remote_channel_.initial_window_size = req.initial_window_size;
			channel->remote_channel_.maximum_packet_size = req.maximum_packet_size;

			channel->local_channel_.initial_window_size = SSH_CHANNEL_WINDOW_DEFAULT;
			channel->local_channel_.maximum_packet_size = SSH_CHANNEL_PACKET_DEFAULT;

			bool accept = true;
			if (phandlers_)
				phandlers_->OnChannelOpenRequest(this, channel, accept
					, channel->local_channel_.initial_window_size
				, channel->local_channel_.maximum_packet_size);
			
			if (accept)
			{
				if (channel->local_channel_.initial_window_size == 0)
					channel->local_channel_.initial_window_size = SSH_CHANNEL_WINDOW_DEFAULT;
				if (channel->local_channel_.maximum_packet_size == 0)
					channel->local_channel_.maximum_packet_size = SSH_CHANNEL_PACKET_DEFAULT;
				SshOpenChannelConfirmation response;
				response.sender_channel = channel->local_channel_.channel_id;
				response.recipient_channel = channel->remote_channel_.channel_id;
				response.initial_window_size = channel->local_channel_.initial_window_size;
				response.maximum_packet_size = channel->local_channel_.maximum_packet_size;
				Str data = ssh_conn_->PrintOpenChannelConfirmation(response, channel->channel_type_);
				SendRawStr(data);
				return;
			}
			else
			{
				SIM_LERROR(handle_ << " open channel error,user not accept");
				DelSshChannel(channel->local_channel_.channel_id);
				//SSH_OPEN_UNKNOWN_CHANNEL_TYPE
				SshOpenChannelFailure fail;
				fail.recipient_channel = req.sender_channel;
				fail.reason_code = SSH_OPEN_ADMINISTRATIVELY_PROHIBITED;
				fail.description = "user not accept";
				Str data = ssh_conn_->PrintOpenChannelFailure(fail);
				SendRawStr(data);
				return;
			}
		}
		static void MsgChannelOpen(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelOpen(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_OPEN_CONFIRMATION
		void OnMsgChannelOpenConfirmation(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshOpenChannelConfirmation req;
			if (!ssh_conn_->ParserOpenChannelConfirmation(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserOpenChannelConfirmation error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel "<< req.recipient_channel<<" sender "<< req.sender_channel);
				return;
			}
			channel->OnOpenConfirmation(req);
		}
		static void MsgChannelOpenConfirmation(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelOpenConfirmation(payload_data, payload_data_len);
			}
		}
		
		//SSH_MSG_CHANNEL_OPEN_FAILURE
		void OnMsgChannelOpenFailure(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshOpenChannelFailure req;
			if (!ssh_conn_->ParserOpenChannelFailure(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserOpenChannelFailure error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << req.recipient_channel );
				return;
			}
			channel->OnOpenFailure(req);
		}
		static void MsgChannelOpenFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelOpenFailure(payload_data, payload_data_len);
			}
		}
	
		//SSH_MSG_CHANNEL_WINDOW_ADJUST
		void OnMsgChannelWindowAdjust(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshChannelWindowAdjust req;
			if (!ssh_conn_->ParserChannelWindowAdjust(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserChannelWindowAdjust error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << req.recipient_channel);
				return;
			}
			channel->OnWindowAdjust(req.bytes_to_add);
		}
		static void MsgChannelWindowAdjust(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelWindowAdjust(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_DATA
		void OnMsgChannelData(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshChannelData req;
			if (!ssh_conn_->ParserChannelData(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserChannelData error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << req.recipient_channel);
				return;
			}
			channel->OnData(req.data);
		}
		static void MsgChannelData(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelData(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_EXTENDED_DATA
		void OnMsgChannelExtData(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshChannelExtData req;
			if (!ssh_conn_->ParserChannelExtData(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserChannelExtData error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << req.recipient_channel);
				return;
			}
			channel->OnExtData(req.data_type_code, req.data);
		}
		static void MsgChannelExtData(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelExtData(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_EOF
		void OnMsgChannelEof(const char* payload_data, sim::uint32_t payload_data_len)
		{
			sim::uint32_t recipient_channel;
			if (!ssh_conn_->ParserChannelEof(payload_data, payload_data_len, recipient_channel))
			{
				SIM_LERROR(handle_ << " ParserChannelEof error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << recipient_channel);
				return;
			}
			channel->OnEof();
		}
		static void MsgChannelEof(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelEof(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_CLOSE
		void OnMsgChannelClose(const char* payload_data, sim::uint32_t payload_data_len)
		{
			sim::uint32_t recipient_channel;
			if (!ssh_conn_->ParserChannelClose(payload_data, payload_data_len, recipient_channel))
			{
				SIM_LERROR(handle_ << " ParserChannelClose error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << recipient_channel);
				return;
			}
			channel->OnClose();
		}
		static void MsgChannelClose(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelClose(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_REQUEST
		void OnMsgChannelRequest(const char* payload_data, sim::uint32_t payload_data_len)
		{
			SshChannelRequest req;
			if (!ssh_conn_->ParserChannelRequest(payload_data, payload_data_len, req))
			{
				SIM_LERROR(handle_ << " ParserChannelExtData error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(req.recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << req.recipient_channel);
				return;
			}
			channel->OnRequest(req);
		}
		static void MsgChannelRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelRequest(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_SUCCESS
		void OnMsgChannelRequestSuccess(const char* payload_data, sim::uint32_t payload_data_len)
		{
			sim::uint32_t recipient_channel;
			if (!ssh_conn_->ParserChannelReqResponse(payload_data, payload_data_len, recipient_channel))
			{
				SIM_LERROR(handle_ << " ParserChannelReqResponse error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << recipient_channel);
				return;
			}
			channel->OnReqResponse(true);
		}
		static void MsgChannelRequestSuccess(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelRequestSuccess(payload_data, payload_data_len);
			}
		}

		//SSH_MSG_CHANNEL_FAILURE
		void OnMsgChannelRequestFailure(const char* payload_data, sim::uint32_t payload_data_len)
		{
			sim::uint32_t recipient_channel;
			if (!ssh_conn_->ParserChannelReqResponse(payload_data, payload_data_len, recipient_channel))
			{
				SIM_LERROR(handle_ << " ParserChannelReqResponse error");
				Close();
				return;
			}
			RefObject<SshChannel> channel = GetChannel(recipient_channel);
			if (NULL == channel)
			{
				SIM_LERROR(handle_ << " not found channel " << recipient_channel);
				return;
			}
			channel->OnReqResponse(false);
		}
		static void MsgChannelRequestFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata)
		{
			SshSession* s = (SshSession*)pdata;
			if (s)
			{
				s->OnMsgChannelRequestFailure(payload_data, payload_data_len);
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

			ssh_conn->SetHandler(SSH_MSG_GLOBAL_REQUEST, MsgGlobalRequest, this);
			ssh_conn->SetHandler(SSH_MSG_REQUEST_SUCCESS, MsgGlobalRequestSuccess, this);
			ssh_conn->SetHandler(SSH_MSG_REQUEST_FAILURE, MsgGlobalRequestFailure, this);

			ssh_conn->SetHandler(SSH_MSG_CHANNEL_OPEN, MsgChannelOpen, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_OPEN_CONFIRMATION, MsgChannelOpenConfirmation, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_OPEN_FAILURE, MsgChannelOpenFailure, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_WINDOW_ADJUST, MsgChannelWindowAdjust, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_DATA, MsgChannelData, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_EXTENDED_DATA, MsgChannelExtData, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_EOF, MsgChannelEof, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_CLOSE, MsgChannelClose, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_REQUEST, MsgChannelRequest, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_SUCCESS, MsgChannelRequestSuccess, this);
			ssh_conn->SetHandler(SSH_MSG_CHANNEL_FAILURE, MsgChannelRequestFailure, this);
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

		bool DelSshChannel(sim::uint32_t channel_id)
		{
			{
				AutoMutex lk(channel_map_lock_);
				//chn->local_channel_.channel_id
				channel_map_.Del(channel_id);
			}
			return true;
		}

		uint32_t NextChannelId()
		{
			AutoMutex lk(channel_id_lock_);
			return channel_id_++;
		}

		//static bool MyTreeTraverseFunc(sim::RbTreeNode<RefObject<SshChannel>>* Now, void*pdata)
		//{
		//	if (Now&&Now->Data)
		//	{
		//		//RefObject<SshChannel> ref = Now->Data;
		//		Now->Data.reset();
		//	}
		//}
		//bool DelAllSshChannel()
		//{
		//	AutoMutex lk(channel_map_lock_);
		//	channel_map_.TraverseTree(MyTreeTraverseFunc, NULL, sim::TraverseTypeLDR);
		//	channel_map_.Clear();
		//	return true;
		//}
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

	//SshChannel
	SshChannel::~SshChannel()
	{
		status_ = SSH_CHANNEL_CLOSEING;
		//Close();
	}
	void SshChannel::SetHandlers(RefObject<SshChannelHandler> phandlers)
	{
		phandlers_ = phandlers;
	}
	bool SshChannel::Open(const Str& channel_type,
		uint32_t initial_window_size ,
		uint32_t maximum_packet_size )
	{

		if (status_ != SSH_CHANNEL_INIT)
			return false;
		status_ = SSH_CHANNEL_OPENING;

		if (initial_window_size == 0)
			initial_window_size = SSH_CHANNEL_WINDOW_DEFAULT;
		if (maximum_packet_size == 0)
			maximum_packet_size = SSH_CHANNEL_PACKET_DEFAULT;

		local_channel_.initial_window_size = initial_window_size;
		local_channel_.maximum_packet_size = maximum_packet_size;

		SshOpenChannelRequest req;
		req.channel_type = channel_type_ = channel_type;
		req.initial_window_size = local_channel_.initial_window_size;
		req.maximum_packet_size = local_channel_.maximum_packet_size;
		req.sender_channel = local_channel_.channel_id;

		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			Str data = ssh_conn->PrintOpenChannelRequest(req);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintOpenChannelRequest error");
				status_ = SSH_CHANNEL_INIT;
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			status_ = SSH_CHANNEL_INIT;
			return false;
		}
	}
	bool SshChannel::Close(bool forced)
	{
		if (forced&&status_!= SSH_CHANNEL_CLOSED)
		{
			status_ = SSH_CHANNEL_CLOSEING;
		}

		/*
		SSH_CHANNEL_OPENED,
		SSH_CHANNEL_EOF,
		SSH_CHANNEL_CLOSED,
		*/
		if (SSH_CHANNEL_OPENED == status_ || SSH_CHANNEL_EOF == status_)
		{
			if(status_ == SSH_CHANNEL_OPENED)
				SendEof();
			status_ = SSH_CHANNEL_CLOSEING;
			//发送关闭请求
			RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
			if (ssh_conn)
			{
				Str data = ssh_conn->PrintChannelClose(remote_channel_.channel_id);
				if (data.size() == 0)
				{
					SIM_LERROR("PrintChannelClose error");
					status_ = SSH_CHANNEL_INIT;
					return false;
				}
				return session_.SendRawStr(data);
			}
			else
			{
				SIM_LERROR("GetSSHv2 return NULL");
				return false;
			}
			return true;
		}
		else
		{
			//直接释放
			session_.DelSshChannel(local_channel_.channel_id);
		}
		return true;

	}
	bool SshChannel::Send(const Str& data)
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;

		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			SshChannelData req;
			req.recipient_channel = remote_channel_.channel_id;
			req.data = data;
			Str data = ssh_conn->PrintChannelData(req);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelData error");
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			return false;
		}
	}
	bool SshChannel::SendExt(uint32_t data_type_code, const Str& data)
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;

		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			SshChannelExtData req;
			req.recipient_channel = remote_channel_.channel_id;
			req.data_type_code = data_type_code;
			req.data = data;
			Str data = ssh_conn->PrintChannelExtData(req);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelExtData error");
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			return false;
		}
	}
	bool SshChannel::SendEof()
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;
		status_ = SSH_CHANNEL_EOF;
		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			Str data = ssh_conn->PrintChannelEof(remote_channel_.channel_id);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelEof error");
				status_ = SSH_CHANNEL_OPENED;
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			status_ = SSH_CHANNEL_OPENED;
			return false;
		}
	}
	bool SshChannel::WindowAdjust(uint32_t bytes_to_add)
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;

		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			SshChannelWindowAdjust req;
			req.recipient_channel = remote_channel_.channel_id;
			req.bytes_to_add = bytes_to_add;

			Str data = ssh_conn->PrintChannelWindowAdjust(req);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelWindowAdjust error");
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			return false;
		}
	}

	void SshChannel::OnRelease()
	{
		//调用一次
		if (status_ != SSH_CHANNEL_CLOSED)
		{
			status_ = SSH_CHANNEL_CLOSED;
			if (phandlers_)
				phandlers_->OnClosed(this);
			phandlers_.reset();
		}
	}
	//channel request
	bool SshChannel::SendRequest(const SshChannelRequest&req)
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;

		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			SshChannelRequest treq = req;
			treq.recipient_channel = remote_channel_.channel_id;
			Str data = ssh_conn->PrintChannelRequest(treq);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelRequest error");
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			return false;
		}
	}
	bool SshChannel::SendResponse(bool success)
	{
		if (status_ != SSH_CHANNEL_OPENED)
			return false;

		//窗口

		//发送请求
		RefObject<SshConnection>ssh_conn = session_.GetSSHv2();
		if (ssh_conn)
		{
			Str data = ssh_conn->PrintChannelReqResponse(success, remote_channel_.channel_id);
			if (data.size() == 0)
			{
				SIM_LERROR("PrintChannelReqResponse error");
				return false;
			}
			return session_.SendRawStr(data);
		}
		else
		{
			SIM_LERROR("GetSSHv2 return NULL");
			return false;
		}
	}

	//基于SendRequest
	bool SshChannel::Shell(bool want_reply)
	{
		SshChannelRequest req;
		req.type = SSH_CHANNEL_REQ_SHELL;
		req.want_reply = want_reply;
		return SendRequest(req);
	}
	bool SshChannel::Env(const Str &name, const Str &value, bool want_reply)
	{
		SshChannelRequest req;
		req.type = SSH_CHANNEL_REQ_ENV;
		req.want_reply = want_reply;
		req.ts.env.name = name;
		req.ts.env.value = value;
		return SendRequest(req);
	}
	bool SshChannel::Exec(const Str &cmd, bool want_reply)
	{
		SshChannelRequest req;
		req.type = SSH_CHANNEL_REQ_EXEC;
		req.want_reply = want_reply;
		req.ts.exec.command = cmd;
		return SendRequest(req);
	}
	bool SshChannel::PtyReq(const Str &term, bool want_reply, const Str &modes, int width, int height,
		int width_px, int height_px)
	{
		SshChannelRequest req;
		req.type = SSH_CHANNEL_REQ_PTY_REQ;
		req.want_reply = want_reply;
		req.ts.pty_req.term_value = term;
		req.ts.pty_req.encoded_terminal_modes = modes;
		req.ts.pty_req.width_characters = width;
		req.ts.pty_req.height_rows = height;
		req.ts.pty_req.width_pixels = width_px;
		req.ts.pty_req.height_pixels = height_px;
		return SendRequest(req);
	}
	uint32_t SshChannel::GetId()
	{
		return local_channel_.channel_id;
	}
	void SshChannel::OnOpenConfirmation(const SshOpenChannelConfirmation &req)
	{
		status_ = SSH_CHANNEL_OPENED;
		remote_channel_.initial_window_size = req.initial_window_size;
		remote_channel_.maximum_packet_size = req.maximum_packet_size;
		if (phandlers_)
			phandlers_->OnOpenConfirmation(this);
	}
	void SshChannel::OnOpenFailure(const SshOpenChannelFailure &req)
	{
		status_ = SSH_CHANNEL_INIT;
		if (phandlers_)
			phandlers_->OnOpenFailure(this, req);
	}
	void SshChannel::OnData(const Str &data)
	{
		if (phandlers_)
			phandlers_->OnData(this, data);
	}
	void SshChannel::OnExtData(uint32_t data_type_code, const Str& data)
	{
		if (phandlers_)
			phandlers_->OnExtData(this, data_type_code, data);
	}
	void SshChannel::OnClose()
	{
		//第一次发送回复
		Close();
		//第二次关闭
		Close();
	}
	void SshChannel::OnEof()
	{
		SendEof();
	}
	void SshChannel::OnRequest(const SshChannelRequest&req)
	{
		if (phandlers_)
			phandlers_->OnRequest(this, req);
	}
	void SshChannel::OnReqResponse(bool success)
	{
		if (phandlers_)
			phandlers_->OnResponse(this, success);
	}
	void SshChannel::OnWindowAdjust(const uint32_t &bytes_to_add)
	{
		//
		remote_channel_.initial_window_size += bytes_to_add;
	}
	RefObject<SshChannel> SshChannel::GetRef()
	{
		return session_.GetChannel(local_channel_.channel_id);
	}
	SshSession&SshChannel::GetSession()
	{
		return session_;
	}

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

	bool SshSession::CloseChannelTreeTraverseFunc(sim::RbTreeNode<RefObject<SshChannel> >* Now, void*pdata)
	{
		Now->Data->OnRelease();
		return true;
	}

	bool SshSession::Close()
	{
		if (status_ != SSH_CLOSED)
		{
			status_ = SSH_CLOSED;
			{
				AutoMutex lk(channel_map_lock_);
				channel_map_.TraverseTree(CloseChannelTreeTraverseFunc,this);
				channel_map_.Clear();
			}
			return SOCK_SUCCESS == async_.Close(handle_);
		}
		//关闭

		
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
		//SIM_LERROR(handle_ << " SendRawData " << buff_len);
		async_.Send(handle_, buff, buff_len);
		return true;
	}
}
#endif