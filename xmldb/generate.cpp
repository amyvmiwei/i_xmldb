#include "generate.h"
#include "app.h"
#include "config.h"
#include "pugixml/pugixml.hpp"

#include <sstream>
#include <assert.h>

bool GenerateSql::generate(DataCenter& Adata, int AIndex)
{
	std::stringstream ss;
	ss << XMLDBApp::Instance().getParam().sqlOutPath << "update_" << AIndex << ".sql";
	std::string sqlfile = ss.str();

	FILE* pFile = fopen(sqlfile.c_str(), "w+");
	if (!pFile)
	{
		//assert(false);
		return false;
	}

	int count = Adata.m_create.size();
	for (int i = 0; i < count; ++i)
	{
		_generateSql(Adata.m_create[i], pFile);
	}

	fprintf(pFile, "\n\n");
	int nupdate = Adata.m_update.size();
	for (int i = 0; i < nupdate; ++i)
	{
		_generateUpdateTableSql(Adata.m_update[i], pFile);
	}

	fprintf(pFile, "\n\n");
	int ndelete = Adata.m_delete.size();
	for (int i = 0; i < ndelete; ++i)
	{
		_generateDropTableSql(Adata.m_delete[i], pFile);
	}

	fclose(pFile);
	return true;
}



// TODO;
// 1.注意int 与 binary 的互斥等相关问题.

// 2.不能2个主键等问题.

// 3.注意PROPERTY,是否多个标记，以及create table, alter table , add colum, add INDEX, ADD UNIQUE INDEX.
// example:
// ALTER TABLE `test_zdy`.`debris` 
// CHANGE COLUMN `tableID` `tableID` INT(11) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT,
// ADD COLUMN `debriscol` VARCHAR(45) BINARY NULL AFTER `createTime`,
// ADD INDEX `index2` (`id` ASC),
// ADD UNIQUE INDEX `tableID_UNIQUE` (`tableID` ASC);

// 4. 根据类型来填写size;datetime.
// CREATE TABLE `debris` (
// 	`id` int(11) NOT NULL DEFAULT '-1',
// 	`gui` varchar(36) NOT NULL,
// 	`playerid` int(11) NOT NULL DEFAULT '0',
// 	`tableID` int(11) NOT NULL DEFAULT '0',
// 	`bagType` int(11) NOT NULL DEFAULT '0',
// 	`position` int(11) NOT NULL DEFAULT '-1',
// 	`overlap` int(11) NOT NULL DEFAULT '0',
// 	`createTime` datetime NOT NULL DEFAULT '2014-08-23 00:00:00',
// 	PRIMARY KEY(`id`)
// 	) ENGINE = InnoDB DEFAULT CHARSET = utf8;


int GenerateSql::_generateSql(const DBTable& s, FILE* file)
{
	_generateDropTableSql(s, file);
	_generateCreateTableSql(s, file);
	return 0;
}

void GenerateSql::_generateDropTableSql(const DBTable& s, FILE* file)
{
	_writeStrArg(0, file, "DROP TABLE IF EXISTS `%s`;\n", s.name.c_str());
}

void GenerateSql::_generateDropTableSql(const TableName& s, FILE* file)
{
	_writeStrArg(0, file, "DROP TABLE IF EXISTS `%s`;\n", s.c_str());
}

void GenerateSql::_generateCreateTableSql(const DBTable& s, FILE* file)
{
	int nTabIndex = 0;
	// CREATE TABLE `version` (
	_writeStrArg(nTabIndex, file, "CREATE TABLE `%s` (\n", s.name.c_str());
	{
		++nTabIndex;
		int fieldCount = s.fields.size();
		for (int i = 0; i < fieldCount; ++i)
		{
			const DBField& field = s.fields[i];

			// title,and type(11)
			int sizeRet = ParseConfig::Instance().typeSizeByFieldType(field.type, field.arraySize);
			if (0 == sizeRet)
				_writeStrArg(nTabIndex, file, "`%s` %s ", field.name.c_str(), field.type.c_str());
			else
				_writeStrArg(nTabIndex, file, "`%s` %s(%d) ", field.name.c_str(), field.type.c_str(), field.arraySize);

			//unsigned 
			if (field.unsignedFlag)
				fprintf(file, "UNSIGNED");

			// zero fill.
			if (field.zeroFill)
				fprintf(file, "ZEROFILL");

			// can null.
			//if (field.notNull == true)
				fprintf(file, "NOT NULL ");

			// auto increment.
			if (field.autoIncrement)
				fprintf(file, "AUTO_INCREMENT ");


			// default value.
			if (!ParseConfig::Instance().blobType(field.type))
			{
				if (field.defaultValue.empty() == false)
					fprintf(file, "DEFAULT %s ", field.defaultValue.empty() ? "NULL" : field.defaultValue.c_str());
				else
				{
					//获得c类型以及c格式.
					CTypeInfo * typeInfo = ParseConfig::Instance().typeInfos(field.type.c_str());
					if (typeInfo)
					{
						if (typeInfo->defaultValue.empty())
							assert(false);
						else
							fprintf(file, "DEFAULT %s ", typeInfo->defaultValue.c_str());
					}
					else
					{
						assert(false);
					}
				}
			}			

			// TODO：MUST has key.			
			// 			if (i != (fieldCount - 1))
			fprintf(file, ",");
			fprintf(file, "\n");
		}


		// primary key, index key.
		bool isfirst = true;
		for (int i = 0; i < fieldCount; ++i)
		{
			const DBField& field = s.fields[i];
			// 
			if (field.property == DB_PROPERTY_PRIMARY_KEY)
			{
				if (isfirst)
				{
					fprintf(file, "\tPRIMARY KEY (`%s`)", field.name.c_str());
					isfirst = false;
				}
				else
					fprintf(file, ",\n\tPRIMARY KEY (`%s`)", field.name.c_str());
			}

			if (field.property == DB_PROPERTY_INDEX_KEY)
			{
				if (isfirst)
				{
					fprintf(file, "\tKEY `%s_index` (`%s`)", field.name.c_str(), field.name.c_str());
					isfirst = false;
				}
				else
					fprintf(file, ",\n\tKEY `%s_index` (`%s`)", field.name.c_str(), field.name.c_str());
			}
		}

		--nTabIndex;
	}

	//) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
	_writeStr(nTabIndex, file, ") ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;\n");

}


void GenerateSql::_generateUpdateFieldSql(const int nTabIndex, const DBField& field, FILE* file)
{
	// 如果是修改index那么就不需要进来这里了.
	assert(field.opcode != FIELD_OPCODE_ADDINDEX && field.opcode != FIELD_OPCODE_DROPINDEX);
	// title,and 

	FieldName updateName = field.name1;
	if (updateName.empty())
		updateName = field.name.c_str();

	if (field.opcode == FIELD_OPCODE_CHANGE)
		_writeStrArg(nTabIndex, file, "`%s` `%s` ", field.name.c_str(), updateName.c_str());
	else
		_writeStrArg(nTabIndex, file, "`%s` ", field.name.c_str());

	//type(11)
	int sizeRet = ParseConfig::Instance().typeSizeByFieldType(field.type, field.arraySize);
	if (0 == sizeRet)
		_writeStrArg(nTabIndex, file, "%s ", field.type.c_str());
	else
		_writeStrArg(nTabIndex, file, "%s(%d) ", field.type.c_str(), field.arraySize);

	//unsigned 
	if (field.unsignedFlag)
		fprintf(file, "UNSIGNED");

	// zero fill.
	if (field.zeroFill)
		fprintf(file, "ZEROFILL");

	// can null.
	//if (field.notNull == true)
		fprintf(file, "NOT NULL ");

	// auto increment.
	if (field.autoIncrement)
		fprintf(file, "AUTO_INCREMENT ");

	if (!ParseConfig::Instance().blobType(field.type))
	{
		// default value.
		if (field.defaultValue.empty() == false)
			fprintf(file, "DEFAULT %s ", field.defaultValue.empty() ? "NULL" : field.defaultValue.c_str());
		else
		{
			//获得c类型以及c格式.
			CTypeInfo * typeInfo = ParseConfig::Instance().typeInfos(field.type.c_str());
			if (typeInfo)
			{
				if (typeInfo->defaultValue.empty())
					assert(false);
				else
					fprintf(file, "DEFAULT %s ", typeInfo->defaultValue.c_str());
			}
			else
			{
				assert(false);
			}
		}
	}


	// TODO：MUST has key.			
	// 			if (i != (fieldCount - 1))

	if (field.opcode == FIELD_OPCODE_ADD)
	{
		if (!field.afterName.empty())
			_writeStrArg(0, file, " AFTER `%s`", field.afterName.c_str());		
	}
	if (FIELD_OPCODE_CHANGE == field.opcode)
	{
		if (!field.afterName.empty())
			_writeStrArg(0, file, " AFTER `%s`", field.afterName.c_str());
	}

// 	fprintf(file, ",");
// 	fprintf(file, "\n");

	// UPDATE 不支持property.
// 	if (field.property == DB_PROPERTY_PRIMARY_KEY)
// 	{
// 		if (isfirst)
// 		{
// 			fprintf(file, "\tPRIMARY KEY (`%s`)", field.name.c_str());
// 			isfirst = false;
// 		}
// 		else
// 			fprintf(file, ",\n\tPRIMARY KEY (`%s`)", field.name.c_str());
// 	}
// 
// 	if (field.property == DB_PROPERTY_INDEX_KEY)
// 	{
// 		if (isfirst)
// 		{
// 			fprintf(file, "\tKEY `%s_index` (`%s`)", field.name.c_str(), field.name.c_str());
// 			isfirst = false;
// 		}
// 		else
// 			fprintf(file, ",\n\tKEY `%s_index` (`%s`)", field.name.c_str(), field.name.c_str());
// 	}
}

void GenerateSql::_generateUpdateTableSql(const DBTable& s, FILE* file)
{
	/*
	ALTER TABLE `test_zdy`.`equip`
	DROP COLUMN `strengthenLevel`,
	CHANGE COLUMN `tableID` `tableID` INT(11) ZEROFILL NOT NULL DEFAULT '0' ,
	ADD COLUMN `equipcol` VARCHAR(45) BINARY NOT NULL AFTER `position`,
	ADD COLUMN `equipcol1` INT NULL AFTER `equipcol`,
	ADD COLUMN `equipcol2` INT ZEROFILL NOT NULL AUTO_INCREMENT AFTER `equipcol1`,
	ADD COLUMN `equipcol3` VARCHAR(45) NULL AFTER `equipcol2`,
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`id`, `equipcol`),
	ADD UNIQUE INDEX `equipcol_UNIQUE` (`equipcol` ASC),
	ADD UNIQUE INDEX `equipcol3_UNIQUE` (`equipcol3` ASC),
	ADD UNIQUE INDEX `equipcol2_UNIQUE` (`equipcol2` ASC),
	ADD UNIQUE INDEX `equipcol1_UNIQUE` (`equipcol1` ASC);
	*/

	//idea
	// 1. <filed .... />  -> <change ..../>  <add ..../> <drop ..0 />
	// 2. 直接与以前的比对，找出差异;（删除的呢?）

	// ALTER TABLE `BUILD`
	_writeStrArg(0, file, "ALTER TABLE `%s` \n", s.name.c_str());
	bool first = true;
	int fieldCount = s.fields.size();
	for (int i = 0; i < fieldCount; ++i)
	{
		const DBField& field = s.fields[i];
		if (!first && (field.opcode != FIELD_OPCODE_NO_UPDATE))
		{
			fprintf(file, ",");
			fprintf(file, "\n");
		}

		
		if (field.opcode == FIELD_OPCODE_ADD)
		{
			_writeStrArg(0, file, "ADD COLUMN  ");
			_generateUpdateFieldSql(0, field, file);
			first = false;
		}
		else if (field.opcode == FIELD_OPCODE_CHANGE)
		{
			_writeStrArg(0, file, "CHANGE COLUMN  ");
			_generateUpdateFieldSql(0, field, file);
			first = false;
		}
		else if (field.opcode == FIELD_OPCODE_DROP)
		{
			_writeStrArg(0, file, "DROP COLUMN `%s`", field.name.c_str());
			first = false;
		}
		else if (field.opcode == FIELD_OPCODE_ADDINDEX)
		{
			_writeStrArg(0, file, "ADD INDEX `%s_index` (`%s` %s)", field.name.c_str(), field.name.c_str(), field.type.c_str());
			first = false;
		}
		else if (field.opcode == FIELD_OPCODE_DROPINDEX)
		{
			_writeStrArg(0, file, "DROP INDEX `%s_index`", field.name.c_str());
			first = false;
		}
		else if (field.opcode == FIELD_OPCODE_NO_UPDATE)
		{
			// do nothing.
		}
		else
		{
			//assert(false);
		}
	}

	_writeStr(0, file, ";\n");
}






