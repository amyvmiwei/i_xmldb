#include "def.h"
#include "config.h"
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

LABLE_TYPE getLableType(const char* const s)
{
	if (NULL == s)
	{
// 		Log::inst()._error("getLableType param is null!");
// 		assert(false);
		return LABLE_TYPE_INVALID;
	}

	static const char* const lableopcodestring[LABLE_TYPE_COUNT] = { "table", "batch"};
	std::string str = s;
	for (int i = LABLE_TYPE_TABLE; i < LABLE_TYPE_COUNT; ++i)
	{
		if (str == lableopcodestring[i])
			return static_cast<LABLE_TYPE>(i);
	}

// 	Log::inst()._error("getLableType 没找到匹配的[%s]", s);
// 	assert(false);
	return LABLE_TYPE_INVALID;
}


TABLE_OPCODE getTableOpcode(const char* const s)
{
	if (NULL == s)
	{
		//Log::inst()._error("getTableOpcode param is null!");
		assert(false);
		return TABLE_OPCODE_INVALID;
	}

/*	static const char* const create = "CREATE";
	static const char* const update = "UPDATE";
	static const char* const erase = "DELETE";*/
	
	static const char* const opcodestring[TABLE_OPCODE_COUNT] = { "CREATE", "UPDATE", "DELETE" };
	std::string str = s;
	for (int i = TABLE_OPCODE_CREATE; i < TABLE_OPCODE_COUNT; ++i)
	{
		if (str == opcodestring[i])
			return static_cast<TABLE_OPCODE>(i);
	}

// 	Log::inst()._error("getTableOpcode 没找到匹配的[%s]", s);
// 	assert(false);
	return TABLE_OPCODE_INVALID;
}


FIELD_OPCODE getFieldOpcode(const char* const s)
{
	if (NULL == s)
	{
// 		Log::inst()._error("getFieldOpcode param is null!");
// 		assert(false);
		return FIELD_OPCODE_INVALID;
	}

	std::string str = s;
	static const char* const FIELDopcodestring[FIELD_OPCODE_COUNT] = { "field", "CHANGE", "ADD", "DROP", "ADDINDEX", "DROPINDEX"};
	for (int i = FIELD_OPCODE_NO_UPDATE; i < FIELD_OPCODE_COUNT; ++i)
	{
		if (str == FIELDopcodestring[i])
			return static_cast<FIELD_OPCODE>(i);
	}
	
// 	Log::inst()._error("getFieldOpcode 没找到匹配的[%s]", s);
// 	assert(false);
	return FIELD_OPCODE_INVALID;
}

