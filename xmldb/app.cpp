#include "app.h"
#include "config.h"
#include "generate.h"

#include "./pugixml/pugixml.hpp"
#include <sstream>
#include <assert.h>


//from bak path, load the last file,and parse the version;
static int getOldVersion()
{
	//bak file path;
	const std::string &bak_file_path = XMLDBApp::Instance().getParam().workBakFile;

	const int kMaxVersionSize = 0xffff;
	int maxOldversion = -1;

	for (int i = 0; i < kMaxVersionSize; i++)
	{
		std::stringstream ss;
		ss << bak_file_path << "old_" << i << ".xml";
		if (!fileExists(ss.str().c_str())) {
			break;
		}
		maxOldversion=i;
	}

	return maxOldversion;
}


static int _parseField(pugi::xml_node& node, DBField& field)
{
	field.name			= node.attribute("name").as_string();
	field.name1			= node.attribute("name1").as_string();
	field.type			= node.attribute("type").as_string();
	//	field.arrayGroupID	= node.attribute("array").as_int();
	field.arrayGroupID	= 0;
	field.arraySize		= node.attribute("size").as_int();	
	field.notNull		= node.attribute("notnull").as_bool();
	field.defaultValue	= node.attribute("default").as_string();
	field.property		= static_cast<DB_PROPERTY>(node.attribute("property").as_int());

	field.autoIncrement = node.attribute("autoIncrement").as_bool();
	field.unique		= node.attribute("unique").as_bool();
	field.binary		= node.attribute("binary").as_bool();
	field.unsignedFlag	= node.attribute("unsigned").as_bool();
	field.zeroFill		= node.attribute("zeroFull").as_bool();

	field.comment		= node.attribute("comment").as_string();
	field.afterName		= node.attribute("after").as_string();

	//.
	field.upperNames	= field.name;
	toUpper(field.upperNames);

	field.notNull = true;

	std::string opcode = node.name();
	//if (opcode.compare("DROP", true) != 0)
	if (!_compare(opcode,"DROP",true))
	{
		CTypeInfo * typeInfo = ParseConfig::Instance().typeInfos(field.type.c_str());
		if (typeInfo)
		{
			if (typeInfo->defaultValue.empty())
			{	//assert(false);
			}
			if (field.defaultValue.empty())
				field.defaultValue = typeInfo->defaultValue;

			if (ParseConfig::Instance().stringType(field.type.c_str()))
			{	//assert(field.arraySize > 0);
			}

			//获得c类型以及c格式.
			field.typeFormat = typeInfo->cFormat;
			field.cType = typeInfo->cType;
		}
		else
		{
			assert(false);
		}
	}

	return 0;
}

int _parseTableArray(pugi::xml_node& node, DBTable::ArrayData& arrayData, DBTable& table)
{
	for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
	{
		// 后面都是field.但是首选要加入到array中;
		DBField field;
		int groupid = _parseField(child, field);
		field.opcode = getFieldOpcode(child.name());

		field.arrayGroupID = 1;

		// 添加入arrayData信息里面;
		if (field.opcode == FIELD_OPCODE_DROP)
		{
			// do nothing.
		}
		else
		{
			FieldName belongName = child.attribute("name").as_string();

			//if (field.opcode == FIELD_OPCODE_CHANGE)
			{
				FieldName belongName1 = child.attribute("name1").as_string();
				if (!belongName1.empty())
				{
					arrayData.belongeField.push_back(belongName1);
					belongName = belongName1;
				}
				else
					arrayData.belongeField.push_back(belongName);
			}
			// 			else
			// 				arrayData.belongedField.push_back(belongName);

			char enumName[512];
			string_camel_to_all_upper(arrayData.info.name.c_str(), enumName, 512);

			char belongFieldUpperName[512];
			string_camel_to_all_upper(belongName.c_str(), belongFieldUpperName, 512, false);

			std::string enumElemName = enumName;
			enumElemName += "_";
			enumElemName += belongFieldUpperName;

			arrayData.belongeEnumName.push_back(enumElemName.c_str());
		}

		table.fields.push_back(field);
	}
	return 0;
}

