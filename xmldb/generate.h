#pragma once
#include "def.h"

class GenerateSql
{
public:
	static bool generate(DataCenter& dc, int version);
private:
	static int		_generateSql			(const DBTable& s, FILE* file);

	// for generate sql.
	static void	_generateDropTableSql	(const DBTable& s, FILE* file);
	static void	_generateDropTableSql	(const TableName& s, FILE* file);
	static void	_generateCreateTableSql	(const DBTable& s, FILE* file);
	static void	_generateUpdateTableSql	(const DBTable& s, FILE* file);

	static void	_generateUpdateFieldSql	(const int nTabIndex, const DBField& s, FILE* file);

};

class GenerateCpp
{
public:
	static void generate(DataCenter & dc);
	
private:
	static bool	_generateTables		(const DBTable& t);
	static int	_generateTableHeader(const DBTable& s, FILE* file);
	static void	_generateTableCpp	(const DBTable& s, FILE* file);

	static void	_generateConstructionHead(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateConstruction(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateEnum(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateFunction(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateMember(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateInsertHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateInsertFunc(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateUpdateHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateUpdateFunc(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateDeleteHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateDeleteFunc(const DBTable& s, FILE* file, int nTabIndex);
	static void _generateReplaceHead(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateReplaceFunc(const DBTable& s, FILE* file, int nTabIndex);


	static int		_generateSelectFunc(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID);
	static void	_generateSyncSelectFuncHeader(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID);
	static void	_generateSyncSelectFuncSource(const DBTable& s, FILE* file, int nTabIndex, bool usePlayerID);

	static int		_generateSelectDBHandler(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateSelectProcedure(const DBTable& s, FILE* file, int nTabIndex);

	static void	_generateInitFromDBHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateInitFromDB(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateHandlerResult(const DBTable& s, FILE* file, int nTabIndex);

	static void	_generateSelectArrayHead(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateSelectArray(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateInsertArrayHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateInsertArray(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateUpdateArrayHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateUpdateArray(const DBTable& s, FILE* file, int nTabIndex);
	static void	_generateDeleteArrayHead(const DBTable& s, FILE* file, int nTabIndex);
	static int		_generateDeleteArray(const DBTable& s, FILE* file, int nTabIndex);

	static void _generateThisName(const DBTable& s, FILE* file, int nTabIndex);
	// for tab.[type, name]
private:
	static void	_collectFieldsMaxLength(const DBTable& t);

	enum	NEED_TAB_INDEX
	{
		NEED_TAB_INDEX_INVALID = -1,
		NEED_TAB_INDEX_MEMBER_TYPE,
		NEED_TAB_INDEX_MEMBER_NAME,
		NEED_TAB_INDEX_FUNCTION_TYPE,
		NEED_TAB_INDEX_FUNCIOTN_NAME,
		NEED_TAB_INDEX_COUNT,
	};

	static int		_needTabCount(NEED_TAB_INDEX type, const char* const str, int extraLen=0);
private:
	// for cpp tab align.
	static int					m_memberTypeMaxLength;
	static int					m_fuctionTypeMaxLength;
	static int					m_memberNameMaxLength;
	static int					m_functionNameMaxLength;
};

class GenerateBatch
{
public:
	GenerateBatch();
	~GenerateBatch();
	bool generate(DataCenter & dc);
private:
	bool	_processTemplate		(DBBatchArray& _array);

	void	_generateBatchSql		(const DBBatchArray& _array);
	void	_generateEachBatchSql	(const DBBatch& _batch, FILE* f);
private:
	DataCenter*					m_currentDataCenter;
	std::string					m_templateString;
	std::string					m_generateString;

	std::string					m_batchCppPath;
	std::string					m_batchSqlPath;
	std::string					m_AddInVCToolPath;
};



namespace pugi
{
	class xml_node;
}

class GenerateEnum
{
public:
	void generate(const DataCenter& data);

private:
	void		enumAllArray(pugi::xml_node*, const DBTable& t, const DBTable::ArrayData& a);

private:
	std::string			m_packetxmlFile;
};