//////////////////////////////////////////////////////////////////////////
int	GenerateCpp::m_memberTypeMaxLength = 0;
int	GenerateCpp::m_fuctionTypeMaxLength = 0;
int	GenerateCpp::m_memberNameMaxLength = 0;
int	GenerateCpp::m_functionNameMaxLength = 0;
void GenerateCpp::generate(DataCenter & AData)
{
	int count = AData.m_create.size();
	for (int i = 0; i < count; ++i)
	{
		const DBTable& tb = AData.m_create[i];
		_collectFieldsMaxLength(tb);
		_generateTables(AData.m_create[i]);
	}
}


bool GenerateCpp::_generateTables(const DBTable& t)
{
	std::string cppPath = XMLDBApp::Instance().getParam().cppOutPath;
	if (cppPath.empty())
	{
		//Log::inst()._error("GenerateCpp _generateTables error! get cpp out path error!");
		assert(false);
		return false;
	}

	std::string cfile = cppPath;
	format_append(cfile, "db_%s.h", t.name.c_str());

	printf("generate cpp %s\n", cfile.c_str());

	FILE * pH = fopen(cfile.c_str(), "w+");
	if (!pH)
	{
		assert(false);
		return false;
	}
	_generateTableHeader(t, pH);
	fclose(pH);


	std::string cppFile = cppPath;
	format_append(cppFile, "db_%s.cpp", t.name.c_str());
	FILE* pCpp = fopen(cppFile.c_str(), "w+");
	if (!pCpp)
	{
		assert(false);
		return false;
	}
	_generateTableCpp(t, pH);
	fclose(pCpp);
	return true;
}


void GenerateCpp::_generateTableCpp(const DBTable& t, FILE* f)
{
	int nTab = -1;
	//_writeStr(nTab, f, "#include \"stdafx.h\" \n");
	_writeStrArg(nTab, f, "#include \"db_%s.h\"  \n", t.name.c_str());
	_writeStr(nTab, f, "#include \"player.h\" \n");

	_writeStr(nTab, f, "\n\n");
	_writeStr(nTab, f, "namespace tech\n");
	_writeStr(nTab, f, "{\n");
	{
		_generateConstruction(t, f, nTab);			fprintf(f, "\n");

		_generateSyncSelectFuncSource(t, f, nTab, false); fprintf(f, "\n");
		_generateSyncSelectFuncSource(t, f, nTab, true); fprintf(f, "\n");
		_generateInsertFunc(t, f, nTab);			fprintf(f, "\n");
		_generateUpdateFunc(t, f, nTab);			fprintf(f, "\n");
		_generateDeleteFunc(t, f, nTab);			fprintf(f, "\n");
		_generateReplaceFunc(t, f, nTab);			fprintf(f, "\n");

		_generateSelectArray(t, f, nTab);			fprintf(f, "\n");
		_generateInsertArray(t, f, nTab);			fprintf(f, "\n");
		_generateUpdateArray(t, f, nTab);			fprintf(f, "\n");
		_generateDeleteArray(t, f, nTab);			fprintf(f, "\n");

		_generateInitFromDB(t, f, nTab);			fprintf(f, "\n");

	}
	_writeStr(nTab, f, "}\n");
	_writeStr(nTab, f, "\n");
}

int GenerateCpp::_generateTableHeader(const DBTable& s, FILE* file)
{
	// include.
	fprintf(file, "#pragma once\n");
	fprintf(file, "#include \"dbValueSerializer.h\" \n");
	fprintf(file, "#include \"dbInterface.h\" \n");
	fprintf(file, "#include \"dbdef.h\" \n");
	fprintf(file, "\n");
	fprintf(file, "#include \"../Utility/dbAsync.h\" \n");
	fprintf(file, "#include \"../Utility/dbResult.h\" \n");
	fprintf(file, "\n");
	fprintf(file, "#include \"../BaseSupport/BaseTypes.hpp\" \n");
	fprintf(file, "#include \"../BaseSupport/array.h\" \n");
	fprintf(file, "#include \"../BaseSupport/bitset.h\" \n");
	fprintf(file, "#include \"../BaseSupport/Assert.h\" \n");
	fprintf(file, "#include \"../BaseSupport/string.h\" \n");
	fprintf(file, "\n");
	fprintf(file, "namespace tech\n");
	fprintf(file, "{\n");

	int nTabIndex = 0;
	_writeTab(nTabIndex, file);

	fprintf(file, "class DB_%s\n", s.className.c_str());
	_writeTab(nTabIndex, file);
	fprintf(file, "{\n");
	_writeStr(nTabIndex, file, "public:\n\n");

	// enum. function. member.
	_generateEnum(s, file, nTabIndex);			fprintf(file, "\n");
	_generateConstructionHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateThisName(s, file, nTabIndex);			fprintf(file, "\n");
	_generateFunction(s, file, nTabIndex);			fprintf(file, "\n\n");

	_generateSyncSelectFuncHeader(s, file, nTabIndex, false); fprintf(file, "\n");
	_generateSyncSelectFuncHeader(s, file, nTabIndex, true); fprintf(file, "\n");
	_generateInsertHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateUpdateHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateDeleteHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateReplaceHead(s, file, nTabIndex);			fprintf(file, "\n");

	_generateSelectFunc(s, file, nTabIndex, false);	fprintf(file, "\n");
	_generateSelectFunc(s, file, nTabIndex, true);		fprintf(file, "\n");
	_generateSelectDBHandler(s, file, nTabIndex);			fprintf(file, "\n");
	_generateSelectProcedure(s, file, nTabIndex);			fprintf(file, "\n");

	_generateSelectArrayHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateInsertArrayHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateUpdateArrayHead(s, file, nTabIndex);			fprintf(file, "\n");
	_generateDeleteArrayHead(s, file, nTabIndex);			fprintf(file, "\n");

	fprintf(file, "\n");

	_writeStr(nTabIndex, file, "private:\n");
	_generateInitFromDBHead(s, file, nTabIndex);	fprintf(file, "\n");
	_generateHandlerResult(s, file, nTabIndex);	fprintf(file, "\n");

	_writeStr(nTabIndex, file, "private:\n");
	_generateMember(s, file, nTabIndex);

	_writeTab(nTabIndex, file);
	fprintf(file, "\n");
	_writeTab(nTabIndex, file);
	fprintf(file, "};\n");

	fprintf(file, "}\n\n");
	return 0;
}


void GenerateCpp::_generateConstructionHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	FieldName uppername = s.name;
	toUpper(uppername, 0);
	_writeStrArg(nTabIndex, file, "DB_%s();\n", uppername.c_str());
	_writeStrArg(nTabIndex, file, "~DB_%s();\n", uppername.c_str());
	--nTabIndex;
}

void GenerateCpp::_generateConstruction(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "%s::%s()\n", s.classNameEx.c_str(), s.classNameEx.c_str());
	_writeStrArg(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			// 来初始化这些变量;
			const DBField& field = s.fields[i];
			if (field.arrayGroupID == 0)
			{
				if (!ParseConfig::Instance().stringType(field.type))
				{
					// 去掉blob的构造函数;
					if (ParseConfig::Instance().blobType(field.type))
					{
						_writeStrArg(nTabIndex, file, "m_%s.length = 0;\n", field.name.c_str(), field.defaultValue.c_str());
						_writeStrArg(nTabIndex, file, "m_%s.pData = 0;\n", field.name.c_str(), field.defaultValue.c_str());
						continue;
					}

					if (ParseConfig::Instance().dateTimeType(field.type))
						_writeStrArg(nTabIndex, file, "m_%s = 0;\n", field.name.c_str());
					else
						_writeStrArg(nTabIndex, file, "m_%s = %s;\n", field.name.c_str(), field.defaultValue.c_str());
				}
				else
				{
					// string 不做处理.因为scl::string内部处理了.
				}
			}
			else
			{
				// array 不做处理,因为scl::array内部也处理了.
			}
		}
		--nTabIndex;
	}
	_writeStrArg(nTabIndex, file, "}\n");

	_writeStrArg(nTabIndex, file, "%s::~%s()\n", s.classNameEx.c_str(), s.classNameEx.c_str());
	_writeStrArg(nTabIndex, file, "{\n");
	{
		++nTabIndex;

		--nTabIndex;
	}
	_writeStrArg(nTabIndex, file, "}\n");
	--nTabIndex;
}

int GenerateCpp::_generateFunction(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	// normal field.
	for (int i = 0; i < s.fields.size(); ++i)
	{
		const DBField& field = s.fields[i];
		if (field.arrayGroupID == 0)
		{
			// setter.
			//_writeStrArg(nTabIndex, file, "void\t set%s(%s %s)\t", firstCharUpper.c_str(), field.cType.c_str(), field.name.c_str());
			{
				std::string sType = "void";
				int typeAddTab = _needTabCount(NEED_TAB_INDEX_FUNCTION_TYPE, sType.c_str());
				_writeStr(nTabIndex, file, sType.c_str());
				_writeTab(typeAddTab, file);

				FieldName firstCharUpper = field.name;

				//if (firstCharUpper.compare("id", true) == 0)
				if (firstCharUpper == "id")
					toUpper(firstCharUpper);
				else
					toUpper(firstCharUpper, 0);

				std::string sname;
				format_append(sname, "set%s", firstCharUpper.c_str());
				int nameAddTab = _needTabCount(NEED_TAB_INDEX_FUNCIOTN_NAME, sname.c_str());
				_writeStr(0, file, sname.c_str());
				_writeTab(nameAddTab, file);

				_writeStrArg(0, file, "(%s %s)\t{ ", field.cType.c_str(), field.name.c_str());
			}

			{
				++nTabIndex;
				_writeStrArg(0, file, "m_%s = %s;\t", field.name.c_str(), field.name.c_str());
				_writeStrArg(0, file, "m_normal_bit_set.set(NORMAL_ATTR_INDEX_%s);\t", field.upperNames.c_str());
				--nTabIndex;
			}
			_writeStr(0, file, " }\n");

			//getter.
			//_writeStrArg(nTabIndex, file, "%s\t %s() const { ", field.cType.c_str(), field.name.c_str());
			{
				std::string stype;
				format_append(stype, "%s", field.cType.c_str());
				int typeaddtab = _needTabCount(NEED_TAB_INDEX_FUNCTION_TYPE, stype.c_str());
				_writeStr(nTabIndex, file, stype.c_str());
				_writeTab(typeaddtab, file);

				std::string sname;
				format_append(sname, "%s", field.name.c_str());
				int nameaddtab = _needTabCount(NEED_TAB_INDEX_FUNCIOTN_NAME, sname.c_str());
				_writeStr(0, file, sname.c_str());
				_writeTab(nameaddtab, file);

				_writeStr(0, file, "() const\t{ ");
			}

			if (ParseConfig::Instance().stringType(field.type))
				_writeStrArg(0, file, "return m_%s.c_str();", field.name.c_str());
			else
				_writeStrArg(0, file, "return m_%s;", field.name.c_str());
			_writeStr(0, file, " }\n");
		}
	}

	if (s.arrayInfos.size() > 0)
		fprintf(file, "\n");

	for (int i = 0; i < s.arrayInfos.size(); ++i)
	{
		const DBTable::ArrayData& data = s.arrayInfos[i];
		ArrayName firstCharUpper = data.info.name;
		toUpper(firstCharUpper, 0);
		//setter.
		{//_writeStrArg(nTabIndex, file, "void\t set%s(%s v, int _index) \t {", firstCharUpper.c_str(), data.info.cType.c_str());
			std::string sType = "void";
			int typeAddTab = _needTabCount(NEED_TAB_INDEX_FUNCTION_TYPE, sType.c_str());
			_writeStr(nTabIndex, file, sType.c_str());
			_writeTab(typeAddTab, file);

			std::string sname;
			format_append(sname, "set%s", firstCharUpper.c_str());
			int nameAddTab = _needTabCount(NEED_TAB_INDEX_FUNCIOTN_NAME, sname.c_str());
			_writeStr(0, file, sname.c_str());
			_writeTab(nameAddTab, file);

			_writeStrArg(0, file, "(%s v, int _index)\t{ ", data.info.cType.c_str());
		}
		_writeStrArg(0, file, "m_%s[_index]=v; m_enum_%s_bit_set.set(_index); }\n", data.info.name.c_str(), data.info.name.c_str());

		//getter.
		{
			//_writeStrArg(nTabIndex, file, "%s\t %s(int _index) const { ", data.info.cType.c_str(), data.info.name.c_str()); 
			std::string stype;
			format_append(stype, "%s", data.info.cType.c_str());
			int typeaddtab = _needTabCount(NEED_TAB_INDEX_FUNCTION_TYPE, stype.c_str());
			_writeStr(nTabIndex, file, stype.c_str());
			_writeTab(typeaddtab, file);

			std::string sname;
			format_append(sname, "%s", data.info.name.c_str());
			int nameaddtab = _needTabCount(NEED_TAB_INDEX_FUNCIOTN_NAME, sname.c_str());
			_writeStr(0, file, sname.c_str());
			_writeTab(nameaddtab, file);

			_writeStr(0, file, "(int _index) const\t{ ");
		}
		if (ParseConfig::Instance().stringType(data.info.type))
			_writeStrArg(0, file, "return m_%s[_index].c_str();", data.info.name.c_str());
		else
			_writeStrArg(0, file, "return m_%s[_index];", data.info.name.c_str());
		_writeStr(0, file, " }\n");
	}
	--nTabIndex;
	return 0;
}


