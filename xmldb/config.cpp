#include "config.h"

#include "pugixml/pugixml.hpp"
#include <assert.h>

using pugi::xml_node;
using pugi::xml_document;
using pugi::xml_parse_result;


// 配置文件 目前手写，后面移动到config.xml里面;

void stringUpperFirstChar(const std::string& s)
{
	if (s.empty())
		return;

	char className[256];
	strncpy(className, s.c_str(), 256);
	if (className[0] >= 'a' && className[0] <= 'z') //转小写
		className[0] -= 32;

}




ParseConfig::ParseConfig()
{
}


ParseConfig::~ParseConfig()
{
}


bool ParseConfig::loadConfig(const char* const filename)
{
	xml_document doc;
	xml_parse_result parseResult = doc.load_file(filename, pugi::parse_default, pugi::encoding_auto);
	if (!parseResult)
	{
		//log_debug("packetxml : open file [%s] faild info[%s]", filename, parseResult.description());
		return false;
	}

	bool hasData = false;
	xml_node root = doc.first_child();
	for (xml_node typeChild = root.first_child(); typeChild; typeChild=typeChild.next_sibling())
	{
		CTypeInfo info;//			= m_Types.push_back_fast();
		info.dbType				= typeChild.attribute("db").as_string();
		info.cType				= typeChild.attribute("c").as_string();
		info.cFormat			= typeChild.attribute("format").as_string();

		info.dbResultType		= typeChild.attribute("dbResultType").as_string();
		info.defaultValue		= typeChild.attribute("defaultValue").as_string();

		info.isArray			= typeChild.attribute("array").as_bool();
		info.canBinary			= typeChild.attribute("binary").as_bool();
		info.canAutoIncrement	= typeChild.attribute("autoincrement").as_bool();
		info.canUnsigned		= typeChild.attribute("unsigned").as_bool();
		info.canzerofill		= typeChild.attribute("zerofill").as_bool();
		m_Types.push_back(info);
		hasData = true;
	}

	return hasData;
}

CTypeInfo* ParseConfig::typeInfos(FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(false);
		return NULL;
	}

	int currentTypes = m_Types.size();
	for (int i = 0; i < currentTypes; ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), info.dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType, true))
			return &info;
	}

	assert(false);
	return NULL;
}

CTypeInfo* ParseConfig::typeInfos(const char* const c)
{
	FieldType fType = c;
	return typeInfos(fType);
}

bool ParseConfig::dateTimeType(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(false);
		return false;
	}

	//if (dbType.compare("datetime", dbType.length(), true) == 0)
	if (_compare(dbType, "datetime", true))
		return true;
	return false;
}

bool ParseConfig::stringType(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(false);
		return false;
	}

	//if (dbType.compare("CHAR", dbType.length(), true) == 0)
	if (_compare(dbType, "CHAR", true))
		return true;
	//if (dbType.compare("VARCHAR", dbType.length(), true) == 0)
	if (_compare(dbType, "VARCHAR", true))
		return true;

	return false;
}

bool ParseConfig::blobType(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(false);
		return false;
	}

	//if (dbType.compare("BLOB", dbType.length(), true) == 0)
	if (_compare(dbType, "BLOB", true))
		return true;
	return false;
}


// 	CREATE TABLE `test_zdy`.`test` (
// 		`id` INT NOT NULL,
// 		`id1` SMALLINT NULL,
// 		`testcol` TINYINT NULL,
// 		`testcol1` MEDIUMINT NULL,
// 		`testcol2` BIGINT NULL,
// 		`testcol3` FLOAT NULL,
// 		`testcol4` DOUBLE NULL,
// 		`testcol5` CHAR(100) NULL,				// char
// 		`testcol6` VARCHAR(101) NULL,			// varchar
// 		`testcol7` TINYBLOB NULL,
// 		`testcol8` TINYTEXT NULL,
// 		`testcol9` BLOB(200) NULL,				// blob
// 		`testcol10` TEXT(100) NULL,				// text.
// 		`testcol11` MEDIUMBLOB NULL,
// 		`testcol12` MEDIUMTEXT NULL,
// 		`testcol13` LONGTEXT NULL,
// 		`testcol14` LONGBLOB NULL,
// 		PRIMARY KEY(`id`))
// 		ENGINE = InnoDB;// 	CREATE TABLE `test` (
// 		`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
// 		`id1` smallint(5) unsigned zerofill NOT NULL,
// 		`testcol` tinyint(3) unsigned zerofill NOT NULL,
// 		`testcol1` mediumint(8) unsigned zerofill NOT NULL,
// 		`testcol2` bigint(20) unsigned zerofill NOT NULL,
// 		`testcol3` float unsigned zerofill NOT NULL,
// 		`testcol4` double unsigned zerofill NOT NULL,
// 		`testcol5` char(100) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol6` varchar(101) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol7` tinyblob NOT NULL,
// 		`testcol8` tinytext CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol9` tinyblob NOT NULL,
// 		`testcol10` text CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol11` mediumblob NOT NULL,
// 		`testcol12` mediumtext CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol13` longtext CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
// 		`testcol14` longblob NOT NULL,
// 		`testcol15` datetime NOT NULL,
// 		PRIMARY KEY(`id`)
// 		) ENGINE = InnoDB DEFAULT CHARSET = utf

int ParseConfig::typeSizeByFieldType(const FieldType& dbType, const int configSize)
{
	if (dbType.empty())
	{
		assert(0);
		return 0;
	}

	for (int i = 0; i < m_Types.size(); ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType.c_str(), true))
		{
			if (info.isArray) 
			{
				return configSize;
			}
			return 0;
		}
	}

	// 	if (dbType.compare("CHAR", dbType.length(), true) == 0)
	// 		return configSize;
	// 	if (dbType.compare("VARCHAR", dbType.length(), true) == 0)
	// 		return configSize;
	// // 	if (dbType.compare("BLOB", dbType.length(), true) == 0)
	// // 		return configSize;
	// // 	if (dbType.compare("TEXT", dbType.length(), true) == 0)
	// // 		return configSize;

	return 0;
}


bool ParseConfig::_canBinary(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(0);
		return 0;
	}

	for (int i = 0; i < m_Types.size(); ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType.c_str(), true))
		{
			return info.canBinary;
		}
	}

	assert(false);
	return false;
}

bool ParseConfig::_canAutoincrement(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(0);
		return 0;
	}

	for (int i = 0; i < m_Types.size(); ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType.c_str(), true))
		{
			return info.canAutoIncrement;
		}
	}

	assert(false);
	return false;
}

bool ParseConfig::_canUnsigned(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(0);
		return 0;
	}

	for (int i = 0; i < m_Types.size(); ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType.c_str(), true))
		{
			return info.canUnsigned;
		}
	}

	assert(false);
	return false;
}

bool ParseConfig::_canZerofill(const FieldType& dbType)
{
	if (dbType.empty())
	{
		assert(0);
		return 0;
	}

	for (int i = 0; i < m_Types.size(); ++i)
	{
		CTypeInfo& info = m_Types[i];
		//if (info.dbType.compare(dbType.c_str(), dbType.length(), true) == 0)
		if (_compare(info.dbType, dbType.c_str(), true))
		{
			return info.canzerofill;
		}
	}

	assert(false);
	return false;
}