int _parseBatchChild(pugi::xml_node& node, DBBatch& batch)
{
	for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
	{
		std::string tagName = child.name();
		//if (tagName.compare("param", true) == 0)
		if (tagName == "param")
		{
			DBBatchParam bParam;// = batch._params.push_back_fast();
			bParam._tableName = child.attribute("table").as_string();
			bParam._returnField = child.attribute("retField").as_string();
			bParam._fieldName = child.attribute("field").as_string();
			bParam._paramName = child.attribute("param").as_string();
			bParam._returnName = child.attribute("result").as_string();
			bParam._typeName = child.attribute("type").as_string();
			batch._params.push_back(bParam);
		}
		//else if (tagName.compare("table", true) == 0)
		else if (tagName == "table")
		{
			DBBatchChild bChild;// = batch._children.push_back_fast();
			bChild._name = child.attribute("name").as_string();
			bChild._playerIDName = child.attribute("idName").as_string();
			bChild._className = child.attribute("class").as_string();
			bChild._classFunc = child.attribute("classFunc").as_string();
			batch._children.push_back(bChild);
		}
		else
		{
			//assert(0 && "batch 标签名字有问题");
		}
	}
	return 0;
}

int _parseArray(pugi::xml_node& child, DBArray& def)
{
	def.name = child.attribute("name").as_string();
	def.type = child.attribute("type").as_string();
	def.stringTypeSize = child.attribute("typeSize").as_int();

	def.upperNames = def.name;
	//def.upperNames.toupper();
	toUpper(def.upperNames);

	// 检查如果是数组类型，数组的大小必须大于0;
	if (ParseConfig::Instance().stringType(def.type.c_str()) && (def.stringTypeSize <= 0))
	{
		//assert(false);
		return false;
	}

	CTypeInfo* pInfo = ParseConfig::Instance().typeInfos(def.type);
	if (!pInfo)
	{
		//assert(false);
		return false;
	}

	def.cType = pInfo->cType;
	def.comment = child.attribute("comment").as_string();
	return 0;
}

static bool _parseTable(pugi::xml_node& node, DBTable& table)
{
	//assert(table.name.empty() == false);

	char className[256];
	strncpy(className, table.name.c_str(), 256);
	if (className[0] >= 'a' && className[0] <= 'z') //转小写
		className[0] -= 32;

	table.className = className;

	table.classNameEx = "DB_";
	table.classNameEx += className;

	table.arrayInfos.clear();

	bool firstField = true;
	// parse field.
	for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
	{
		std::string fieldTagname = child.name();
		if (fieldTagname == "array")
		{
			// 开始找array的child.
			DBTable::ArrayData arrayData;
			_parseArray(child, arrayData.info);
			_parseTableArray(child, arrayData, table);
			table.arrayInfos.push_back(arrayData);
		}
		else
		{
			DBField field;
			int groupid = _parseField(child, field);
			field.opcode = getFieldOpcode(child.name());
			table.fields.push_back(field);
		}

		// 确定表的第一个字段是id;
		if (firstField)
		{
			firstField = false;
			if (table.fields.size() <= 0)
			{
				//assertf(false, "Table的第一个字段必须是id");
				return 0;
			}
			const DBField& f = table.fields[0];
			//if (f.name.compare("id", false) != 0 && f.name1.compare("id", false) != 0)
			if ((f.name != "id") && (f.name1 != "id"))
			{
				//assertf(false, "Table的第一个字段必须是id");
				return 0;
			}
		}
	}

	return 0;
}

