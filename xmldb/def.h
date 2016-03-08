#pragma once
#include <string>
#include <vector>
#include <map>

#include <stdio.h>

#include <sys/types.h> 
#include <sys/stat.h>
#include <memory.h>

#ifdef WIN32
#include <io.h>
#include <errno.h>
#include <Windows.h>
#else
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#endif

typedef std::string		ArrayName;
typedef std::string		TableName;
typedef std::string		FieldName;
typedef std::string		FieldType;
typedef std::string		TypeFormat;
typedef std::string		DefaultValue;
typedef std::string		Comment;
typedef std::string		CharacterSet;
typedef std::string		BatchName;
typedef std::string		BatchParam;
typedef std::string		BatchParamName;
typedef std::string		ClassName;
typedef std::string		ClassFunc;
typedef std::string		EnumName;


// app params.
struct Param
{
	std::string cppOutPath;
	std::string sqlOutPath;
	std::string vcProjToolPath;
	std::string defFile;
	std::string workBakFile;
	std::string workPath;
};


//filed property type.
//for example: in mysql, PRIMARY_KEY.
enum DB_PROPERTY
{
	DB_PROPERTY_INVALID = 0,

	DB_PROPERTY_PRIMARY_KEY,	// 主键(NOT NULL)
	DB_PROPERTY_INDEX_KEY,		// 索引键
	DB_PROPERTY_UNIQUE_KEY,		//------唯一约束键(can null)
	DB_PROPERTY_FULLTEXT_KEY,	//------

	DB_PROPERTY_COUNT,
};

// xml childs.
// db
//	table
//		field.
//	batch
//		field.
enum LABLE_TYPE
{
	LABLE_TYPE_INVALID = -1,
	LABLE_TYPE_TABLE,
	LABLE_TYPE_BATCH,
	LABLE_TYPE_COUNT,
};
LABLE_TYPE getLableType(const char* const s);


// table opcode flag.
enum TABLE_OPCODE
{
	TABLE_OPCODE_INVALID = -1,
	TABLE_OPCODE_CREATE,
	TABLE_OPCODE_UPDATE,
	TABLE_OPCODE_DELETE,
	TABLE_OPCODE_COUNT,
};
TABLE_OPCODE getTableOpcode(const char* const s);

// 每个field 的操作INDEX;
// 主要用在table update 的所有field标签;
enum FIELD_OPCODE
{
	FIELD_OPCODE_INVALID = -1,

	FIELD_OPCODE_NO_UPDATE=0,			// field. 用于数组的不更新field.
	FIELD_OPCODE_CHANGE,			// 改变列，记住加入name1
	FIELD_OPCODE_ADD,				// 增加列
	FIELD_OPCODE_DROP,				// 删除列

	FIELD_OPCODE_ADDINDEX,			// 增加索引键
	FIELD_OPCODE_DROPINDEX,			// 删除索引键 

	FIELD_OPCODE_COUNT,
};
FIELD_OPCODE getFieldOpcode(const char* const s);


class DBField;
class DBArray;
class DBTable;
class DataCenter;


class DBField {
public:
	// xml data.
	FieldName		name;				// name.
	FieldName		name1;				// for alter name name1.
	FieldType		type;				// type;
	int				arrayGroupID;		// this for c struct.
	int				arraySize;			// type size  example `int(11)` 11.
	bool			notNull;			// notNull = true then add tag NOT NULL.
	DefaultValue	defaultValue;		// default value.
	DB_PROPERTY		property;			// primary key or index key.
	Comment			comment;			// comments.
	bool			autoIncrement;		// is auto increment.
	bool			unique;				// is unique.
	bool			binary;				// is binary data.
	bool			unsignedFlag;		// is unsigned data.
	bool			zeroFill;			// is fill this data use zero.
	FieldName		afterName;			// alter, add column 'bb' .... after 'aa'

	//TODO:
	// temporary data
	FieldType		cType;
	TypeFormat		typeFormat;
	FIELD_OPCODE	opcode;
	FieldName		upperNames;

	DBField()
	{
		arrayGroupID = 0;
		arraySize = 0;
		//notNull = false;

		opcode = FIELD_OPCODE_INVALID;
	}
	//bool operator == (const DBField& b);
};
bool compare(const DBField& fOld, const DBField& fNew);
typedef std::vector<DBField> TableFieldArray;

class DBArray
{
public:
	DBArray()
	{
		stringTypeSize = 0;
	}

	ArrayName		name;
	FieldType		type;
	FieldType		cType;
	TypeFormat		typeFormat;
	Comment			comment;
	int				stringTypeSize;		// 如果是string类型;

