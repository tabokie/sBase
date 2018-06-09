#ifndef SBASE_DB_ENGINE_H_
#define SBASE_DB_ENGINE_H_

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
const Schema kDatabaseFileManifestSchema(nullptr, "db_file",\
 kDatabaseFileManifestSchemaAttr.begin(), kDatabaseFileManifestSchemaAttr.end());
// TableHandle ~ TableName ~ PageHandle
const ::std::vector<Attribute> kDatabaseTableManifestSchemaAttr{\
	Attribute("TableName", fixchar32T),\
	Attribute("RootPage", uintT) };
const Schema kDatabaseTableManifestSchema(nullptr, "db_table",\
 kDatabaseTableManifestSchemaAttr.begin(), kDatabaseTableManifestSchemaAttr.end());
// in table root page //
// Index ~ Name ~ Type ~ Primary ~ Unique // ~ Null ~ Unique
const ::std::vector<Attribute> kTableSchemaManifestSchemaAttr{\
	Attribute("Index", tinyintT),\
	Attribute("FieldName", fixchar32T ),\
	Attribute("FieldType", tinyintT),\
	Attribute("Primary", tinyintT),\
	Attribute("Unique", tinyintT) };
const Schema kTableSchemaManifestSchema(nullptr, "table_schema",\
 kTableSchemaManifestSchemaAttr.begin(), kTableSchemaManifestSchemaAttr.end());
 // IndexType ~ IndexPivot ~ PageHandle
const ::std::vector<Attribute> kTableIndexManifestSchemaAttr{\
	Attribute("IndexType", tinyintT),\
	Attribute("IndexField", tinyintT),\
	Attribute("RootPage", uintT)	};
const Schema kTableIndexManifestSchema(nullptr, "table_index",\
 kTableIndexManifestSchemaAttr.begin(), kTableIndexManifestSchemaAttr.end());

// Slice* GetFileSlice(FileHandle hFile, std::string path){
// 	Slice* ret = new Slice(&kDatabaseFileManifestSchema);
// 	ret.SetValue("FileHandle", Value(tinyintT, new RealValue<tinyintT>(hFile)) );
// 	// ret.SetValue("FilePath", Value(fixchar32T, new ReadValue<FixChar>( FixChar(32,path) )) ) 
// 	ret.SetValue("FilePath", Value(fixchar32T, path ) ); 
// 	return ret;
// }

typedef uint8_t TableHandle;

class Engine: public NoCopy{

	struct TableMetaData{
		PageHandle table_root;
		size_t fieldSize;
		Schema* table_schema;
		PageHandle bflow_root;
		PageHandle bplus_root;
		TableMetaData(PageHandle hPage):table_root(hPage),table_schema(nullptr){ }
	};
	using TableMetaDataPtr = ::std::shared_ptr<TableMetaData>;

	struct DatabaseMetaData{
		FileHandle database_root;
		HashMap<FileHandle, std::string> file_manifest(5); // fileHandle to filePath
		HashMap<std::string, TableMetaDataPtr> table_manifest(5); // tableName to meta
	} database_;

