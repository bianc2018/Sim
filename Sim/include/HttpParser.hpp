/*
	http 解析器 第二个实现
*/
//与httpparser.hpp互斥
#ifndef SIM_HTTP_PARSER_HPP_
#define SIM_HTTP_PARSER_HPP_
#include <stdio.h>
#include <string>

//多线程情况下运行
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif

#include "BaseParser.hpp"

//100M
#define MAX_HTTP_BODY_SIZE 100*1024*1024

#ifndef SIM_HTTP_PAESER_TYPE
#define SIM_HTTP_PAESER_TYPE 1
#endif

#define SIM_HTTP_SPACE			' '
#define SIM_HTTP_CR				'\r'
#define SIM_HTTP_LF				'\n'
#define SIM_HTTP_CRLF			"\r\n"
#define SIM_HTTP_VERSION_0_9	"HTTP/0.9"
#define SIM_HTTP_VERSION_1_0	"HTTP/1.0"
#define SIM_HTTP_VERSION_1_1	"HTTP/1.1"
//Content-Length
#define SIM_HTTP_CL				"Content-Length"
#define SIM_HTTP_CON				"Connection"
#define SIM_HTTP_CT				"Content-Type"
namespace sim
{
	class HttpParser;

	//<scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>
	//httpurl
	struct HttpUrl
	{
		//协议
		Str scheme;
		//主机
		Str host;
		//端口
		unsigned port;
		//资源路径
		Str path;
		HttpUrl() :scheme(""), port(0), path("")
		{

		}
	};

	//长度类型
	typedef  unsigned long long ContentLength_t;

	//请求头
	struct HttpRequestHead
	{
		Str Method;
		Str Url;
		Str Version;

		KvMap Head;

		HttpRequestHead()
		{
			Clear();
		}
		void Clear()
		{
			Method = "GET";
			Url = "/";
			Version = SIM_HTTP_VERSION_1_1;
			Head.Release();
		}
	};

	struct HttpResponseHead
	{
		Str Version;
		Str Status;
		Str Reason;

		KvMap Head;


		HttpResponseHead()
		{
			Clear();
		}
		void Clear()
		{
			Status = "200";
			Reason = "OK";
			Version = SIM_HTTP_VERSION_1_1;
			Head.Release();
		}
	};

	typedef void(*HTTP_REQUEST_HANDLER)(HttpParser*ctx, HttpRequestHead *Head, 
		ContentLength_t content_lenght, ContentLength_t offset,const char*buff, ContentLength_t len,void *pdata);

	typedef void(*HTTP_RESPONSE_HANDLER)(HttpParser*ctx, HttpResponseHead *Head,
		ContentLength_t content_lenght, ContentLength_t offset,
		const char*buff, ContentLength_t len, void *pdata);

	enum HttpParserStatus
	{
		HTTP_START_LINE_CR,
		HTTP_START_LINE_LF,
		HTTP_HEAD_CR,
		HTTP_HEAD_LF,
		HTTP_BODY,
		HTTP_CHUNKED_SIZE_CR,
		HTTP_CHUNKED_SIZE_LF,
		//分块传输
		HTTP_CHUNKED,
		HTTP_CHUNKED_DATA_END,//\r\n
		HTTP_CHUNKED_END,//\r\n\r\n
		HTTP_COMPLETE,//完整
	};

	class HttpParser :public BaseParser
	{
	public:
		HttpParser():
			status_(HTTP_START_LINE_CR),
			max_body_size_(MAX_HTTP_BODY_SIZE),
			BaseParser(SIM_HTTP_PAESER_TYPE),
			content_lenght_(0),
			content_offset_(0)
		{

		}
		~HttpParser() {};

