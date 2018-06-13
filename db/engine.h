#ifndef SBASE_DB_ENGINE_H_
#define SBASE_DB_ENGINE_H_

#include <cstring>

#include ".\util\reflection.hpp"
#include ".\db\slice.hpp" // kSchema
#include ".\storage\page_manager.h"
#include ".\storage\file_format.hpp"
#include ".\storage\page_ref.hpp"
#include ".\util\utility.hpp"
#include ".\db\cursor.h"

#include <memory> // shared_ptr
#include <vector>
#include <string>

namespace sbase{

// in database root //
// FileHandle ~ FilePath
const ::std::vector<Attribute> kDatabaseFileManifestSchemaAttr{\
	Attribute("FileHandle", tinyintT),\
	Attribute("FilePath", fixchar32T) };
const Schema kDatabaseFileManifestSchema("db_file",\
 kDatabaseFileManifestSchemaAttr.begin(), kDatabaseFileManifestSchemaAttr.end());
// TableHandle ~ TableName ~ PageHandle
const ::std::vector<Attribute> kDatabaseTableManifestSchemaAttr{\
	Attribute("TableName", fixchar32T),\
	Attribute("RootPage", uintT) };
const Schema kDatabaseTableManifestSchema("db_table",\
 kDatabaseTableManifestSchemaAttr.begin(), kDatabaseTableManifestSchemaAttr.end());
// in table root page //
// Index ~ Name ~ Type ~ Primary ~ Unique // ~ Null ~ Unique
// index means input order, used when user io
const ::std::vector<Attribute> kTableSchemaManifestSchemaAttr{\
	Attribute("Index", tinyintT),\
	Attribute("FieldName", fixchar32T ),\
	Attribute("FieldType", tinyintT),\
	Attribute("Primary", tinyintT),\
	Attribute("Unique", tinyintT) };
const Schema kTableSchemaManifestSchema("table_schema",\
 kTableSchemaManifestSchemaAttr.begin(), kTableSchemaManifestSchemaAttr.end());
 // IndexType ~ IndexPivot ~ PageHandle
const ::std::vector<Attribute> kTableIndexManifestSchemaAttr{\
	Attribute("IndexType", tinyintT),\
	Attribute("IndexField", tinyintT),\
	Attribute("RootPage", uintT)	};
const Schema kTableIndexManifestSchema("table_index",\
 kTableIndexManifestSchemaAttr.begin(), kTableIndexManifestSchemaAttr.end());


class Engine: public NoCopy{

	// Meta Data //
	struct TableMetaData{
		// depend on database root:
		PageHandle table_root;
		// flag to validate following field
		bool loaded;
		// depend on table root
		Schema schema;
		size_t nPrimaryIndex;
		PageHandle bflow_root;
		TableMetaData(std::string name, PageHandle hPage):table_root(hPage),loaded(false),schema(name){ }
	};
	using TableMetaDataPtr = ::std::shared_ptr<TableMetaData>;
	struct DatabaseMetaData{
		// database root is access by defaultPath or input path
		bool loaded; // simply means root file is closed
		FileHandle database_root;
		HashMap<FileHandle, std::string> file_manifest; // fileHandle to filePath
		HashMap<std::string, TableMetaDataPtr> table_manifest; // tableName to meta
		DatabaseMetaData():file_manifest(5),table_manifest(5),loaded(false){ }
	} database_;
	// Runtime //
	PageManager manager;
	struct CursorHolder{
		TableMetaDataPtr pTable;
		BFlowCursor curMain;
		size_t nIndex;
		BPlusCursor curIndex;
		CursorHolder(PageManager* manager):pTable(nullptr),curMain(manager),nIndex(0),curIndex(manager){ }
	} cursor_;
 public:
 	Engine():cursor_(&manager){ }
 	~Engine(){ }
 	inline TableMetaDataPtr GetTable(std::string name){
 		if(!database_.loaded)return nullptr;
 		TableMetaDataPtr ret = nullptr;
 		database_.table_manifest.Get(name, ret);
 		return ret;
 	}
 	// Meta Info feedback //
 	// Creating strategy:
 	// database + index => File 0 named root
 	// table root + bflow => File x named TableName
 	Status CreateDatabase(std::string name = kDatabaseRootPath);
 	Status CreateTable(Schema& schema);
 	Status LoadDatabase(std::string name = kDatabaseRootPath);
 	Status LoadTable(std::string name);
 	Status DropDatabase(void); // only current database
 	Status DropTable(std::string name);
 	Status CloseDatabase(void);
 	Status CloseTable(std::string name);
 	// Cursor //
 	Status OpenCursor(std::string table, std::string field){
 		auto pTable = GetTable(table);
 		if(!pTable)return Status::InvalidArgument("Cannot find field.");
 		int idx = pTable->schema.GetAttributeIndex(field);
 		if(idx < 0)return Status::InvalidArgument("Cannot find field.");
 		PageHandle hIndex = pTable->schema.GetIndexHandle(idx);
 		if(hIndex == 0)return Status::InvalidArgument("No index on this field.");
 		cursor_.curIndex.Set(pTable->schema[0].type(), hIndex);
 		return Status::OK();
 	}
 	Status OpenCursor(std::string table){
 		auto pTable = GetTable(table);
 		if(!pTable)return Status::InvalidArgument("Cannot find field.");
 		cursor_.curMain.Set(&pTable->schema, pTable->bflow_root);
 		return Status::OK();
 	}
 	// for primary-bplus, not leaf
 	Status DescendToLeaf(Value val){
 		std::cout << "Engine DescendToLeaf"<< std::endl;
 		Status status;
 		while(true){
 			std::cout << "Descending" << std::endl;
 			status = cursor_.curIndex.Descend(&val);
 			if(!status.ok())break;
 		}
 		status = cursor_.curMain.MoveTo(cursor_.curIndex.protrude());
 		if(!status.ok())return status;
 		return Status::OK();
 	}
 	Status Insert(Slice& slice){
 		std::cout << "Engine Insert" << std::endl;
 		Status status;
 		Value primary = slice[0];
 		status = cursor_.curIndex.Rewind();
 		std::cout << "Rewinded" << std::endl;
 		if(!status.ok())return status;
 		PageHandle hMain;
 		status = DescendToLeaf(primary);
 		std::cout << "Descended" << std::endl;
 		if(!status.ok())return status;
 		status = cursor_.curMain.Insert(&slice);
 		std::cout << "Inserted" << std::endl;
 		if(!status.ok())return status;
 		return Status::OK();
 	}
 	Status Get(Value val, std::vector<Slice>& ret){
 		std::cout << "Engine Get" << std::endl;
 		// if primary, check bflow first
 		// index query
 		Status status;
 		status = cursor_.curIndex.Rewind();
 		if(!status.ok())return status;
 		status = DescendToLeaf(val);
 		if(!status.ok())return status;
 		std::cout << "Descended" << std::endl;
 		status = cursor_.curMain.GetMatch(&val, ret);
 		if(!status.ok())return status;
 		return Status::OK();
 	}

 	// Status GetInRange(Value* min, Value* max, std::vector<Slice>& ret);

};

// struct RuntimeHolder{
// 	int pc;
// 	std::vector<Value*> valueStack;
// 	std::vector<Slice*> recordStack;
// };

} // namespace sbase


#endif // SBASE_DB_ENGINE_H_