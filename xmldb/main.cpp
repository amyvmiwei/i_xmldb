#include "def.h"
#include "app.h"
#include "config.h"

#include "arg.h"

#include <stdio.h>
#include <string.h>
#include <string>

void help();

int main(int argc, char ** argv)
{
	//parse argv.
	Arg arg(argc, argv);
	if (arg.needHelp()) {
		help();
		return 0;
	}

	//load xml data file;
	std::string filename = arg.param(1);
	if (filename.empty()) {
		printf("filename is null!\n");
		return 0;
	}

	Param param;

	if (arg.hasOption("-cppPath"))
		param.cppOutPath = arg.getOption("-cppPath");
	if (arg.hasOption("-sqlPath"))
		param.sqlOutPath = arg.getOption("-sqlPath");

	if (arg.hasOption("-vcProjPath"))
		param.vcProjToolPath = arg.getOption("-vcProjPath");
	if (arg.hasOption("-defFile"))
		param.defFile = arg.getOption("-defFile");
	if (arg.hasOption("-workBak"))
		param.workBakFile = arg.getOption("-workBak");

	if (param.cppOutPath.empty())
		param.cppOutPath = "./";
	if (param.sqlOutPath.empty())
		param.sqlOutPath = "./";
	if (param.vcProjToolPath.empty())
		param.vcProjToolPath = "./";
	if (param.defFile.empty())
		param.defFile = "./";
	if (param.workBakFile.empty())
		param.workBakFile = "./";

	bool ret = ParseConfig::Instance().loadConfig("config.xml");
	//assert(ret);

	XMLDBApp::Instance().setParam(param);
	XMLDBApp::Instance().execute();
}

void help()
{
	printf("--===hello world===--\n");
}