		virtual bool Parser(const char*data, unsigned int len)
		{
			//并行情况
#ifdef SIM_PARSER_MULTI_THREAD
			sim::AutoMutex lk(parser_lock_);
#endif
			/*if (req_hanler_)
				return ParserRequest(data, len);
			if (response_handler_)
				return ParserResponse(data, len);
			return false;*/
			unsigned int offset = 0;
			while (offset < len)
			{
				if (status_ == HTTP_START_LINE_CR)
				{
					if (FindCR(data, len, offset))
						status_ = HTTP_START_LINE_LF;
				}
				else if (status_ == HTTP_START_LINE_LF)
				{
					if (data[offset] == SIM_HTTP_LF)
					{
						++offset;
						if (!OnStartLine())
						{
							printf("OnStartLine error,parser error at %d\n", __LINE__);
							return false;
						}
					}
					else
					{
						printf("SIM_HTTP_LF not found, parser error at %d\n", __LINE__);
						return false;
					}
				}
				else if (status_ == HTTP_HEAD_CR)
				{
					if (FindCR(data, len, offset))
						status_ = HTTP_HEAD_LF;
				}
				else if (status_ == HTTP_HEAD_LF)
				{
					if (data[offset] == SIM_HTTP_LF)
					{
						++offset;
						if (!OnHead())
						{
							printf("OnHead error,parser error at %d\n", __LINE__);
							return false;
						}
					}
					else
					{
						printf("SIM_HTTP_LF not found,parser error at %d\n", __LINE__);
						return false;
					}
				}
				else if (status_ == HTTP_CHUNKED_SIZE_CR)
				{
					if (FindCR(data, len, offset))
						status_ = HTTP_CHUNKED_SIZE_LF;
				}
				else if (status_ == HTTP_CHUNKED_SIZE_LF)
				{
					if (data[offset] == SIM_HTTP_LF)
					{
						++offset;
						if (!OnChunkSize())
						{
							printf("OnChunkSize error,parser error at %d\n", __LINE__);
							return false;
						}
					}
					else
					{
						printf("SIM_HTTP_LF not found,parser error at %d\n", __LINE__);
						return false;
					}
				}
				else if (status_ == HTTP_BODY|| status_ == HTTP_CHUNKED)
				{
					if (!OnBody(data, len, offset))
					{
						printf("OnBody error,parser error at %d\n", __LINE__);
						return false;
					}
				}
				else if (status_ == HTTP_COMPLETE)
				{
					status_ = HTTP_START_LINE_CR;
				}
				else if (status_ == HTTP_CHUNKED_DATA_END)
				{
					temp_ += data[offset++];
					if (temp_.size() >= 2)
					{
						if (temp_ == SIM_HTTP_CRLF)
						{
							status_ = HTTP_CHUNKED_SIZE_CR;
							temp_ = "";
						}
						else
						{
							printf("HTTP_CHUNKED_DATA_END error,parser error at %d\n", __LINE__);
							return false;
						}
					}
				}
				else if (status_ == HTTP_CHUNKED_END)
				{
					temp_ += data[offset++];
					if (temp_.size() >= 4)
					{
						if (temp_ == SIM_HTTP_CRLF SIM_HTTP_CRLF)
						{
							status_ = HTTP_START_LINE_CR;
							temp_ = "";
						}
						else
						{
							printf("HTTP_CHUNKED_END error,parser error at %d\n", __LINE__);
							return false;
						}
					}
				}
				else
				{
					printf("nuknow status %d error,parser error at %d\n", status_, __LINE__);
					return false;
				}
				//是否每次都要回调
				/*if (is_cb_process_)
				{
					OnHandler();
				}*/
			}
			return true;
		}

		static Str PrintStartLine(const Str &Version, const Str &Status, const Str &Reason)
		{
			return Version + SIM_HTTP_SPACE + Status + SIM_HTTP_SPACE + Reason;
		}
		static Str PrintHead(const KvMap &Head)
		{
			Str data = "";
			Head.Traverse(HttpParser::PrintHead, &data);
			return data;
		}