//load xml data into dataCenter;
static bool loadXml(const char * xml, bool isOldXml, DataCenter& dc)
{
	pugi::xml_document doc;
	pugi::xml_parse_result ret = doc.load_file(xml, pugi::parse_default, pugi::encoding_auto);
	if (!ret) {
		return false;
	}

	pugi::xml_node root = doc.first_child();
	std::string nodeName = root.name(); //db;

	for (pugi::xml_node child = root.first_child(); child; child = child.next_sibling()) {
		LABLE_TYPE lt = getLableType(child.name());
		
		if (LABLE_TYPE_TABLE == lt) {
			TABLE_OPCODE op = getTableOpcode(child.attribute("opcode").as_string());

			if (TABLE_OPCODE_CREATE == op) {
				DBTable t;
				t.name = child.attribute("name").as_string();
				_parseTable(child, t);
				dc.m_create.push_back(t);
			} else if (TABLE_OPCODE_UPDATE == op) {
				DBTable t;
				t.name = child.attribute("name").as_string();
				_parseTable(child, t);
				dc.m_update.push_back(t);
			} else if (TABLE_OPCODE_DELETE == op) {
				TableName tn = child.attribute("name").as_string();
				dc.m_delete.push_back(tn);
			} else {
				//cout<<;
				return false;
			}
			
		} else if (LABLE_TYPE_BATCH == lt) {
			DBBatch _batch;
			_batch.name		= child.attribute("name").as_string();
			_batch.param	= child.attribute("param").as_string();
			_batch.paramType = child.attribute("paramType").as_string();
			_batch.paramSize = child.attribute("paramSize").as_int();
			_batch.comment	= child.attribute("comment").as_string();

			if (ParseConfig::Instance().stringType(_batch.paramType.c_str()))
			{
				if (_batch.paramSize <= 0)
				{
					//assert(false && "Batch param is string,but size is <= 0");
				}
			}

			_parseBatchChild(child, _batch);
			dc.m_batchs.push_back(_batch);
		} else {
			//cout <<;;
			return false;
		}
	}
}

int _checkMysqlField(const DBTable& t, const DBField& f)
{
	if (f.name.empty())
	{
		//assert(false);
		return 1;
	}
	if (f.type.empty())
	{
		//assert(false);
		return 2;
	}

	// 1.检查每个field的 attribute 互斥关系;

	//1. 如果是主键，必须不能为空,不能为binary,
	if (f.property == DB_PROPERTY_PRIMARY_KEY)
	{
		if (f.notNull == false)
		{
			//assert(false);
			return 3;
		}
		if (f.binary)
		{
			//assert(false);
			return 4;
		}
	}

	//0. notnull:all.
	//1. unqiue:all.
	//2. binary: char,varchar, tinytext, mediumtext, text, longtext.
	//3. autoincrement: int,tinyint, smallint, MEDIUMINT,int, bigint,还有唯一.
	//4. unsigned: int,tinyint, samllint, mediumint, int, bigint, float, double.
	//5. zerofill: int,tinyint, samllint, mediumint, int, bigint, float, double.

	CTypeInfo* configInfo = ParseConfig::Instance().typeInfos(f.type.c_str());
	if (!configInfo)
	{
		//assert(0);
		return -1;
	}

	if (f.binary && (configInfo->canBinary != f.binary))
	{
		//assert(0);
		return -2;
	}

	if (f.autoIncrement && (configInfo->canAutoIncrement != f.autoIncrement))
	{
		//assert(0);
		return -3;
	}

	if (f.unsignedFlag && (configInfo->canUnsigned != f.unsignedFlag))
	{
		//assert(0);
		return -4;
	}

	if (f.zeroFill && (configInfo->canzerofill != f.zeroFill))
	{
		//assert(0);
		return -5;
	}


	// 	// 如果是add，需要有个after。至少是id后面.
	// 	if (f.opcode == FIELD_OPCODE_ADD && f.afterName.empty())
	// 	{
	// 		assert(0);
	// 		return -6;
	// 	}

	// 如果是change，需要2个名字.
	if (f.opcode == FIELD_OPCODE_CHANGE && f.name1.empty())
	{
		//assert(0);
		return -7;
	}


	return 0;
}

