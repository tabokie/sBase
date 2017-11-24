#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

/*
	\brief Manager for direct command to db.
*/
db GLOBAL_DATABASE_POINTER_=NULL;
void setCurrentDatabase(){
	GLOBAL_DATABASE_POINTER_ = ;
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


typedef enum{OPEN_DB_SUCCESS, OPEN_DB_NOT_FOUND} openCommandResult;
openCommandResult openDatabase(char* dbname){

}

