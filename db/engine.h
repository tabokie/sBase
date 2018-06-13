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
#include <deque> 

#ifndef LOGFUNC
#define LOGFUNC()					do{std::cout << __func__ << std::endl;}while(0)
#endif // LOGFUNC

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
		int idxIndex; // the cursor's index
		BPlusCursor curIndex;
		CursorHolder(PageManager* manager):pTable(nullptr),curMain(manager),idxIndex(0),curIndex(manager){ }
	} cursor_;
	struct RuntimeHolder{
		bool isPrimary;
		std::deque<PageHandle> handles;
		std::deque<Slice> slices;
		// scoping
		Value* min;
		Value* max;
		bool leftEqual;
		bool rightEqual;
		RuntimeHolder():isPrimary(true){ }
	} runtime_;
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
 	Status Transaction(void){
 		cursor_.pTable = nullptr;
 		cursor_.idxIndex = 0;
 		runtime_.isPrimary = true;
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		return Status::OK();
 	}
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
 		LOGFUNC();
 		auto pTable = GetTable(table);
 		if(!pTable)return Status::InvalidArgument("Cannot find field.");
 		cursor_.curMain.Set(&pTable->schema, pTable->bflow_root);
 		return Status::OK();
 	}
 	// if primary index, return one handle; else return nil/one
 	Status PrepareMatch(Value* key){
 		runtime_.min = key;
 		runtime_.max = key;
 		runtime_.leftEqual = true;
 		runtime_.rightEqual = true;
 		cursor_.curIndex.Rewind();
 		PageHandle ret;
 		Status status = DescendToLeaf(key, ret);
 		runtime_.handles.push_back(ret);
 		return Status::OK();
 	}
 	// for non-primary
 	Status PrepareSequence(Value* min, Value* max, bool leftEqual = true, bool rightEqual = true){
 		// if(cursor_.idxIndex == 0)return PrepareMatch(min);
 		// PageHandle ret;
 		// Status status = DescendToLeaf(min, ret);
 		// status = cursor_.curIndex.GetSequence(min,max,make_tuple<bool,bool>(leftEqual, rightEqual), runtime_.handles);

 		return Status::OK();
 	}
 	Status NextSlice(Slice*& ret){
 		LOGFUNC();
 		if(runtime_.slices.size() <= 0){ // no more slice
 			if(runtime_.handles.size() <= 0){ // no more handles
 				ret = nullptr;
 				return Status::OK();
 			}
 			cursor_.curMain.MoveTo(runtime_.handles.front());
 			bool left = runtime_.leftEqual;
 			bool right = runtime_.rightEqual;
 			cursor_.curMain.Get(runtime_.min, runtime_.max, left, right, runtime_.slices);
 			runtime_.handles.pop_front();
 			if(left){ // have more left
 				runtime_.handles.push_front(cursor_.curMain.leftHandle());
 			}
 		}
 		std::cout << "NextSlice: " << std::string((runtime_.slices.front().GetValue(0)))<<", "<<std::string((runtime_.slices.front().GetValue(1)))<<std::endl;
 		// have slice in store
 		// ret = &(runtime_.slices.front());
 		ret = new Slice(runtime_.slices.front());
 		std::cout << "NextSlice: " << std::string((ret->GetValue(0)))<<", "<<std::string((ret->GetValue(1)))<<std::endl;
 		runtime_.slices.pop_front();
 		std::cout << "NextSlice: " << std::string((ret->GetValue(0)))<<", "<<std::string((ret->GetValue(1)))<<std::endl;
 		return Status::OK();
 	}
 	// Status PopSlice(void){
 	// 	runtime_.slices.pop_front();
 	// 	return Status::OK();
 	// }
 	Status DeleteSlice(void){return Status::OK();}
 	Status InsertSlice(Slice* slice){
 		LOGFUNC();
 		std::cout << cursor_.curMain.currentHandle() << std::endl;
 		cursor_.curMain.MoveTo(runtime_.handles.front());
 		std::cout << cursor_.curMain.currentHandle() << std::endl;
 		Status status = cursor_.curMain.Insert(slice);
 		if(status.IsIOError()){ // bflow is full
 			// Slice* newSlice = nullptr;
 			// PageHandle newPage;
 			// // split bflow
 			// status = cursor_.curMain.Split(newSlice, newPage);
 			// if(!status.ok())return status;
 			// status = cursor_.curMain.Insert(&slice);
 			// if(!status.ok())return status;
 			// *newSlice = cursor_.curMain.GetMin();
 			// // update index
 			// status = DescendToLeaf(newPivot);
 			// if(!status.ok())return status;
 			// status = cursor_.curIndex.Insert(&newPivot, newPage);
 			// if(!status.ok())return status;
 			return Status::IOError("hh");
 		}
 		else if(!status.ok())return status;
 		return Status::OK();
 	}
 	Status TransactionEnd(void){return Status::OK();}
 private:
 	// for primary-bplus, not leaf
 	Status DescendToLeaf(Value* val, PageHandle& ret){
 		LOGFUNC();
 		Status status;
 		while(true){
 			status = cursor_.curIndex.Descend(val);
 			if(!status.ok())break;
 		}
 		ret = cursor_.curIndex.protrude();
 		return Status::OK();
 	}


 	// Status InsertIndex(Value key, PageHandle hPage){
 	// 	Status status = DescendToLeaf(key);
 	// 	if(!status.ok())return status;
 	// 	status = cursor_.curIndex.Insert(key, hPage);
 	// 	if(status.IsIOError()){ // index page full
 	// 		while(true){ 				
 	// 			status = cursor_.curIndex.InsertOnSplit(&key, hPage);
 	// 			if(!status.ok())return status;
 	// 			status = cursor_.curIndex.Ascend();
 	// 			if(status.IsIOError()){ // already at top
 	// 				cursor_.curIndex.MakeRoot(&key, hPage);

 	// 			}
 	// 			else if(!status.ok())return status;
 	// 			status = cursor_.curIndex.Insert(&key, hPage);
 	// 			if(!status.IsIOError()){
 	// 				return status;
 	// 			}

 	// 		}

 	// 	}
 	// 	else if(!status.ok())return status;
 	// 	return Status::OK();
 	// }
 	// Status UpdateAllIndex(Slice* slice, PageHandle hPage){
 	// 	Status status;
 	// 	if(slice == nullptr || cursor_.pTable == nullptr)return Status::Corruption("Invalid slice or ptable.");
 	// 	int idxIndex = cursor_.idxIndex;
 	// 	for(int i = 0; i < slice->attributeCount(); i++){
 	// 		PageHandle hIndex = cursor_.pTable->schema.GetIndexHandle(i);
 	// 		if(hIndex == 0) continue; // no index on it
 	// 		cursor_.curIndex.Set( slice->GetValue(i).type(), hIndex );
 	// 		DescendToLeaf(slice->GetValue(i));
 	// 		status = cursor_.curIndex.Insert(slice->GetValue(i), hPage);
 	// 		if(status.IsIOError()){ // index page full

 	// 		}
 	// 		else if(!status.ok())return status;
 	// 	}
 	// 	// restore index cursor
 	// 	PageHandle hIndex = pTable->schema.GetIndexHandle(idxIndex);
 	// 	if(hIndex == 0) return Status::Corruption("Index cursor corrupted.");
 	// 	cursor_.curIndex.Set(pTable->schema[idxIndex].type(), hIndex);
 	// 	return Status::OK();
 	// }


};

} // namespace sbase


#endif // SBASE_DB_ENGINE_H_ 