	ArrayName		upperNames;
	ArrayName		nameForEnum;		// playerAttr --> PLAYER_ATTR
};

class DBTable
{
public:
	DBTable() {

	}

	class ArrayData {
	public:
		DBArray					info;
		std::vector<FieldName>	belongeField;
		std::vector<EnumName>	belongeEnumName;

		ArrayData() {
			belongeField.clear();
			belongeEnumName.clear();
		}
	};
	typedef std::vector<ArrayData> TableArrayInfo;


	ArrayData* getArrayData(const char * const name) {
		for (size_t i = 0; i < arrayInfos.size(); ++i)
		{
			if (arrayInfos[i].info.name == name)
				return &(arrayInfos[i]);
		}
		return NULL;
	}

	const DBField* getFieldData(const char* const name) const
	{
		int nsize = fields.size();
		for (int i = 0; i < nsize; ++i)
		{
			if (!fields[i].name1.empty() && fields[i].name1== name)
				return &(fields[i]);
			if (fields[i].name == name)
				return &(fields[i]);
		}
		return NULL;
	}

public:
	TableName		name;
	CharacterSet	characterSet;
	TableFieldArray	fields;
	TableArrayInfo	arrayInfos;

	//TODO:
	// temporary data
	TableName					className;
	TableName					classNameEx;
};
bool _makeSelectTableString(const DBTable& t, std::string& s, const char* idName);

typedef std::vector<DBTable>		DBTableArray;
typedef std::vector<TableName>		DBDeleteArray;



// procedure.参数信息
struct DBBatchParam
{
	TableName	_paramName;
	FieldName	_returnField;
	TableName	_returnName;
	TableName	_tableName;
	FieldName	_fieldName;
	FieldType	_typeName;
};
typedef std::vector<DBBatchParam> DBBatchParamArray;

// procedure.列信息
struct DBBatchChild
{
	TableName	_name;
	TableName	_playerIDName;
	ClassName	_className;
	ClassFunc	_classFunc;
};

typedef std::vector<DBBatchChild> DBBatchChildren;

// procedure信息
struct DBBatch
{
	BatchName				name;
	BatchParam				param;
	FieldType				paramType;
	int						paramSize;
	Comment					comment;
	DBBatchChildren			_children;
	DBBatchParamArray		_params;
};

typedef std::vector<DBBatch>	DBBatchArray;



class DataCenter 
{
public:
	void Clear() {
		m_create.clear();
		m_update.clear();
		m_delete.clear();
		m_batchs.clear();
	}

	DBTable* getTableByNameFromCreateArray(const char* const name) {
		if (!name)
		{
			//assert(false);
			return NULL;
		}

		int createSize = m_create.size();
		for (int i = 0; i < createSize; ++i)
		{
			DBTable& table = m_create[i];
			if (table.name == name)
				return &table;
		}

		//assert(false);
		return NULL;
	}

	DBTableArray	m_create;
	DBTableArray	m_update;
	DBDeleteArray	m_delete;
	DBBatchArray	m_batchs;
};


void	_writeTab(int index, FILE* file);
void	_writeStr(int index, FILE* file, const char* const s);
void	_writeStrArg(int index, FILE* f, const char* s, ...);

void	addInVCProj(const char* const toolPath, const char* const vcProjPath, const char* const fileName);

std::string& replace(std::string& str, const std::string oldValue, const char* const newValue);


static bool fileExists(const char * path)
{
#ifdef WIN32
	return (_access(path, 0) == 0);
#else
	return (access(path, 0) == 0);
#endif
}

static void toUpper(std::string& str, int iIndex=-1)
{
	if (-1 == iIndex)
	{
		for(size_t i = 0, len = str.size(); i < len; ++i)
			str[i] = toupper(str[i]);
	}
	else
	{
		if (iIndex >= 0 && iIndex < str.length())
			str[iIndex] = toupper(str[iIndex]);
	}
}

static
void string_camel_to_all_upper(const char* const camel, char* const allupper, const int allupper_capacity, bool ignoreNumber = true, bool disableAdjacentUnderscore = true)
{
	const int len = strlen(camel);
	int j = 0;
	for (int i = 0; i < len; ++i)
	{
		if (j >= allupper_capacity - 1)
			break;
		char c = camel[i];

		if (i > 0)
		{
			bool number = !ignoreNumber && isdigit(c);
			bool prev_is_underscore = camel[i - 1] == '_' && disableAdjacentUnderscore;
			if ((isupper(c) || number) && !prev_is_underscore)
				allupper[j++] = '_';
		}
		allupper[j++] = toupper(c);
	}
	allupper[j++] = 0;
}

void format_append(std::string& s, const char* format, ...);

bool _compare(std::string, std::string, bool b);