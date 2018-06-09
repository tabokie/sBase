#ifndef SBASE_DB_ENGINE_H_
#define SBASE_DB_ENGINE_H_

#include ".\util\reflection.hpp"
#include ".\db\slice.hpp"
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
	Attribute("file handle", tinyintT),\
	Attribute("file path", fixchar32T) };
const Schema kDatabaseFileManifestSchema(nullptr, "db_file",\
 kDatabaseFileManifestSchemaAttr.begin(), kDatabaseFileManifestSchemaAttr.end());
// TableHandle ~ TableName ~ PageHandle
const ::std::vector<Attribute> kDatabaseTableManifestSchemaAttr{\
	Attribute("table handle", tinyintT),\
	Attribute("table name", fixchar32T),\
	Attribute("root page", intT) };
const Schema kDatabaseTableManifestSchema(nullptr, "db_table",\
 kDatabaseTableManifestSchemaAttr.begin(), kDatabaseTableManifestSchemaAttr.end());
// in table root page //
// IndexType ~ IndexPivot ~ PageHandle
const ::std::vector<Attribute> kTableIndexManifestSchemaAttr{\
	Attribute("index type", tinyintT),\
	Attribute("index field", tinyintT),\
	Attribute("root page", intT)	};
const Schema kTableIndexManifestSchema(nullptr, "table_index",\
 kTableIndexManifestSchemaAttr.begin(), kTableIndexManifestSchemaAttr.end());
// Index ~ Name ~ Type ~ Primary ~ Unique // ~ Null ~ Unique
const ::std::vector<Attribute> kTableSchemaManifestSchemaAttr{\
	Attribute("index", tinyintT),\
	Attribute("field name", fixchar32T ),\
	Attribute("field type", tinyintT),\
	Attribute("primary", tinyintT),\
	Attribute("unique", tinyintT) };
const Schema kTableSchemaManifestSchema(nullptr, "table_schema",\
 kTableSchemaManifestSchemaAttr.begin(), kTableSchemaManifestSchemaAttr.end());


class Engine: public NoCopy{

	struct TableMetaData{
		PageHandle table_root;
		Schema* table_schema;
		PageHandle bflow_root;
		PageHandle bplus_root;
		TableMetaData(PageHandle hPage):table_root(hPage),table_schema(nullptr){ }
	};
	using TableMetaDataPtr = ::std::shared_ptr<TableMetaData>;

	struct DatabaseMetaData{
		HashMap<FileHandle, ::std::string> file_manifest;
		HashMap<::std::string, TableMetaDataPtr> table_manifest;
	} database_;

	PageManager manager;
 public:
 	Status LoadDatabase(void){
 		// open database root
 		auto status = manager.OpenFile(kDatabaseRootPath, database_.database_root);
 		if(!status.ok())return Status::IOError("Cannot open database core data.");
 		if(database_.database_root != kDatabaseRootFile)return Status::Corruption("Database core corrupted.");
 		PageRef databaseRootRef(&manager, kDatabaseRootPage, kReadOnly);
 		ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
 		if(dbRootHeader->hBlockCode != kDatabaseRoot)return Status::Corruption("Database root page corrupted.");
 		char* cur = databaseRootRef.ptr + dbRootHeader->oManifest0;
 		Slice* fileSlice = kDatabaseFileManifestSchema.NewObject();
 		for(int i = 0; i < dbRootHeader->nManifest0; i++,cur += kDatabaseFileManifestSchema.length()){
 			// encode file manifest
 			fileSlice.Read(cur);
 			file_manifest.Insert(fileSlice[0], fileSlice[1]);
 		}
 		cur = databaseRootRef.ptr + dbRootHeader->oManifest1;
 		Slice* tableSlice = kDatabaseTableManifestSchema.NewObject();
 		for(int i = 0; i < dbRootHeader->nManifest1; i++, cur += kDatabaseTableManifestSchema.length()){
 			// encode table root page
 			tableSlice.Read(cur);
 			table_manifest.Insert(tableSlice[0], tableSlice[1]);
 		}
 		return Status::OK();
 	}

};


// struct RuntimeHolder{
// 	int pc;
// 	std::vector<Value*> valueStack;
// 	std::vector<Slice*> recordStack;
// };

// struct CursorHolder{
// 	BFlowCursor* baseCursor;
// 	BPlusCursor* primaryCursor;
// 	BCursor* indexCursor;
// };

} // namespace sbase


#endif // SBASE_DB_ENGINE_H_