int _checkMysqlTableFields(const DBTable& t)
{
	//1.只能有一个主键;
	int primaryKeyCount = 0;
	int autoIncrementCount = 0;
	int fields = t.fields.size();
	for (int i = 0; i < fields; ++i)
	{
		const DBField& f = t.fields[i];
		if (f.property == DB_PROPERTY_PRIMARY_KEY)
			primaryKeyCount++;
		if (f.autoIncrement)
			autoIncrementCount++;
	}

	if (primaryKeyCount != 1)
	{
		//assert(false);
		return -99;
	}
	if (autoIncrementCount != 1 && autoIncrementCount != 0)
	{
		//assert(false);
		return -100;
	}

	for (int i = 0; i < fields; ++i)
	{
		const DBField& f = t.fields[i];

		int ret = _checkMysqlField(t, f);
		if (0 != ret)
			return ret;
	}
	return 0;
}

void _checkMysql(DataCenter& AData)
{
	int count = AData.m_create.size();
	for (int i = 0; i < count; ++i)
	{
		int err = _checkMysqlTableFields(AData.m_create[i]);
		if (err != 0)
		{
			//assert(false);
			return;
		}
	}

	// 	count = AData.m_update.size();
	// 	for (int i = 0; i < count; ++i)
	// 	{
	// 		int err = _checkMysqlTableFields(AData.m_update[i]);
	// 		if (err != 0)
	// 		{
	// 			assert(false);
	// 			return;
	// 		}
	// 	}
}

void saveOldXml(const char* filePathNew, const char* filePathOld)
{
	std::string oldBak = XMLDBApp::Instance().getParam().workBakFile;
	oldBak += "oldBak.xml";
	remove(oldBak.c_str());
	rename(filePathOld, oldBak.c_str());

	// for copyfile new.xml -> newTmp.xml
	std::string newTempXml = XMLDBApp::Instance().getParam().workPath;
	newTempXml += "newTmp.xml";
	remove(newTempXml.c_str());

	FILE* r = fopen(filePathNew, "r+");
	if (!r)
	{
		//assertf(false, "fopen new.xml failed!");
		return;
	}
	fseek(r, 0, SEEK_END);
	int size = ftell(r);
	fseek(r, 0, SEEK_SET);
	char* pData = new char[size + 1];
	fread(pData, size, 1, r);
	pData[size] = 0;
	fclose(r);

	FILE* w = fopen(newTempXml.c_str(), "w+");
	if (!w)
	{
//		assertf(false, "fopen newTmpXml.xml failed!");
		return;
	}
	fwrite(pData, size, 1, w);
	fclose(w);

	// rename newTmp.xml->old.xml
	rename(newTempXml.c_str(), filePathOld);
}

//////////////////////////////////////////////////////////////////////////
void appendCreateField(const DBTable& t, pugi::xml_node* tNode)
{
	for (int i = 0; i < t.fields.size(); ++i)
	{
		const DBField& f = t.fields[i];
		pugi::xml_node fieldNode = tNode->append_child("field");

		fieldNode.append_attribute("name") = f.name.c_str();
		fieldNode.append_attribute("type") = f.type.c_str();

		if (ParseConfig::Instance().stringType(f.name.c_str()))
			fieldNode.append_attribute("size") = f.arraySize;

		if (f.property == DB_PROPERTY_PRIMARY_KEY)
			fieldNode.append_attribute("property") = 1;
		else if (f.property == DB_PROPERTY_INDEX_KEY)
			fieldNode.append_attribute("property") = 2;

		fieldNode.append_attribute("default") = f.defaultValue.c_str();
		fieldNode.append_attribute("comment") = f.comment.c_str();
	}
}

void appendChangeFiled(const DBField& t, pugi::xml_node* tNode)
{
	tNode->append_attribute("name") = t.name.c_str();
	if (!t.name1.empty())
		tNode->append_attribute("name1") = t.name1.c_str();
	tNode->append_attribute("type") = t.type.c_str();

	if (ParseConfig::Instance().stringType(t.type.c_str()))
		tNode->append_attribute("size") = t.arraySize;

	if (t.property == DB_PROPERTY_PRIMARY_KEY)
		tNode->append_attribute("property") = 1;
	else if (t.property == DB_PROPERTY_INDEX_KEY)
		tNode->append_attribute("property") = 2;

	tNode->append_attribute("comment") = t.comment.c_str();
}


