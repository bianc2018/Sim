#ifndef LPT_PROTOCOL_HPP_
#define LPT_PROTOCOL_HPP_
#define IP_ADDR_LEN 32
#define IPT_CLI_ID_LEN	32

#include "Socket.hpp"
#include "Json.hpp"
#include "Logger.hpp"

#include <string>
#include <vector>

//json序列化不允许在命名空间内
//namespace ipt
//{
	//IP地址链接
	struct UdpAddr
	{
		//客户端标识，用于定位对端主机
		std::string strCliID;
		//打开监听的端口
		std::string strIp;
		int nPort;


		SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
			SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(strCliID)
			SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(strIp)
			SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(nPort)
		SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
	};

	struct KeepAliveReq
	{
		//客户端标识，用于定位对端主机
		UdpAddr stListenAddr;

		SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
			SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(stListenAddr)
		SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
	};

	struct KeepAliveRes
	{
		std::vector<UdpAddr> stAddrList;
		
		SIM_DEF_JSON_SERIALIZE_IN_STRUCT()
			SIM_JSON_SERIALIZE_VALUE_IN_STRUCT_2(stAddrList)
		SIM_DEF_JSON_SERIALIZE_IN_STRUCT_END()
	};

//}
#endif // !LPT_PROTOCOL_HPP_