		static Str PrintRequest(HttpRequestHead &Head,
			ContentLength_t content_lenght, ContentLength_t &offset,
			const char*buff, ContentLength_t len)
		{
			Str data = "";
			//打印开头
			if (0 == offset)
			{
				data += PrintStartLine(Head.Method, Head.Url, Head.Version) + SIM_HTTP_CRLF;
			}
			return data+PrintHttpMessage(Head, content_lenght, offset, buff, len);
		}
		static Str PrintResponse(HttpResponseHead &Head,
			ContentLength_t content_lenght, ContentLength_t &offset,
			const char*buff, ContentLength_t len)
		{
			Str data = "";
			//打印开头
			if (0 == offset)
			{
				data += PrintStartLine(Head.Version, Head.Status, Head.Reason) + SIM_HTTP_CRLF;
			}
			return data + PrintHttpMessage(Head, content_lenght, offset, buff, len);

		}

		//获取解析状态
		virtual HttpParserStatus GetParserStatus()
		{
			return status_;
		}

		//设置最大body缓存
		virtual bool SetMaxHttpBodySize(const unsigned int &max_body_size)
		{
			if (max_body_size <= 0)
				return false;
			max_body_size_ = max_body_size;
			return true;
		}

	public:
		// path?RequestUriParams
		//解析请求uri
		static bool ParserRequestUri(const Str &uri, Str &path, KvMap &out)
		{
			int size = uri.size();
			int i = 0;
			for (; i < size; ++i)
			{
				if (uri[i] == '?')
					break;
				path += uri[i];
			}
			if (i < size)
			{
				return ParserRequestUriParams(uri.c_str() + i + 1, out);
			}
			return true;
		}
		//ie=utf-8&f=8&rsv_bp=1&tn=baidu
		//解析请求Uri 上面的参数
		static bool ParserRequestUriParams(const Str &params, KvMap &out)
		{
			//
			bool status = true;//true find '=' false find '&'
			Str key, value;
			int size = params.size();
			for (int i = 0; i < size; ++i)
			{
				if (params[i] == '=')
				{
					status = false;
				}
				else if (params[i] == '&')
				{
					if (key.empty())
						return false;//异常

					status = true;
					out.Append(key, value, HM_APPEND);
					key = "";
					value = "";
				}
				else
				{
					//ie=utf-8&
					if (status)
						key += params[i];
					else
						value += params[i];
				}
			}
			if (!key.empty())
			{
				out.Append(key, value, HM_APPEND);
			}
			return true;
		}

		//<scheme>://<host>:<port>/<path>
		//解析Url
		static bool ParserUrl(const Str &url, HttpUrl &out)
		{
			Str tmp;
			bool find_scheme = false, find_host = false, find_port = false/*,find_path=false*/, has_port = false;
			unsigned int url_len = url.size();
			for (unsigned int i = 0; i < url_len; ++i)
			{
				if (':' == url[i])
				{
					//scheme
					if (i + 1 < url_len && url[i + 1] == '/'&&i + 2 < url_len&&url[i + 2] == '/')
					{
						out.scheme = tmp;
						find_scheme = true;
						tmp = "";
						i += 2;
						continue;
					}
					else
					{
						out.host = tmp;
						find_host = true;
						tmp = "";
						has_port = true;//接下来是port
						continue;
					}
				}
				else if ('/' == url[i])
				{
					if (!find_host)
					{
						out.host = tmp;
						find_host = true;
						tmp = "";
					}
					else if (!find_port&&has_port)
					{
						out.port = HttpParser::StrToInt(tmp);
						find_port = true;
						tmp = "";
					}
					//find_path = true;
				}
				tmp += url[i];
			}

			//填充默认值
			if (!find_scheme)
			{
				out.scheme = "http";
			}
			if (!find_host)
			{
				if (tmp.size() == 0)
					return false;
				out.host = tmp;
				tmp = "";
			}
			if (!find_port)
			{
				if (!has_port)
				{
					if (out.scheme == "http")
						out.port = 80;
					else if (out.scheme == "https")
						out.port = 443;
				}
				else
				{
					//:结尾
					if (tmp.size() == 0)
					{
						return false;
					}
					else
					{
						out.port = HttpParser::StrToInt(tmp);
						tmp = "";
					}
				}

			}
			if (tmp.size() == 0)
				out.path = "/";
			else
				out.path = tmp;
			return true;
		}