	PageManager manager;
 public:
 	// Creating strategy:
 	// database + index => File 0 named root
 	// table root + bflow => File x named TableName
 	Status CreateDatabase(std::string path = kDatabaseRootPath){
 		auto status = manager.NewFile(path, kBlockLen, database_.database_root);
 		// write database root page
 		PageHandle hPage;
 		status = manager.NewPage(database_.database_root, kDatabaseRoot, hPage);
 		assert(GetPageNum(hPage) == kDatabaseRootPageNum); // first page
 		PageRef databaseRef(&manager, hPage, kLazyModify);
 		ManifestBlockHeader* dbHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRef.ptr);
 		dbHeader->hBlockCode = kDatabaseRoot;
 		dbHeader->oManifest0 = sizeof(ManifestBlockHeader);
 		dbHeader->oManifest1 = kBlockLen / 2; // split half
 		// write file section
 	}
 	Status CreateTable(Schema& schema){
 		// check name
 		std::string tmp;
 		if(table_manifest.Get(schema.name(), tmp))return Status::InvalidArgument("Duplicate table.");
 		// check primary
 		int primarySize = 0;
 		for(int i = 0; i < schema.attributeCount(); i++){if(isPrimary(i))primarySize++;}
 		if(primarySize > 1 || primarySize <= 0)return Status::InvalidArgument("Incorrect number of primary key.");
 		// create table root
 		FileHandle hFile;
 		auto status = manager.NewFile(schema.name(), kBlockLen, hFile);
 		if(!status.ok())return status;
 		// need store hFile-path to database
 		Slice fileMapRecord(&kDatabaseFileManifestSchema,\
 			Value(tinyintT, new RealValue<uint8_t>(hFile)),\
 			Value(fixchar32T, schema.name()));
 		PageHandle hPage;
 		status = manager.NewPage(hFile, kTableRoot, hPage);
 		// need store tableName-rootPage to database
 		Slice tableMapRecord(&kDatabaseTableManifestSchema,\
 			Value(fixchar32T, schema.name()),\
 			Value(uintT, new RealValue<uint32_t>(hPage)));
 		// write to table root
 		PageRef tableRootRef(&manager, hPage, kLazyModify);
 		ManifestBlockHeader* tableHeader = reinterpret_cast<ManifestBlockHeader*>(tableRootRef.ptr);
 		tableHeader->hBlockCode = kTableRoot;
 		tableHeader->oManifest0 = sizeof(ManifestBlockHeader);
 		tableHeader->oManifest1 = kBlockLen/2;
 		char* cur = tableRootRef.ptr + tableHeader->oManifest0;
 		int fieldSize = schema.attributeCount();
 		int indexPrimary = -1;
 		int stripe = kTableSchemaManifestSchema.length();
 		for(int i = 0; i < fieldSize; i++, cur += stripe){
 			auto attr = schema.GetAttribute(i);
 			Slice fieldSlice(&kTableSchemaManifestSchema,\
 				Value(tinyintT, new RealValue<int8_t>(i)),\
 				Value(fixchar32T, attr.name()),\
 				Value(tinyintT, new RealValue<int8_t>(attr.type())),\
 				Value(tinyintT, new RealValue<int8_t>(schema.isPrimary(i))),\
 				Value(tinyintT, new RealValue<int8_t>(schema.isUnique(i)))  );
 			if(schema.isPrimary(i)){
 				indexPrimary = i; // assert only one primary
 			}
 			fieldSlice.Write(cur);
 		}
 		// create bflow table
 		status = manager.NewPage(hFile, kBFlowPage, hPage);
 		Slice bflowTableRecord(&kTableIndexManifestSchema, \
 			Value(tinyintT, new RealValue<int8_t>(kBFlowIndex)),\
 			Value(tinyintT, new RealValue<int8_t>(indexPrimary)),\
 			Value(uintT, new RealValue<uint32_t>(hPage)) );
 		// create bplus index
 		status = manager.NewPage(database_.database_root, kBPlusPage, hPage);
 		Slice bplusTableRecord(&kTableIndexManifestSchema, \
 			Value(tinyintT, new RealValue<int8_t>(kBPlusIndex)),\
 			Value(tinyintT, new RealValue<int8_t>(indexPrimary)),\
 			Value(uintT, new RealValue<uint32_t>(hPage)) );
 		// write tables to table root
 		cur = tableRootRef.ptr + tableHeader->oManifest1;
 		bflowTableRecord.Write(cur);
 		bplusTableRecord.Write(cur + kTableIndexManifestSchema.length());
 		// write file map to database root
 		PageRef databaseRootRef(&manager, GetPageHandle(database_.database_root, kDatabaseRootPageNum), kLazyModify);
 		ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
 		stripe = kDatabaseFileManifestSchema.length();
 		if(dbRootHeader->oManifest0 + stripe*(dbRootHeader->nManifest0+1)\
 			> dbRootHeader->oManifest1){
 			// need make room
 			if(dbRootHeader->oManifest1 + stripe * (dbRootHeader->nManifest1+1) > kBlockLen){
 				return Status::Corruption("Fatal");
 			}
 			for(int i = dbRootHeader->nManifest1; i>0; i--){
 				memcpy(dbRootHeader->oManifest1 + stripe * i, dbRootHeader->oManifest1 + stripe * (i-1); stripe);
 			}
 		}
 		fileMapRecord.Write(databaseRef.ptr + dbRootHeader->oManifest0 + dbRootHeader->nManifest0 * stripe);
 		// write table map to database root
 		tableMapRecord.Write(databaseRef.ptr + dbRootHeader->oManifest1 + dbRootHeader->nManifest1 * stripe);
 		dbRootHeader->nManifest0++;
 		dbRootHeader->nManifest1++;
 		return Status::OK();
 	}
 	Status LoadDatabase(std::string path = kDatabaseRootPath){
 		// open database root
 		auto status = manager.OpenFile(path, database_.database_root);
 		if(!status.ok())return Status::IOError("Cannot open database core data.");
 		PageRef databaseRootRef(&manager, GetPageHandle(database_.database_root, kDatabaseRootPageNum), kReadOnly);
 		ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
 		if(dbRootHeader->hBlockCode != kDatabaseRoot)return Status::Corruption("Database root page corrupted.");
 		char* cur = databaseRootRef.ptr + dbRootHeader->oManifest0;
 		Slice* fileSlice = kDatabaseFileManifestSchema.NewObject();
 		for(int i = 0; i < dbRootHeader->nManifest0; i++,cur += kDatabaseFileManifestSchema.length()){
 			// encode file manifest
 			fileSlice->Read(cur);
 			file_manifest.Insert(fileSlice[0], fileSlice[1]);
 		}
 		cur = databaseRootRef.ptr + dbRootHeader->oManifest1;
 		Slice* tableSlice = kDatabaseTableManifestSchema.NewObject();
 		for(int i = 0; i < dbRootHeader->nManifest1; i++, cur += kDatabaseTableManifestSchema.length()){
 			// encode table root page
 			tableSlice->Read(cur);
 			table_manifest.Insert(tableSlice[0], tableSlice[1]);
 			LoadTable(tableSlice[0]);
 		}
 		return Status::OK();
 	}
 	Status LoadTable(std::string name){
 		TableMetaDataPtr pMeta = nullptr;
 		Status status;
 		table_manifest.Get(name, pMeta);
 		if(!pMeta)return InvalidArgument("Table not found.");
 		if(!manager.fileIsOpened(GetFileHandle(pMeta->table_root))){
 			std::string fileName = "";
 			database_.file_manifest.Get(GetFileHandle(pMeta->table_root), fileName);
 			if(fileName.size() == 0)return Status::Corruption("Cannot fetch file path.");
 			FileHandle hFile;
 			status = manager.OpenFile(fileName, hFile);
 			if(hFile != GetFileHandle(pMeta->table_root))return Status::Corruption("File handle doesn't match.");
 		}
 		PageRef rootRef(&manager, pMeta->table_root, kReadOnly);
 		ManifestBlockHeader* tableRootHeader = reinterpret_cast<ManifestBlockHeader*>(rootRef.ptr);
 		if(tableRootHeader->hBlockCode != kTableRoot)return Status::Corruption("Table root page corrupted.");

 		pMeta->fieldSize = tableRootHeader->nManifest0;
 		pMeta->table_schema = new Schema[tableRootHeader->nManifest0];
 		char* cur = rootRef.ptr + tableRootHeader->oManifest0;
 		Slice* fieldSlice = kTableSchemaManifestSchema.NewObject();
 		for(int i = 0; i < tableRootHeader->nManifest0; i++, cur += fieldSlice->length()){
 			fieldSlice->Read(cur);
 			table_schema[i] = fieldSlice;
 		}
 		Slice* indexSlice = kTableIndexManifestSchema.NewObject();
 		cur = rootRef.ptr + tableRootHeader->oManifest1;
 		for(int i = 0; i < tableRootHeader->nManifest1; i++ , cur += indexSlice.length()){
 			indexSlice->Read(cur);

 		}
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