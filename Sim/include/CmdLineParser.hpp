/*
	一个简单的命令行解析器
*/
#ifndef SIM_CMD_LINE_PARSER_HPP_
#define SIM_CMD_LINE_PARSER_HPP_
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include <sstream>
namespace sim
{
	class CmdLineParser
	{
		template<typename T1, typename T2>
		T2 SimTo(const T1 &t)
		{
			std::stringstream oss;
			oss << t;
			T2 t2;
			oss >> t2;
			return t2;
		}
	public:
		CmdLineParser(int argc, char* argv[])
		{
		/*	printf("argc %d argv %p\n", argc, argv);
			for (int i = 0; i < argc; ++i)
			{
				printf("%d %s\n", i, argv[i]);
			}*/
			Parser(argc, argv);
		}
		//-p
		std::string GetCmdLineParams(const std::string&key, const std::string&notfind)
		{
			if (cmd_line_params_.count(key))
			{
				return cmd_line_params_[key];
			}
			return notfind;
		}

		template<typename T>
		T GetCmdLineParams(const std::string&key, const T&notfind)
		{
			return SimTo<std::string, T>(GetCmdLineParams(key,SimTo<T, std::string>(notfind)));
		}

		bool HasParam(const std::string&key)
		{
			if (cmd_line_params_.count(key))
			{
				return true;
			}
			return false;
		}
	private:
		bool Parser(int argc, char* argv[])
		{
			if (argc < 1)
				return false;
			cmd_line_ = argv[0];

			std::string temp = "";
			for (int i = 1; i < argc; ++i)
			{
				if (strlen(argv[i]) == 0)
					continue;
				if (argv[i][0] == '-')
				{
					if (!temp.empty())
					{
						cmd_line_params_[temp] = "";
					}
					temp = std::string(argv[i] + 1);
				}
				else
				{
					if (!temp.empty())
					{
						cmd_line_params_[temp] = argv[i];
						temp = "";
					}
				}
			}
			if (!temp.empty())
			{
				cmd_line_params_[temp] = "";
			}
			return true;
		}
	private:
		std::map<std::string, std::string> cmd_line_params_;
		std::string cmd_line_;
	};
}
#endif //!SIM_CMD_LINE_PARSER_HPP_