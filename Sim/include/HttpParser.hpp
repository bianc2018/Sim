/*
	HTTP ������
*/
#ifndef SIM_HTTP_PARSER_HPP_
#define SIM_HTTP_PARSER_HPP_
#include <stdio.h>
#include <string>

#define SIM_PARSER_MULTI_THREAD
//���߳����������
#ifdef SIM_PARSER_MULTI_THREAD
#include "Mutex.hpp"
#endif
//100M
#define MAX_HTTP_BODY_SIZE 100*1024*1024

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

	typedef std::string Str;


	//<scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<frag>
	//httpurl
	struct HttpUrl
	{
		//Э��
		Str scheme;
		//����
		Str host;
		//�˿�
		unsigned port;
		//��Դ·��
		Str path;
		HttpUrl() :scheme(""),port(0), path("")
		{

		}
	};

	//׷��ģʽ
	enum HttpMapAppendMode
	{
		//����
		HM_COVER,
		//�����ڲ��������Ѵ��ں��Ե�
		HM_ADD_IF_NO_EXIST,
		//׷��һ����
		HM_APPEND,
	};
	typedef bool(*HTTP_MAP_TRA_FUNC)(const Str& key, const Str& val, void*pdata);
	struct HttpMap
	{
		struct HttpMapNode
		{
			Str Key, Value;
			HttpMapNode*next;
			HttpMapNode():next(NULL){}
		};
	public:
		HttpMapNode*pHead;
	public:
		HttpMap() :pHead(NULL)
		{}
		HttpMap(const HttpMap&other) :pHead(NULL)
		{
			operator=(other);
		}
		HttpMap&operator=(const HttpMap&other) 
		{
			if (this != &other)
			{
				Release();
				HttpMapNode*pn = other.pHead;
				while (pn)
				{
					Append(pn->Key, pn->Value);
					pn = pn->next;
				}
			}
			return (*this);
		}

		~HttpMap()
		{
			Release();
		}

		Str Get(const Str& key, const Str &notfound)
		{
			if (pHead == NULL)
			{
				return notfound;
			}
			HttpMapNode* pn = pHead;
			while (pn)
			{
				if (pn->Key == key)
					return pn->Value;
				pn = pn->next;
			}
			return notfound;
		}
		//������key��Сд
		Str GetCase(const Str& key, const Str &notfound);

		void Append(const Str& key, const Str& val, HttpMapAppendMode mode = HM_ADD_IF_NO_EXIST)
		{
			if (pHead == NULL)
			{
				pHead = new HttpMapNode;
				pHead->Key = key;
				pHead->Value = val;
				return;
			}
			HttpMapNode* pn = pHead;
			while (pn->next != NULL)
			{
				//���������ͬ��ֵ����
				if (pn->Key == key)
				{
					if (HM_ADD_IF_NO_EXIST == mode)
					{
						return;//������
					}
					else if (HM_COVER == mode)
					{
						//����
						pn->Value = val;
						return;
					}
				}

				pn = pn->next;
			}
			pn->next = new HttpMapNode;;
			pn->next->Key = key;
			pn->next->Value = val;
			return;
		}

		void AppendMap(const HttpMap&other, HttpMapAppendMode mode = HM_ADD_IF_NO_EXIST)
		{
			HttpMapNode*pn = other.pHead;
			while (pn)
			{
				Append(pn->Key, pn->Value, mode);
				pn = pn->next;
			}
		}

		int Count(const Str& key)
		{
			int count = 0;
			HttpMapNode* pn = pHead;
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
			HttpMapNode* pn = pHead;
			while (pn)
			{
				++count;
				pn = pn->next;
			}
			return count;
		}

		void Release()
		{
			//�ͷ��ڴ�
			while (pHead)
			{
				HttpMapNode* pn = pHead->next;
				delete pHead;
				pHead = pn;
			}
			pHead = NULL;
		}

		//����
		void Traverse(HTTP_MAP_TRA_FUNC func, void*pdata)
		{
			if (NULL == func)
				return;

			HttpMapNode* pn = pHead;
			while (pn)
			{
				if (false == func(pn->Key, pn->Value, pdata))
					break;
				pn = pn->next;
			}
			return ;
		}
	};
	//��������
	typedef  unsigned long long ContentLength_t;
	//content ״̬
	struct HttpContent
	{
		//��ǰ��
		Str chunk;
		//�Ƿ�Ϊ�����ġ�true�������������ˣ�false ֻ������һ��
		bool is_complete;
		//�Ƿ����һ�λص����ص�֮�����
		bool is_last;
		//������ĳ���
		ContentLength_t content_length;
		//��ǰ��Ŀ�ͷ
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
		
		HttpMap Head;

		HttpContent Content;

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

		HttpMap Head;

		HttpContent Content;
		void Clear()
		{
			Status = "";
			Reason = "";
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
		HTTP_CHUNK,//body��
		HTTP_COMPLETE,//����
	};

	class HttpParser
	{
	public:
		HttpParser(HTTP_REQUEST_HANDLER req_hanler, void*pdata,bool is_cb_process = false)
			:req_hanler_(req_hanler),
			pdata_(pdata), 
			response_handler_(NULL),
			status_(HTTP_START_LINE_CR),
			is_cb_process_(is_cb_process),
			max_body_size_(MAX_HTTP_BODY_SIZE)
		{
			
		}

		HttpParser(HTTP_RESPONSE_HANDLER response_handler, void*pdata, bool is_cb_process = false)
			:req_hanler_(NULL), 
			pdata_(pdata), 
			response_handler_(response_handler), 
			status_(HTTP_START_LINE_CR),
			is_cb_process_(is_cb_process),
			max_body_size_(MAX_HTTP_BODY_SIZE)
		{}
		~HttpParser() {};

		bool Parser(const char*data, unsigned int len)
		{
			//�������
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
				//�Ƿ�ÿ�ζ�Ҫ�ص�
				if (is_cb_process_)
				{
					OnHandler();
				}
			}
			return true;
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

		//��ȡ����״̬
		HttpParserStatus GetParserStatus()
		{
			return status_;
		}

		//�������body����
		bool SetMaxHttpBodySize(const unsigned int &max_body_size)
		{
			if(max_body_size<=0)
				return false;
			max_body_size_ = max_body_size;
			return true;
		}
		
	public:
		//ȥ��ǰ��Ŀո�
		static Str Trim(const Str&s)
		{
			int size = s.size();
			if (size <= 0)
				return "";
			int start = 0, end = size - 1;
			for (int i = 0; i < size; ++i)
			{
				if (s[i] != SIM_HTTP_SPACE)
				{
					start = i;
					break;
				}
			}
			for (int i = size - 1; i >= 0; --i)
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
		//ת��Ϊ��Сд
		static Str ToLower(const Str &s)
		{
			Str result = s;
			unsigned int size = s.size();
			for (int i = 0; i < size; ++i)
				if (s[i] >= 'A'&&s[i] <= 'Z')
					result[i] = 'a' + s[i] - 'A';
			return result;
		}
		static Str ToUpper(const Str &s)
		{
			Str result = s;
			unsigned int size = s.size();
			for (int i = 0; i < size; ++i)
				if (s[i] >= 'a'&&s[i] <= 'z')
					result[i] = 'A' + s[i] - 'a';
			return result;
		}
		static int StrToInt(const Str&s, int fail = -1)
		{
			return StrToNum<int>(s, fail);
		}
		template <typename T>
		static T StrToNum(const Str&s, T fail = -1)
		{
			int size = s.size();
			if (size == 0)
				return fail;

			int i = 0;
			T num = 0;
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
			return is_neg ? -num : num;
		}
		static Str IntToStr(const int&s, Str fail = "")
		{
			const int temp_buff_size = 256;
			char temp_buff[temp_buff_size] = { 0 };
			snprintf(temp_buff, temp_buff_size, "%d", s);
			return temp_buff;
		}

		//<scheme>://<host>:<port>/<path>
		//����Url
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
						has_port = true;//��������port
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

			//���Ĭ��ֵ
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
					//:��β
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

		//��ȡHttpMap ��content�ֶ� ��С�Ҳ������߲����ڷ���0
		static  ContentLength_t GetHeadContentLen(HttpMap &Head)
		{
			//ContentLength_t
			return HttpParser::StrToNum<ContentLength_t>(Head.GetCase(SIM_HTTP_CL, "0"), 0);
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
				//head ������
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
			//��Ч�ֽ���
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
			unsigned int copy_bytes = valid_bytes > need_bytes ? need_bytes : valid_bytes;//ȡ��С
			pcontent->chunk += Str(data + offset, copy_bytes);
			offset += copy_bytes;

			need_bytes = pcontent->content_length - (pcontent->offset + pcontent->chunk.size());
			//�Ѿ�������󻺴�
			if (max_body_size_ < pcontent->offset + pcontent->chunk.size())
			{
				if(need_bytes > 0)
				{
					status_ = HTTP_CHUNK;
					pcontent->is_complete = false;
					pcontent->is_last = true;
					OnHandler();
					//���
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
		Str temp_;//����
		//int content_length_;
		HttpParserStatus status_;

		HttpRequest t_request_;
		HTTP_REQUEST_HANDLER req_hanler_;
		HttpResponse t_response_;
		HTTP_RESPONSE_HANDLER response_handler_;
		void*pdata_;

		//�Ƿ�ص�����
		bool is_cb_process_;

		unsigned int max_body_size_;
	};

	Str HttpMap::GetCase(const Str & key, const Str & notfound)
	{
		if (pHead == NULL)
		{
			return notfound;
		}
		HttpMapNode* pn = pHead;
		while (pn)
		{
			if (HttpParser::ToLower(pn->Key) == HttpParser::ToLower(key))
				return pn->Value;
			pn = pn->next;
		}
		return notfound;
	}
}
#endif