int GenerateCpp::_generateEnum(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;

	_writeTab(nTabIndex, file);
	fprintf(file, "enum NORMAL_ATTR_INDEX\n");
	_writeTab(nTabIndex, file);
	fprintf(file, "{\n");

	{
		nTabIndex++;
		_writeTab(nTabIndex, file);
		fprintf(file, "NORMAL_ATTR_INDEX_INVALID=-1,\n");
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID == 0)
			{
				_writeTab(nTabIndex, file);
				fprintf(file, "NORMAL_ATTR_INDEX_%s,\n", field.upperNames.c_str());
			}
		}
		_writeTab(nTabIndex, file);
		fprintf(file, "NORMAL_ATTR_INDEX_MAX_COUNT_,\n");
		nTabIndex--;
	}

	_writeTab(nTabIndex, file);
	fprintf(file, "};\n");

	//////////////////////////////////////////////////////////////////////////
	// to generate enum xml.
	//	fprintf(file, "\n");
	//
	// 	for (int i = 0; i < s.arrayInfos.size(); ++i)
	// 	{
	// 		const DBTable::ArrayData& data = s.arrayInfos[i];
	// 
	// 		_writeTab(nTabIndex, file);
	// 		fprintf(file, "enum ARRAY_%s_ATTR_INDEX\n", data.info.upperNames.c_str());
	// 		_writeTab(nTabIndex, file);
	// 		fprintf(file, "{\n");
	// 
	// 		++nTabIndex;
	// 		_writeTab(nTabIndex, file);
	// 		fprintf(file, "ARRAY_%s_ATTR_INDEX_INVALID=-1,\n", data.info.upperNames.c_str());
	// 		int belongs = data.belongedField.size();
	// 		for (int n = 0; n < belongs; ++n)
	// 		{
	// 			_writeTab(nTabIndex, file);
	// 
	// 			FieldName belongFieldUpperName = data.belongedField[n];
	// 			belongFieldUpperName.toupper();
	// 
	// 			fprintf(file, "ARRAY_%s_ATTR_INDEX_%s,\n", data.info.upperNames.c_str(), belongFieldUpperName.c_str());
	// 		}
	// 		_writeTab(nTabIndex, file);
	// 		fprintf(file, "ARRAY_%s_ATTR_INDEX_MAX_COUNT_,\n", data.info.upperNames.c_str());
	// 		--nTabIndex;
	// 
	// 		_writeTab(nTabIndex, file);
	// 		fprintf(file, "};\n");
	// 	}

	--nTabIndex;

	return 0;
}

int GenerateCpp::_generateMember(const DBTable& s, FILE* file, int nTabIndex)
{
	// generate member attribute.
	{
		++nTabIndex;

		// normal type.
		int normalCounter = 0;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];

			if (field.arrayGroupID == 0)
			{
				if (ParseConfig::Instance().stringType(field.type))
				{
					//_writeStrArg(nTabIndex, file, "scl::string<%d>\tm_%s;\t//%s\n", field.arraySize + 1, field.name.c_str(), field.comment.c_str());
					std::string type;
					format_append(type, "mw_base::string<%d>", field.arraySize + 1);					
					// type;
					_writeStr(nTabIndex, file, type.c_str());
					int needCount = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, type.c_str());
					_writeTab(needCount, file);
				}
				else
				{
					//_writeStrArg(nTabIndex, file, "%s\tm_%s;\t//%s\n", field.cType.c_str(), field.name.c_str(), field.comment.c_str());
					std::string type;
					format_append(type, "%s", field.cType.c_str());
					// type;
					_writeStr(nTabIndex, file, type.c_str());
					int needCount = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, type.c_str());
					_writeTab(needCount, file);
				}

				std::string nameStream;
				format_append(nameStream, "m_%s;", field.name.c_str());
				_writeStr(0, file, nameStream.c_str());
				int nameNeedTab = _needTabCount(NEED_TAB_INDEX_MEMBER_NAME, nameStream.c_str());
				_writeTab(nameNeedTab, file);

				_writeStrArg(0, file, "//%s\n", field.comment.c_str());

				++normalCounter;
			}
		}

		// generate arrays;
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			if (ParseConfig::Instance().stringType(data.info.type))
			{
				std::string type;
				format_append(type, "mw_base::array<mw_base::string<%d>, %d>", data.info.stringTypeSize + 1, data.belongeField.size());
				// type;
				_writeStr(nTabIndex, file, type.c_str());
				int needCount = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, type.c_str());
				_writeTab(needCount, file);
			}
			else
			{
				std::string type;
				format_append(type, "mw_base::array<%s, %d>", data.info.cType.c_str(), data.belongeField.size());
				// type;
				_writeStr(nTabIndex, file, type.c_str());
				int needCount = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, type.c_str());
				_writeTab(needCount, file);
			}

			std::string nameStream;
			format_append(nameStream, "m_%s;", data.info.name.c_str());
			_writeStr(0, file, nameStream.c_str());
			int nameNeedTab = _needTabCount(NEED_TAB_INDEX_MEMBER_NAME, nameStream.c_str());
			_writeTab(nameNeedTab, file);

			_writeStrArg(0, file, "//%s\n", data.info.comment.c_str());
		}

		// bitset.
		fprintf(file, "\n");

		std::string bittype;
		format_append(bittype, "mw_base::Bitset<%d>", normalCounter);
		_writeStr(nTabIndex, file, bittype.c_str());
		int bittypeTabs = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, bittype.c_str());
		_writeTab(bittypeTabs, file);

		std::string bitName = ("m_normal_bit_set;");
		_writeStr(0, file, bitName.c_str());
		int bitNameTabs = _needTabCount(NEED_TAB_INDEX_MEMBER_NAME, bitName.c_str());
		_writeTab(bitNameTabs, file);

		_writeStr(0, file, "//\n");

		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			std::string bittype;
			format_append(bittype, "mw_base::Bitset<%d>", data.belongeField.size());
			_writeStr(nTabIndex, file, bittype.c_str());
			int bittypeTabs = _needTabCount(NEED_TAB_INDEX_MEMBER_TYPE, bittype.c_str());
			_writeTab(bittypeTabs, file);

			std::string bitName;
			format_append(bitName, "m_enum_%s_bit_set;", data.info.name.c_str());
			_writeStr(0, file, bitName.c_str());
			int bitNameTabs = _needTabCount(NEED_TAB_INDEX_MEMBER_NAME, bitName.c_str());
			_writeTab(bitNameTabs, file);

			_writeStr(0, file, "//\n");
		}

		--nTabIndex;
	}

	return 0;
}

void GenerateCpp::_generateSyncSelectFuncHeader(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID)
{
	++nTabIndex;
	if (!usePlayerID)
		_writeStr(nTabIndex, file, "void syncSelect(mw_util::DataBase& db);\n");
	else
		_writeStr(nTabIndex, file, "void syncSelect_playerId(mw_util::DataBase& db, const mw_base::int64 _playerDBID);\n");
	--nTabIndex;
}

void GenerateCpp::_generateSyncSelectFuncSource(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID)
{
	++nTabIndex;
	if (!usePlayerID)
		_writeStrArg(nTabIndex, file, "void %s::syncSelect(mw_util::DataBase& db)\n", s.classNameEx.c_str());
	else
		_writeStrArg(nTabIndex, file, "void %s::syncSelect_playerId(mw_util::DataBase& db, const mw_base::int64 _playerDBID)\n", s.classNameEx.c_str());
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		// begine.
		_writeStr(nTabIndex, file, "string64k s = \"SELECT ");

		FieldName tempPlayerID;
		//normal members.
		bool hasdata = false;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;

			if (ParseConfig::Instance().dateTimeType(field.type))
			{
				if (!hasdata)
					_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				else
					_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				hasdata = true;
			}
			else
			{
				if (!hasdata)
					_writeStrArg(0, file, "`%s`", field.name.c_str());
				else
					_writeStrArg(0, file, ",`%s`", field.name.c_str());
				hasdata = true;
			}
		}

		//array member.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				if (ParseConfig::Instance().dateTimeType(s.arrayInfos[i].info.type))
				{
					if (!hasdata)
						_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					hasdata = true;
				}
				else
				{
					if (!hasdata)
						_writeStrArg(0, file, "`%s`", data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",`%s`", data.belongeField[n].c_str());
					hasdata = true;
				}
			}
		}

		if (hasdata)
		{
			// 有数据;
			_writeStrArg(0, file, " FROM %s \";\n", s.name.c_str());
			if (!usePlayerID)
				_writeStr(nTabIndex, file, "s.format_append(\"WHERE `id`=%%lld;\", m_id);\n");
			else
				_writeStr(nTabIndex, file, "s.format_append(\"WHERE `playerID`=%%lld;\", _playerDBID);\n");
			_writeStr(nTabIndex, file, "mw_util::DBResult* r = db.execute(s.c_str(), s.length());\n");
			_writeStr(nTabIndex, file, "if (r==NULL) return;\n");
			_writeStr(nTabIndex, file, "while(r->next())\n");
			{
				++nTabIndex;
				_writeStr(nTabIndex, file, "_initFromDB(*r);\n");
				--nTabIndex;
			}			
		}
		else
		{
			assert(false);
			_writeStr(nTabIndex, file, "Assert(false);");
		}

		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;
	return;
}

void GenerateCpp::_generateInsertHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "void insert(mw_util::DataBase& db);\n");
	--nTabIndex;
}

