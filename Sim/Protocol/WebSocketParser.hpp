/*
* websocket 解析器 
*/
#ifndef SIM_WEB_SOCKET_HPP_
#define SIM_WEB_SOCKET_HPP_

#include <stdlib.h>
#include <time.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#ifndef OS_WINDOWS
	#define OS_WINDOWS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  
#endif
#include <WinSock2.h>
#elif defined(linux) || defined(__linux) || defined(__linux__)
#ifndef OS_LINUX
	#define OS_LINUX
#endif  
#include <netinet/in.h>
#include <sys/time.h>
#else
#error "不支持的平台"
#endif

#include "BaseParser.hpp"
#include "Base64.hpp"
#include "Sha1.hpp"

#ifndef SIM_WEBSOCKET_PAESER_TYPE
#define SIM_WEBSOCKET_PAESER_TYPE 2
#endif

//多线程情况下运行
#ifdef SIM_WS_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

//0x0表示附加数据帧
#define SIM_WS_OPCODE_ADDITIONAL	0x0
//0x1表示文本数据帧
#define SIM_WS_OPCODE_TEXT		0x1
//0x2表示二进制数据帧  binary
#define SIM_WS_OPCODE_BINARY		0x2
//0x3-7暂时无定义，为以后的非控制帧保留
#define SIM_WS_OPCODE_03			0x3
#define SIM_WS_OPCODE_04			0x4
#define SIM_WS_OPCODE_05			0x5
#define SIM_WS_OPCODE_06			0x6
#define SIM_WS_OPCODE_07			0x7
//0x8表示连接关闭
#define SIM_WS_OPCODE_CLOSE		0x8
//0x9表示ping
#define SIM_WS_OPCODE_PING		0x9
//0xA表示pong
#define SIM_WS_OPCODE_PONG		0xA
//0xB-F暂时无定义，为以后的控制帧保留
#define SIM_WS_OPCODE_0B			0xB
#define SIM_WS_OPCODE_0C			0xC
#define SIM_WS_OPCODE_0D			0xD
#define SIM_WS_OPCODE_0E			0xE
#define SIM_WS_OPCODE_0F			0xF

//magic 
#define SIM_WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

//https://www.cnblogs.com/wfwenchao/p/5541455.html
//网络序转换
namespace sim
{
	typedef unsigned long long PayLoadLength_t;

#ifndef OS_WINDOWS
	PayLoadLength_t htonll(PayLoadLength_t val)
	{
		if (1 == htonl(1))
			return val;
		return (((PayLoadLength_t)htonl(val)) << 32) + htonl(val >> 32);
	}

	PayLoadLength_t ntohll(PayLoadLength_t val)
	{
		if (1 == htonl(1))
			return val;
		return (((PayLoadLength_t)ntohl(val)) << 32) + ntohl(val >> 32);
	}
#endif

	class WebSocketParser;
	
	

	enum WebSocketParserStatus
	{
		//第一个字节 
		WS_FIRSET_BYTES,
		//第二个字节
		WS_SECOND_BYTES,
		//2字节长度
		WS_PAYLOAD_LEN_2,
		//8字节长度
		WS_PAYLOAD_LEN_8,
		//masking key 8
		WS_MASKING_KEY,
		//载荷
		WS_PAYLOAD,
		//完整报文
		WS_COMPLETE,
	};

	//websocket 传输帧头
	struct WebSocketFrameHead
	{
		//1位，用于描述消息是否结束，如果为1则该消息为消息尾部,如果为零则还有后续数据包;
		bool fin;
		//为0
		bool rsv1;
		bool rsv2;
		bool rsv3;

