#pragma once

#include "def.h"

#include <vector>

struct CTypeInfo
{
	FieldType		dbType;
	FieldType		cType;
	TypeFormat		cFormat;
	TypeFormat		dbResultType;	//fieldAsInt("fieldname")
	DefaultValue		defaultValue;

	bool				isArray;
	bool				canBinary;
	bool				canAutoIncrement;
	bool				canUnsigned;
	bool				canzerofill;
};

class ParseConfig
{
public:
	ParseConfig();
	~ParseConfig();

	static ParseConfig& Instance() {static ParseConfig _pc; return _pc;}

	bool		loadConfig(const char * const s);

	// 根据db类型获得ctype ctypeFormat.
	CTypeInfo*	typeInfos(FieldType& dbType);
	CTypeInfo*	typeInfos(const char* const c);

	// 如果是时间类型，那么就按照from_unixtime来序列化.
	bool		dateTimeType(const FieldType& dbType);

	// 是否是string类型;
	bool		stringType(const FieldType& dbType);

	bool        blobType(const FieldType& dbType);

	// 组sql 的时候 int(11),来根据类型填写后面显示符.
	int			typeSizeByFieldType(const FieldType& dbType, const int configSize);


	// for check field. TODO:可以从表里读取;
	bool		_canBinary(const FieldType& dbType);
	bool		_canAutoincrement(const FieldType& dbType);
	bool		_canUnsigned(const FieldType& dbType);
	bool		_canZerofill(const FieldType& dbType);

private:
	std::vector<CTypeInfo>	m_Types;
};