int GenerateCpp::_generateInsertFunc(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::insert(mw_util::DataBase& db)\n", s.classNameEx.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;

		_writeStrArg(nTabIndex, file, "string64k s = \"INSERT INTO `%s` (\";\n", s.name.c_str());
		_writeStr(nTabIndex, file, "ValueSerializer valueSerializer;\n");
		//normal dirty.
		_writeStr(nTabIndex, file, "bool hasData = false;\n");
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;
			_writeStrArg(nTabIndex, file, "if (m_normal_bit_set[NORMAL_ATTR_INDEX_%s])", field.upperNames.c_str());
			{
				_writeStr(nTabIndex, file, "{");
				++nTabIndex;

				_writeStrArg(1, file, "s += (\"`%s`,\");", field.name.c_str());
				if (ParseConfig::Instance().dateTimeType(field.type))
					_writeStrArg(1, file, "valueSerializer.pushDatetimeValue(m_%s);", field.name.c_str());
				else
					_writeStrArg(1, file, "valueSerializer << m_%s;", field.name.c_str());
				_writeStr(1, file, "hasData = true;");
				--nTabIndex;
				_writeStr(nTabIndex, file, "}\n");
			}
		}

		//enums dirty.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				_writeStrArg(nTabIndex, file, "if (m_enum_%s_bit_set[%s])", data.info.name.c_str(), data.belongeEnumName[n].c_str());
				{
					_writeStr(nTabIndex, file, "{");
					++nTabIndex;
					_writeStrArg(1, file, "s += (\"`%s`,\");", data.belongeField[n].c_str());
					if (ParseConfig::Instance().dateTimeType(data.info.type))
						_writeStrArg(1, file, "valueSerializer.pushDatetimeValue(m_%s[%s]);", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					else
						_writeStrArg(1, file, "valueSerializer << m_%s[%s];", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					_writeStr(1, file, "hasData = true;");
					--nTabIndex;
					_writeStr(nTabIndex, file, "}\n");
				}
			}
		}

		_writeStr(nTabIndex, file, "\n");
		_writeStr(nTabIndex, file, "if (!hasData)\n");
		_writeStr(nTabIndex + 1, file, "return;\n");

		_writeStr(nTabIndex, file, "s.substract(1);\n");
		_writeStr(nTabIndex, file, "s.append(\") VALUES (\");\n");
		_writeStr(nTabIndex, file, "s.append(valueSerializer.GetValueString().c_str());\n");
		_writeStr(nTabIndex, file, "s += \");\";\n");
		//_writeStr(nTabIndex, file, "p.executeSql(s.c_str());\n");
		_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");

		--nTabIndex;
	}
	_writeTab(nTabIndex, file);
	fprintf(file, "}\n");
	--nTabIndex;
	return 0;
}

void GenerateCpp::_generateUpdateHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "void update(mw_util::DataBase& db);\n");
	--nTabIndex;
}

int GenerateCpp::_generateUpdateFunc(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::update(mw_util::DataBase& db)\n", s.classNameEx.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		_writeStrArg(nTabIndex, file, "string64k s = \"UPDATE `%s` SET \";\n", s.name.c_str());
		_writeStr(nTabIndex, file, "ValueSerializer sqlSerializer;\n");
		
		//first check set id already.
		_writeStr(nTabIndex, file, "//check id set.\n");
		_writeStrArg(nTabIndex, file, "assertf(m_normal_bit_set[NORMAL_ATTR_INDEX_ID], \"[db_%s] update 必须设置id.\");\n\n", s.name.c_str());
		
		// normal dirty flags.
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;
			if (_compare(field.name, "id", true))
				continue;
			_writeStrArg(nTabIndex, file, "if (m_normal_bit_set[NORMAL_ATTR_INDEX_%s])", field.upperNames.c_str());
			{
				_writeStr(nTabIndex, file, "{");
				++nTabIndex;

				_writeStrArg(0, file, "sqlSerializer.tagName(\"%s\");", field.name.c_str());
				if (ParseConfig::Instance().dateTimeType(field.type))
					_writeStrArg(1, file, "sqlSerializer.pushDatetimeValue(m_%s);", field.name.c_str());
				else
					_writeStrArg(0, file, "sqlSerializer << m_%s;", field.name.c_str());

				--nTabIndex;
				_writeStr(nTabIndex, file, "}\n");
			}
		}

		// enum dirty flags.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				_writeStrArg(nTabIndex, file, "if (m_enum_%s_bit_set[%s])", data.info.name.c_str(), data.belongeEnumName[n].c_str());
				{
					_writeStr(nTabIndex, file, "{");
					++nTabIndex;

					_writeStrArg(0, file, "sqlSerializer.tagName(\"%s\");", data.belongeField[n].c_str());
					if (ParseConfig::Instance().dateTimeType(data.info.type))
						_writeStrArg(1, file, "sqlSerializer.pushDatetimeValue(m_%s[%s]);", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					else
						_writeStrArg(0, file, "sqlSerializer << m_%s[%s];", data.info.name.c_str(), data.belongeEnumName[n].c_str());

					--nTabIndex;

					_writeStr(nTabIndex, file, "}\n");
				}
			}
		}

		fprintf(file, "\n");
		_writeStr(nTabIndex, file, "if (sqlSerializer.hasData())\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;
			_writeStr(nTabIndex, file, "sqlSerializer.tagWHERE();\n");
			_writeStr(nTabIndex, file, "sqlSerializer.tagName(\"id\");\n");
			_writeStr(nTabIndex, file, "sqlSerializer << m_id;\n");

			_writeStr(nTabIndex, file, "ValueTables& valueString = sqlSerializer.GetValueString();\n");
			_writeStr(nTabIndex, file, "s.append(valueString.c_str());\n");
			_writeStr(nTabIndex, file, "s.append(';');\n");
			_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");
			--nTabIndex;
		}
		_writeStr(nTabIndex, file, "}\n");

		--nTabIndex;
	}

	_writeTab(nTabIndex, file);
	fprintf(file, "}\n");
	--nTabIndex;
	return 0;
}

void GenerateCpp::_generateDeleteHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "void erase(mw_util::DataBase& db);\n");
	--nTabIndex;
}

int GenerateCpp::_generateDeleteFunc(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::erase(mw_util::DataBase& db)\n", s.classNameEx.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		_writeTab(nTabIndex, file);
		fprintf(file, "string256 s;\n");
		_writeTab(nTabIndex, file);
		fprintf(file, "s.format_append(\"DELETE FROM %s WHERE `id`=%%lld;\", m_id);\n", s.name.c_str());
		_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");
		--nTabIndex;
	}
	_writeTab(nTabIndex, file);
	fprintf(file, "}\n");
	return 0;
}

void GenerateCpp::_generateReplaceFunc(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::replace(kit::Database& db, bool sync)\n", s.classNameEx.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		_writeStrArg(nTabIndex, file, "string64k s = \"REPLACE INTO `%s` (\";\n", s.name.c_str());
		_writeStr(nTabIndex, file, "ValueSerializer valueSerializer;\n");
		//normal dirty.
		//		_writeStr(nTabIndex, file, "bool hasData = false;\n");
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;
			// 			_writeStrArg(nTabIndex, file, "if (m_normal_bit_set[NORMAL_ATTR_INDEX_%s])", field.upperNames.c_str());
			// 			{
			// 				_writeStr(nTabIndex, file, "{");
			++nTabIndex;

			_writeStrArg(1, file, "s += (\"`%s`,\");\n", field.name.c_str());
			if (ParseConfig::Instance().dateTimeType(field.type))
				_writeStrArg(1, file, "valueSerializer.pushDatetimeValue(m_%s);\n", field.name.c_str());
			else
				_writeStrArg(1, file, "valueSerializer << m_%s;\n", field.name.c_str());
			//				_writeStr(1, file, "hasData = true;");
			--nTabIndex;
			//				_writeStr(nTabIndex, file, "}\n");
			//			}
		}

		//enums dirty.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				// 				_writeStrArg(nTabIndex, file, "if (m_enum_%s_bit_set[%s])", data.info.name.c_str(), data.belongedEnumName[n].c_str());
				// 				{
				// 					_writeStr(nTabIndex, file, "{");
				++nTabIndex;
				_writeStrArg(1, file, "s += (\"`%s`,\");\n", data.belongeField[n].c_str());
				if (ParseConfig::Instance().dateTimeType(data.info.type))
					_writeStrArg(1, file, "valueSerializer.pushDatetimeValue(m_%s[%s]);\n", data.info.name.c_str(), data.belongeEnumName[n].c_str());
				else
					_writeStrArg(1, file, "valueSerializer << m_%s[%s];\n", data.info.name.c_str(), data.belongeEnumName[n].c_str());
				//					_writeStr(1, file, "hasData = true;");
				--nTabIndex;
				// 					_writeStr(nTabIndex, file, "}\n");
				// 				}
			}
		}

		_writeStr(nTabIndex, file, "\n");
		// 		_writeStr(nTabIndex, file, "if (!hasData)\n");
		// 		_writeStr(nTabIndex + 1, file, "return;\n");

		_writeStr(nTabIndex, file, "s.substract(1);\n");
		_writeStr(nTabIndex, file, "s.append(\") VALUES (\");\n");
		_writeStr(nTabIndex, file, "s.append(valueSerializer.GetValueString().c_str());\n");
		_writeStr(nTabIndex, file, "s += \");\";\n");
		_writeStr(nTabIndex, file, "if (sync)\n");
		_writeStr(nTabIndex + 1, file, "db.execute(s.c_str(), s.length());\n");
		_writeStr(nTabIndex, file, "else\n");
		_writeStr(nTabIndex + 1, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");

		--nTabIndex;
	}
	_writeTab(nTabIndex, file);
	fprintf(file, "}\n");
	--nTabIndex;
}

void GenerateCpp::_generateReplaceHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "void replace(kit::Database& db, bool sync=false);\n");
	--nTabIndex;
}

int GenerateCpp::_generateSelectFunc(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "template<typename TClass>\n");
	if (!usePlayerID)
		_writeStr(nTabIndex, file, "void select(mw_util::DataBase& db, int userData, mw_base::class_function func)\n");
	else
		_writeStr(nTabIndex, file, "void select_playerId(mw_util::DataBase& db, const int _playerDBID, int userData, mw_base::class_function func)\n");
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		// begine.
		_writeStr(nTabIndex, file, "string64k s = \"SELECT ");

		FieldName tempPlayerID;
		//normal members.
		bool hasdata = false;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;

			if (ParseConfig::Instance().dateTimeType(field.type))
			{
				if (!hasdata)
					_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				else
					_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				hasdata = true;
			}
			else
			{
				if (!hasdata)
					_writeStrArg(0, file, "`%s`", field.name.c_str());
				else
					_writeStrArg(0, file, ",`%s`", field.name.c_str());
				hasdata = true;
			}
		}

		//array member.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				if (ParseConfig::Instance().dateTimeType(s.arrayInfos[i].info.type))
				{
					if (!hasdata)
						_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					hasdata = true;
				}
				else
				{
					if (!hasdata)
						_writeStrArg(0, file, "`%s`", data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",`%s`", data.belongeField[n].c_str());
					hasdata = true;
				}
			}
		}

		if (hasdata)
		{
			// 有数据;
			_writeStrArg(0, file, " FROM %s \";\n", s.name.c_str());
			if (!usePlayerID)
				_writeStr(nTabIndex, file, "s.format_append(\"WHERE `id`=%%ld\", m_id);\n");
			else
				_writeStr(nTabIndex, file, "s.format_append(\"WHERE `playerID`=%%ld\", _playerDBID);\n");
			_writeStr(nTabIndex, file, "executeSqlInterface(db, DBResult_DispatchTo<TClass>, userData, func, s.c_str());\n");
		}
		else
		{
			assert(false);
			_writeStr(nTabIndex, file, "Assert(false);");
		}

		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;
	return 0;
}

int GenerateCpp::_generateSelectDBHandler(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "template<typename T>\n");
	_writeStr(nTabIndex, file, "static void DBResult_DispatchTo(kit::Result& r)\n");
	{
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;

			// add callback.
			_writeStr(nTabIndex, file, "T* obj = T::playerIDToObj(r.getUserData().value);\n");
			_writeStr(nTabIndex, file, "if (obj == NULL)\n");
			_writeStr(nTabIndex, file, "{\n");
			{
				++nTabIndex;

				_writeStrArg(nTabIndex, file, "LOG_ERROR(\"[%s] playerIDToObjIs NULL! \"); \n", s.name.c_str());
				_writeStr(nTabIndex, file, "return;\n");
				--nTabIndex;
			}
			_writeStr(nTabIndex, file, "}\n");

			//_writeStrArg(nTabIndex, file, "obj->dbResultCallback(\"%s\",1);\n", s.name.c_str());
			_writeStr(nTabIndex, file, "\n");

			_writeStr(nTabIndex, file, "SCL_ASSERT_TRY\n");
			_writeStr(nTabIndex, file, "{\n");
			{
				nTabIndex++;
				_writeStr(nTabIndex, file, "_handlerResult<T>(r, obj, r.getClassHandler());\n");
				nTabIndex--;
			}
			_writeStr(nTabIndex, file, "}\n");

			_writeStr(nTabIndex, file, "SCL_ASSERT_CATCH\n");
			_writeStr(nTabIndex, file, "{\n");
			{
				++nTabIndex;
				// add todo into here.
				--nTabIndex;
			}
			_writeStr(nTabIndex, file, "}\n");

			--nTabIndex;
		}
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;

	return 0;
	return 0;
}

