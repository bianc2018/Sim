/*
	ssh 公钥生成
*/
#include "SSHv2.hpp"
#include "CmdLineParser.hpp"

void print_help()
{
	printf("usg:-type rsa or dsa -size >=1024 -pub pubfilepath -pri prifilepeth\n");
	getchar();
}
int main(int argc, char *argv[])
{
	sim::CmdLineParser cmd(argc, argv);
#if 1
	cmd.InitCmdLineParams("type", "rsa")
		.InitCmdLineParams("size", 2048);
#endif

	//检查参数
	if (cmd.HasParam("h") || cmd.HasParam("help"))
	{
		print_help();
		return -1;
	}
	sim::Str type = cmd.GetCmdLineParams("type", "");
	unsigned int size = cmd.GetCmdLineParams("size", 1024);
	sim::Str pub = cmd.GetCmdLineParams("pub", "");
	sim::Str pri = cmd.GetCmdLineParams("pri", "");
	if(type.empty())
	{
		print_help();
		return -1;
	}
	if (type != "rsa" && type != "dsa")
	{
		print_help();
		return -1;
	}
	if(pub.empty())
	{
		pub = "id_" + type + "_" + sim::BaseParser::NumToStr(size, "%u") + ".pub";
	}
	if (pri.empty())
	{
		pri = "id_"+type + "_" + sim::BaseParser::NumToStr(size, "%u") ;
	}
	if (size < 1024)
	{
		print_help();
		return -1;
	}
	
	//生成
//	sim::SshAuthentication auth(sim::SshClient);
	if (type == "rsa")
	{
		printf("GenerateRsaKey(%u) ...\n", size);
		RSA*rsa =sim::SshAuthentication::GenerateRsaKey(size);
		if (NULL == rsa)
		{
			printf("GenerateRsaKey Failed\n");
			getchar();
			return -1;
		}
		printf("WritePrivateKey(%s) ...\n", pri.c_str());
		if (!sim::SshAuthentication::WriteKey(rsa, pri.c_str(), NULL))
		{
			RSA_free(rsa);
			printf("Pri WriteKey  Failed\n");
			getchar();
			return -1;
		}
		printf("WritePublicKey(%s) ...\n", pub.c_str());
		if (!sim::SshAuthentication::WriteSshRsaPubKey(rsa, pub.c_str()))
		{
			RSA_free(rsa);
			printf("pubkey WriteKey  Failed\n");
			getchar();
			return -1;
		}
		RSA_free(rsa);
	}
	else if (type == "dsa")
	{
		printf("GenerateDsaKey(%u) ...\n", size);
		DSA*dsa = sim::SshAuthentication::GenerateDsaKey(size);
		if (NULL == dsa)
		{
			printf("GenerateDsaKey Failed\n");
			getchar();
			return -1;
		}
		printf("WritePrivateKey(%s) ...\n", pri.c_str());
		if (!sim::SshAuthentication::WriteKey(dsa, pri.c_str(), NULL))
		{
			DSA_free(dsa);
			printf("Pri WriteKey  Failed\n");
			getchar();
			return -1;
		}
		printf("WritePublicKey(%s) ...\n", pub.c_str());
		if (!sim::SshAuthentication::WriteSshDsaPubKey(dsa, pub.c_str()))
		{
			DSA_free(dsa);
			printf("Pri WriteKey  Failed\n");
			getchar();
			return -1;
		}
		DSA_free(dsa);
	}
	printf("end\n");
	getchar();
	return 0;
}