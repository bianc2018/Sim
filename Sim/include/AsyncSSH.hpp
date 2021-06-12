/*
	异步ssh 实现
*/
#ifndef SIM_ASYNC_SSH_HPP_
#define SIM_ASYNC_SSH_HPP_

#include "Async.hpp"
#define SIM_PARSER_MULTI_THREAD
#include "SSHv2.hpp"

namespace sim
{
	class AsyncSsh;
	class SshSession;

	//channel
	struct SshChannel
	{
		struct ChnData
		{
			uint32_t channel_id;
			uint32_t initial_window_size;
			uint32_t maximum_packet_size;
		};
	public:
		ChnData remote_channel;
		ChnData local_channel;
		Str channel_type;
	};

	

	class SshSessionHandler
	{
	public:
		
		////连接建立
		virtual void OnConnectEstablished(SshSession*session) { return; }

		virtual void OnConnectAccepted(SshSession*session, SshSession*client) { return; };

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
		//handshake
		SSH_HANDSHAKE,
		SSH_AUTH,
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
		{

		}
		~SshSession()
		{
			Close();
		}

		AsyncHandle GetHandle();
		AsyncHandle GetServerHandle();
		AsyncSsh& GetAsync();
		
		//设置句柄
		virtual bool SetHandler(RefObject<SshSessionHandler> phandlers)
		{
			phandlers_ = phandlers;
			return true;
		}

		//客户端
		bool Connect(const Str&host, unsigned port = 22)
		{
			if (status_ != SSH_INIT)
				return false;
			if (host.size() == 0)
				return false;

			//获取ip
			const int buff_size = 64;
			char ip[buff_size] = { 0 };
			if (!Socket::GetFirstIpByName(host.c_str(),ip,buff_size))
			{
				return false;
			}
			
			//连接
			if (SOCK_SUCCESS != async_.AddTcpConnect(handle_, ip, port))
				return false;
			return true;
		}
		bool Login(const Str&username, const Str&password);
		bool Login(const Str&key_file);

		//服务端
		//加载私钥
		bool LoadHostPrivateKey(const Str&file);
		bool Accept(unsigned port = 22);

		bool Close()
		{
			return SOCK_SUCCESS==async_.Close(handle_);
		}

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
					ssh_conn_ = RefObject<SshConnection>(new SshConnection(other->ssh_conn_->PointType()));
				if (!ssh_conn_->DupFrom(other->ssh_conn_.get()))
					return false;
			}
			phandlers_ = other->phandlers_;
			return true;
		}

	private:
		//发送底层数据
		bool SendRawData(const char*buff, unsigned int buff_len);
		
		//加载验证公钥
		bool LoadAuthorizedKeys(const Str&usename, const Str&file);
	private:
		//网络回调接口
		void OnNetClose(AsyncCloseReason reason, int error);
		
		void OnNetAccept(AsyncHandle client);

		void OnNetConnect();

		void OnNetRecvData(char *buff, unsigned int buff_len);

		void OnNetSendComplete(char *buff, unsigned int buff_len);
	private:
		//回调
		void OnMsgVersion(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgVersion(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		void OnMsgKexInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgKexInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//KEXDH_INIT
		void OnMsgKexDHInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgKexDHInit(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//KEXDH_REPLY
		void OnMsgKexDHReply(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgKexDHReply(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//NEWKEYS
		void OnMsgNewKeys(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgNewKeys(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SERVICE_REQUEST
		void OnMsgServiceRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgServiceRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		void OnMsgServiceAccept(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgServiceAccept(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//USERAUTH_REQUEST
		void OnMsgUserAuthRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthRequest(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);
		
		//USERAUTH_PK_OK
		void OnMsgUserAuthPKok(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthPKok(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SSH_MSG_USERAUTH_FAILURE
		void OnMsgUserAuthFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SSH_MSG_USERAUTH_FAILURE
		void OnMsgUserAuthFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthFailure(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SSH_MSG_USERAUTH_SUCCESS
		void OnMsgUserAuthSucccess(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthSucccess(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SSH_MSG_USERAUTH_BANNER
		void OnMsgUserAuthBanner(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthBanner(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);

		//SSH_MSG_USERAUTH_PASSWD_CHANGEREQ
		void OnMsgUserAuthPassWdChangeReq(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len);
		static void MsgUserAuthPassWdChangeReq(sim::SshTransport*parser,
			const char*payload_data, sim::uint32_t payload_data_len, void*pdata);
	private:
		RefObject<SshConnection> NewSshConnection(SshPointType type)
		{
			RefObject<SshConnection> ssh_conn(new SshConnection(type));
			if (NULL == ssh_conn)
				return NULL;

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
			
		}
	private:
		Mutex channel_map_lock_;
		//local channel id->channel obj
		RbTree< RefObject<SshChannel> > channel_map_;

		AsyncSsh&async_;
		AsyncHandle handle_;

		//ssh协议解析器
		RefObject<SshConnection> ssh_conn_;

		RefObject<SshSessionHandler> phandlers_;

		SSH_SESSION_STATUS status_;
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
			RefObject<SshSession> session = GetSession(handle);
			if (session)
			{
				NewSession(client);
				session->OnNetAccept(client);
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
}
#endif