int GenerateCpp::_generateSelectProcedure(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "template<typename T>\n");
	_writeStr(nTabIndex, file, "static void DBResult_Procedure(kit::Result& r, int _playerDBID, scl::class_function _func)\n");
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		// add callback.
		_writeStr(nTabIndex, file, "T* obj = T::playerIDToObj(_playerDBID);\n");
		_writeStr(nTabIndex, file, "if (obj == NULL)\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;

			_writeStrArg(nTabIndex, file, "LOG_ERROR(\"[%s] playerIDToObjIs NULL! \"); \n", s.name.c_str());
			_writeStr(nTabIndex, file, "return;\n");
			--nTabIndex;
		}
		_writeStr(nTabIndex, file, "}\n");


		_writeStr(nTabIndex, file, "SCL_ASSERT_TRY\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			nTabIndex++;
			_writeStr(nTabIndex, file, "_handlerResult<T>(r, obj, _func);\n");
			nTabIndex--;
		}
		_writeStr(nTabIndex, file, "}\n");

		_writeStr(nTabIndex, file, "SCL_ASSERT_CATCH\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;
			// add todo into here.
			--nTabIndex;
		}
		_writeStr(nTabIndex, file, "}\n");

		// 		// add callback.
		// 		_writeStr(nTabIndex, file, "// 为了batch的handlerResult绕道而行!\n");
		// 		_writeStr(nTabIndex, file, "//T* obj = static_cast<T*>(p);\n");
		// 		_writeStr(nTabIndex, file, "//assertf(obj, \"不可能出现的情况!\");\n");
		// 		_writeStrArg(nTabIndex, file, "//obj->dbResultCallback(\"%s\",1);\n", s.name.c_str());

		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;

	return 0;
}

void GenerateCpp::_generateInitFromDBHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "void _initFromDB(mw_util::DBResult& result);\n");
	--nTabIndex;
}

int GenerateCpp::_generateInitFromDB(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::_initFromDB(mw_util::DBResult& result)\n", s.classNameEx.c_str());
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		//normal members.
		bool hasdata = false;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;

			CTypeInfo* pinfo = ParseConfig::Instance().typeInfos(field.type.c_str());
			assert(pinfo);

			if (ParseConfig::Instance().dateTimeType(field.type))
				_writeStrArg(nTabIndex, file, "m_%s = result.%s(\"%s_uint64\");\n", field.name.c_str(), pinfo->dbResultType.c_str(), field.name.c_str());
			else
				_writeStrArg(nTabIndex, file, "m_%s = result.%s(\"%s\");\n", field.name.c_str(), pinfo->dbResultType.c_str(), field.name.c_str());
		}

		//enum member.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			if (data.belongeField.size() > 0)
				_writeStrArg(nTabIndex, file, "m_%s.resize(%d);\n", data.info.name.c_str(), data.belongeField.size());

			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				CTypeInfo* pinfo = ParseConfig::Instance().typeInfos(s.arrayInfos[i].info.type.c_str());
				assert(pinfo);

				if (ParseConfig::Instance().dateTimeType(s.arrayInfos[i].info.type))
					_writeStrArg(nTabIndex, file, "m_%s[%s] = result.%s(\"%s_uint64\");\n", data.info.name.c_str(), data.belongeEnumName[n].c_str(), pinfo->dbResultType.c_str(), data.belongeField[n].c_str());
				else
					_writeStrArg(nTabIndex, file, "m_%s[%s] = result.%s(\"%s\");\n", data.info.name.c_str(), data.belongeEnumName[n].c_str(), pinfo->dbResultType.c_str(), data.belongeField[n].c_str());
			}
		}

		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;
	return 0;
}


void GenerateCpp::_generateHandlerResult(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStr(nTabIndex, file, "template<typename T>\n");
	_writeStr(nTabIndex, file, "static void _handlerResult(kit::Result& r, void * p, scl::class_function _func)\n");
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		_writeStr(nTabIndex, file, "T* obj = static_cast<T*>(p);\n");
		_writeStr(nTabIndex, file, "if (NULL == obj)\n");
		{
			_writeStr(nTabIndex, file, "{\n");
			++nTabIndex;
			_writeStrArg(nTabIndex, file, "assertf(false, \"%s DBResult_DispatchTo has a NULL user data!\");\n", s.name.c_str());
			_writeStr(nTabIndex, file, "return;\n");
			--nTabIndex;
			_writeStr(nTabIndex, file, "}\n");
		}

		_writeStr(nTabIndex, file, "\n");

		_writeStr(nTabIndex, file, "scl::class_function resultClassHandler = _func;\n");
		_writeStr(nTabIndex, file, "if (NULL == resultClassHandler)\n");
		{
			_writeStr(nTabIndex, file, "{\n");
			++nTabIndex;
			_writeStrArg(nTabIndex, file, "assertf(false, \"%s DBResult_DispatchTo has a NULL ClassHandler class_function!\");\n", s.name.c_str());
			_writeStr(nTabIndex, file, "return;\n");
			--nTabIndex;
			_writeStr(nTabIndex, file, "}\n");
		}

		_writeStr(nTabIndex, file, "\n");

		_writeStrArg(nTabIndex, file, "typename T::DBHandler_%s handler = reinterpret_cast<typename T::DBHandler_%s>(resultClassHandler);\n", s.name.c_str(), s.name.c_str());
		_writeStr(nTabIndex, file, "\n");
		_writeStrArg(nTabIndex, file, "static scl::array<DB_%s, MAX_DB_RESULT_COUNT> _array;\n", s.className.c_str());
		_writeStr(nTabIndex, file, "_array.clear();\n");
		_writeStr(nTabIndex, file, "while (r.next())\n");
		{
			_writeStr(nTabIndex, file, "{\n");
			++nTabIndex;
			_writeStrArg(nTabIndex, file, "DB_%s& _%s = _array.push_back_fast();\n", s.className.c_str(), s.name.c_str());
			_writeStrArg(nTabIndex, file, "_%s._initFromDB(r);\n", s.name.c_str());
			--nTabIndex;
			_writeStr(nTabIndex, file, "}\n");
		}
		_writeStrArg(nTabIndex, file, "(obj->*handler)(_array.c_array(), _array.size());\n", s.name.c_str());

		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;

	return;
}


void GenerateCpp::_generateSelectArrayHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "static int selectArray(mw_util::DataBase& db, DB_%s* _array, int _count);\n", s.className.c_str());
	--nTabIndex;
}

void GenerateCpp::_generateSelectArray(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "int %s::selectArray(mw_util::DataBase& db, DB_%s* _array, int _count)\n", s.classNameEx.c_str(), s.className.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		// begine.
		_writeStr(nTabIndex, file, "string4k s = \"SELECT ");

		FieldName tempPlayerID;
		//normal members.
		bool hasdata = false;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& field = s.fields[i];
			if (field.arrayGroupID != 0)
				continue;

			if (ParseConfig::Instance().dateTimeType(field.type))
			{
				if (!hasdata)
					_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				else
					_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", field.name.c_str(), field.name.c_str());
				hasdata = true;
			}
			else
			{
				if (!hasdata)
					_writeStrArg(0, file, "`%s`", field.name.c_str());
				else
					_writeStrArg(0, file, ",`%s`", field.name.c_str());
				hasdata = true;
			}
		}

		//array member.
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				if (ParseConfig::Instance().dateTimeType(s.arrayInfos[i].info.type))
				{
					if (!hasdata)
						_writeStrArg(0, file, "unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",unix_timestamp(`%s`) as %s_uint64", data.belongeField[n].c_str(), data.belongeField[n].c_str());
					hasdata = true;
				}
				else
				{
					if (!hasdata)
						_writeStrArg(0, file, "`%s`", data.belongeField[n].c_str());
					else
						_writeStrArg(0, file, ",`%s`", data.belongeField[n].c_str());
					hasdata = true;
				}
			}
		}

		if (hasdata)
		{
			// 有数据;
			_writeStrArg(0, file, " FROM %s ;\";\n", s.name.c_str());

			_writeStr(nTabIndex, file, "mw_util::DBResult* r = db.execute(s.c_str(), s.length());\n");
			_writeStr(nTabIndex, file, "if (r==NULL) return 0;\n");
			_writeStr(nTabIndex, file, "int _counter__ = 0;\n");
			_writeStr(nTabIndex, file, "while(r->next())\n");
			_writeStr(nTabIndex, file, "{\n");
			{
				++nTabIndex;
				_writeStr(nTabIndex, file, "if (_counter__ >= _count) break;\n");
				_writeStrArg(nTabIndex, file, "_array[_counter__]._initFromDB(*r);\n");
				_writeStr(nTabIndex, file, "_counter__++;\n");

				--nTabIndex;
			}
			_writeStr(nTabIndex, file, "}\n");
			_writeStr(nTabIndex, file, "return _counter__;\n");
		}
		else
		{
			assert(false);
			_writeStr(nTabIndex, file, "Assert(false);");
		}
		--nTabIndex;
	}

	//_writeStr(nTabIndex, file, "return 0;\n");
	_writeStr(nTabIndex, file, "}\n");
	--nTabIndex;
}

void GenerateCpp::_generateInsertArrayHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "static void insertArray(mw_util::DataBase& db, DB_%s* _array, int _count);\n", s.className.c_str());
	--nTabIndex;
}

