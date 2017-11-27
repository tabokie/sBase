#include "bplus.h"

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

typedef struct INDEX_META_t{
	int row_id;// test
}* index_meta_t;

typedef struct PAGER_t{
	// INID -> page_no and relative addr
	void* base_addr;
	int offset;
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
insertToTableResult insertToTable(char* tableName, ){

}

 typedef enum{SELECT_FROM_TABLE_SUCCESS, SELECT_FROM_TABLE_NOT_FOUND, SELECT_FROM_TABLE_FAILURE} selectFromTableResult;
 selectFromTableResult selectFromTable(char* tableName;){
 	
 }



#endif	/*	DB_MANAGER_H_	*/