		//获取KvMap 中content字段 大小找不到或者不存在返回0
		static  ContentLength_t GetHeadContentLen(KvMap &Head)
		{
			//ContentLength_t
			return HttpParser::StrToNum<ContentLength_t>(Head.GetCase(SIM_HTTP_CL, "0"), 0);
		}
		//检查头是否为关闭
		static  bool IsClose(KvMap &Head)
		{
			return HttpParser::ToLower(Head.GetCase(SIM_HTTP_CON, "Close")) == HttpParser::ToLower("Close");
		}
		//检查是否为chunk
		static  bool IsChunked(KvMap &Head)
		{
			return HttpParser::ToLower(Head.GetCase("Transfer-Encoding", "")) == HttpParser::ToLower("chunked");
		}
		//检查报文是否完整
		static  bool IsComplete(KvMap &Head, ContentLength_t content_offset, ContentLength_t len)
		{
			if (IsChunked(Head))
			{
				if (len == 0)
					return true;
			}
			else
			{
				ContentLength_t content_lenght = GetHeadContentLen(Head);
				//报文完整了
				if (content_lenght <= content_offset + len)
					return true;
			}
			return false;
		}
		//url编码
		static Str EncodeUrl(const Str& URL)
		{
			Str result = "";
			for (unsigned int i = 0; i < URL.size(); i++)
			{
				char c = URL[i];
				if (
					('0' <= c && c <= '9') ||
					('a' <= c && c <= 'z') ||
					('A' <= c && c <= 'Z') ||
					c == '/' || c == '.'
					)
				{
					result += c;
				}
				else
				{
					int j = (short int)c;
					if (j < 0)
					{
						j += 256;
					}
					int i1, i0;
					i1 = j / 16;
					i0 = j - i1 * 16;
					result += '%';
					result += DecToHexChar(i1);
					result += DecToHexChar(i0);
				}
			}
			return result;
		}