int GenerateCpp::_generateInsertArray(const DBTable& s, FILE* file, int nTabIndex)
{
	// 多个insert组合.
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::insertArray(mw_util::DataBase& db, DB_%s* _array, int _count)\n", s.classNameEx.c_str(), s.className.c_str());
	_writeStr(nTabIndex, file, "{\n");
	{
		++nTabIndex;
		// for check id is dirty.
		_writeStr(nTabIndex, file, "if (_count <= 0)\n");
		_writeStr(nTabIndex+1, file, "return;\n");
		_writeStrArg(nTabIndex, file, "for (int n = 0; n < _count; ++n)\n");
		{
			_writeStrArg(nTabIndex, file, "{\n");
			++nTabIndex;
			{
				_writeStr(nTabIndex, file, "if (!_array[n].m_normal_bit_set[NORMAL_ATTR_INDEX_ID])\n");
				_writeStr(nTabIndex, file, "{\n");
				{
					_writeStr(nTabIndex + 1, file, "Assert(false && \"必须设置id，否者更新what?\");\n");
					_writeStr(nTabIndex + 1, file, "return;\n");
				}
				_writeStr(nTabIndex, file, "}\n");
			}
			--nTabIndex;
			_writeStrArg(nTabIndex, file, "}\n");
		}
		// end check id is dirty.

		// 1. first generate insert into `table` (`column`, 'column'...);
		//INSERT INTO users(name, age)
		//VALUES('姚明', 25), ('比尔.盖茨', 50), ('火星人', 600);
		_writeStr(nTabIndex, file, "// 这里操作所有的列,所以外部必须设置所有的列!\n");
		_writeStr(nTabIndex, file, "bool hasData = false;\n");
		_writeStrArg(nTabIndex, file, "mw_base::string<64*1024> s = \"INSERT INTO `%s`(\";\n", s.name.c_str());
		bool bFirst = true;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& f = s.fields[i];
			if (f.arrayGroupID != 0)
				continue;

			_writeStrArg(nTabIndex, file, "if (_array[0].m_normal_bit_set[NORMAL_ATTR_INDEX_%s])", f.upperNames.c_str());
			_writeStr(nTabIndex, file, "{");

			if (bFirst)
				_writeStrArg(1, file, "s += (\"`%s`\");", f.name.c_str());
			else
				_writeStrArg(1, file, "s += (\",`%s`\");", f.name.c_str());

			_writeStr(1, file, "hasData = true;");
			_writeStr(nTabIndex, file, "}\n");

			bFirst = false;
		}

		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				_writeStrArg(nTabIndex, file, "if (_array[0].m_enum_%s_bit_set[%s])", data.info.name.c_str(), data.belongeEnumName[n].c_str());
				_writeStr(0, file, "{");

				if (bFirst)
					_writeStrArg(0, file, "s += (\"`%s`\");", data.belongeField[n].c_str());
				else
					_writeStrArg(0, file, "s += (\",`%s`\");", data.belongeField[n].c_str());
				
				_writeStr(1, file, "hasData = true;");
				_writeStr(0, file, "}\n");

				bFirst = false;
			}
		}

		_writeStr(nTabIndex, file, "if(!hasData)\n");
		_writeStr(nTabIndex + 1, file, "return;\n");
		_writeStr(nTabIndex, file, "s += (\") VALUES \");\n");


		// 2. then values.
		_writeStr(nTabIndex, file, "bool firstValue=true;\n");
		_writeStr(nTabIndex, file, "for (int i = 0; i <_count; ++i)\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;
			_writeStr(nTabIndex, file, "ValueSerializer sqlSerializer;\n");

			for (int i = 0; i < s.fields.size(); ++i)
			{
				const DBField& f = s.fields[i];
				if (f.arrayGroupID != 0)
					continue;

				_writeStrArg(nTabIndex, file, "if (_array[0].m_normal_bit_set[NORMAL_ATTR_INDEX_%s])", f.upperNames.c_str());
				_writeStr(nTabIndex, file, "{");
				if (ParseConfig::Instance().dateTimeType(f.type))
					_writeStrArg(nTabIndex, file, "sqlSerializer.pushDatetimeValue(_array[i].m_%s);", f.name.c_str());
				else
					_writeStrArg(nTabIndex, file, "sqlSerializer << _array[i].m_%s;", f.name.c_str());
				_writeStr(nTabIndex, file, "}\n");
			}

			for (int i = 0; i < s.arrayInfos.size(); ++i)
			{
				const DBTable::ArrayData& data = s.arrayInfos[i];
				for (int n = 0; n < data.belongeField.size(); ++n)
				{

					_writeStrArg(nTabIndex, file, "if (_array[0].m_enum_%s_bit_set[%s])", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					_writeStr(nTabIndex, file, "{");
					if (ParseConfig::Instance().dateTimeType(data.info.type))
						_writeStrArg(nTabIndex, file, "sqlSerializer.pushDatetimeValue(_array[i].m_%s[%s]);", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					else
						_writeStrArg(nTabIndex, file, "sqlSerializer << _array[i].m_%s[%s];", data.info.name.c_str(), data.belongeEnumName[n].c_str());
					_writeStr(nTabIndex, file, "}\n");
				}
			}
			_writeStr(nTabIndex, file, "\n");
			_writeStr(nTabIndex, file, "if (firstValue)\n");
			_writeStr(nTabIndex + 1, file, "s += \"(\";\n");
			_writeStr(nTabIndex, file, "else\n");
			_writeStr(nTabIndex + 1, file, "s += \",(\";\n");
			_writeStr(nTabIndex, file, "s += sqlSerializer.GetValueString().c_str();\n");
			_writeStr(nTabIndex, file, "s += \")\";\n");
			_writeStr(nTabIndex, file, "firstValue=false;\n");

			--nTabIndex;
		}

		_writeStr(nTabIndex, file, "}\n");
		_writeStr(nTabIndex, file, "s.append(';');\n");
		_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");
		--nTabIndex;
	}

	_writeStr(nTabIndex, file, "}\n");
	--nTabIndex;

	return 0;
}

void GenerateCpp::_generateUpdateArrayHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "static void updateArray(mw_util::DataBase& db, DB_%s* _array, int _count);\n", s.className.c_str());
	--nTabIndex;
}

int GenerateCpp::_generateUpdateArray(const DBTable& s, FILE* file, int nTabIndex)
{
	// 这里就可以用set '' case '' in (...)
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::updateArray(mw_util::DataBase& db, DB_%s* _array, int _count)\n", s.classNameEx.c_str(), s.className.c_str());
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;

		// for check id is dirty.
		_writeStrArg(nTabIndex, file, "for (int n = 0; n < _count; ++n)\n");
		{
			_writeStrArg(nTabIndex, file, "{\n");
			++nTabIndex;
			{
				_writeStr(nTabIndex, file, "if (!_array[n].m_normal_bit_set[NORMAL_ATTR_INDEX_ID])\n");
				_writeStr(nTabIndex, file, "{\n");
				{
					_writeStr(nTabIndex + 1, file, "Assert(false && \"必须设置id，否者更新what?\");\n");
					_writeStr(nTabIndex + 1, file, "return;\n");
				}
				_writeStr(nTabIndex, file, "}\n");
			}
			--nTabIndex;
			_writeStrArg(nTabIndex, file, "}\n");
		}
		// end check id is dirty.

		_writeStr(nTabIndex, file, "// 这里操作所有列，所以外部必须设置所有列吧!\n");
		_writeStrArg(nTabIndex, file, "mw_base::string<64*1024> s = \"UPDATE `%s` SET \";\n", s.name.c_str());
		_writeStr(nTabIndex, file, "bool bFirst = true;\n");
		_writeStr(nTabIndex, file, "string64k fieldValues;\n");
		_writeStr(nTabIndex, file, "bool hasData = false;\n");
		_writeStr(nTabIndex, file, "ValueSerializer sqlSerializer;\n");

		bool bFirst = true;
		for (int i = 0; i < s.fields.size(); ++i)
		{
			const DBField& f = s.fields[i];

			// 数组的稍后操作.
			if (f.arrayGroupID != 0)
				continue;

			//屏蔽掉id列.
			if (_compare(f.name, "id", true))
				continue;

			if (!bFirst)
			{
				_writeStr(nTabIndex, file, "fieldValues.clear();\n");
				_writeStr(nTabIndex, file, "hasData = false;\n");
			}
			bFirst = false;

			_writeStrArg(nTabIndex, file, "for (int n = 0; n < _count; ++n)\n");
			{
				_writeStrArg(nTabIndex, file, "{\n");
				++nTabIndex;
				{
					_writeStrArg(nTabIndex, file, "if (!_array[n].m_normal_bit_set[NORMAL_ATTR_INDEX_%s])\n", f.upperNames.c_str());
					_writeStr(nTabIndex + 1, file, "continue;\n");
					_writeStr(nTabIndex, file, "sqlSerializer.clear();\n");

					if (ParseConfig::Instance().dateTimeType(f.type))
						_writeStrArg(nTabIndex, file, "sqlSerializer.pushDatetimeValue(_array[n].m_%s);\n", f.name.c_str());
					else
						_writeStrArg(nTabIndex, file, "sqlSerializer << _array[n].m_%s;\n", f.name.c_str());

					// append.


					_writeStr(nTabIndex, file, "fieldValues.format_append(\"WHEN %%lld THEN %%s \", _array[n].m_id, sqlSerializer.GetValueString().c_str());\n");
					_writeStr(nTabIndex, file, "hasData = true;\n");
				}
				--nTabIndex;
				_writeStrArg(nTabIndex, file, "}\n");
			}

			_writeStr(nTabIndex, file, "if (hasData)\n");
			_writeStr(nTabIndex, file, "{\n");
			{
				++nTabIndex;
				_writeStr(nTabIndex, file, "if (bFirst)\n");
				{
					_writeStrArg(nTabIndex + 1, file, "s.append(\"`%s` = CASE `id` \");\n", f.name.c_str());
				}
				_writeStr(nTabIndex, file, "else\n");
				{
					_writeStrArg(nTabIndex + 1, file, "s.append(\"END, `%s` = CASE `id` \");\n", f.name.c_str());
				}
				_writeStrArg(nTabIndex, file, "s += fieldValues.c_str();\n");
				_writeStrArg(nTabIndex, file, "bFirst = false;\n");
				--nTabIndex;
			}
			_writeStr(nTabIndex, file, "}\n");
			_writeStr(nTabIndex, file, "\n");
		}

		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			const DBTable::ArrayData& data = s.arrayInfos[i];
			for (int n = 0; n < data.belongeField.size(); ++n)
			{
				if (!bFirst)
				{
					_writeStr(nTabIndex, file, "fieldValues.clear();\n");
					_writeStr(nTabIndex, file, "hasData = false;\n");
				}
				bFirst = false;

				_writeStrArg(nTabIndex, file, "for (int n = 0; n < _count; ++n)\n");
				{
					_writeStrArg(nTabIndex, file, "{\n");
					++nTabIndex;
					{
						_writeStrArg(nTabIndex, file, "if (!_array[n].m_enum_%s_bit_set[%s])\n", data.info.name.c_str(), data.belongeEnumName[n].c_str());
						_writeStr(nTabIndex + 1, file, "continue;\n");
						_writeStr(nTabIndex, file, "sqlSerializer.clear();\n");

						if (ParseConfig::Instance().dateTimeType(data.info.type))
							_writeStrArg(nTabIndex, file, "sqlSerializer.pushDatetimeValue(_array[n].m_%s[%s]);", data.info.name.c_str(), data.belongeEnumName[n].c_str());
						else
							_writeStrArg(nTabIndex, file, "sqlSerializer << _array[n].m_%s[%s];\n", data.info.name.c_str(), data.belongeEnumName[n].c_str());

						_writeStr(nTabIndex, file, "fieldValues.format_append(\"WHEN %%lld THEN %%s \", _array[n].m_id, sqlSerializer.GetValueString().c_str());\n");
						_writeStr(nTabIndex, file, "hasData = true;\n");
					}
					--nTabIndex;
					_writeStrArg(nTabIndex, file, "}\n");
				}

				_writeStr(nTabIndex, file, "if (hasData)\n");
				_writeStr(nTabIndex, file, "{\n");
				{
					++nTabIndex;
					_writeStr(nTabIndex, file, "if (bFirst)\n");
					{
						_writeStrArg(nTabIndex + 1, file, "s.append(\"`%s` = CASE `id` \");\n", data.belongeField[n].c_str());
					}
					_writeStr(nTabIndex, file, "else\n");
					{
						_writeStrArg(nTabIndex + 1, file, "s.append(\"END, `%s` = CASE `id` \");\n", data.belongeField[n].c_str());
					}
					_writeStrArg(nTabIndex, file, "s += fieldValues.c_str();\n");
					_writeStrArg(nTabIndex, file, "bFirst = false;\n");
					--nTabIndex;
				}
				_writeStr(nTabIndex, file, "}\n");
				_writeStr(nTabIndex, file, "\n");
			}
		}

		_writeStr(nTabIndex, file, "if (!bFirst)\n");
		_writeStr(nTabIndex, file, "{\n");
		{
			++nTabIndex;
			_writeStr(nTabIndex, file, "s.append(\"END WHERE `id` in (\");\n");
			_writeStr(nTabIndex, file, "for (int n = 0; n < _count; ++n)\n");
			_writeStr(nTabIndex + 1, file, "s.format_append(\"%%lld,\", _array[n].m_id);\n");
			_writeStr(nTabIndex, file, "s.substract(1);\n");
			_writeStr(nTabIndex, file, "s.append(\");\");\n");
			_writeStr(nTabIndex, file, "\n");

			_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");
			_writeStr(nTabIndex, file, "return;\n");
			--nTabIndex;
		}
		_writeStr(nTabIndex, file, "}\n");
		_writeStr(nTabIndex, file, "// 代表没有数据更新 ...\n");
		_writeStr(nTabIndex, file, "Assert(false);\n");
		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	return 0;
}

void GenerateCpp::_generateDeleteArrayHead(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "static void eraseArray(mw_util::DataBase& db, DB_%s* _array, int _count);\n", s.className.c_str());
	--nTabIndex;
}

int GenerateCpp::_generateDeleteArray(const DBTable& s, FILE* file, int nTabIndex)
{
	// delte组合.
	++nTabIndex;
	_writeStrArg(nTabIndex, file, "void %s::eraseArray(mw_util::DataBase& db, DB_%s* _array, int _count)\n", s.classNameEx.c_str(), s.className.c_str());
	{
		_writeStr(nTabIndex, file, "{\n");
		++nTabIndex;
		_writeStr(nTabIndex, file, "bool hasData = false;\n");
		_writeStrArg(nTabIndex, file, "mw_base::string4k s = \"DELETE FROM `%s` WHERE `id` in (\";\n", s.name.c_str());
		_writeStr(nTabIndex, file, "for (int i = 0; i < _count; ++i)\n");
		{
			_writeStr(nTabIndex, file, "{\n");
			++nTabIndex;
			_writeStr(nTabIndex, file, "s.format_append(\"%%lld,\", _array[i].m_id);\n");
			_writeStr(nTabIndex, file, "hasData = true;\n");
			--nTabIndex;
			_writeStr(nTabIndex, file, "}\n");
		}

		_writeStr(nTabIndex, file, "if (hasData)\n");
		{
			_writeStr(nTabIndex, file, "{\n");
			++nTabIndex;
			_writeStr(nTabIndex, file, "s.substract(1);\n");
			_writeStr(nTabIndex, file, "s += \");\";\n");
			_writeStr(nTabIndex, file, "executeSqlInterface(db, NULL, NULL, NULL, s.c_str());\n");
			--nTabIndex;
			_writeStr(nTabIndex, file, "}\n");
		}
		--nTabIndex;
		_writeStr(nTabIndex, file, "}\n");
	}
	--nTabIndex;
	return 0;
}

