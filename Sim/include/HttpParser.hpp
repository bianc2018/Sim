/*
	HTTP 解析器
*/
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
		HttpUrl() :scheme(""),port(0), path("")
		{

		}
	};

	//长度类型
	typedef  unsigned long long ContentLength_t;
	//content 状态
	struct HttpContent
	{
		//当前块
		Str chunk;
		//是否为完整的。true就是整个报文了，false 只是其中一块
		bool is_complete;
		//是否最后一次回调，回调之后回收
		bool is_last;
		//报文体的长度
		ContentLength_t content_length;
		//当前块的开头
		ContentLength_t offset;

		HttpContent()
		{
			Clear();
		}
		void Clear()
		{
			chunk = "";
			is_complete = true;
			content_length = 0;
			offset = 0;
			is_last = false;
		}
	};

	struct HttpRequest
	{
		Str Method;
		Str Url;
		Str Version;
		
		KvMap Head;

		HttpContent Content;

		HttpRequest()
		{
			Clear();
		}
		void Clear()
		{
			Method = "";
			Url = "";
			Version = SIM_HTTP_VERSION_1_1;
			
			Head.Release();
		}
	};

	struct HttpResponse
	{
		Str Version;
		Str Status;
		Str Reason;

		KvMap Head;

		HttpContent Content;

		HttpResponse()
		{
			Clear();
		}
		void Clear()
		{
			Status = "200";
			Reason = "OK";
			Version = SIM_HTTP_VERSION_1_1;
			Content.Clear();
			Head.Release();
		}
	};
	
	typedef void(*HTTP_REQUEST_HANDLER)(HttpParser*ctx, HttpRequest *request, void *pdata);

	typedef void(*HTTP_RESPONSE_HANDLER)(HttpParser*ctx, HttpResponse *response, void *pdata);

	enum HttpParserStatus
	{
		HTTP_START_LINE_CR,
		HTTP_START_LINE_LF,
		HTTP_HEAD_CR,
		HTTP_HEAD_LF,
		HTTP_BODY,
		HTTP_CHUNK,//body块
		HTTP_COMPLETE,//完整
	};

	class HttpParser:public BaseParser
	{
	public:
		HttpParser(HTTP_REQUEST_HANDLER req_hanler, void*pdata,bool is_cb_process = false)
			:req_hanler_(req_hanler),
			pdata_(pdata), 
			response_handler_(NULL),
			status_(HTTP_START_LINE_CR),
			is_cb_process_(is_cb_process),
			max_body_size_(MAX_HTTP_BODY_SIZE),
			BaseParser(SIM_HTTP_PAESER_TYPE)
		{
			
		}

		HttpParser(HTTP_RESPONSE_HANDLER response_handler, void*pdata, bool is_cb_process = false)
			:req_hanler_(NULL), 
			pdata_(pdata), 
			response_handler_(response_handler), 
			status_(HTTP_START_LINE_CR),
			is_cb_process_(is_cb_process),
			max_body_size_(MAX_HTTP_BODY_SIZE),
			BaseParser(SIM_HTTP_PAESER_TYPE)
		{}
		~HttpParser() {};

		virtual bool Parser(const char*data, unsigned int len)
		{
			//并行情况
#ifdef SIM_PARSER_MULTI_THREAD
			sim::AutoMutex lk( parser_lock_);
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
				if (is_cb_process_)
				{
					OnHandler();
				}
			}
			return true;
		}

		static Str PrintStartLine(const Str &Version, const Str &Status, const Str &Reason)
		{
			return Version + SIM_HTTP_SPACE + Status + SIM_HTTP_SPACE + Reason;
		}
		static Str PrintHead(const KvMap &Head)
		{
			Str data="";
			Head.Traverse(HttpParser::PrintHead, &data);
			return data;
		}
		static Str PrintRequest(HttpRequest *request)
		{
			if (NULL == request|| request->Content.is_complete == false)
				return "";
			Str data = request->Method+SIM_HTTP_SPACE+request->Url+SIM_HTTP_SPACE+request->Version+ SIM_HTTP_CRLF;
			if (request->Head.Count(SIM_HTTP_CL) <= 0)
			{
				request->Head.Append(SIM_HTTP_CL, IntToStr(request->Content.chunk.size()));
			}
			request->Head.Traverse(HttpParser::PrintHead, &data);
			data += SIM_HTTP_CRLF;
			data += request->Content.chunk;
			return data;
		}
		static Str PrintResponse(HttpResponse *response)
		{
			if (NULL == response||response->Content.is_complete == false)
				return "";
			Str data = response->Version + SIM_HTTP_SPACE + response->Status + SIM_HTTP_SPACE + response->Reason + SIM_HTTP_CRLF;
			if (response->Head.Count(SIM_HTTP_CL) <= 0)
				response->Head.Append(SIM_HTTP_CL, IntToStr(response->Content.chunk.size()));
			response->Head.Traverse(HttpParser::PrintHead, &data);
			data += SIM_HTTP_CRLF;
			data += response->Content.chunk;
			return data;

		}

		//获取解析状态
		HttpParserStatus GetParserStatus()
		{
			return status_;
		}

		//设置最大body缓存
		bool SetMaxHttpBodySize(const unsigned int &max_body_size)
		{
			if(max_body_size<=0)
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
			bool find_scheme = false, find_host = false,find_port=false/*,find_path=false*/,has_port=false;
			unsigned int url_len= url.size();
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
	private:
		bool OnStartLine()
		{
			if (req_hanler_)
			{
				ParserStartLine(temp_, t_request_.Method, t_request_.Url, t_request_.Version);
			}
			else
			{
				ParserStartLine(temp_, t_response_.Version, t_response_.Status, t_response_.Reason);
			}
			status_ = HTTP_HEAD_CR;
			temp_ = "";
			return true;
		}
		bool OnHead()
		{
			if (temp_.size()>0)
			{
				Str key, value;
				if (!ParserHead(temp_, key, value))
				{
					temp_ = "";
					return false;
				}
				if (req_hanler_)
				{
					t_request_.Head.Append(key, value);
				}
				else
				{
					t_response_.Head.Append(key, value);
				}
				status_ = HTTP_HEAD_CR;
				temp_ = "";
				return true;
			}
			else
			{
				//head 结束了
				HttpContent *pcontent = NULL;
				if (req_hanler_)
				{
					t_request_.Content.content_length = GetHeadContentLen(t_request_.Head);
					pcontent = &t_request_.Content;
				}
				else
				{
					t_response_.Content.content_length = GetHeadContentLen(t_response_.Head);
					pcontent = &t_response_.Content;
				}

				if (pcontent->content_length == 0)
				{
					pcontent->is_complete = true;
					status_ = HTTP_COMPLETE;
					OnHandler();
					return true;
				}
				status_ = HTTP_BODY;
				return true;
			}
		}
		bool OnBody(const char*data, unsigned int len, unsigned int &offset)
		{
			HttpContent *pcontent = NULL;
			if (req_hanler_)
			{
				pcontent = &t_request_.Content;
			}
			else
			{
				pcontent = &t_response_.Content;
			}
			//有效字节数
			unsigned int valid_bytes = len - offset;
			unsigned int need_bytes = pcontent->content_length - (pcontent->offset+pcontent->chunk.size());
			if (need_bytes <= 0)
			{
				pcontent->is_complete = true;
				pcontent->is_last = true;
				status_ = HTTP_COMPLETE;
				OnHandler();
				return  true;
			}
			unsigned int copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//取最小
			pcontent->chunk += Str(data + offset, copy_bytes);
			offset += copy_bytes;

			need_bytes = pcontent->content_length - (pcontent->offset + pcontent->chunk.size());
			//已经超过最大缓存
			if (max_body_size_ < pcontent->offset + pcontent->chunk.size())
			{
				if(need_bytes > 0)
				{
					status_ = HTTP_CHUNK;
					pcontent->is_complete = false;
					pcontent->is_last = true;
					OnHandler();
					//清空
					pcontent->offset += pcontent->chunk.size();
					pcontent->chunk = "";
					status_ = HTTP_BODY;
					pcontent->is_last = false;
					return  true;
				}
			}

			
			//printf("copy_bytes %u need_bytes %u content_lenght %u body %u\n", copy_bytes, need_bytes, content_length_, pbody->size());
			if (need_bytes <= 0)
			{
				status_ = HTTP_COMPLETE;
				pcontent->is_last = true;
				pcontent->is_complete = true;
				OnHandler();
				return  true;
			}
			//status_ = HTTP_HEAD_CR;
			return true;
		}
		bool FindCR(const char*data, unsigned int len, unsigned int &offset)
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
		void OnHandler()
		{
			if (req_hanler_)
				req_hanler_(this, &t_request_, pdata_);
			else if (response_handler_)
				response_handler_(this, &t_response_, pdata_);

			if (status_ == HTTP_COMPLETE)
			{
				t_response_.Clear();
				t_request_.Clear();
				status_ = HTTP_START_LINE_CR;
				temp_ = "";
			}
		}
	private:
		static bool PrintHead(const Str& key, const Str& val, void*pdata)
		{
			Str *pd = (Str*)pdata;
			(*pd) += key + ": " + val + SIM_HTTP_CRLF;
			return true;
		}
		static bool ParserStartLine(const Str&s, Str&d1, Str&d2, Str&d3)
		{
			int index  = 0;
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
	private:
		
#ifdef SIM_PARSER_MULTI_THREAD
		sim::Mutex parser_lock_;
#endif
		Str temp_;//缓存
		//int content_length_;
		HttpParserStatus status_;

		HttpRequest t_request_;
		HTTP_REQUEST_HANDLER req_hanler_;
		HttpResponse t_response_;
		HTTP_RESPONSE_HANDLER response_handler_;
		void*pdata_;

		//是否回调过程
		bool is_cb_process_;

		unsigned int max_body_size_;
	};
}
#endif