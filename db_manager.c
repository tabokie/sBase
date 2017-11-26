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
}* db_t;

typedef DB_ARRAY_t{
	int num;
	char** dbnames;
	db_t* databases;
}* dbs_t;

#define MAX_DB_SIZE		(3)
dbs_t GLOBAL_DATABASE_ARRAY_=initialDBs();
dbs_t initialDBs(void){
	dbs_t new=New(struct DB_ARRAY_t,1);
	new->num=0;
	new->dbname=New(char*, MAX_DB_SIZE);
	new->databases=New(struct DATABASE_t,MAX_DB_SIZE);
	return new;
}

/*
	\brief Manager for direct command to db.
*/

db_t GLOBAL_DATABASE_POINTER_=NULL;
void setCurrentDatabase(db_t p){
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

typedef enum{CREATE_DB_SUCCESS, CREATE_DB_FULL, CREATE_DB_FAILURE} createDatabaseResult;
createDatabaseResult createDatabase(char* dbname){
	if(GLOBAL_DATABASE_ARRAY_->num>=MAX_DB_SIZE)return CREATE_DB_FULL;
	else if(GLOBAL_DATABASE_ARRAY_->databases[GLOBAL_DATABASE_ARRAY_->num]=newDatabase(dbname)==NULL){
		return CREATE_DB_FAILURE;
	}
	GLOBAL_DATABASE_ARRAY_->dbnames[GLOBAL_DATABASE_ARRAY_->num]=dbname;
	GLOBAL_DATABASE_ARRAY_->num++;
	return CREATE_DB_SUCCESS;
}

db_t newDatabase(char* dbname){
	if(strlen(dbname)==0)return NULL;
	else{
		db_t new=New(struct DATABASE_t,1);
		new->dbname=dbname;
		new->table_num=0;
		new->table_name=NULL;
		new->table_id=NULL;
		new->table_meta=NULL;
		return new;
	}
}

typedef enum{OPEN_DB_SUCCESS, OPEN_DB_NOT_FOUND} openCommandResult;
openCommandResult openDatabase(char* dbname){
	int i;
	For(i,0,GLOBAL_DATABASE_ARRAY_->num){
		if(strcmp(GLOBAL_DATABASE_ARRAY_->dbnames[i],dbname)==0){
			setCurrentDatabase(GLOBAL_DATABASE_ARRAY_->databases[i]);
			break;
		}
	}
	if(i>=GLOBAL_DATABASE_ARRAY_->num)return OPEN_DB_NOT_FOUND;
	return OPEN_DB_SUCCESS;
}

