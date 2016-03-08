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

	// ����db���ͻ��ctype ctypeFormat.
	CTypeInfo*	typeInfos(FieldType& dbType);
	CTypeInfo*	typeInfos(const char* const c);

	// �����ʱ�����ͣ���ô�Ͱ���from_unixtime�����л�.
	bool		dateTimeType(const FieldType& dbType);

	// �Ƿ���string����;
	bool		stringType(const FieldType& dbType);

	bool        blobType(const FieldType& dbType);

	// ��sql ��ʱ�� int(11),������������д������ʾ��.
	int			typeSizeByFieldType(const FieldType& dbType, const int configSize);


	// for check field. TODO:���Դӱ����ȡ;
	bool		_canBinary(const FieldType& dbType);
	bool		_canAutoincrement(const FieldType& dbType);
	bool		_canUnsigned(const FieldType& dbType);
	bool		_canZerofill(const FieldType& dbType);

private:
	std::vector<CTypeInfo>	m_Types;
};