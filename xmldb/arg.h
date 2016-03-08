#pragma once

#include <string.h> //for strcmp

#ifdef WIN32
#define scl_strcasecmp		_stricmp
#else
#define scl_strcasecmp		strcasecmp
#endif 

class Arg
{
	int m_argc;
	char ** m_argv;
public:
	Arg(const int argc, char ** argv) : m_argc(argc), m_argv(argv) {}

	// if has "-h" or "-help"
	bool needHelp() {
		if (m_argc < 2 || hasOption("-h") || hasOption("-help"))
			return true;
		return false;
	}

	// check if has 'option' params
	bool hasOption(const char * option) {
		for (int i = 1; i < m_argc; ++i) {
			if (scl_strcasecmp(m_argv[i], option) == 0)
				return true;
		}
		return false;
	}

	//get option.
	const char* getOption(const char * option, const char * def = NULL) {
		for (int i = 1; i < m_argc; ++i) {
			if (scl_strcasecmp(m_argv[i], option) == 0) {
				if (m_argv[i+1][0] != '-')
					return m_argv[i+1];
				else
					return def ? def : "";
			}
		}

		return "";
	}

	const char * param(int index) {
		if (index < 0 || index >= m_argc)
			return "";
		return m_argv[index];
	}

	int paramIndex(int index, const int defvalue=0) {
		if (index < 0 || index >= m_argc)
			return defvalue;
		int ret = ::strtol(m_argv[index], NULL, 10);
		return ret;
	}
};