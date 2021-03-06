#include "engine.h"

#include <iostream>

namespace sbase{
Status Engine::CreateDatabase(std::string path){
	auto status = manager.NewFile(path, kBlockSize, database_.database_root);
	if(!status.ok())return status;
	// write database root page
	PageHandle hPage;
	status = manager.NewPage(database_.database_root, kDatabaseRoot, hPage);
	if(!status.ok())return status;
	assert(GetPageNum(hPage) == kDatabaseRootPageNum); // first page
	PageRef databaseRef(&manager, hPage, kLazyModify);
	ManifestBlockHeader* dbHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRef.ptr);
	dbHeader->hBlockCode = kDatabaseRoot;
	dbHeader->oManifest0 = sizeof(ManifestBlockHeader);
	dbHeader->oManifest1 = kBlockLen >> 1; // split half
	dbHeader->nManifest0 = 1; // one file map
	dbHeader->nManifest1 = 0; // no table map
	// write one file map record (root page)
	Slice fileMapRecord(&kDatabaseFileManifestSchema,\
		{Value(tinyintT, new RealValue<uint8_t>(database_.database_root)),\
		Value(fixchar32T, path)} );
	fileMapRecord.Write(databaseRef.ptr + dbHeader->oManifest0);
	// load database
	database_.file_manifest.Insert(database_.database_root, path);
	database_.loaded = true;
	return Status::OK();
}
Status Engine::CreateTable(Schema& schema){
	if(!database_.loaded)return Status::Corruption("Database not loaded.");
	// check if name used
	TableMetaDataPtr pTableMeta;
	if(database_.table_manifest.Get(schema.name(), pTableMeta))return Status::InvalidArgument("Duplicate table.");
	// check if one and only one primary
	int primarySize = 0;
	for(int i = 0; i < schema.attributeCount(); i++){if(schema.isPrimary(i))primarySize++;}
	if(primarySize != 1)return Status::InvalidArgument("Incorrect number of primary key.");
	// create table root
	FileHandle hFile;
	auto status = manager.NewFile(schema.name(), kBlockSize, hFile);
	if(!status.ok())return status;
	// need store hFile-path to database, create slice
	Slice fileMapRecord(&kDatabaseFileManifestSchema,\
		{Value(tinyintT, new RealValue<uint8_t>(hFile)),\
		Value(fixchar32T, schema.name())} );
	database_.file_manifest.Insert(hFile, schema.name());	
	// create table root
	PageHandle hPage;
	status = manager.NewPage(hFile, kTableRoot, hPage);
	if(!status.ok())return status;
	// create table struct
	pTableMeta = make_shared<TableMetaData>(schema.name(), hPage);
	database_.table_manifest.Insert(schema.name(), pTableMeta);
	// need store tableName-rootPage to database, create slice
	Slice tableMapRecord(&kDatabaseTableManifestSchema,\
		{Value(fixchar32T, schema.name()),\
		Value(uintT, new RealValue<uint32_t>(hPage))} );
	// write to table root
	PageRef tableRootRef(&manager, hPage, kLazyModify);
	ManifestBlockHeader* tableHeader = reinterpret_cast<ManifestBlockHeader*>(tableRootRef.ptr);
	tableHeader->hBlockCode = kTableRoot;
	tableHeader->oManifest0 = sizeof(ManifestBlockHeader);
	tableHeader->oManifest1 = kBlockLen / 2;
	tableHeader->nManifest0 = schema.attributeCount(); // schema part
	tableHeader->nManifest1 = 2; // bflow and bplus

	pTableMeta->schema = schema;
	char* cur = tableRootRef.ptr + tableHeader->oManifest0;
	int fieldSize = schema.attributeCount();
	int indexPrimary = -1;
	int stripe = kTableSchemaManifestSchema.length();
	for(int i = 0; i < fieldSize; i++, cur += stripe){
		auto attr = schema.GetAttribute(i);
		// std::cout << schema.GetUserIndex(i) << "," << attr.name() << "," << attr.type() << "," << schema.isUnique(i) << std::endl;
		Slice fieldSlice(&kTableSchemaManifestSchema,\
			{Value(tinyintT, new RealValue<int8_t>(schema.GetUserIndex(i))),\
			Value(fixchar32T, attr.name()),\
			Value(tinyintT, new RealValue<int8_t>(attr.type())),\
			Value(tinyintT, new RealValue<int8_t>(schema.isUnique(i)))}  );
		if(schema.isPrimary(i)){
			indexPrimary = i; // assert only one primary
		}
		fieldSlice.Write(cur);
	}
	assert(indexPrimary == 0);
	// create bflow table
	status = manager.NewPage(hFile, kBFlowPage, hPage);
	Slice bflowTableRecord(&kTableIndexManifestSchema, \
		{Value(tinyintT, new RealValue<int8_t>(kBFlowIndex)),\
		Value(tinyintT, new RealValue<int8_t>(indexPrimary)),\
		Value(fixchar32T, schema.name()),\
		Value(uintT, new RealValue<uint32_t>(hPage))} );
	PageHandle hBFlow = hPage;
	pTableMeta->bflow_root = hPage;
	PageRef bflowRootRef(&manager, hPage, kLazyModify);
	BFlowHeader* bflowHeader = reinterpret_cast<BFlowHeader*>(bflowRootRef.ptr + sizeof(BlockHeader));
	bflowHeader->nSize = 0;
	bflowHeader->hPri = 0;
	bflowHeader->hNext = 0;
	// create bplus index
	status = manager.NewPage(database_.database_root, kBPlusPage, hPage);
	Slice bplusTableRecord(&kTableIndexManifestSchema, \
		{Value(tinyintT, new RealValue<int8_t>(kBPlusIndex)),\
		Value(tinyintT, new RealValue<int8_t>(indexPrimary)),\
		Value(fixchar32T, schema.GetAttribute(0).name()),\
		Value(uintT, new RealValue<uint32_t>(hPage))} );
	pTableMeta->schema.SetIndex(indexPrimary, hPage);
	PageRef bplusRootRef(&manager, hPage, kLazyModify);
	// init bplus index page : infinity branch point to bflow
	BPlusHeader* bplusHeader = reinterpret_cast<BPlusHeader*>(bplusRootRef.ptr + sizeof(BlockHeader));
	bplusHeader->nSize = 1; // only one : infinity
	bplusHeader->hRight = 0;
	TypeT primaryType = schema.GetAttribute(indexPrimary).type();
	std::vector<Attribute> tmp{Attribute("Key",primaryType), Attribute("Handle",uintT)};
	Schema bplusRecordSchema("BPlusRecord", tmp.begin(), tmp.end());
	Slice bplusInfinityRecord(&bplusRecordSchema,\
	 {Value::InfinityValue(primaryType),\
	 Value(uintT, new RealValue<uint32_t>(hBFlow))});
	bplusInfinityRecord.Write(bplusRootRef.ptr + sizeof(BlockHeader) + sizeof(BPlusHeader));
	// write tables to table root
	cur = tableRootRef.ptr + tableHeader->oManifest1;
	bflowTableRecord.Write(cur);
	bplusTableRecord.Write(cur + kTableIndexManifestSchema.length());
	// write file map to database root
	PageRef databaseRootRef(&manager, GetPageHandle(database_.database_root, kDatabaseRootPageNum), kLazyModify);
	// memset(databaseRootRef.ptr, 1, 4096);
	ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
	stripe = kDatabaseFileManifestSchema.length();
	if(dbRootHeader->oManifest0 + stripe*(dbRootHeader->nManifest0+1)\
		> dbRootHeader->oManifest1){
		// need make room
		if(dbRootHeader->oManifest1 + stripe * (dbRootHeader->nManifest1+1) > kBlockLen){
			return Status::Corruption("Fatal");
		}
		for(int i = dbRootHeader->nManifest1; i>0; i--){
			memcpy(databaseRootRef.ptr+ dbRootHeader->oManifest1 + stripe * i, databaseRootRef.ptr+dbRootHeader->oManifest1 + stripe * (i-1), stripe);
		}
	}
	fileMapRecord.Write(databaseRootRef.ptr + dbRootHeader->oManifest0 + dbRootHeader->nManifest0 * stripe);
	// write table map to database root
	tableMapRecord.Write(databaseRootRef.ptr + dbRootHeader->oManifest1 + dbRootHeader->nManifest1 * kDatabaseTableManifestSchema.length());
	dbRootHeader->nManifest0++;
	dbRootHeader->nManifest1++;
	pTableMeta->loaded = true; // validate all the data
	return Status::OK();
}
Status Engine::LoadDatabase(std::string path){
	// open database root
	auto status = manager.OpenFile(path, database_.database_root);
	if(!status.ok())return Status::IOError("Cannot open database core data.");
	PageRef databaseRootRef(&manager, GetPageHandle(database_.database_root, kDatabaseRootPageNum), kReadOnly);
	ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
	if(dbRootHeader->hBlockCode != kDatabaseRoot)return Status::Corruption("Database root page corrupted.");
	char* cur = databaseRootRef.ptr + dbRootHeader->oManifest0;
	Slice fileSlice = kDatabaseFileManifestSchema.NewObject();
	for(int i = 0; i < dbRootHeader->nManifest0; i++,cur += kDatabaseFileManifestSchema.length()){
		// encode file manifest
		fileSlice.Read(cur);
		// std::cout << "get file map: " << static_cast<int>((*fileSlice)[0].get<int8_t>()) << " - " << static_cast<std::string>((*fileSlice)[1]) << std::endl;
		database_.file_manifest.Insert(static_cast<FileHandle>(fileSlice[0].get<int8_t>()), static_cast<std::string>(fileSlice[1]) );
	}
	cur = databaseRootRef.ptr + dbRootHeader->oManifest1;
	Slice tableSlice = kDatabaseTableManifestSchema.NewObject();
	for(int i = 0; i < dbRootHeader->nManifest1; i++, cur += kDatabaseTableManifestSchema.length()){
		// encode table root page
		tableSlice.Read(cur);
		// std::cout << "get table map: " << static_cast<std::string>((*tableSlice)[0]) << " - " << (*tableSlice)[1].get<uint32_t>() << std::endl;
		// std::cout << "encode table: " << static_cast<std::string>(tableSlice[0]) << "," << tableSlice[1].get<uint32_t>() << std::endl;
		database_.table_manifest.Insert(static_cast<std::string>(tableSlice[0]), make_shared<TableMetaData>(static_cast<std::string>(tableSlice[0]), tableSlice[1].get<uint32_t>()));
		LoadTable(static_cast<std::string>(tableSlice[0]));
	}
	database_.loaded = true;
	return Status::OK();
}
Status Engine::LoadTable(std::string name){
	TableMetaDataPtr pMeta = nullptr;
	Status status;
	database_.table_manifest.Get(name, pMeta);
	if(!pMeta)return Status::InvalidArgument("Table not found.");
	if(!manager.fileIsOpened(GetFileHandle(pMeta->table_root))){
		std::string fileName = "";
		database_.file_manifest.Get(GetFileHandle(pMeta->table_root), fileName);
		if(fileName.length() == 0)return Status::Corruption("Cannot fetch file path.");
		FileHandle hFile;
		status = manager.OpenFile(fileName, hFile);
		if(hFile != GetFileHandle(pMeta->table_root))return Status::Corruption("File handle doesn't match.");
	}
	PageRef rootRef(&manager, pMeta->table_root, kReadOnly);
	ManifestBlockHeader* tableRootHeader = reinterpret_cast<ManifestBlockHeader*>(rootRef.ptr);
	if(tableRootHeader->hBlockCode != kTableRoot)return Status::Corruption("Table root page corrupted.");
	// read schema map
	char* cur = rootRef.ptr + tableRootHeader->oManifest0;
	Slice fieldSlice = kTableSchemaManifestSchema.NewObject();
	for(int i = 0; i < tableRootHeader->nManifest0; i++, cur += fieldSlice.length()){
		fieldSlice.Read(cur);
		// index and unique
		// std::cout << fieldSlice[0].get<int8_t>() << ","<< static_cast<std::string>(fieldSlice[1].get<FixChar32>()) << "," << fieldSlice[2].get<int8_t>() << "," << fieldSlice[3].get<int8_t>() << std::endl;
		pMeta->schema.AppendField(
			Attribute( static_cast<std::string>(fieldSlice[1].get<FixChar32>()),\
				static_cast<TypeT>(fieldSlice[2].get<int8_t>()) ), \
			static_cast<int8_t>(fieldSlice[0].get<int8_t>()),\
			static_cast<bool>(fieldSlice[3].get<int8_t>()) );
	}
	// read index map
	Slice indexSlice = kTableIndexManifestSchema.NewObject();
	cur = rootRef.ptr + tableRootHeader->oManifest1;
	pMeta->bflow_root = 0;
	for(int i = 0; i < tableRootHeader->nManifest1; i++ , cur += indexSlice.length()){
		indexSlice.Read(cur);
		if(static_cast<int>(indexSlice[0].get<int8_t>()) == static_cast<int>(kBFlowIndex)){
			pMeta->bflow_root = indexSlice[3].get<uint32_t>();
		}
		// ERROR: set bflow index into schema
		else pMeta->schema.SetIndex( static_cast<size_t>(indexSlice[1].get<int8_t>()) , indexSlice[3].get<uint32_t>(), std::string(indexSlice[2].get<FixChar32>()));
	}
	pMeta->loaded = true;
	if(pMeta->bflow_root == 0)return Status::Corruption("Cannot find primary index handle.");
	return Status::OK();
}
Status Engine::DropDatabase(void){
	Status status;
	std::string table;
	database_.table_manifest.IterateClear();
	table = database_.table_manifest.IterateKeyNext();
	while(table.length() > 0){
		status = DropTable(table);
		if(!status.ok())std::cout << status.ToString() << std::endl;
		table = database_.table_manifest.IterateKeyNext();
	}
	// now drop database
	database_.loaded = false;
	status = manager.DeleteFile(database_.database_root);
	if(!status.ok())return status;
	database_.file_manifest.Clear();
	database_.table_manifest.Clear();
	return Status::OK();
}
Status Engine::DropTable(std::string name){
	TableMetaDataPtr pTable = nullptr;
	database_.table_manifest.Get(name, pTable);
	if(!pTable)return Status::InvalidArgument("Cannot find table meta data.");
	pTable->loaded = false;
	Status status;
	for(int i = 0; i < pTable->schema.attributeCount(); i++){
		PageHandle hIndex = pTable->schema.GetIndexHandle(i);
		if(hIndex == 0)continue;
		pTable->schema.SetIndex(i, 0); // delete index from mem first
		// open cursor
		cursor_.curIndex.Set(pTable->schema[i].type(), hIndex);
		auto tmp = cursor_.curIndex.DeleteAllPage();
		if(!tmp.ok())status = tmp;
	}
	if(!status.ok())return status;
	FileHandle hFile = GetFileHandle(pTable->table_root);
	status = manager.DeleteFile(hFile);
	if(!status.ok())return status;
	database_.table_manifest.Delete(name);
	database_.file_manifest.Delete(hFile);
	// delete from database root file
	PageRef databaseRootRef(&manager, GetPageHandle(database_.database_root, kDatabaseRootPageNum), kLazyModify);
	ManifestBlockHeader* dbRootHeader = reinterpret_cast<ManifestBlockHeader*>(databaseRootRef.ptr);
	if(dbRootHeader->hBlockCode != kDatabaseRoot)return Status::Corruption("Database root page corrupted.");
	char* cur = databaseRootRef.ptr + dbRootHeader->oManifest0;
	Slice fileSlice = kDatabaseFileManifestSchema.NewObject();
	for(int i = 0; i < dbRootHeader->nManifest0; i++,cur += kDatabaseFileManifestSchema.length()){
		// encode file manifest
		fileSlice.Read(cur);
		if(fileSlice[0].get<int8_t>() == hFile){
			for(int k = 0; k + i < dbRootHeader->nManifest0 -1; k++,cur+=kDatabaseFileManifestSchema.length() )
				memcpy(cur, cur+kDatabaseFileManifestSchema.length(), kDatabaseFileManifestSchema.length());
			dbRootHeader->nManifest0 --;
			break;
		}
	}
	cur = databaseRootRef.ptr + dbRootHeader->oManifest1;
	Slice tableSlice = kDatabaseTableManifestSchema.NewObject();
	for(int i = 0; i < dbRootHeader->nManifest1; i++, cur += kDatabaseTableManifestSchema.length()){
		// encode table root page
		tableSlice.Read(cur);
		if(static_cast<std::string>(tableSlice[0]) == name){
			for(int k = 0; k + i < dbRootHeader->nManifest1 -1; k++,cur+=kDatabaseTableManifestSchema.length() )
				memcpy(cur, cur+kDatabaseTableManifestSchema.length(), kDatabaseTableManifestSchema.length());
			dbRootHeader->nManifest1 --;
			break;
		}
	}
	return Status::OK();
}
Status Engine::DropIndex(std::string table, std::string name){ // index name not field name
	auto pTable = GetTable(table);
	if(!pTable)return Status::InvalidArgument("Cannot find table.");
	int index = pTable->schema.GetIndexByName(name);
	if(index < 0)return Status::InvalidArgument("Cannot find index on this table.");
	if(index == 0)return Status::InvalidArgument("Cannot drop primary key index");
	PageHandle hIndex = pTable->schema.GetIndexHandle(index);
	pTable->schema.SetIndex(index, 0); // delete index from mem first
	// open cursor
	cursor_.curIndex.Set(pTable->schema[index].type(), hIndex);
	return cursor_.curIndex.DeleteAllPage();
}
Status Engine::CloseDatabase(void){
	Status status;
	std::string table;
	database_.table_manifest.IterateClear();
	table = database_.table_manifest.IterateKeyNext();
	while(table.length() > 0){
		status = CloseTable(table);
		table = database_.table_manifest.IterateKeyNext();
	}
	// now close database
	database_.loaded = false;
	status = manager.CloseFile(database_.database_root);
	if(!status.ok())return status;
	database_.file_manifest.Clear();
	database_.table_manifest.Clear();
	return Status::OK();
}
Status Engine::CloseTable(std::string name){
	TableMetaDataPtr pTable = nullptr;
	database_.table_manifest.Get(name, pTable);
	if(!pTable)return Status::InvalidArgument("Cannot find table meta data.");
	pTable->loaded = false;
	FileHandle hFile = GetFileHandle(pTable->table_root);
	auto status = manager.CloseFile(hFile);
	if(!status.ok())return status;
	database_.table_manifest.Delete(name);
	return Status::OK();
}