		//url解码
		static std::string DecodeUrl(const std::string& URL)
		{
			std::string result = "";
			for (unsigned int i = 0; i < URL.size(); i++)
			{
				char c = URL[i];
				if (c != '%')
				{
					result += c;
				}
				else
				{
					char c1 = URL[++i];
					char c0 = URL[++i];
					int num = 0;
					num += HexCharToDec(c1) * 16 + HexCharToDec(c0);
					result += char(num);
				}
			}
			return result;
		}
	protected:
		virtual bool OnStartLine() = 0;
		virtual bool OnHead() = 0;
		virtual bool OnChunkSize()
		{
			if (temp_.size() == 0)
			{
				printf("OnChunkSize temp_.size() == 0,parser error at %d\n", __LINE__);
				return false;
			}

			//printf("%s\n", temp_.c_str());
			ContentLength_t chunk_size= 0;
			for (int i = 0; i < temp_.size(); ++i)
			{
				short int num = HexCharToDec(temp_[i]);
				if (num == -1)
				{
					printf("OnChunkSize error %c->-1,parser error at %d\n", __LINE__, temp_[i]);
					return false;
				}
				chunk_size = chunk_size * 16 + num;
			}
			temp_ = "";
			if (0 == chunk_size)
			{
				status_ = HTTP_COMPLETE;
				OnHandler(NULL, 0);
				return true;
			}
			content_lenght_ += chunk_size;
			status_ = HTTP_CHUNKED;
			return true;
		}
		virtual bool OnBody(const char*data, unsigned int len, unsigned int &offset)
		{
			//有效字节数
			ContentLength_t valid_bytes = len - offset;
			ContentLength_t need_bytes = content_lenght_ - content_offset_;
			if (need_bytes <= 0)
			{
				return  false;
			}
			ContentLength_t copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//取最小
			if (copy_bytes <= 0)
				return true;

			OnHandler(data + offset, copy_bytes);//返回
			offset += copy_bytes;
			content_offset_ += copy_bytes;
			
			return true;
		}
		virtual bool FindCR(const char*data, unsigned int len, unsigned int &offset)
		{
			for (; offset < len; ++offset)
			{
				if (data[offset] == SIM_HTTP_CR)
				{
					++offset;
					return true;
				}
				temp_ += data[offset];
			}
			return false;
		}
		virtual void OnHandler(const char*buff, ContentLength_t len)=0;
	protected:
		static bool PrintHead(const Str& key, const Str& val, void*pdata)
		{
			Str *pd = (Str*)pdata;
			(*pd) += key + ": " + val + SIM_HTTP_CRLF;
			return true;
		}
		static bool ParserStartLine(const Str&s, Str&d1, Str&d2, Str&d3)
		{
			int index = 0;
			Str t = "";
			for (int i = 0; i < s.size(); ++i)
			{
				if (s[i] == SIM_HTTP_SPACE)
				{
					if (t.size() != 0)
					{
						if (index == 0)
						{
							d1 = t;
						}
						else if (index == 1)
						{
							d2 = t;
						}
						else if (index == 2)
						{
							//d3 = t;
							break;
						}
						++index;
						t = "";

					}
				}
				else
				{
					t += s[i];
				}
			}

			if (index == 2)
			{
				if (t.size() != 0)
					d3 = t;
				return true;
			}
			return false;
		}
		static bool ParserHead(const Str&s, Str&key, Str&value)
		{
			for (int i = 0; i < s.size(); ++i)
			{
				if (s[i] == ':')
				{
					++i;
					for (; i < s.size(); ++i)
						value += s[i];
				}
				else
				{
					key += s[i];
				}
			}
			key = Trim(key);
			value = Trim(value);
			if (key.size() == 0)
				return false;
			return true;
		}

		template<typename HttpMsg>
		static Str PrintHttpMessage(HttpMsg &Head,
			ContentLength_t content_lenght, ContentLength_t &offset,
			const char*buff, ContentLength_t len)
		{
			Str data = "";
			//打印开头
			if (0 == offset)
			{
				if (!IsChunked(Head.Head))//
				{
					if (Head.Head.Count(SIM_HTTP_CL) <= 0)
					{
						Head.Head.Append(SIM_HTTP_CL, NumToStr<ContentLength_t>(content_lenght, "%llu"));
					}
				}
				data += PrintHead(Head.Head) + SIM_HTTP_CRLF;
			}
			if (!IsChunked(Head.Head))//
			{
				if (offset < content_lenght&&buff&&len > 0)
				{
					ContentLength_t need = content_lenght - offset;
					ContentLength_t copy = need > len ? len : need;
					data += Str(buff, copy);
					offset += copy;
				}
			}
			else
			{
				data += NumToStr<ContentLength_t>(len, "%X") + SIM_HTTP_CRLF;
				if (len == 0)
				{
					data += SIM_HTTP_CRLF ;
				}
				else
				{
					data += Str(buff, len);
					offset += len;
					data += SIM_HTTP_CRLF;
				}

			}

			return data;
		}