		/*
		* 用于表示消息接收类型，如果接收到未知的opcode，接收端必须关闭连接。长连接探活包就是这里标识的。
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
		unsigned char masking_key[4];

		//载荷数据，回调中返回
		//Str payload_data;
		WebSocketFrameHead()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	//回调
	typedef void(*WEBSOCKET_HANDLER)(WebSocketParser* parser,
		WebSocketFrameHead* pFrame, PayLoadLength_t payload_offset,
		const char*payload_data, PayLoadLength_t data_len,
		void* pdata);

	class WebSocketParser :public sim::BaseParser
	{
	public:
		WebSocketParser(WEBSOCKET_HANDLER handler=NULL, void*pdata=NULL)
			:BaseParser(SIM_WEBSOCKET_PAESER_TYPE),handler_(handler),pdata_(pdata)
			, status_(WS_FIRSET_BYTES), payload_offset_(0)
		{
			memset(&t_frame_, 0, sizeof(t_frame_));
		}
		virtual bool Parser(const char* data, unsigned int len)
		{
			//并行情况
#ifdef SIM_WS_PARSER_MULTI_THREAD
			sim::AutoMutex lk(parser_lock_);
#endif
			unsigned int offset = 0;
			while (offset < len)
			{
				if (status_ == WS_FIRSET_BYTES)
				{
					payload_offset_ = 0;
					OnFirstBytes((unsigned char)data[offset++]);
				}
				else if (status_ == WS_SECOND_BYTES)
				{
					OnSecondBytes((unsigned char)data[offset++]);
				}
				else if (status_ == WS_PAYLOAD_LEN_2)
				{
					if (temp_.size() == 2)
					{
						memcpy(&t_frame_.payload_length, temp_.c_str(), 2);
						t_frame_.payload_length= htons((unsigned short)t_frame_.payload_length);
						if (t_frame_.mask)
							status_ = WS_MASKING_KEY;
						else
							status_ = WS_PAYLOAD;
						temp_ = "";
					}
					else
					{
						temp_ += data[offset++];
					}
				}
				else if (status_ == WS_PAYLOAD_LEN_8)
				{
					if (temp_.size() == 8)
					{
						memcpy(&t_frame_.payload_length, temp_.c_str(), 8);
						t_frame_.payload_length = htonll((unsigned short)t_frame_.payload_length);
						if (t_frame_.mask)
							status_ = WS_MASKING_KEY;
						else
						{
							status_ = WS_PAYLOAD;
							
						}
						temp_ = "";
					}
					else
					{
						temp_ += data[offset++];
					}
				}
				else if (status_ == WS_MASKING_KEY)
				{
					if (temp_.size() == 4)
					{
						memcpy(&t_frame_.masking_key, temp_.c_str(), 4);
						status_ = WS_PAYLOAD;
						temp_ = "";
					}
					else
					{
						temp_ += data[offset++];
					}
				}
				else if (status_ == WS_PAYLOAD)
				{
					if (false == OnPayload(data, len, offset))
					{
						return false;
					}
				}
				else if (status_ == WS_COMPLETE)
				{
					status_ = WS_FIRSET_BYTES;
				}
				else
				{
					return false;
				}
			}
			return true;
		}
		virtual void SetHandler(WEBSOCKET_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
		}

	public:
		//
		static  Str PrintFrameHead(WebSocketFrameHead& FrameHead,bool is_GenerateMaskingKey=true)
		{
			Str temp;
			unsigned char first = 0;
			if (FrameHead.fin)
			{
				first |= (1 << 7);
			}
			if (FrameHead.rsv1)
			{
				first |= (1 << 6);
			}
			if (FrameHead.rsv2)
			{
				first |= (1 << 5);
			}
			if (FrameHead.rsv3)
			{
				first |= (1 << 4);
			}
			first |= FrameHead.opcode;
			temp += (char)first;

			unsigned char second = 0;
			if (FrameHead.mask)
			{
				second |= (1 << 7);
			}
			if (FrameHead.payload_length < 126)
			{
				second |= (unsigned char)FrameHead.payload_length;
				temp += (char)second;
			}
			else if (FrameHead.payload_length < 0xff)
			{
				second |= 126;
				temp += (char)second;
				unsigned short len = ntohs((unsigned short)FrameHead.payload_length);
				temp += Str((const char*)&len, 2);
			}
			else
			{
				second |= 127;
				temp += (char)second;
				PayLoadLength_t len = ntohll(FrameHead.payload_length);
				temp += Str((const char*)&len, 8);
			}

			if (FrameHead.mask)
			{
				if (is_GenerateMaskingKey)
					GenerateMaskingKey(FrameHead.masking_key);
				temp += Str((char*)FrameHead.masking_key, 4);
			}
			return temp;
		}

		static  Str PrintFrame(WebSocketFrameHead& FrameHead, PayLoadLength_t &payload_offset
			,const char*payload_data, PayLoadLength_t data_len, bool is_GenerateMaskingKey = true)
		{
			Str data;
			if (payload_offset == 0)
			{
				data = PrintFrameHead(FrameHead, is_GenerateMaskingKey);
			}
			if (payload_data != NULL && data_len > 0&& payload_offset< FrameHead.payload_length)
			{
				PayLoadLength_t need_bytes = FrameHead.payload_length - payload_offset;
				PayLoadLength_t copy_bytes = data_len > need_bytes ? need_bytes : data_len;//取最小
				if (copy_bytes != 0)
				{
					if (FrameHead.mask)
					{
						//掩码操作
						for (int i = 0; i < copy_bytes; ++i)
						{
							data += (FrameHead.masking_key[(payload_offset+i)%4]) ^ (payload_data[i]);
						}
					}
					else
					{
						data += Str(payload_data, copy_bytes);
					}
					payload_offset += copy_bytes;
				}
			}
			return data;
		}
		
		static  Str PrintFrame(WebSocketFrameHead& FrameHead
			, const char*payload_data, PayLoadLength_t data_len, bool is_GenerateMaskingKey = true)
		{
			PayLoadLength_t payload_offset;
			FrameHead.payload_length = data_len;
			return PrintFrame(FrameHead, payload_offset, payload_data, data_len, is_GenerateMaskingKey);
		}
		//unsigned char masking_key
		static bool GenerateMaskingKey(unsigned char *masking_key)
		{
			return GenerateRandArray(masking_key,4);
		}
		
		//Sec-WebSocket-Key
		static Str GenerateSecWebSocketKey()
		{
			//随机生成
			const int rand_data_len = 32;
			unsigned char rand_data[rand_data_len] = { 0 };
			GenerateRandArray(rand_data, rand_data_len);

			const int base64_buff_size = 128;
			char base64_buff[base64_buff_size] = { 0 };
			if (0 == Base64::encode(rand_data, rand_data_len,
				base64_buff, base64_buff_size))
				return "";
			return base64_buff;
		}
		
		//Sec-WebSocket-Accept
		static Str GenerateSecWebSocketAccept(const Str&sec_websocket_key)
		{
			//“Sec-WebSocket-Key”加上一个魔幻字符串“258EAFA5-E914-47DA-95CA-C5AB0DC85B11”。
			//使用SHA-1加密，之后进行BASE-64编码，将结果做为“Sec-WebSocket-Accept”头的值，返回给客户端。
			Str sec = sec_websocket_key + SIM_WS_MAGIC;
			
			unsigned message_digest_array[SIM_SHA1_SIZE] = { 0 };
			Sha1 sha1;
			const int base64_buff_size = 128;
			char base64_buff[base64_buff_size] = { 0 };

			sha1.Input(sec.c_str(), sec.size());
			if (false == sha1.Result(message_digest_array))
				return "";

			for (int i = 0; i < SIM_SHA1_SIZE; i++) {
				message_digest_array[i] = ::htonl(message_digest_array[i]);
			}
			if (0 == Base64::encode((const unsigned char*)message_digest_array, SIM_SHA1_SIZE * sizeof(unsigned),
				base64_buff, base64_buff_size))
				return "";
			return base64_buff;
		}
	protected:
		virtual void OnFirstBytes(unsigned  char bytes)
		{
			t_frame_.fin  = (1 << 7)&bytes;
			t_frame_.rsv1 = (1 << 6)&bytes;
			t_frame_.rsv2 = (1 << 5)&bytes;
			t_frame_.rsv3 = (1 << 4)&bytes;

			//后四位0x0f
			t_frame_.opcode = 0x0f & bytes;
			
			//第二个字节
			status_ = WS_SECOND_BYTES;
		}
		virtual void OnSecondBytes(unsigned  char bytes)
		{
			//mask
			t_frame_.mask = (1 << 7)&bytes;
			t_frame_.payload_length = 0x7f & bytes;

			if (t_frame_.payload_length == 126)
			{
				status_ = WS_PAYLOAD_LEN_2;
			}
			else if (t_frame_.payload_length == 127)
			{
				status_ = WS_PAYLOAD_LEN_8;
			}
			else 
			{
				if (t_frame_.mask)
					status_ = WS_MASKING_KEY;
				else
				{
					status_ = WS_PAYLOAD;
				}
			}
		}
		virtual bool OnPayload(const char*data, unsigned int len, unsigned int &offset)
		{
			PayLoadLength_t valid_bytes = len - offset;
			if (valid_bytes <= 0)
				return true;

			PayLoadLength_t need_bytes = t_frame_.payload_length - payload_offset_;
			if (need_bytes <= 0)
			{
				status_ = WS_COMPLETE;
				OnHandler(NULL, 0);
				return true;
			}

			PayLoadLength_t copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//取最小
			if (copy_bytes <= 0)
				return true;

			//是否掩码
			if (!t_frame_.mask)
			{
				OnHandler(data + offset, copy_bytes);//返回
			}
			else
			{
				//掩码操作
				temp_ = "";
				for (int i = 0; i < copy_bytes; ++i)
				{
					temp_ += (t_frame_.masking_key[(payload_offset_+i)%4])^((unsigned char)data[offset+i]);
				}
				OnHandler(temp_.c_str(), temp_.size());
				temp_ = "";
			}
			offset += copy_bytes;
			payload_offset_ += copy_bytes;
			return true;
		}
		virtual void OnHandler(const char*buff, PayLoadLength_t len)
		{
			//报文完整了
			if (t_frame_.payload_length <= payload_offset_ + len)
				status_ = WS_COMPLETE;

			if (handler_)
				handler_(this, &t_frame_, payload_offset_, buff, len, pdata_);

			if (status_ == WS_COMPLETE)
			{
				memset(&t_frame_, 0, sizeof(t_frame_));
				temp_ = "";
				payload_offset_ = 0;
				status_ = WS_FIRSET_BYTES;
			}
		}
	protected:
#ifdef SIM_WS_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
		//头
		WebSocketFrameHead t_frame_;
		//缓存数据
		Str temp_;
		//载荷偏移
		PayLoadLength_t payload_offset_;

		WEBSOCKET_HANDLER handler_;
		void* pdata_;

		WebSocketParserStatus status_;
	};
}
#endif