Status Engine::MakeIndex(std::string table, std::string field, std::string name){
	auto pTable = GetTable(table);
	if(!pTable )return Status::InvalidArgument("Cannot find table.");
	int idxIndex = pTable->schema.GetAttributeIndex(field);
	if(idxIndex < 0)return Status::InvalidArgument("Cannot find field.");
	if(!pTable->schema.isUnique(idxIndex))return Status::InvalidArgument("Cannot build index on non-unique key.");
	PageHandle hIndex = pTable->schema.GetIndexHandle(idxIndex);
	if(hIndex != 0)return Status::InvalidArgument("Already have index on this field.");

	// build bplus index from database root
	// assert(all file needed is opened)
	PageHandle indexPage;
	manager.NewPage(database_.database_root, kBPlusPage, indexPage);
	if(indexPage == 0)return Status::IOError("Cannot create page for index.");
	Slice indexRecord(&kTableIndexManifestSchema,\
		{Value(tinyintT, new RealValue<int8_t>(kBPlusIndex)), \
		Value(tinyintT, new RealValue<int8_t>(idxIndex)),\
		Value(fixchar32T, new RealValue<FixChar32>(name)),\
		Value(uintT, new RealValue<uint32_t>(indexPage))	});
	// write header
	PageRef* indexRef = new PageRef(&manager, indexPage, kLazyModify);
	BPlusHeader* indexHeader = reinterpret_cast<BPlusHeader*>(indexRef->ptr); // ERROR
	// BPlusHeader* indexHeader = reinterpret_cast<BPlusHeader*>(indexRef->ptr + sizeof(BlockHeader));
	indexHeader->nSize = 0;
	indexHeader->hRight = 0;
	delete indexRef;
	// Set cursor // ERROR
	cursor_.pTable = pTable;
	// Open cursor
	TypeT indexType = pTable->schema.GetAttribute(idxIndex).type();
	cursor_.curIndex.Set(indexType, indexPage);
	// insert all slice to index
	cursor_.curMain.Set(&pTable->schema, pTable->bflow_root);
	PrepareSequencePrimary(nullptr, nullptr);
	SharedSlicePtr mainSlice = nullptr;
	PageHandle mainHandle;
	runtime_.keepReading = true;
	while(true){
		NextSlice(mainSlice, mainHandle);
		if(mainHandle == 0 || mainSlice == nullptr){
			NextHandle(mainHandle);
			NextSlice(mainSlice, mainHandle);
			if(mainHandle == 0 || mainSlice == nullptr)break;
		}
		Value key = mainSlice->GetValue(idxIndex);
		InsertIndex(idxIndex, &key, mainHandle);
	}
	cursor_.curIndex.Rewind();
	indexPage = cursor_.curIndex.currentHandle(); // ERROR
	indexRecord.SetValue(3, Value(uintT, new RealValue<uint32_t>(indexPage)) );
	// write to file
	PageRef tableRootRef(&manager, pTable->table_root, kLazyModify);
	ManifestBlockHeader* tableHeader = reinterpret_cast<ManifestBlockHeader*>(tableRootRef.ptr);
	// not safe
	char* cur = tableRootRef.ptr + tableHeader->oManifest1 + tableHeader->nManifest1 * kTableIndexManifestSchema.length();
	if(cur >= tableRootRef.ptr + kBlockLen)return Status::Corruption("Not enough space in table manifest.");
	indexRecord.Write(cur);
	tableHeader->nManifest1 ++;
	// alter in memory data
	cursor_.pTable->schema.SetIndex(idxIndex, indexPage, name);

	return Status::OK();
} 

} // namespace sbase