bool _makeSelectTableString(const DBTable& s, std::string& str, const char* idName)
{
 	str = "SELECT ";
 	//normal members.
 	bool hasdata = false;
 	for (int i = 0; i < s.fields.size(); ++i)
 	{
 		const DBField& field = s.fields[i];
 		if (field.arrayGroupID != 0)
 			continue;
 		
 		//TODO:MIWEI;
 		FieldName _name_ =  field.name1.empty() ? field.name : field.name1;

 		if (ParseConfig::Instance().dateTimeType(field.type))
 		{
 			if (!hasdata)
 				format_append(str, "unix_timestamp(`%s`) as %s_uint64", _name_.c_str(), _name_.c_str());
 			else
 				format_append(str, ",unix_timestamp(`%s`) as %s_uint64", _name_.c_str(), _name_.c_str());
 			hasdata = true;
 		}
 		else
 		{
 			if (!hasdata)
 				format_append(str, "`%s`", _name_.c_str());
 			else
 				format_append(str, ",`%s`", _name_.c_str());
 			hasdata = true;
 		}
 	}
 
 
 	//array member.
 	for (int i = 0; i < s.arrayInfos.size(); ++i)
 	{
 		const DBTable::ArrayData& data = s.arrayInfos[i];
 		for (int n = 0; n < data.belongeField.size(); ++n)
 		{
 			//if (s.arrayInfos[i].info.type ==)
 			if (ParseConfig::Instance().dateTimeType(s.arrayInfos[i].info.type))
 			{
 				if (!hasdata)
 					format_append(str, "unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
 				else
 					format_append(str, ",unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
 				hasdata = true;
 			}
 			else
 			{
 				if (!hasdata)
 					format_append(str, "`%s`", data.belongeField[n].c_str());
 				else
 					format_append(str, ",`%s`", data.belongeField[n].c_str());
 				hasdata = true;
 			}
 		}
 	}
 
 	if (hasdata)
 	{
 		// 有数据;
 		format_append(str, " FROM %s ", s.name.c_str());
 		format_append(str, "WHERE `%s`=_id;\n", idName);
 	}
 	else
 	{
 	}

	return true;
}




void _writeTab(int index, FILE* file)
{
 	std::string s;
 	for (int i = 0; i < index; ++i)
 	{
 		s += ("\t");
 	}
 	fprintf(file, s.c_str());
}

void _writeStr(int index, FILE* file, const char* const s)
{
	_writeTab(index, file);
	fprintf(file, s);
}

void _writeStrArg(int index, FILE* f, const char* s, ...)
{
	_writeTab(index, f);

	const int MAX_ASSERT_STRING_LENGTH = 1024 * 8;
	char formatStringBuffer[1024 * 8] = { 0 };
	va_list args;
	va_start(args, s);
	vsnprintf(formatStringBuffer, sizeof(formatStringBuffer), s, args);
	va_end(args);

	std::string tempstr = formatStringBuffer;

	fprintf(f, tempstr.c_str());
}


void addInVCProj(const char* const toolPath, const char* const vcProjPath, const char* const fileName)
{
}


std::string& replace(std::string& str, const std::string oldValue, const char* const newValue)
{
//	assert((oldValue.empty() == false) || newValue);

	std::string stemp;
	//std::string::size_type pos = str.find(oldValue);
	//if (pos != std::string::npos)

	int pos = str.find(oldValue);
	while (pos != -1)
	{
		int oldLen = oldValue.length();
		stemp += str.substr(0, pos);
		stemp += newValue;
		int endFlag = oldLen + pos;
		str = str.substr(endFlag);
		//stemp += str;
		pos = str.find(oldValue);
	}
	if ((pos == -1) && (str.length() > 0))
	{
		stemp += str;
	}

	str = stemp;
	return str;
}

bool compare(const DBField& fOld, const DBField& fNew)
{
// 	if (fOld.name1.compare(fNew.name1.c_str(), true) != 0)
// 		return false;
// 
// // 	// 代表change;
// // 	if (!fNew.name1.empty())
// // 		return false;
// 	
// 	if (fNew.type.compare(fOld.type.c_str(), true) != 0)
// 		return false;
// 	
// 	if (ParseConfig::inst().stringType(fNew.type) && (fNew.arraySize!=fOld.arraySize))
// 		return false;
// 
// 	if (fNew.defaultValue.compare(fOld.defaultValue.c_str(), true) != 0)
// 		return false;
// 
// 	if (fNew.property != fOld.property)
// 		return false;
// 
// // 	if (fNew.comment.compare(fOld.comment.c_str(), true) != 0)
// // 		return false;
// 	
// 	if (fNew.autoIncrement!=fOld.autoIncrement || fNew.unique!=fOld.unique || fNew.binary!=fOld.binary || fNew.unsignedFlag!=fOld.unsignedFlag || fNew.zeroFill!=fOld.zeroFill)
// 		return false;
// 	
// 	if (fNew.afterName.compare(fOld.afterName.c_str(), true) != 0)
// 		return false;
// 
// // 	bool			autoIncrement;		// is auto increment.
// // 	bool			unique;				// is unique.
// // 	bool			binary;				// is binary data.
// // 	bool			unsignedFlag;		// is unsigned data.
// // 	bool			zeroFill;			// is fill this data use zero.
// // 
// // 	FieldName		afterName;			// alter, add column 'bb' .... after 'aa'
// 


	return true;
}

void format_append(std::string& ssss, const char* format, ...)
{
	const int MAX_ASSERT_STRING_LENGTH = 1024 * 11;
	char formatStringBuffer[1024 * 4] = { 0 };
	va_list args;
	va_start(args, format);
	vsnprintf(formatStringBuffer, sizeof(formatStringBuffer), format, args);
	va_end(args);
	
	ssss += formatStringBuffer;
}

bool _compare(std::string a, std::string b, bool igore)
{
	if (igore)
	{
		toUpper(a);
		toUpper(b);
		return a == b;
	}
	else
		return a == b;
}