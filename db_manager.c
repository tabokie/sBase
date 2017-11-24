#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

/*
	\brief Manager for direct command to db.
*/

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
	int capacity;
	entry_type* entries;
}* mem_block;

typedef struct DATA_TABLE_t{
	int block_size;
	addr_type* base_addr;// base addr for each memory blocks
	mem_block* entries;// flexible memory blocks, easy to copy
}* data_table;

typedef struct DATABASE_t{
	char* dbname;
	index_tree ref;
	data_table data;
}* db;


db GLOBAL_DATABASE_POINTER_=NULL;
void setCurrentDatabase(db p){
	GLOBAL_DATABASE_POINTER_ = p;
	return;
}
void resetCurrentDatabase(){
	GLOBAL_DATABASE_POINTER_=NULL;
	return;
}


typedef enum{DB_ACCESSIBLE, DB_NOT_ACCESSIBLE} databaseFetchResult;
databaseFetchResult fetchDatabase(void){
	if(GLOBAL_DATABASE_POINTER_==NULL)return DB_NOT_ACCESSIBLE;
	else return DB_ACCESSIBLE;
}

typedef enum{CREATE_DB_SUCCESS, CREATE_DB_FAILURE} createDatabaseResult;
createDatabaseResult createDatabase(char* dbname,)

typedef enum{OPEN_DB_SUCCESS, OPEN_DB_NOT_FOUND} openCommandResult;
openCommandResult openDatabase(char* dbname){

}

