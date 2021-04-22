/*
* websocket ������ 
*/
#ifndef SIM_WEB_SOCKET_HPP_
#define SIM_WEB_SOCKET_HPP_
#include "BaseParser.hpp"
//���߳����������
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

//0x0��ʾ��������֡
#define SIM_WEBSOCKE_OPCODE_ADDITIONAL	0x0
//0x1��ʾ�ı�����֡
#define SIM_WEBSOCKE_OPCODE_TEXT		0x1
//0x2��ʾ����������֡  binary
#define SIM_WEBSOCKE_OPCODE_BINARY		0x2
//0x3-7��ʱ�޶��壬Ϊ�Ժ�ķǿ���֡����
#define SIM_WEBSOCKE_OPCODE_03			0x3
#define SIM_WEBSOCKE_OPCODE_04			0x4
#define SIM_WEBSOCKE_OPCODE_05			0x5
#define SIM_WEBSOCKE_OPCODE_06			0x6
#define SIM_WEBSOCKE_OPCODE_07			0x7
//0x8��ʾ���ӹر�
#define SIM_WEBSOCKE_OPCODE_CLOSE		0x8
//0x9��ʾping
#define SIM_WEBSOCKE_OPCODE_PING		0x9
//0xA��ʾpong
#define SIM_WEBSOCKE_OPCODE_PONG		0xA
//0xB-F��ʱ�޶��壬Ϊ�Ժ�Ŀ���֡����
#define SIM_WEBSOCKE_OPCODE_0B			0xB
#define SIM_WEBSOCKE_OPCODE_0C			0xC
#define SIM_WEBSOCKE_OPCODE_0D			0xD
#define SIM_WEBSOCKE_OPCODE_0E			0xE
#define SIM_WEBSOCKE_OPCODE_0F			0xF

namespace sim
{
	class WebSocketParser;
	
	typedef unsigned long long PayLoadLength_t;

	//websocket ����֡
	struct WebSocketFrame
	{
		//1λ������������Ϣ�Ƿ���������Ϊ1�����ϢΪ��Ϣβ��,���Ϊ�����к������ݰ�;
		bool fin;
		//Ϊ0
		bool rsv1;
		bool rsv2;
		bool rsv3;

		//���ڱ�ʾ��Ϣ�������ͣ�������յ�δ֪��opcode�����ն˱���ر����ӡ�������̽������������ʶ�ġ�
		/*
		* OPCODE����ķ�Χ��

��������0x0��ʾ��������֡
��������0x1��ʾ�ı�����֡
��������0x2��ʾ����������֡
��������0x3-7��ʱ�޶��壬Ϊ�Ժ�ķǿ���֡����
��������0x8��ʾ���ӹر�
��������0x9��ʾping
��������0xA��ʾpong
��������0xB-F��ʱ�޶��壬Ϊ�Ժ�Ŀ���֡����
		*/
		unsigned char opcode;
		
		//1λ�����ڱ�ʶPayloadData�Ƿ񾭹����봦���ͻ��˷���������֡��Ҫ�������봦�����Դ�λ��1��������Ҫ���롣
		bool mask;

		//���ݳ���
		/*
		* Payload length === x�����

����	��� xֵ��0-125������payload����ʵ���ȡ�
����	��� xֵ��126�������2���ֽ��γɵ�16λ�޷�����������ֵ��payload����ʵ���ȡ�
����	��� xֵ��127�������8���ֽ��γɵ�64λ�޷�����������ֵ��payload����ʵ���ȡ�

����	���⣬���payload lengthռ���˶���ֽڵĻ���payload length�Ķ����Ʊ�����������big endian����Ҫ��λ��ǰ����
		*/
		PayLoadLength_t payload_length;

		//if mask=true 4�ֽ�
		unsigned int masking_key;

		//�غ�����
		Str payload_data;
	};

	//�ص�
	typedef void(*WEBSOCKET)(WebSocketParser* parser,
		WebSocketFrame* pFrame,

		void* pdata);

	class WebSocketParser :public sim::BaseParser
	{
	public:
		virtual bool Parser(const char* data, unsigned int len)
		{
			//�������
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
