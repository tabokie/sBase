#include "bplus.h"

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

typedef struct INDEX_META_t{
	int row_id;// test
}* index_meta_t;

#define PAGE_SIZE		(100)
#define TOP_MAX			(PAGE_SIZE/offset)
typedef struct PAGER_t{
	// INID -> page_no and relative addr
	void* base_addr;
	int offset;
	int top;// initial as 0
}* pager_t;

typedef char* table_name_t;
typedef char* row_name_t;

typedef struct ROW_META_t{
	row_name_t row_name;
	int offset;
}* row_meta_t;

typedef struct DATA_TABLE_t{
	// meta data
	int table_id;
	table_name_t table_name;
	int row_num;
	row_meta_t* row_meta;// row_id to row_name and row offset

	// content part
	// give INID and row_offset to fetch single data
	// give insert content and get INID
	int page_num;
	int capacity;
	pager_t* pages;

	// index part
	int index_num;
	index_meta_t* index_meta;
	index_tree* indexs;
}* table_meta_t;

typedef int table_id_t ;
typedef struct DATABASE_t{
	char* dbname;
	int table_num;
	char** table_name;
	table_id_t* table_id;
	table_meta_t* table_;
}* database_t;

typedef struct DB_MANAGER_t{
	int num;
	char** dbnames;
	database_t* databases;
}* manager_t;

/*
	First Abstraction: Database
	Including: table
*/
// initial preset database
// current: NULL
#ifndef MAX_DB_SIZE
#define MAX_DB_SIZE		(3)
#endif

#define INITIAL_DBS		(manager_t)(NULL);
//manager_t initialDBs(void);
manager_t GLOBAL_DATABASE_ARRAY_=INITIAL_DBS;

// switch workspace
database_t GLOBAL_DATABASE_POINTER_=NULL;
void setCurrentDatabase(database_t p);
void resetCurrentDatabase(void);

// check operation legelity
typedef enum{DB_ACCESSIBLE, DB_NOT_ACCESSIBLE} fetchDatabaseResult;
fetchDatabaseResult fetchDatabase(void);

// command: create {dbname}
typedef enum{CREATE_DB_SUCCESS, CREATE_DB_FULL, CREATE_DB_FAILURE} createDatabaseResult;
createDatabaseResult createDatabase(char* dbname);
// appended function
database_t newDatabase(char* dbname);

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
typedef enum{INSERT_TO_TABLE_SUCCESS, INSERT_TO_TABLE_NOT_FOUND, INSERT_TO_TABLE_FAILURE} insertToTableResult; 
insertToTableResult insertToTable(char* tableName, uncertain_type data){
	char* formatting_data = pickle(data);
	table_meta_t target_table = getTable(char* tableName);
	int i,j;
	for(i=target_table->page_num-1;i>=0;i--){
		if(target_table->pages[i]->top<TOP_MAX)break;
	}
	if(i<0){
		if(target_table->page_num==target_table->capacity){// preset space full
			page_t* new=(pager_t*)malloc(sizeof(pager_t)*target_table->capacity*2);
			for(j=0;j<target_table->capacity;j++){
				new[j]=target_table->pages[j];
			}
			free(target_table->pages);
			target_table->pages=new;
			target_table->capacity*=2;
		}
		target_table->pages[target_table->page_num++]=newPage(offset);
		i=target_table->page_num-1;
	}

	addr_type inid = insertToPage(target_table->pages[i], formatting_data);
	for row_id: 
		insertToIndex(,inid)
}

 typedef enum{SELECT_FROM_TABLE_SUCCESS, SELECT_FROM_TABLE_NOT_FOUND, SELECT_FROM_TABLE_FAILURE} selectFromTableResult;
 selectFromTableResult selectFromTable(char* tableName;){
 	
 }


addr_type insertToPage(pager_t target, char* data){
	if(target->top>=TOP_MAX)return NULL;
	addr_type insertPoint = target->base_addr+target->offset*top;
	memcpy(insertPoint, data);
	target->top++;
	return insertPoint;
}

#endif	/*	DB_MANAGER_H_	*/