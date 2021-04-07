/*
	HTTP 解析器
*/
#ifndef SIM_HTTP_PARSER_HPP_
#define SIM_HTTP_PARSER_HPP_
#include <stdio.h>
#include <string>

#define SIM_HTTP_SPACE			' '
#define SIM_HTTP_CR				'\r'
#define SIM_HTTP_LF				'\n'
#define SIM_HTTP_CRLF			"\r\n"
#define SIM_HTTP_VERSION_0_9	"HTTP/0.9"
#define SIM_HTTP_VERSION_1_0	"HTTP/1.0"
#define SIM_HTTP_VERSION_1_1	"HTTP/1.1"
//Content-Length
#define SIM_HTTP_CL				"Content-Length"
namespace sim
{
	class HttpParser;

	typedef std::string Str;

	typedef bool(*HTTP_HEAD_TRA_FUNC)(const Str& key, const Str& val, void*pdata);
	struct HttpHead
	{
		struct HttpHeadNode
		{
			Str Key, Value;
			HttpHeadNode*next;
			HttpHeadNode():next(NULL){}
		};
	public:
		HttpHeadNode*pHead;
	public:
		HttpHead() :pHead(NULL)
		{}
		HttpHead(const HttpHead&other) :pHead(NULL)
		{
			operator=(other);
		}
		HttpHead&operator=(const HttpHead&other) 
		{
			if (this != &other)
			{
				Release();
				HttpHeadNode*pn = other.pHead;
				while (pn)
				{
					Append(pn->Key, pn->Value);
					pn = pn->next;
				}
			}
			return (*this);
		}
		~HttpHead()
		{
			Release();
		}

		Str Get(const Str& key, const Str &notfound)
		{
			if (pHead == NULL)
			{
				return notfound;
			}
			HttpHeadNode* pn = pHead;
			while (pn)
			{
				if (pn->Key == key)
					return pn->Value;
				pn = pn->next;
			}
			return notfound;
		}

		void Append(const Str& key, const Str& val)
		{
			if (pHead == NULL)
			{
				pHead = new HttpHeadNode;
				pHead->Key = key;
				pHead->Value = val;
				return;
			}
			HttpHeadNode* pn = pHead;
			while (pn->next != NULL)
			{
				pn = pn->next;
			}
			pn->next = new HttpHeadNode;;
			pn->next->Key = key;
			pn->next->Value = val;
			return;
		}

		int Count(const Str& key)
		{
			int count = 0;
			HttpHeadNode* pn = pHead;
			while (pn)
			{
				if (pn->Key == key)
					++count;
				pn = pn->next;
			}
			return count;
		}
		
		int Count()
		{
			int count = 0;
			HttpHeadNode* pn = pHead;
			while (pn)
			{
				++count;
				pn = pn->next;
			}
			return count;
		}

		void Release()
		{
			//释放内存
			while (pHead)
			{
				HttpHeadNode* pn = pHead->next;
				delete pHead;
				pHead = pn;
			}
			pHead = NULL;
		}

		//遍历
		void Traverse(HTTP_HEAD_TRA_FUNC func, void*pdata)
		{
			if (NULL == func)
				return;

			HttpHeadNode* pn = pHead;
			while (pn)
			{
				if (false == func(pn->Key, pn->Value, pdata))
					break;
				pn = pn->next;
			}
			return ;
		}
	};

	struct HttpRequest
	{
		Str Method;
		Str Url;
		Str Version;
		
		HttpHead Head;

		Str Body;

		void Clear()
		{
			Method = "";
			Url = "";
			Version = SIM_HTTP_VERSION_1_1;
			Body = "";
			Head.Release();
		}
	};

	struct HttpResponse
	{
		Str Version;
		Str Status;
		Str Reason;

		HttpHead Head;

		Str Body;
		void Clear()
		{
			Status = "";
			Reason = "";
			Version = SIM_HTTP_VERSION_1_1;
			Body = "";
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
	};

	class HttpParser
	{
	public:
		HttpParser(HTTP_REQUEST_HANDLER req_hanler, void*pdata) 
			:req_hanler_(req_hanler), pdata_(pdata), response_handler_(NULL),status_(HTTP_START_LINE_CR), content_length_(0)
		{
			
		}

		HttpParser(HTTP_RESPONSE_HANDLER response_handler, void*pdata)
			:req_hanler_(NULL), pdata_(pdata), response_handler_(response_handler), status_(HTTP_START_LINE_CR), content_length_(0)
		{}
		~HttpParser() {};

