#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

/*
	\template
	// @hang
	DICT
		name->hash->constrained hash
		hash->value
		API: insert / fetch
	ENTRY
		meta_data:
			row_name to struct_offset
		content
	\db_structure
	DATABASE:
		db_name(string)
		tables(dict)->array
	TABLE: 
		table_name(string)
		row_name(dict)->array
			and row meta data
		entries(entry list)
			tuple return addr
			addr return tuple
		index(index list)
	INDEX:
		index_tree
*/




/*
typedef int hashable_t;
hashable_t (*makeHashable)(key_type);
// receive byte size for stored value, and hash function for generating the key
int initialDict(int size, int( *hash)(hashable_t key)){

}
*/
typedef struct INDEX_META_t{

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
#define ENTRY_BLOCK_SIZE		(20)
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
}* db;

typedef DB_ARRAY_t{
	int num;
	db* databases;
}* dbs_t;

#define MAX_DB_SIZE		(3)
dbs_t* GLOBAL_DATABASE_ARRAY_[MAX_DB_SIZE]={0};
/*
	\brief Manager for direct command to db.
*/





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
createDatabaseResult createDatabase(char* dbname){

}

typedef enum{OPEN_DB_SUCCESS, OPEN_DB_NOT_FOUND} openCommandResult;
openCommandResult openDatabase(char* dbname){

}