void appendUpdateField(const DBTable& t, pugi::xml_node* tNode)
{
	for (int i = 0; i < t.fields.size(); ++i)
	{
		const DBField& f = t.fields[i];

		if (f.opcode == FIELD_OPCODE_ADD)
		{
			pugi::xml_node fieldNode = tNode->append_child("ADD");
			appendChangeFiled(f, &fieldNode);
		}
		else if (f.opcode == FIELD_OPCODE_CHANGE)
		{
			pugi::xml_node fieldNode = tNode->append_child("CHANGE");
			appendChangeFiled(f, &fieldNode);
		}
		else if (f.opcode == FIELD_OPCODE_DROP)
		{
			pugi::xml_node fieldNode = tNode->append_child("DROP");
			appendChangeFiled(f, &fieldNode);
		}
	}
}


void generateDiffXML(const char* diffPath, DataCenter & m_diffData)
{
	int addTableSize = m_diffData.m_create.size();
	int uptTableSize = m_diffData.m_update.size();
	int delTableSize = m_diffData.m_delete.size();

	if (addTableSize+uptTableSize+delTableSize <= 0)
		return;

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("db");

	for (int i = 0; i < addTableSize; ++i)
	{
		pugi::xml_node table = root.append_child("table");
		const DBTable& tb = m_diffData.m_create[i];
		pugi::xml_attribute nameAttr = table.append_attribute("name");
		nameAttr = tb.name.c_str();
		pugi::xml_attribute opcodeAttr = table.append_attribute("opcode");
		opcodeAttr = "CREATE";

		// 开始做fields;
		appendCreateField(tb, &table);
	}

	for (int i = 0; i < uptTableSize; ++i)
	{
		pugi::xml_node table = root.append_child("table");
		const DBTable& tb = m_diffData.m_update[i];
		pugi::xml_attribute nameAttr = table.append_attribute("name");
		nameAttr = tb.name.c_str();
		pugi::xml_attribute opcodeAttr = table.append_attribute("opcode");
		opcodeAttr = "UPDATE";

		appendUpdateField(tb, &table);
	}

	for (int i = 0; i < delTableSize; ++i)
	{
		pugi::xml_node table = root.append_child("table");
		const TableName& tb = m_diffData.m_delete[i];
		pugi::xml_attribute nameAttr = table.append_attribute("name");
		nameAttr = tb.c_str();
		pugi::xml_attribute opcodeAttr = table.append_attribute("opcode");
		opcodeAttr = "DELETE";
	}


	doc.save_file(diffPath, PUGIXML_TEXT("\t"), pugi::format_default | pugi::format_save_file_text);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////


bool diffField(const DBTable& tOld, const DBTable& tNew, DataCenter& m_diffData)
{
	DBTable * updateTable = new DBTable;
	for (int i = 0; i < tNew.fields.size(); ++i)
	{
		const DBField& fNew = tNew.fields[i];

		bool hasDataInOld = false;
		int j = 0;
		for (; j < tOld.fields.size(); ++j)
		{
			const DBField& fOld = tOld.fields[j];
			//if (fNew.name.compare(fOld.name.c_str(), true) == 0)
			if (fNew.name == fOld.name.c_str())
			{
				hasDataInOld = true;
				break;
			}
		}

		if (!hasDataInOld)
		{
			// 要增加的;
			DBField fAdd = fNew;
			fAdd.opcode = FIELD_OPCODE_ADD;
			updateTable->fields.push_back(fAdd);
		}
		else
		{
			//assert(j != tOld.fields.size());
			const DBField& fOld = tOld.fields[j];
			bool same = compare(fOld, fNew);
			if (!same)
			{
				// 要更新的.
				//const DBField& _fOld = tOld.fields[j];
				DBField fUpdate = fNew;
				fUpdate.opcode = FIELD_OPCODE_CHANGE;
				updateTable->fields.push_back(fUpdate);
			}
			else
			{
			}
		}
	}

	// 从old里面，只要在new里面没找到那么就是要删除的;
	for (int i = 0; i < tOld.fields.size(); ++i)
	{
		const DBField& fOld = tOld.fields[i];

		int j = 0;
		bool hasInNew = false;
		for (; j < tNew.fields.size(); ++j)
		{
			const DBField& fNew = tNew.fields[j];
			//if (fOld.name.compare(fNew.name.c_str(), true) == 0)
			if (fOld.name == fNew.name.c_str())
			{
				hasInNew = true;
				break;
			}
		}

		if (!hasInNew)
		{
			// 要删除的;
			DBField fDel = fOld;
			fDel.opcode = FIELD_OPCODE_DROP;
			updateTable->fields.push_back(fDel);
		}
	}

	if (updateTable->fields.size() > 0)
	{
		updateTable->name = tOld.name;
		m_diffData.m_update.push_back(*updateTable);
		delete updateTable;
		updateTable = NULL;
		return false;
	}
	delete updateTable;
	updateTable = NULL;
	return true;
}

void diffTable(DataCenter & m_newData, DataCenter & m_oldData, DataCenter & m_diffData)
{
	for (int i = 0; i < m_newData.m_create.size(); ++i)
	{
		const DBTable& tNew = m_newData.m_create[i];
		bool hasDataInOld = false;
		int j = 0;
		for (; j < m_oldData.m_create.size(); ++j)
		{
			const DBTable& tOld = m_oldData.m_create[j];

			//if (tNew.name.compare(tOld.name.c_str(), true) == 0)
			if (tNew.name == tOld.name.c_str())
			{	
				hasDataInOld = true;
				break;
			}
		}

		if (hasDataInOld)
		{
			const DBTable& _tOld = m_oldData.m_create[j]; 
			bool sameField = diffField(_tOld, tNew, m_diffData);
			if (!sameField)
			{
				// modify,
			}
		}
		else
		{
			// ADD.
			m_diffData.m_create.push_back(tNew);
		}
	}


	// 在new中没有找到old的数据,那么就代表要drop的数据;
	for (int i = 0; i < m_oldData.m_create.size(); ++i)
	{
		const DBTable& tOld = m_oldData.m_create[i];
		bool hasDataInOld = false;
		int j = 0;
		for (j = 0; j < m_newData.m_create.size(); ++j)
		{
			const DBTable& tNew = m_newData.m_create[j];
			//if (tOld.name.compare(tNew.name.c_str(), true) == 0)
			if (tOld.name == tNew.name.c_str())
			{
				hasDataInOld = true;
				break;
			}
		}

		if (!hasDataInOld)
		{
			// 删除的.
			m_diffData.m_delete.push_back(tOld.name.c_str());
		}
	}
}


void tidyChangeNameChild(DBTable& tb)
{
	// all fields.
	for (int i = 0; i < tb.fields.size(); ++i)
	{
		DBField& f = tb.fields[i];
		if (f.name1.empty())
			continue;
		f.name = f.name1;
	}

	// parse 的时候已经 把table的belong的field 判断是否有name1如果有就用name1 or name.
	// @see: ParseDBXML::_parseTableArray
}
void tidyChangeName(DataCenter& d)
{
	// 整理后的d里面所有table的field，的name都为最新的name，
	// name1将不再作用;
	// 1.用于生成cpp,batch的时候用的都是name.
	// 2.sql的话用的是diffData，所以不需要修改;
	for (int i = 0; i < d.m_create.size(); ++i)
	{
		DBTable& tb = d.m_create[i];
		tidyChangeNameChild(tb);
	}

	for (int i = 0; i < d.m_update.size(); ++i)
	{
		DBTable& tb = d.m_update[i];
		tidyChangeNameChild(tb);
	}
}



void __generate(bool first, const char* newXml, const char* oldXml, int version, 
	DataCenter& dcNew, DataCenter& dcOld, DataCenter &dcDiff)
{
	//如果有update的情况，需要把name1-->name;
	tidyChangeName(dcNew);
	
	//生成sql语句;
// 	if (first)
// 		GenerateSql::generate(dcNew, version);
// 	else
		GenerateSql::generate(dcDiff, version);

	//生成cpp文件;
	GenerateCpp::generate(dcNew);
	
	//生成batch文件
	GenerateBatch batch;
	batch.generate(dcNew);

	//生成枚举文件
	GenerateEnum e;
	e.generate(dcNew);

	//生成old文件
	saveOldXml(newXml, oldXml);
}

void XMLDBApp::execute()
{
	//load old version,and current version;
	m_currentVersion = getOldVersion();

	//load old bak file,and current new file.
	//diff different. 
	//and save to data what is new data or remove data or modify data.
	int newVersion = m_currentVersion+1;
	std::stringstream osOld,osNew;

	osOld << XMLDBApp::Instance().getParam().workBakFile << "old_" << m_currentVersion << ".xml";
	osNew << XMLDBApp::Instance().getParam().workBakFile << "old_" << newVersion << ".xml";

	std::string newXml = XMLDBApp::Instance().getParam().workPath + "new.xml";
	std::string oldXml = osOld.str();
	std::string saveXML = osNew.str();

	DataCenter dcNew, dcOld;
	if (!loadXml(newXml.c_str(), false, dcNew))
	{
		return;
	}
	else
	{
		_checkMysql(dcNew);
	}

	if (m_currentVersion == -1) {
		//first time. 
		__generate(true, newXml.c_str(), saveXML.c_str(), 
			newVersion, dcNew, dcOld, dcNew);

		return;

	} else {
		if (!loadXml(oldXml.c_str(), true, dcOld))
		{
			return;
		}
		else
		{
			_checkMysql(dcOld);
		}

		// diff dcNew and dcOld.find which is new, or update, or delete.
		DataCenter diffData;
		diffTable(dcNew, dcOld, diffData);

		int addTableSize = diffData.m_create.size();
		int uptTableSize = diffData.m_update.size();
		int delTableSize = diffData.m_delete.size();
		if (addTableSize + uptTableSize + delTableSize <= 0)
			return;

// 		//ok. now generate diff sqls.
// 		std::stringstream ss;
// 		ss << XMLDBApp::Instance().getParam().workBakFile << "diff_" << newVersion << ".xml";
// 		std::string diffXml = ss.str();
// 		generateDiffXML(diffXml.c_str(), diffData);

		__generate(false, newXml.c_str(), saveXML.c_str(), 
			newVersion, dcNew, dcOld, diffData);
	}
}


void XMLDBApp::_addInVCProj(DataCenter& AData)
{
	// 根据当前所有的create table，获得文件名称;
	// note: -filter文件夹.
	//.execcmd(string.format("vcinjector 	\"%s/server/server.vcxproj\" -file \"packetHandler%s.cpp\" -filter \"handler\"", PROJECT_DIR, name))
	//ShellExecute(NULL, "vcinjector %s")
	//ShellExecute(NULL, "open", "calc.exe", NULL, NULL, SW_SHOWNORMAL);
	std::string m_currentFileName = getParam().workPath;
	std::string m_cppPath = getParam().cppOutPath;

	for (int i = 0; i < AData.m_create.size(); ++i)
	{
		std::string s;
		TableName& name = AData.m_create[i].name;
		format_append(s, "%svcinjector.exe 	\"%sserver.vcxproj\" -file \"db_%s.h\" -filter \"db\"", m_currentFileName.c_str(), m_cppPath.c_str(), name.c_str());
		system(s.c_str());

		std::string scpp;
		format_append(scpp, "%svcinjector.exe 	\"%sserver.vcxproj\" -file \"db_%s.cpp\" -filter \"db\"", m_currentFileName.c_str(), m_cppPath.c_str(), name.c_str());
		system(scpp.c_str());

		printf("vcxproj 里面添加 db_%s.h\n", name.c_str());
	}
}
