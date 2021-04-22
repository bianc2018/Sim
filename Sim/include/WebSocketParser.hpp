/*
* websocket 解析器 
*/
#ifndef SIM_WEB_SOCKET_HPP_
#define SIM_WEB_SOCKET_HPP_
#include "BaseParser.hpp"
//多线程情况下运行
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

//0x0表示附加数据帧
#define SIM_WEBSOCKE_OPCODE_ADDITIONAL	0x0
//0x1表示文本数据帧
#define SIM_WEBSOCKE_OPCODE_TEXT		0x1
//0x2表示二进制数据帧  binary
#define SIM_WEBSOCKE_OPCODE_BINARY		0x2
//0x3-7暂时无定义，为以后的非控制帧保留
#define SIM_WEBSOCKE_OPCODE_03			0x3
#define SIM_WEBSOCKE_OPCODE_04			0x4
#define SIM_WEBSOCKE_OPCODE_05			0x5
#define SIM_WEBSOCKE_OPCODE_06			0x6
#define SIM_WEBSOCKE_OPCODE_07			0x7
//0x8表示连接关闭
#define SIM_WEBSOCKE_OPCODE_CLOSE		0x8
//0x9表示ping
#define SIM_WEBSOCKE_OPCODE_PING		0x9
//0xA表示pong
#define SIM_WEBSOCKE_OPCODE_PONG		0xA
//0xB-F暂时无定义，为以后的控制帧保留
#define SIM_WEBSOCKE_OPCODE_0B			0xB
#define SIM_WEBSOCKE_OPCODE_0C			0xC
#define SIM_WEBSOCKE_OPCODE_0D			0xD
#define SIM_WEBSOCKE_OPCODE_0E			0xE
#define SIM_WEBSOCKE_OPCODE_0F			0xF

namespace sim
{
	class WebSocketParser;
	
	typedef unsigned long long PayLoadLength_t;

	//websocket 传输帧
	struct WebSocketFrame
	{
		//1位，用于描述消息是否结束，如果为1则该消息为消息尾部,如果为零则还有后续数据包;
		bool fin;
		//为0
		bool rsv1;
		bool rsv2;
		bool rsv3;

		//用于表示消息接收类型，如果接收到未知的opcode，接收端必须关闭连接。长连接探活包就是这里标识的。
		/*
		* OPCODE定义的范围：

　　　　0x0表示附加数据帧
　　　　0x1表示文本数据帧
　　　　0x2表示二进制数据帧
　　　　0x3-7暂时无定义，为以后的非控制帧保留
　　　　0x8表示连接关闭
　　　　0x9表示ping
　　　　0xA表示pong
　　　　0xB-F暂时无定义，为以后的控制帧保留
		*/
		unsigned char opcode;
		
		//1位，用于标识PayloadData是否经过掩码处理，客户端发出的数据帧需要进行掩码处理，所以此位是1。数据需要解码。
		bool mask;

		//数据长度
		/*
		* Payload length === x，如果

　　	如果 x值在0-125，则是payload的真实长度。
　　	如果 x值是126，则后面2个字节形成的16位无符号整型数的值是payload的真实长度。
　　	如果 x值是127，则后面8个字节形成的64位无符号整型数的值是payload的真实长度。

　　	此外，如果payload length占用了多个字节的话，payload length的二进制表达采用网络序（big endian，重要的位在前）。
		*/
		PayLoadLength_t payload_length;

		//if mask=true 4字节
		unsigned int masking_key;

		//载荷数据
		Str payload_data;
	};

	//回调
	typedef void(*WEBSOCKET)(WebSocketParser* parser,
		WebSocketFrame* pFrame,

		void* pdata);

	class WebSocketParser :public sim::BaseParser
	{
	public:
		virtual bool Parser(const char* data, unsigned int len)
		{
			//并行情况
#ifdef SIM_PARSER_MULTI_THREAD
			sim::AutoMutex lk(parser_lock_);
#endif
			return true;
		}
	protected:
#ifdef SIM_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
	};
}
#endif
