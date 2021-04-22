/* 
	Base64 加解密
*/
#ifndef SIM_BASE64_HPP_
#define SIM_BASE64_HPP_
namespace sim
{
	static const char base64_chars[65] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	/*static const unsigned int64_chars[] =
	{
		1,2
	};*/

	class Base64
	{
	public:
		static unsigned int encode(unsigned char const*input, unsigned int input_len,
			char *output, unsigned int output_len) 
		{
			if (output == NULL /*|| input == NULL*/ /*|| input_len == 0*/ || output_len == 0)
				return 0;

			unsigned int output_offset = 0;
			int i = 0;
			int j = 0;
			unsigned char char_array_3[3];
			unsigned char char_array_4[4];

			while (input_len--) 
			{
				char_array_3[i++] = *(input++);
				if (i == 3) {
					char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
					char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
					char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
					char_array_4[3] = char_array_3[2] & 0x3f;

					for (i = 0; (i < 4); i++)
					{
						//ret += base64_chars[char_array_4[i]];
						if (output_offset >= output_len)
							return 0;//异常
						output[output_offset++] = base64_chars[char_array_4[i]];
					}
					i = 0;
				}
			}

			if (i)
			{
				for (j = i; j < 3; j++)
					char_array_3[j] = '\0';

				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for (j = 0; (j < i + 1); j++)
				{
					//ret += base64_chars[char_array_4[j]];
					if (output_offset >= output_len)
						return 0;//异常
					output[output_offset++] = base64_chars[char_array_4[j]];
				}

				while ((i++ < 3))
				{
					//ret += '=';
					if (output_offset >= output_len)
						return 0;//异常
					output[output_offset++] = '=';
				}

			}

			return output_offset;
		}
		static unsigned int decode(char const*input, unsigned int input_len,
			unsigned char *output, unsigned int output_len)
		{
			if (output == NULL || input == NULL || input_len == 0 || output_len == 0)
				return 0;

			unsigned int output_offset = 0;

			int i = 0;
			int j = 0;
			int in_ = 0;
			unsigned char char_array_4[4], char_array_3[3];

			while (input_len-- && (input[in_] != '=') && is_base64(input[in_])) {
				char_array_4[i++] = input[in_];
				in_++;
				if (i == 4) {
					for (i = 0; i < 4; i++)
						char_array_4[i] = find_index64(char_array_4[i]);

					char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
					char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
					char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

					for (i = 0; (i < 3); i++)
					{
						//ret += char_array_3[i];
						if (output_offset >= output_len)
							return 0;//异常
						output[output_offset++] = char_array_3[i];
					}
					i = 0;
				}
			}

			if (i) {
				for (j = i; j < 4; j++)
					char_array_4[j] = 0;

				for (j = 0; j < 4; j++)
					char_array_4[j] = find_index64(char_array_4[j]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (j = 0; (j < i - 1); j++)
				{
					//ret += char_array_3[j];
					if (output_offset >= output_len)
						return 0;//异常
					output[output_offset++] = char_array_3[j];
				}
			}

			return output_offset;
		}
	private:
		static inline bool is_base64(unsigned char c) {
			return (isalnum(c) || (c == '+') || (c == '/'));
		}
		static inline int find_index64(unsigned char c)
		{
			//后续制表
			for (int i = 0; i < 65; ++i)
			{
				if (c == base64_chars[i])
					return i;
			}
			return -1;
		}
	};

}
#endif