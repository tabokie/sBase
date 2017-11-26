#include "bplus.h"

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

typedef struct INDEX_META_t{
	int a;// test
}* index_meta_t;

// first implementation: one index to two value
typedef struct VALUE_TYPE_t{
	int value1;
	int value2;
}value_type;

typedef struct ENTRY_t{
	index_type primary_index;
	value_type value;
}entry_type;

typedef struct MEMORY_BLOCK_t{
	int size;
	entry_type* entries;
}* mem_block;

typedef char* table_name_t;
typedef char* row_name_t;
#ifndef ENTRY_BLOCK_SIZE
#define ENTRY_BLOCK_SIZE		(20)
#endif
typedef struct DATA_TABLE_t{
	// meta data
	table_id_t table_id;
	table_name_t table_name;
	int row_num;
	row_name_t* row_names;

	// content part
	addr_type* base_addr;// base addr for each memory blocks
	mem_block* entry_block;// flexible memory blocks, easy to copy

	// index part
	index_meta_t index_meta;
	index_tree* indexs;
}* data_table_t;

typedef int table_id_t ;
typedef data_table_t table_meta_t;
typedef struct DATABASE_t{
	char* dbname;
	int table_num;
	char** table_name;
	table_id_t* table_id;
	table_meta_t* table_meta;
}* db_t;

typedef struct DB_ARRAY_t{
	int num;
	char** dbnames;
	db_t* databases;
}* dbs_t;

/*
	First Abstraction: Database
	Including: table
*/
// initial preset database
// current: NULL
#ifndef MAX_DB_SIZE
#define MAX_DB_SIZE		(3)
#endif
dbs_t GLOBAL_DATABASE_ARRAY_=initialDBs();
dbs_t initialDBs(void);

// switch workspace
db_t GLOBAL_DATABASE_POINTER_=NULL;
void setCurrentDatabase(db_t p);
void resetCurrentDatabase(void);

// check operation legelity
typedef enum{DB_ACCESSIBLE, DB_NOT_ACCESSIBLE} fetchDatabaseResult;
fetchDatabaseResult fetchDatabase(void);

// command: create {dbname}
typedef enum{CREATE_DB_SUCCESS, CREATE_DB_FULL, CREATE_DB_FAILURE} createDatabaseResult;
createDatabaseResult createDatabase(char* dbname);
// appended function
db_t newDatabase(char* dbname);

// command use {dbname}
typedef enum{OPEN_DB_SUCCESS, OPEN_DB_NOT_FOUND} openDatabaseResult;
openDatabaseResult openDatabase(char* dbname);

// command drop {dbname}
typedef enum{DROP_DATABASE_SUCCESS, DROP_NO_DATABASE} dropDatabaseResult;
dropDatabaseResult dropDatabase(void){
	switch(fetchDatabase()){
		case DB_ACCESSIBLE:{
			resetCurrentDatabase();
			return DROP_DATABASE_SUCCESS;
		}
		case DB_NOT_ACCESSIBLE:
			return DROP_NO_DATABASE;
	}
}


/*
	Second Abstraction: Table
	including: index_trees, entries, I/O methods
*/




#endif	/*	DB_MANAGER_H_	*/