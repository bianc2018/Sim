#ifndef LPT_SRV_HPP_
#define LPT_SRV_HPP_
#include "IptProtocol.hpp"
namespace ipt
{
	class CIpcServer
	{
	public:
		//监听端口
		CIpcServer(int nPort);
		~CIpcServer();

		//监听请求,会堵塞
		bool RunAccept();

	private:
		//缓存的服务地址
		std::vector<UdpAddr> m_stAddrList;
		int m_nPort;
	};

	CIpcServer::CIpcServer(int nPort)
		:m_nPort(nPort)
	{

	}
	inline CIpcServer::~CIpcServer()
	{

	}
	inline bool CIpcServer::RunAccept()
	{
		sim::Socket sServer(sim::UDP);
		if (sServer.Bind("0.0.0.0", m_nPort) != 0)
			return false;
		
		SIM_LDEBUG("Listen[Udp][0.0.0.0:" << m_nPort << "]");

		while (true)
		{
			const int nIpLen = 32;
			char czIp[nIpLen] = { 0 };
			unsigned short nPort = 0;

			//接收信息
			const int nDataLen = 4 * 1024;
			char czData[nDataLen] = { 0 };

			int nReadLen = sServer.Recvfrom(czData, nDataLen, czIp, nIpLen, &nPort);
			if (nReadLen > 0)
			{
				std::string strMsg(czData, nReadLen);
				sim::JsonObjectPtr pJson= sim::JsonObject::Parser(strMsg);
				if (pJson)
				{
					KeepAliveReq stReq;
					if (pJson->DeSerialize(stReq))
					{
						bool bFind = false;
						for (auto &addr : m_stAddrList)
						{
							if (addr.strCliID == stReq.stListenAddr.strCliID)
							{
								if (addr.strIp != czIp|| addr.nPort != nPort)
								{
									addr.strIp = czIp;
									addr.nPort = nPort;
									SIM_LDEBUG("Update [" << addr.strCliID << "][" << addr.strIp << ":" << addr.nPort << "]");
								}
								
								bFind = true;
								break;
							}
						}
						if (!bFind)
						{
							UdpAddr addr;
							addr.strCliID = stReq.stListenAddr.strCliID;

							addr.strIp = czIp;
							addr.nPort = nPort;
							m_stAddrList.push_back(addr);
							SIM_LDEBUG("Add [" << addr.strCliID << "][" << addr.strIp << ":" << addr.nPort << "]");
						}

						//回复
						KeepAliveRes stRes;
						stRes.stAddrList = m_stAddrList;
						sim::JsonObjectPtr pResJson = sim::JsonObject::NewObject();
						if (pResJson->Serialize(stRes))
						{
							strMsg = pResJson->Print();
							sServer.SendTo(strMsg.c_str(), strMsg.size(), czIp, nPort, 500);
						}
						sim::JsonObject::Free(pResJson);
					}
					sim::JsonObject::Free(pJson);

				}
			}
		}
		return false;
	}
}
#endif // !LPT_SRV_HPP_