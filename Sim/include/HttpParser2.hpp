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
							return false;
					}
					else
					{
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
							return false;
					}
					else
					{
						return false;
					}
				}
				else if (status_ == HTTP_BODY)
				{
					if (!OnBody(data, len, offset))
						return false;
				}
				else if (status_ == HTTP_COMPLETE)
				{
					status_ = HTTP_START_LINE_CR;
				}
				else
				{
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
				if (Head.Head.Count(SIM_HTTP_CL) <= 0)
				{
					Head.Head.Append(SIM_HTTP_CL, NumToStr<ContentLength_t>(content_lenght,"%llu"));
				}
				data += PrintHead(Head.Head) + SIM_HTTP_CRLF;
			}
			if (offset < content_lenght&&buff&&len>0)
			{
				ContentLength_t need = content_lenght - offset;
				ContentLength_t copy = need > len ? len : need;
				data += Str(buff, copy);
				offset += copy;
			}
			return data;
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
				if (Head.Head.Count(SIM_HTTP_CL) <= 0)
				{
					Head.Head.Append(SIM_HTTP_CL, NumToStr<ContentLength_t>(content_lenght, "%llu"));
				}
				data += PrintHead(Head.Head) + SIM_HTTP_CRLF;
			}
			if (offset < content_lenght&&buff&&len>0)
			{
				ContentLength_t need = content_lenght - offset;
				ContentLength_t copy = need > len ? len : need;
				data += Str(buff, copy);
				offset += copy;
			}
			return data;

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
	protected:
		virtual bool OnStartLine() = 0;
		virtual bool OnHead() = 0;
		virtual bool OnBody(const char*data, unsigned int len, unsigned int &offset)
		{
			//有效字节数
			unsigned int valid_bytes = len - offset;
			unsigned int need_bytes = content_lenght_ - content_offset_;
			if (need_bytes <= 0)
			{
				status_ = HTTP_COMPLETE;
				OnHandler(NULL, 0);
				return  true;
			}
			unsigned int copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//取最小
			OnHandler(data + offset, copy_bytes);//返回
			offset += copy_bytes;

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
			if (temp_.size() > 0)
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
			}
		}
		
		virtual void OnHandler(const char*buff, ContentLength_t len)
		{
			//报文完整了
			if (content_lenght_ <= content_offset_ + len)
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
			}
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
			if (temp_.size() > 0)
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
		}
		virtual void OnHandler(const char*buff, ContentLength_t len)
		{
			//报文完整了
			if (content_lenght_ <= content_offset_ + len)
				status_ = HTTP_COMPLETE;

			if (handler_)
				handler_(this, &t_response_, content_lenght_, content_offset_, buff, len, pdata_);

			if (status_ == HTTP_COMPLETE)
			{
				t_response_.Clear();
				status_ = HTTP_START_LINE_CR;
				temp_ = "";
				content_lenght_ = 0;
				content_offset_ = 0;
			}
		}
	private:
		HTTP_RESPONSE_HANDLER handler_;
		void*pdata_;

		//缓存项
		HttpResponseHead t_response_;
	};
}
#endif