		bool Parser(const char*data, unsigned int len)
		{
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
				else
				{
					return false;
				}
			}
			return true;
		}

		static Str PrintRequest(HttpRequest *request)
		{
			if (NULL == request)
				return "";
			Str data = request->Method+SIM_HTTP_SPACE+request->Url+SIM_HTTP_SPACE+request->Version+ SIM_HTTP_CRLF;
			if (request->Head.Count(SIM_HTTP_CL) <= 0)
				request->Head.Append(SIM_HTTP_CL, IntToStr(request->Body.size()));
			request->Head.Traverse(HttpParser::PrintHead, &data);
			data += SIM_HTTP_CRLF;
			data += request->Body;
			return data;
		}
		static Str PrintResponse(HttpResponse *response)
		{
			if (NULL == response)
				return "";
			Str data = response->Version + SIM_HTTP_SPACE + response->Status + SIM_HTTP_SPACE + response->Reason + SIM_HTTP_CRLF;
			if (response->Head.Count(SIM_HTTP_CL) <= 0)
				response->Head.Append(SIM_HTTP_CL, IntToStr(response->Body.size()));
			response->Head.Traverse(HttpParser::PrintHead, &data);
			data += SIM_HTTP_CRLF;
			data += response->Body;
			return data;

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
				if (req_hanler_)
				{
					content_length_ = StrToInt(t_request_.Head.Get(SIM_HTTP_CL, "0"), 0);
				}
				else
				{
					content_length_ = StrToInt(t_response_.Head.Get(SIM_HTTP_CL, "0"), 0);
				}
				if (content_length_ == 0)
				{
					OnHandler();
					return true;
				}
				status_ = HTTP_BODY;
				return true;
			}
		}
		bool OnBody(const char*data, unsigned int len, unsigned int &offset)
		{
			Str *pbody = NULL;
			if (req_hanler_)
			{
				pbody = &t_request_.Body;
			}
			else
			{
				pbody = &t_response_.Body;
			}
			//有效字节数
			unsigned int valid_bytes = len - offset;
			unsigned int need_bytes = content_length_ - pbody->size();
			if (need_bytes <= 0)
			{
				OnHandler();
				return  true;
			}
			unsigned int copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//取最小
			(*pbody) += Str(data + offset, copy_bytes);
			offset += copy_bytes;
			
			need_bytes = content_length_ - pbody->size();
			//printf("copy_bytes %u need_bytes %u content_lenght %u body %u\n", copy_bytes, need_bytes, content_length_, pbody->size());
			if (need_bytes <= 0)
			{
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
			t_response_.Clear();
			t_request_.Clear();
			status_ = HTTP_START_LINE_CR;
			content_length_ = 0;
			temp_ = "";
		}
	private:
		static int StrToInt(const Str&s,int fail =- 1)
		{
			int size = s.size();
			if (size == 0)
				return fail;

			int i = 0;
			int num = 0;
			bool is_neg = false;
			if (s[0] == '-')
			{
				is_neg = true;
				i = 1;
			}

			for (; i < size; ++i)
			{
				if (s[i] >= '0'&& s[i] <= '9')
					num = num * 10 + (s[i] - '0');
				else
					return fail;
			}
			return is_neg?-num:num;
		}
		static Str IntToStr(const int&s, Str fail = "")
		{
			const int temp_buff_size = 256;
			char temp_buff[temp_buff_size] = { 0 };
			snprintf(temp_buff, temp_buff_size, "%d", s);
			return temp_buff;
		}
		static bool PrintHead(const Str& key, const Str& val, void*pdata)
		{
			Str *pd = (Str*)pdata;
			(*pd) += key + ": " + val + SIM_HTTP_CRLF;
			return true;
		}
		//去除前后的空格
		static Str Trim(const Str&s)
		{
			int size = s.size();
			if (size <= 0)
				return "";
			int start = 0, end = size-1;
			for (int i = 0; i < size; ++i)
			{
				if (s[i] != SIM_HTTP_SPACE)
				{
					start = i;
					break;
				}
			}
			for (int i = size-1; i >=0; --i)
			{
				if (s[i] != SIM_HTTP_SPACE)
				{
					end = i;
					break;
				}
			}
			Str res;
			for (int i = start; i <= end; ++i)
			{
				res += s[i];
			}
			return  res;
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
		Str temp_;//缓存
		int content_length_;
		HttpParserStatus status_;
		HttpRequest t_request_;
		HTTP_REQUEST_HANDLER req_hanler_;
		HttpResponse t_response_;
		HTTP_RESPONSE_HANDLER response_handler_;
		void*pdata_;

	};


}
#endif