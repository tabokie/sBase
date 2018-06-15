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

using SlicePtr = shared_ptr<Slice>;
using ValuePtr = shared_ptr<Value>;

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
		bool keepReading;
		RuntimeHolder():isPrimary(true){ }
	} runtime_;
 public:
 	Engine():cursor_(&manager){ }
 	~Engine(){ }
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
 	// Phrase 1: Open Database and Table //
 	Status Transaction(void){
 		// LOGFUNC();
 		cursor_.pTable = nullptr;
 		cursor_.idxIndex = 0;
 		runtime_.isPrimary = true;
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		runtime_.keepReading = false;
 		return Status::OK();
 	}
 	Status OpenCursor(std::string table, std::string field){
 		// LOGFUNC();
 		auto pTable = GetTable(table);
 		if(!pTable)return Status::InvalidArgument("Cannot find field.");
 		int idx = pTable->schema.GetAttributeIndex(field);
 		if(idx < 0)return Status::InvalidArgument("Cannot find field.");
 		PageHandle hIndex = pTable->schema.GetIndexHandle(idx);
 		if(hIndex == 0)return Status::InvalidArgument("No index on this field.");
 		cursor_.idxIndex = idx;
 		cursor_.curIndex.Set(pTable->schema[idx].type(), hIndex);
 		return Status::OK();
 	}
 	Status OpenCursor(std::string table){ // table
 		// LOGFUNC();
 		auto pTable = GetTable(table);
 		cursor_.pTable = pTable; // ERROR
 		if(!pTable)return Status::InvalidArgument("Cannot find field.");
 		cursor_.curMain.Set(&pTable->schema, pTable->bflow_root);
 		return Status::OK();
 	}
 	// Phrase 2: Prepare Data Handle //
 	// if primary index, return one handle
 	// else read by pack
 	Status PrepareMatch(Value* key){
 		// LOGFUNC();
 		if(cursor_.idxIndex == 0){
 			runtime_.keepReading = true;
 			return PrepareMatchPrimary(key);
 		}
 		return PrepareMatchNonPrimary(key);
 	}
 	Status PrepareSequence(Value* min, Value* max, bool leftEqual = true, bool rightEqual = true){
 		if(cursor_.idxIndex == 0){
 			runtime_.keepReading = true;
 			return PrepareSequencePrimary(min, max, leftEqual, rightEqual);
 		}
 		return PrepareSequenceNonPrimary(min, max, leftEqual, rightEqual);
 	}
 	Status NextSlice(SlicePtr& ret){
 		PageHandle tmp;
 		auto status = NextSlice(ret, tmp);
 		if(!status.ok())return status;
 		if(ret)return Status::OK();
 		// ret = nullptr
 		status = NextHandle(tmp);
 		if(!status.ok())return status;
 		if(tmp > 0){
 			status = NextSlice(ret, tmp);
 		}
 		return status;
 	}
 	inline void Plot(void){
 		cursor_.curIndex.Plot();
 		cursor_.curMain.Plot();
 		return ;
 	}
 	Status DeleteSlice(void){return Status::OK();}
 	Status InsertSlice(Slice* slice){
 		// LOGFUNC();
 		if(runtime_.handles.size() == 0)return Status::Corruption("");
 		cursor_.curMain.MoveTo(runtime_.handles.front());
 		Status status = cursor_.curMain.Insert(slice);
 		if(status.IsIOError()){ // bflow is full
 			PageHandle newPage;
 			// split bflow
 			status = cursor_.curMain.InsertOnSplit(slice, newPage);
 			if(!status.ok())return status;
 			// update index
			status = UpdateAllIndex(slice, newPage);
			return status;
 		}
 		else if(!status.ok())return status;
 		return Status::OK();
 	}
 	Status DeleteSlice(Slice* slice){
 		if(runtime_.handles.size() == 0)return Status::Corruption("No handle left.");
 		cursor_.curMain.MoveTo(runtime_.handles.front());
 		Status status = cursor_.curMain.Delete(slice);
 		// while(status.IsIOError()){
 		// 	if(cursor_.curMain.ShiftRight().IsIOError())break;
 		// 	status = cursor_.curMain.Delete(slice);
 		// }
 		if(!status.ok())return status;
 		return DeleteFromAllIndex(slice);
 	}
 	Status TransactionEnd(void){return Status::OK();}
 	Status MakeIndex(std::string table, std::string field);
 private:
  inline TableMetaDataPtr GetTable(std::string name){
 		if(!database_.loaded)return nullptr;
 		TableMetaDataPtr ret = nullptr;
 		database_.table_manifest.Get(name, ret);
 		return ret;
 	}
 	// Primary part //
 	Status PrepareMatchPrimary(Value* key){
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		runtime_.min = key;
 		runtime_.max = key;
 		runtime_.leftEqual = true;
 		runtime_.rightEqual = true;
 		runtime_.keepReading = true;
 		auto status = cursor_.curIndex.Rewind();
 		if(!status.ok())return status;
 		runtime_.handles.push_back(0);
 		status = DescendToLeaf(key, runtime_.handles.front());
 		if(!status.ok())return status;
 		ReadCurrent();
 	}
 	Status PrepareSequencePrimary(Value* min, Value* max, bool leftEqual = true, bool rightEqual = true){
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		runtime_.min = min;
 		runtime_.max = max;
 		runtime_.leftEqual = leftEqual;
 		runtime_.rightEqual = rightEqual;
 		runtime_.keepReading = true;
 		if(!min){
 			runtime_.handles.push_back(cursor_.pTable->bflow_root);
 			return ReadCurrent();
 		}
 		else{
 			auto status = cursor_.curIndex.Rewind();
 			if(!status.ok())return status;
 			runtime_.handles.push_back(0);
 			status =  DescendToLeaf(max, runtime_.handles.front()); 			
 			if(!status.ok())return status;
 			return ReadCurrent();
 		}
 	}
 	// fetch current handle and read from it
 	Status NextHandle(PageHandle& ret){
 		ret = 0;
 		if(runtime_.handles.size() > 0)runtime_.handles.pop_front();
 		if(runtime_.handles.size() <= 0)return Status::OK();
 		ret = runtime_.handles.front();
 		return ReadCurrent();
 	}
 	Status ReadCurrent(){
 		if(runtime_.handles.size() <= 0)return Status::Corruption("No more handles.");
 		PageHandle hMain = runtime_.handles.front();
 		if(hMain == 0)return Status::OK();
 		auto status = cursor_.curMain.MoveTo(hMain);
 		if(!status.ok())return status;
 		bool left = runtime_.leftEqual;
 		bool right = runtime_.rightEqual;
 		cursor_.curMain.Get(runtime_.min,runtime_.max,left,right,runtime_.slices);
 		if(runtime_.keepReading && right)runtime_.handles.push_back(cursor_.curMain.rightHandle());
 		return Status::OK();
 	}

 	// Non primary part //
 	Status PrepareMatchNonPrimary(Value* key){
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		runtime_.min = nullptr;
 		runtime_.max = nullptr;
 		runtime_.leftEqual = true;
 		runtime_.rightEqual = true;
 		runtime_.keepReading = false;
 		auto status = cursor_.curIndex.Rewind();
 		if(!status.ok())return status;
 		runtime_.handles.push_back(0);
 		status = DescendToLeaf(key, runtime_.handles.front());
 		if(!status.ok())return status;
 		return ReadCurrent();
 	}
 	// handles is used here
 	Status PrepareSequenceNonPrimary(Value* min, Value* max, bool leftEqual = true, bool rightEqual = true){
 		if(!min && !max)return Status::InvalidArgument("Can not assign full range for non-primary index.");
 		runtime_.handles.clear();
 		runtime_.slices.clear();
 		runtime_.min = nullptr;
 		runtime_.max = nullptr;
 		runtime_.leftEqual = true;
 		runtime_.rightEqual = true;
 		runtime_.keepReading = false;
 		auto status = cursor_.curIndex.Rewind();
 		if(!status.ok())return status;
 		PageHandle firstPage = 0;
 		status = DescendToLeaf(min, firstPage);
 		if(!status.ok())return status;
 		while(true){
 			bool left,right;
	 		cursor_.curIndex.Get(min, max, left, right, runtime_.handles);
	 		if(!right)break;
	 		auto status = cursor_.curIndex.ShiftRight();
	 		if(status.IsIOError())break;
	 		else if(!status.ok())return status;
 		}
 		return ReadCurrent();
 	}
 	// Get slice from pool
 	Status NextSlice(SlicePtr& ret, PageHandle& hRet){
 		if(runtime_.slices.size() <= 0){
 			ret = nullptr;
 			hRet = 0;
 			return Status::OK();
 		}
 		ret = make_shared<Slice>(runtime_.slices.front());
 		runtime_.slices.pop_front();
 		hRet = runtime_.handles.front();
 		return Status::OK();
 	}
 	// for primary-bplus, not leaf
 	Status DescendToLeaf(Value* val, PageHandle& ret){
 		// LOGFUNC();
 		Status status;
 		while(true){
 			status = cursor_.curIndex.Descend(val);
 			if(status.IsIOError())break;
 			else if(!status.ok())return status;
 		}
 		ret = cursor_.curIndex.protrude();
 		return Status::OK();
 	}
 	Status InsertIndex(int idx, Value* key, PageHandle hPage){
 		// update index on i field
		PageHandle tmp;
		cursor_.curIndex.Rewind();
		DescendToLeaf(key, tmp);
		auto status = cursor_.curIndex.Insert(key, hPage);
		while(status.IsIOError()){ // index page full
			status = cursor_.curIndex.InsertOnSplit(key, hPage);
			if(!status.ok())return status;
			status = cursor_.curIndex.Ascend();
			if(status.IsIOError()){ // at top
				status = cursor_.curIndex.MakeRoot(key, hPage);
				return SetIndexRoot(idx, hPage);
			}
			else if(!status.ok())return status;
			status = cursor_.curIndex.Insert(key, hPage);
		}
		if(!status.ok())return status;
		return Status::OK();
 	}
 	Status UpdateAllIndex(Slice* slice, PageHandle hPage){
 		// LOGFUNC();
 		Status status;
 		if(slice == nullptr || cursor_.pTable == nullptr)return Status::Corruption("Invalid slice or ptable.");
 		int idxIndex = cursor_.idxIndex;
 		for(int i = 0; i < slice->attributeCount(); i++){
 			PageHandle hIndex = cursor_.pTable->schema.GetIndexHandle(i);
 			if(hIndex == 0) continue; // no index on it
 			cursor_.curIndex.Set( cursor_.pTable->schema[idxIndex].type(), hIndex );
 			Value* key = new Value(slice->GetValue(i));
 			InsertIndex(i, key, hPage);
 			delete key;
 		}
 		// restore index cursor
 		PageHandle hIndex = cursor_.pTable->schema.GetIndexHandle(idxIndex);
 		if(hIndex == 0) return Status::Corruption("Index cursor corrupted.");
 		cursor_.curIndex.Set(cursor_.pTable->schema[idxIndex].type(), hIndex); // ERROR
 		return Status::OK();
 	}
 	Status DeleteFromAllIndex(Slice* slice){
 		// LOGFUNC();
 		Status status;
 		if(slice == nullptr || cursor_.pTable == nullptr)return Status::Corruption("Invalid slice or ptable.");
 		int idxIndex = cursor_.idxIndex;
 		for(int i = 0; i < slice->attributeCount(); i++){
 			PageHandle hIndex = cursor_.pTable->schema.GetIndexHandle(i);
 			if(hIndex == 0) continue; // no index on it
 			cursor_.curIndex.Set( cursor_.pTable->schema[idxIndex].type(), hIndex );
 			Value* key = new Value(slice->GetValue(i));
 			PageHandle tmp;
 			auto status = DescendToLeaf(key, tmp);
 			if(!status.ok())return status;
 			status = cursor_.curIndex.Delete(key);
 			// while(status.IsIOError()){
 			// 	if(cursor_.curIndex.ShiftRight().IsIOError())break;
 			// 	status = cursor_.curIndex.Delete(key);
 			// }
 			if(!status.ok())return status;
 			delete key;
 		}
 		// restore index cursor
 		PageHandle hIndex = cursor_.pTable->schema.GetIndexHandle(idxIndex);
 		if(hIndex == 0) return Status::Corruption("Index cursor corrupted.");
 		cursor_.curIndex.Set(cursor_.pTable->schema[idxIndex].type(), hIndex); // ERROR
 		return Status::OK();
 	}
 	Status SetIndexRoot(int idx, PageHandle hRoot){
 		// LOGFUNC();
 		if(manager.GetPageType(hRoot) != kBPlusPage)return Status::Corruption("New root not BPlus");
 		if(!cursor_.pTable)return Status::Corruption("Nil pTable");
 		// first alter in memory schema
 		cursor_.pTable->schema.SetIndex(idx, hRoot);
 		// alter file
 		if(manager.GetPageType(cursor_.pTable->table_root) != kTableRoot)return Status::Corruption("Original manifest not");
 		PageRef ref(&manager, cursor_.pTable->table_root, kLazyModify);
 		ManifestBlockHeader* tableRootHeader = reinterpret_cast<ManifestBlockHeader*>(ref.ptr);
 		if(tableRootHeader->hBlockCode!=kTableRoot)return Status::Corruption("Nil page.") ;
		Slice* indexSlice = kTableIndexManifestSchema.NewObject();
 		char* half = ref.ptr + tableRootHeader->oManifest1;
 		for(int i = 0; i < tableRootHeader->nManifest1; i++ , half += indexSlice->length()){
			indexSlice->Read(half);
			if(indexSlice->GetValue(1).get<int8_t>() == idx){
				indexSlice->SetValue(2, Value(uintT, new RealValue<uint32_t>(hRoot)) );
				indexSlice->Write(half);
				return Status::OK();
			}
		}
		indexSlice->SetValue(0, Value(tinyintT, new RealValue<int8_t>(kBPlusIndex)));
		indexSlice->SetValue(1, Value(tinyintT, new RealValue<int8_t>(idx)));
		indexSlice->SetValue(2, Value(uintT, new RealValue<uint32_t>(hRoot)));
		indexSlice->Write(half);
		tableRootHeader->nManifest1++;
		return Status::OK();
 	}


};

} // namespace sbase


#endif // SBASE_DB_ENGINE_H_ 