void GenerateCpp::_generateThisName(const DBTable& s, FILE* file, int nTabIndex)
{
	++nTabIndex;
	FieldName uppername = s.name;
	toUpper(uppername, 0);
	_writeStrArg(nTabIndex, file, "const char* const _thisName() {return \"%s\";}\n", uppername.c_str());
	--nTabIndex;
}

void GenerateCpp::_collectFieldsMaxLength(const DBTable& t)
{
	m_memberTypeMaxLength = 0;
	m_fuctionTypeMaxLength = 0;
	m_memberNameMaxLength = 0;
	m_functionNameMaxLength = 0;

	int nfields = t.fields.size();
	int normalCounter = 0;
	for (int i = 0; i < nfields; ++i)
	{
		const DBField& f = t.fields[i];
		if (f.arrayGroupID == 0)
		{

			std::string s;
			{// member type;
				s.clear();
				if (ParseConfig::Instance().stringType(f.type))
					format_append(s, "mw_base::string<%d>", f.arraySize + 1);
				else
					format_append(s, "%s", f.cType.c_str());
				if (m_memberTypeMaxLength < s.length())
					m_memberTypeMaxLength = s.length();
			}

			{// member name
				s.clear();
				format_append(s, "m_%s;", f.name.c_str());
				if (m_memberNameMaxLength < s.length())
					m_memberNameMaxLength = s.length();
			}

			{ // function type
				s.clear();
				format_append(s, "%s", f.cType.c_str());
				if (m_fuctionTypeMaxLength < s.length())
					m_fuctionTypeMaxLength = s.length();

				s.clear();
				s =  "void";
				if (m_fuctionTypeMaxLength < s.length())
					m_fuctionTypeMaxLength = s.length();
			}

			{ // function name.
				FieldName firstCharUpper = f.name;
				toUpper(firstCharUpper, 0);
				s.clear();
				format_append(s, "set%s", firstCharUpper.c_str());
				if (m_functionNameMaxLength < s.length())
					m_functionNameMaxLength = s.length();
			}

			normalCounter++;
		}
	}

	for (int i = 0; i < t.arrayInfos.size(); ++i)
	{
		const DBTable::ArrayData& data = t.arrayInfos[i];
		ArrayName firstCharUpper = data.info.name;
		toUpper(firstCharUpper, 0);

		std::string s;
		{// member type;
			s.clear();
			if (ParseConfig::Instance().stringType(data.info.type))
				format_append(s, "mw_base::array<mw_base::string<%d>, %d>", data.info.stringTypeSize + 1, data.belongeField.size());
			else
				format_append(s, "mw_base::array<%s, %d>", data.info.cType.c_str(), data.belongeField.size());
			if (m_memberTypeMaxLength < s.length())
				m_memberTypeMaxLength = s.length();
		}

		{// member name
			s.clear();
			format_append(s, "m_%s", data.info.name.c_str());
			if (m_memberNameMaxLength < s.length())
				m_memberNameMaxLength = s.length();
		}

		{ // function type
			s.clear();
			format_append(s, "%s", data.info.cType.c_str());
			if (m_fuctionTypeMaxLength < s.length())
				m_fuctionTypeMaxLength = s.length();

			s.clear();
			s=("void");
			if (m_fuctionTypeMaxLength < s.length())
				m_fuctionTypeMaxLength = s.length();
		}

		{ // function name.
			format_append(s, "set%s", firstCharUpper.c_str());
			if (m_functionNameMaxLength < s.length())
				m_functionNameMaxLength = s.length();
		}
	}


	// bitset.
	std::stringstream ss;
	ss << "mw_base::Bitset<" << normalCounter << ">";
	std::string stype = ss.str();
	if (m_memberTypeMaxLength < stype.length())
		m_memberTypeMaxLength = stype.length();

	std::string sname = "m_normal_bit_set;";
	if (m_memberNameMaxLength < sname.length())
		m_memberNameMaxLength = sname.length();

	for (int i = 0; i < t.arrayInfos.size(); ++i)
	{
		const DBTable::ArrayData& data = t.arrayInfos[i];

		stype.clear();
		//stype.format_append("mw_base::Bitset<%d>", data.belongeField.size());
		std::stringstream ss;
		ss << "mw_base::Bitset<" << data.belongeField.size() << ">";
		stype = ss.str();
		if (m_memberTypeMaxLength < stype.length())
			m_memberTypeMaxLength = stype.length();

		sname.clear();
		std::stringstream ss1111;
		ss1111 << "m_enum_" << data.info.name.c_str() << "_bit_set;";
		//sname.format("m_enum_%s_bit_set;", data.info.name.c_str());
		sname = ss1111.str();
		if (m_memberNameMaxLength < sname.length())
			m_memberNameMaxLength = sname.length();
	}
}

int GenerateCpp::_needTabCount(NEED_TAB_INDEX type, const char* const str, int extraLen)
{
	// 	a	;
	// 	aa	;
	// 	aaa	;
	// 	aaaa	;
	// 	aaaaa	;
	// 	aaaaaa	;
	if (!str)
	{
		assert(false);
		return 0;
	}

	int strLen = strlen(str);
	strLen += extraLen;
	if (strLen < 0)
	{
		assert(false);
		return 0;
	}

	switch (type)
	{
	case GenerateCpp::NEED_TAB_INDEX_INVALID:
		{
			assert(false);
		}
		break;
	case GenerateCpp::NEED_TAB_INDEX_MEMBER_TYPE:
		{
			int ntab = (m_memberTypeMaxLength) / 4;
			ntab++;

			int tab = (strLen) / 4;
			return ntab - tab;
		}
		break;
	case GenerateCpp::NEED_TAB_INDEX_MEMBER_NAME:
		{
			int ntab = (m_memberNameMaxLength) / 4;
			ntab++;

			int tab = (strLen) / 4;
			return ntab - tab;
		}
		break;
	case GenerateCpp::NEED_TAB_INDEX_FUNCTION_TYPE:
		{
			int ntab = (m_fuctionTypeMaxLength) / 4;
			ntab++;

			int tab = (strLen) / 4;
			return ntab - tab;

		}
		break;
	case GenerateCpp::NEED_TAB_INDEX_FUNCIOTN_NAME:
		{
			int ntab = (m_functionNameMaxLength) / 4;
			ntab++;

			int tab = (strLen) / 4;
			return ntab - tab;

		}
		break;
	case GenerateCpp::NEED_TAB_INDEX_COUNT:
		{ 
			assert(false);
		}
		break;
	default:
		break;
	}
	assert(false);

	return 0;
}

//////////////////////////////////////////////////////////////////////////



static std::string& replace(std::string& str, const std::string oldValue, const char* const newValue)
{
	assert((oldValue.empty()==false) || newValue);

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
	if ((pos == -1) && (str.length()>0))
	{
		stemp += str;
	}

	str = stemp;
	return str;
}


GenerateBatch::GenerateBatch() 
{
	m_templateString.clear();
	m_generateString.clear();
	m_AddInVCToolPath.clear();
}


GenerateBatch::~GenerateBatch()
{
	m_templateString.clear();
	m_generateString.clear();
	m_AddInVCToolPath.clear();
}

bool GenerateBatch::generate(DataCenter& Adata)
{
	FILE* ftemplate = fopen("batch.txt", "rb");
	if (!ftemplate)
	{
	//	assertf(false, "batch打开模板文件失败。。。。。batch.txt文件");
		return false;
	}

	fseek(ftemplate, 0, SEEK_END);
	long templateSize = ftell(ftemplate);
	fseek(ftemplate, 0, SEEK_SET);
	char * pData = new char[templateSize + 1];
	if (!pData)
	{
	//	assertf(false, "读batch.txt文件 new char[%d] 失败！！！！内存不足??", templateSize+1);
		return false;
	}

	fread(pData, templateSize, 1, ftemplate);
	pData[templateSize] = 0;
	fclose(ftemplate);

	m_templateString = pData;

	delete[] pData;
	pData = NULL;

	m_currentDataCenter = &Adata;
	m_batchCppPath = XMLDBApp::Instance().getParam().cppOutPath;
	m_batchSqlPath = XMLDBApp::Instance().getParam().sqlOutPath;
	m_AddInVCToolPath = XMLDBApp::Instance().getParam().workPath;

	_processTemplate(Adata.m_batchs);
	_generateBatchSql(Adata.m_batchs);
	return true;
}