		template<typename Handler, typename HttpMsg>
		void OnHandler(Handler &handler,void*pdata, HttpMsg &msg,const char*buff, ContentLength_t len)
		{
			if (IsComplete(msg.Head, content_offset_, len))
			{
				status_ = HTTP_COMPLETE;
			}

			/*printf("status %d content_lenght_ %llu content_offset_ %llu buff  %p len %llu\n",
				status_, content_lenght_, content_offset_,
				buff, len);*/
			if (handler)
				handler(this, &msg, content_lenght_, content_offset_, buff, len, pdata);

			if (status_ == HTTP_COMPLETE)
			{
				if (IsChunked(msg.Head))
					status_ = HTTP_CHUNKED_END;
				else
					status_ = HTTP_START_LINE_CR;
				msg.Clear();
				temp_ = "";
				content_lenght_ = 0;
				content_offset_ = 0;

			}
			else
			{
				if (IsChunked(msg.Head) && content_lenght_ <= content_offset_ + len)
					status_ = HTTP_CHUNKED_DATA_END;
			}
		}

		template<typename HttpMsg>
		bool OnHead(HttpMsg &msg)
		{
			if (temp_.size() > 0)
			{
				Str key, value;
				if (!ParserHead(temp_, key, value))
				{
					temp_ = "";
					return false;
				}
				msg.Head.Append(key, value);
				status_ = HTTP_HEAD_CR;
				temp_ = "";
				return true;
			}
			else
			{
				if (IsChunked(msg.Head))
				{
					content_lenght_ = 0;
					content_offset_ = 0;
					status_ = HTTP_CHUNKED_SIZE_CR;
					return true;
				}
				else
				{
					content_lenght_ = GetHeadContentLen(msg.Head);
					content_offset_ = 0;

					if (content_lenght_ == 0)
					{
						status_ = HTTP_COMPLETE;
						OnHandler(NULL, 0);
						return true;
					}
					status_ = HTTP_BODY;
					return true;
				}
			}
		}
	protected:
		//hex转换为字符
		static char DecToHexChar(short int n)
		{
			if (0 <= n && n <= 9)
			{
				return char(short('0') + n);
			}
			else if (10 <= n && n <= 15)
			{
				return char(short('A') + n - 10);
			}
			else
			{
				return char(0);
			}
		}

		//字符转换为
		static short int HexCharToDec(char c)
		{
			if ('0' <= c && c <= '9')
			{
				return short(c - '0');
			}
			else if ('a' <= c && c <= 'f')
			{
				return (short(c - 'a') + 10);
			}
			else if ('A' <= c && c <= 'F') {
				return (short(c - 'A') + 10);
			}
			else {
				return -1;
			}
		}
	protected:

#ifdef SIM_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
		Str temp_;//缓存
		//int content_length_;
		HttpParserStatus status_;

		unsigned int max_body_size_;

		ContentLength_t content_lenght_;
		ContentLength_t content_offset_;
	};

	class HttpRequestParser:public  HttpParser
	{
		//HTTP_REQUEST_HANDLER
	public:
		HttpRequestParser(HTTP_REQUEST_HANDLER handler=NULL, void*pdata=NULL)
			:handler_(handler),pdata_(pdata)
		{}
		void SetHandler(HTTP_REQUEST_HANDLER handler, void*pdata )
		{
			handler_ = handler;
			pdata_ = pdata;
		}
	protected:
		virtual bool OnStartLine() 
		{
			bool ret = ParserStartLine(temp_, t_request_.Method, t_request_.Url, t_request_.Version);
			status_ = HTTP_HEAD_CR;
			temp_ = "";
			return ret;
		}
		virtual bool OnHead()
		{
			return HttpParser::OnHead(t_request_);
			/*if (temp_.size() > 0)
			{
				Str key, value;
				if (!ParserHead(temp_, key, value))
				{
					temp_ = "";
					return false;
				}
				t_request_.Head.Append(key, value);
				status_ = HTTP_HEAD_CR;
				temp_ = "";
				return true;
			}
			else
			{
				content_lenght_ = GetHeadContentLen(t_request_.Head);
				content_offset_ = 0;

				if (content_lenght_ == 0)
				{
					status_ = HTTP_COMPLETE;
					OnHandler(NULL,0);
					return true;
				}
				status_ = HTTP_BODY;
				return true;
			}*/
		}
		
