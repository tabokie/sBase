#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "b_macro.h"
//#include "bplus.h"
#include "db_manager.h"

/*
	\brief Process meta command and pass db operating command to db manager
*/

void putSystemLog(void){
	// print version
	printf("sBase 0.0.1 (Nov 24 2017, 11:40) on win32\n");

	// meta command
	printf("Type .exit to exit prompt interface\n");
	printf("Type .help, .copyright, .credits, .license for more information\n");
	return;
}

void printPrompt(void){
	printf(">>>");
}


typedef enum{META_COMMAND_SUCCESS, META_COMMAND_NOT_FOUND} metaComandResult;


int main(int argc, char* argv[]){

	// print database system log
	putSystemLog();

	inputBuffer* input=newInputBuffer();

	while(true){
		printPrompt();
		readInput(input);
		if(input->buffer[0]=='.'){
			metaCommand(input->buffer);
		}
		else switch(){

		}
		switch(fetchDatabase()){
			case(DB_NOT_ACCESSIBLE):Error(DB_NOT_ACCESSIBLE);
			case(DB_ACCESSIBLE):continue;
		}
	}

	// get database name
	char* dbname=argv[1];
	switch(openDatabase(dbname)){
		case(OPEN_DB_SUCCESS): continue;
		case(OPEN_DB_NOT_FOUND): Error(Database not found!);
	}

	// enter prompt loop
	while(true){

	}

	return 0;
}