bool GenerateBatch::_processTemplate(DBBatchArray& _array)
{
	int beginIncludePos = m_templateString.find("$BEGIN_INCLUDE$");
	int endIncludePos = m_templateString.find("$END_INCLUDE$");

	// 开始写#include "" 信息
	m_generateString += m_templateString.substr(0, beginIncludePos);
	m_templateString = m_templateString.substr(endIncludePos+strlen("$END_INCLUDE$"));

	std::string streamInclude;
	for (int i = 0; i < _array.size(); ++i)
	{
		const DBBatch& batch = _array[i];
		for (int n = 0; n < batch._children.size(); ++n)
		{
			const TableName& name = batch._children[n]._name;

			char szInclude[512] = { 0 };
			sprintf(szInclude, "#include \"db_%s.h\"\n", name.c_str());
			streamInclude += szInclude;
		}
	}
	m_generateString += streamInclude;

	int beinClassPos = m_templateString.find("$BEGIN_CLASS$");

	m_generateString += m_templateString.substr(0, beinClassPos);

	m_templateString = m_templateString.substr(beinClassPos + strlen("$BEGIN_CLASS$"));
	int endClassPos = m_templateString.find("$END_CLASS$");

	std::string fileend = m_templateString.substr(endClassPos + strlen("$END_CLASS$"));
	m_templateString = m_templateString.substr(0, endClassPos);	

	int fileEndPos = fileend.find("$file_end$");
	fileend = fileend.substr(0, fileEndPos);

	std::string classStrings;
	for (int i = 0; i < _array.size(); ++i)
	{
		const DBBatch& batch = _array[i];
		std::string strClass = m_templateString;
		strClass = replace(strClass, "$className$", batch.name.c_str());

		// context.
		int beginContext = strClass.find("");
		int endContext = strClass.find("");


		int beginHandler = strClass.find("$BEGIN_HANDLER$");
		int endHandler = strClass.find("$END_HANDLER$");
		int middleSize = endHandler - (beginHandler + strlen("$BEGIN_HANDLER$"));
		std::string midString = strClass.substr(beginHandler + strlen("$BEGIN_HANDLER$"), middleSize);

		endHandler = strClass.find("$END_HANDLER$");
		std::string strClassEnd = strClass.substr(endHandler+strlen("$END_HANDLER$"));
		strClass = strClass.substr(0, beginHandler);

		//老方案.
		// 		for (int n = 0; n < batch._children.size(); ++n)
		// 		{
		// 			std::string tempString = midString;
		// 			const TableName& _childName = batch._children[n]._name;
		// 			char className[256];
		// 			strncpy(className, _childName.c_str(), 256);
		// 			if (className[0] >= 'a' && className[0] <= 'z') //转小写
		// 				className[0] -= 32;
		// 
		// 			tempString = replace(tempString, "$child_name$", className);
		// 
		// 			string128 iterString;
		// 			iterString.from_int(n);
		// 			tempString = replace(tempString, "$_iterator_$", iterString.c_str());
		// 			strClass += tempString;
		// 		}


		//新方案[直接输出]
		std::string typenames	=	"";
		std::string params		=	"";
		std::string datas		=	"";
		for (int n = 0; n < batch._children.size(); ++n)
		{
			const TableName& _childName = batch._children[n]._name;

			char className[256];
			strncpy(className, _childName.c_str(), 256);
			if (className[0] >= 'a' && className[0] <= 'z') //转小写
				className[0] -= 32;

			std::string sTypename = "typename T$class_name$";
			std::string sParams = "scl::class_function _$class_name$";
			std::string sData = "\t\tDBBatchHandler_RegisterInfo& info$_iterator_$ = m_registers[$_iterator_$];\n"
				"\t\tinfo$_iterator_$.playerDBID = _playerDBID;\n"
				"\t\tinfo$_iterator_$.handlerFunc = DB_$class_name$::DBResult_Procedure<T$class_name$>;\n"
				"\t\tinfo$_iterator_$.classFunc = _$class_name$; \n";

			typenames	+= replace(sTypename,	"$class_name$", className);
			params		+= replace(sParams,		"$class_name$", className);

			std::stringstream ss____;
			ss____ << n;
			std::string iterString = ss____.str();
			std::string tmpData	= replace(sData,"$_iterator_$", iterString.c_str());
			datas		+= replace(tmpData,		"$class_name$", className);

			if (n < batch._children.size()-1)
			{
				typenames	+= ", ";
				params		+= ", ";
				datas		+= "\n";
			}
		}
		std::string sfunc = "template<"+typenames+">\n";
		sfunc+="\tvoid registerCallback(int _playerDBID, ";
		sfunc+=params;
		sfunc+=")\n";
		sfunc+="\t{\n";
		sfunc+=datas;
		sfunc+="\t}\n";

		strClass += sfunc;
		//end!		


		//注释，用于日志输出context.
		std::string sContext="";
		for (int n = 0; n < batch._children.size(); ++n)
		{
			const TableName& _childName = batch._children[n]._name;
			sContext += _childName.c_str();
			sContext += ",";
		}

		classStrings += strClass;
		//classStrings += strClassEnd;
		std::string ref2 = strClassEnd;
		int has_result_begin_pos = ref2.find("$has_result_begin$");
		int has_result_end_pos = ref2.find("$has_result_end$");
		if (has_result_end_pos == -1 || has_result_begin_pos == -1)
		{
			assert(false);
		}

		int ref2NeedChangePos = has_result_begin_pos + strlen("$has_result_begin$");
		int ref2AfterPos = has_result_end_pos + strlen("$has_result_end$");
		std::string ref2Before = ref2.substr(0, has_result_begin_pos);
		std::string ref2NeedChangeString = ref2.substr(ref2NeedChangePos, has_result_end_pos - ref2NeedChangePos);
		std::string ref2After = ref2.substr(ref2AfterPos);
		std::string needReplaceString;

		// 		for (int n = 0; n < batch._params.size();++n)
		// 		{
		// 			const DBBatchParam& _param = batch._params[n];
		// 			std::string ref2Template = ref2NeedChangeString;
		// 			ref2Template = replace(ref2Template, "$has_field$", _param._tableName.c_str());
		// 			needReplaceString += ref2Template;
		// 		}

		for (int n = 0; n < batch._children.size(); ++n)
		{
			const TableName& _childName = batch._children[n]._name;

			// has_field.
			std::string ref2Template = ref2NeedChangeString;
			ref2Template = replace(ref2Template, "$has_field$", _childName.c_str());
			needReplaceString += ref2Template;
		}

		classStrings += ref2Before;
		classStrings += needReplaceString;
		classStrings += ref2After;

		char szCount[64] = { 0 };
		sprintf(szCount, "%d", batch._children.size());
		classStrings = replace(classStrings, "$arrayCount$", szCount);

		classStrings = replace(classStrings, "$context_infos$", sContext.c_str());

		m_generateString += classStrings;
	}

	m_generateString += fileend;

	std::string cppFile = m_batchCppPath.c_str();
	cppFile += ("batch.h");
	FILE* f = fopen(cppFile.c_str(), "wb");
	if (!f)
	{
		//assertf(0, "生成batch.h，写入时候打开失败");
		return false;
	}

	fprintf(f, m_generateString.c_str());
	fclose(f);

	std::string s;
	printf("vcxproj %s=====", m_AddInVCToolPath.c_str());
	format_append(s, "%svcinjector.exe 	\"%sserver.vcxproj\" -file \"%s.h\" -filter \"db\"", m_AddInVCToolPath.c_str(), m_batchCppPath.c_str(), "batch");
	system(s.c_str());
	printf("vcxproj 里面添加 %s.h\n", "batch");
	//addInVCProj(m_AddInVCToolPath.c_str(), cppFile.c_str(), "batch.h");
	return true;
}

void GenerateBatch::_generateBatchSql(const DBBatchArray& _array)
{
	std::string sqlfile = m_batchSqlPath.c_str();
	sqlfile += ("batch.sql");
	FILE* f = fopen(sqlfile.c_str(), "wb");
	if (!f)
	{
		//assertf(false, "打不开batch.sql文件");
		return;
	}

	for (int i = 0; i < _array.size(); ++i)
	{
		const DBBatch& batch = _array[i];
		_generateEachBatchSql(batch, f);
	}

	fclose(f);
}

void GenerateBatch::_generateEachBatchSql(const DBBatch& b, FILE* f)
{
	if (f == NULL)
	{
		//assertf(false, "生成batch.sql的时候，文件为NULL");
		return;
	}

	int nTab = 0;	
	//_writeStr(nTab, f, "delimiter $$");
	_writeStrArg(nTab, f, "DROP PROCEDURE IF EXISTS `%s`;\n", b.name.c_str());

	if (ParseConfig::Instance().stringType(b.paramType.c_str()))
		_writeStrArg(nTab, f, "CREATE PROCEDURE `%s` (%s %s(%d)) \n", b.name.c_str(), b.param.c_str(), b.paramType.c_str(), b.paramSize);
	else
		_writeStrArg(nTab, f, "CREATE PROCEDURE `%s` (%s %s) \n", b.name.c_str(), b.param.c_str(), b.paramType.c_str());

	_writeStrArg(nTab, f, "BEGIN \n");
	++nTab;


	_writeStr(nTab, f, "\n");
	for (int i = 0; i < b._params.size(); ++i)
	{
		const DBBatchParam& _param = b._params[i];
		_writeStrArg(nTab, f, "DECLARE %s %s;\n", _param._returnName.c_str(), _param._typeName.c_str());
		//_writeStrArg(nTab, f, "SET %s = 0;",);
	}
	_writeStr(nTab, f, "\n");


	// declare ;
	for (int i = 0; i < b._children.size(); ++i)
	{
		const DBBatchChild& _child = b._children[i];
		_writeStrArg(nTab, f, "DECLARE has_%s INT;\n", _child._name.c_str());
		//_writeStrArg(nTab, f, "SET has_%s = 0;\n", _child._name.c_str());
	}

	_writeStr(nTab, f, "\n");
	for (int i = 0; i < b._params.size(); ++i)
	{
		const DBBatchParam& _param = b._params[i];
		_writeStrArg(nTab, f, "SET %s = -1;\n",_param._returnName.c_str());
	}
	_writeStr(nTab, f, "\n");

	// SET.
	for (int i = 0; i < b._children.size(); ++i)
	{
		const DBBatchChild& _child = b._children[i];
		_writeStrArg(nTab, f, "SET has_%s = 0;\n", _child._name.c_str());
	}

	// select params.
	for (int i = 0; i < b._params.size(); ++i)
	{
		const DBBatchParam& _param = b._params[i];	
		_writeStrArg(nTab, f, "SELECT `%s` INTO `%s` FROM `%s` WHERE `%s` = `%s`;\n",
			_param._returnField.c_str(), _param._returnName.c_str(), _param._tableName.c_str(), _param._fieldName.c_str(), _param._paramName.c_str());
	}	

	// select tables;
	_writeStr(nTab, f, "if (_id > -1) then\n");
	nTab++;
	for (int i = 0; i < b._children.size(); ++i)
	{
		const DBBatchChild& _child = b._children[i];
		_writeStrArg(nTab, f, "SELECT COUNT(`id`) INTO has_%s FROM `%s` WHERE `%s` = _id;\n", _child._name.c_str(), _child._name.c_str(), _child._playerIDName.c_str());
	}
	--nTab;
	_writeStr(nTab, f, "end if;\n\n");

	std::string sSql = "SELECT ";
	for (int i = 0; i < b._children.size(); ++i)
	{
		const DBBatchChild& _child = b._children[i];
		if (i == b._children.size() - 1)
			format_append(sSql, "has_%s", _child._name.c_str());
		else
			format_append(sSql, "has_%s,", _child._name.c_str());
	}
	sSql += (";\n");
	_writeStrArg(nTab, f, sSql.c_str());

	_writeStr(nTab, f, "\n");

	_writeStr(nTab, f, "if (_id > -1) then\n");
	nTab++;
	for (int i = 0; i < b._children.size(); ++i)
	{
		const DBBatchChild& _child = b._children[i];

		//LOG_DEBUG("ss " << _child._name.c_str());
		const DBTable *tb = m_currentDataCenter->getTableByNameFromCreateArray(_child._name.c_str());
		if (!tb)
		{
			//assertf(false , "没有找到DBTable【%s】", _child._name.c_str());
			continue;
		}

		std::string s;
		bool makeRet = _makeSelectTableString(*tb, s, _child._playerIDName.c_str());
		if (!makeRet)
		{
			//assertf(false, "batch 生成select的时候。查询select所有项的时候失败");
		}

		_writeStrArg(nTab, f, "if (has_%s > 0) then\n", _child._name.c_str());
		_writeStr(nTab+1, f, s.c_str());
		_writeStr(nTab, f, "end if;\n");
	}
	--nTab;
	_writeStr(nTab, f, "end if;\n");

	--nTab;
	_writeStr(nTab, f, "END; \n");
}




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void GenerateEnum::generate(const DataCenter& Adata)
{
	m_packetxmlFile = XMLDBApp::Instance().getParam().defFile;

	pugi::xml_document doc;
	bool ret = doc.load_file(m_packetxmlFile.c_str(), pugi::parse_default, pugi::encoding_auto);
	if (!ret)
	{
		assert(false);
		return;
	}

	// find db group;
	pugi::xml_node* pParent = NULL;
	for (pugi::xml_node child = doc.first_child(); child; child = child.next_sibling())
	{
		std::string nodeName = child.name();
		std::string groupName = child.attribute("name").as_string();
		if (nodeName == "group" && groupName == "db")
		{
			pParent = &child;

			// 清空;
			for (pugi::xml_node _enumChild = child.first_child(); _enumChild; _enumChild = _enumChild.next_sibling())
			{
				std::string enumName = _enumChild.name();
				child.remove_child(enumName.c_str());
			}
			break;
		}
	}

	if (!pParent)
	{
		assert(false);
		return;
	}



	int count = Adata.m_create.size();
	for (int i = 0; i < count; ++i)
	{
		const DBTable& s = Adata.m_create[i];
		for (int i = 0; i < s.arrayInfos.size(); ++i)
		{
			enumAllArray(pParent, s, s.arrayInfos[i]);
		}
	}

	doc.save_file(m_packetxmlFile.c_str(), PUGIXML_TEXT("\t"), pugi::format_default | pugi::format_save_file_text);
}

void GenerateEnum::enumAllArray(pugi::xml_node* pNode, const DBTable& t, const DBTable::ArrayData& a)
{
	pugi::xml_node enumNode = pNode->append_child("enum");
	pugi::xml_attribute enumNameAttr = enumNode.append_attribute("name");
	char uppername[1024];
	string_camel_to_all_upper(a.info.name.c_str(), uppername, 1024);
	//toUpper(uppername);
	enumNameAttr = uppername;

	for (int i = 0; i < a.belongeField.size(); ++i)
	{
		pugi::xml_node fieldNode = enumNode.append_child("field");

		pugi::xml_attribute fieldNameAttr = fieldNode.append_attribute("name");
		fieldNameAttr = a.belongeEnumName[i].c_str();

		const DBField * pField = t.getFieldData(a.belongeField[i].c_str());
		if (!pField)
		{
			assert(false && "通过数组中fieldName找field失败");
			return;
		}

		pugi::xml_attribute fieldCommentAttr = fieldNode.append_attribute("comment");
		fieldCommentAttr = pField->comment.c_str();
	}
}