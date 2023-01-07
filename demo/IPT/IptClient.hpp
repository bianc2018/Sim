#ifndef LPT_CLI_HPP_
#define LPT_CLI_HPP_
#include "IptProtocol.hpp"
namespace ipt
{
	class CIpcClient
	{
	public:
		CIpcClient(const UdpAddr & stListenAddr,const UdpAddr &stSrvAddr);
		~CIpcClient();

		bool Connect(const std::string& strPeerId);

		bool SendMsg(const std::string& strMsg);

		bool RecvMsg(std::string& strMsg);
	private:

		//���ؼ�����ַ
		UdpAddr m_stListenAddr;
		//����˵�ַ
		UdpAddr m_stSrvAddr;
		//�Զ˵�ַ
		UdpAddr m_stPeerAddr;

		//����
		sim::Socket m_Socket;
	};

	CIpcClient::CIpcClient(const UdpAddr& stListenAddr, const UdpAddr& stSrvAddr)
		:m_stListenAddr(stListenAddr), m_stSrvAddr(stSrvAddr), m_Socket(sim::UDP)
	{

	}
	inline CIpcClient::~CIpcClient()
	{
		m_Socket.Close();
	}
	inline bool CIpcClient::Connect(const std::string& strPeerId)
	{
		m_stPeerAddr.strCliID = strPeerId;
		if (m_Socket.Bind(NULL, m_stListenAddr.nPort) != 0)
			return false;

		while (true)
		{
			//���� todo �����첽�̶߳�ʱ����
			KeepAliveReq stReq;
			stReq.stListenAddr = m_stListenAddr;
			sim::JsonObjectPtr pReqJson = sim::JsonObject::NewObject();
			if (pReqJson->Serialize(stReq))
			{
				std::string strMsg = pReqJson->Print();
				m_Socket.SendTo(strMsg.c_str(), strMsg.size(), m_stSrvAddr.strIp.c_str(), m_stSrvAddr.nPort, 500);
			}
			sim::JsonObject::Free(pReqJson);
			pReqJson = NULL;

			//�ȴ�����˷���
			//������Ϣ
			std::string strMsg;
			if (RecvMsg(strMsg))
			{
				sim::JsonObjectPtr pJson = sim::JsonObject::Parser(strMsg);
				if (pJson)
				{
					KeepAliveRes stRes;
					if (pJson->DeSerialize(stRes))
					{
						//����ֵ���ҵ���Ӧ�Ķ˿�
						for (auto addr : stRes.stAddrList)
						{
							if (addr.strCliID == m_stPeerAddr.strCliID)
							{
								m_stPeerAddr.strIp = addr.strIp;
								m_stPeerAddr.nPort = addr.nPort;
								return true;
							}
						}
					}
					sim::JsonObject::Free(pJson);
				}
			}
		}
		return false;
	}
	inline bool CIpcClient::SendMsg(const std::string& strMsg)
	{
		if (0 == m_Socket.SendTo(strMsg.c_str(), strMsg.size(), m_stPeerAddr.strIp.c_str(), m_stPeerAddr.nPort, 500))
			return true;
		return false;
	}
	inline bool CIpcClient::RecvMsg(std::string& strMsg)
	{
		const int nDataLen = 4 * 1024;
		char czData[nDataLen] = { 0 };
		int nReadLen = m_Socket.Recv(czData, nDataLen, 60 * 1000);
		if (nReadLen > 0)
		{
			strMsg = std::string(czData, nReadLen);
			return true;
		}
		return false;
	}
}
#endif // !LPT_CLI_HPP_