		virtual void OnHandler(const char*buff, ContentLength_t len)
		{
			HttpParser::OnHandler(handler_,pdata_, t_request_, buff, len);
			//报文完整了
			/*if (content_lenght_ <= content_offset_ + len)
				status_ = HTTP_COMPLETE;
			
			if (handler_)
				handler_(this, &t_request_,content_lenght_,content_offset_,buff,len,pdata_);

			if (status_ == HTTP_COMPLETE)
			{
				t_request_.Clear();
				status_ = HTTP_START_LINE_CR;
				temp_ = "";
				content_lenght_ = 0;
				content_offset_ = 0;
			}*/
		}
	private:
		HTTP_REQUEST_HANDLER handler_;
		void*pdata_;

		//缓存项
		HttpRequestHead t_request_;
	};

	class HttpResponseParser :public  HttpParser
	{
		//HTTP_REQUEST_HANDLER
	public:
		HttpResponseParser(HTTP_RESPONSE_HANDLER handler = NULL, void*pdata = NULL)
			:handler_(handler), pdata_(pdata)
		{}
		void SetHandler(HTTP_RESPONSE_HANDLER handler, void*pdata)
		{
			handler_ = handler;
			pdata_ = pdata;
		}
	protected:
		virtual bool OnStartLine()
		{
			bool ret = ParserStartLine(temp_, t_response_.Version, t_response_.Status, t_response_.Reason);
			status_ = HTTP_HEAD_CR;
			temp_ = "";
			return ret;
		}
		virtual bool OnHead()
		{
			return HttpParser::OnHead(t_response_);
			/*if (temp_.size() > 0)
			{
				Str key, value;
				if (!ParserHead(temp_, key, value))
				{
					temp_ = "";
					return false;
				}
				t_response_.Head.Append(key, value);
				status_ = HTTP_HEAD_CR;
				temp_ = "";
				return true;
			}
			else
			{
				if (IsChunked(t_response_.Head))
				{
					content_lenght_ = 0;
					content_offset_ = 0;
					status_ = HTTP_CHUNKED_SIZE_CR;
					return true;
				}
				else
				{
					content_lenght_ = GetHeadContentLen(t_response_.Head);
					content_offset_ = 0;

					if (content_lenght_ == 0)
					{
						status_ = HTTP_COMPLETE;
						OnHandler(NULL, 0);
						return true;
					}
					status_ = HTTP_BODY;
					return true;
				}
			}*/
		}
		virtual void OnHandler(const char*buff, ContentLength_t len)
		{
			HttpParser::OnHandler(handler_,pdata_, t_response_, buff, len);

			//if (IsComplete(t_response_.Head, content_offset_,len))
			//{
			//	status_ = HTTP_COMPLETE;
			//}

			///*printf("status %d content_lenght_ %llu content_offset_ %llu buff  %p len %llu\n",
			//	status_, content_lenght_, content_offset_,
			//	buff, len);*/
			//if (handler_)
			//	handler_(this, &t_response_, content_lenght_, content_offset_, buff, len, pdata_);

			//if (status_ == HTTP_COMPLETE)
			//{
			//	if (IsChunked(t_response_.Head))
			//		status_ = HTTP_CHUNKED_END;
			//	else
			//		status_ = HTTP_START_LINE_CR;
			//	t_response_.Clear();
			//	temp_ = "";
			//	content_lenght_ = 0;
			//	content_offset_ = 0;
			//	
			//}
			//else
			//{
			//	if (IsChunked(t_response_.Head)&& content_lenght_ <= content_offset_ + len)
			//		status_ = HTTP_CHUNKED_DATA_END;
			//}
		}
	private:
		HTTP_RESPONSE_HANDLER handler_;
		void*pdata_;

		//缓存项
		HttpResponseHead t_response_;
	};
}
#endif