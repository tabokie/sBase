#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "b_macro.h"
//#include "bplus.h"
//#include "db_manager.h"

void putSystemLog(void);
void printPrompt(void);
void printUnioned(void);
/*
	\ Read input stream and tokenize.
*/
typedef struct INPUT_t{
	char** tokens;
	int size;
	int capacity;
}* inputBuffer;
#define INIT_BUFFER_s		(20)
inputBuffer newInputBuffer(void);
void clearInput(inputBuffer input);
#define CMD_END_CHR		";"
void readInput(inputBuffer input);
#define SPLITER_CHR		" \n"
void toTokens(inputBuffer input, char* string);
/*
	\brief Process meta command and pass db operating command to db manager
*/

/*
	\brief Interface Function Class
*/
void putSystemLog(void){
	// print version
	printf("sBase 0.0.1 | default | (Nov 24 2017, 11:40) on win32.\n");

	// meta command guidance
	printf("Type \".exit;\" to exit prompt interface.\n");
	printf("Type \".help;\", \".copyright;\", \".credits;\", \".license;\" for more information.\n");
	return;
}
void printPrompt(void){
	printf(">>> ");
}
void printUnioned(void){
	printf("... ");
}



inputBuffer newInputBuffer(void){
	inputBuffer new=New(struct INPUT_t,1);
	new->tokens=New(char*, INIT_BUFFER_s);
	new->size=0;
	new->capacity=INIT_BUFFER_s;
	
	return new;
}
void clearInput(inputBuffer input){
	free(input->tokens);
	input->tokens=New(char*, INIT_BUFFER_s);
	input->size=0;
	input->capacity=INIT_BUFFER_s;
	return;
}

void readInput(inputBuffer input){
	char first_buffer;
	char* second_buffer=New(char, INIT_BUFFER_s+1);
	Initial(second_buffer, 0, INIT_BUFFER_s+1);
	int second_size=INIT_BUFFER_s;
	int second_end=0;
	int clause_end=0;
	//
	while((first_buffer=getchar())!=-1){

		if(strlen(second_buffer)>=second_size){
			char* tp=second_buffer;
			second_buffer=New(char,second_size*2+1);
			Initial(second_buffer, 0, second_size*2+1);
			second_size*=2;
			memcpy(second_buffer, tp, sizeof(char)*(strlen(tp)+1));
			free(tp);
		}
		second_buffer[second_end++]=first_buffer;
		second_buffer[second_end]='\0';
		if(strchr(CMD_END_CHR, first_buffer)!=NULL){
			clause_end=1;
		}
		if(clause_end&&first_buffer=='\n')break;
		else if(first_buffer=='\n')printUnioned();
	}
	
	// tokenize
	toTokens(input, second_buffer);
	free(second_buffer);
	return;
}

#define SPLITER_CHR		" \n"
void toTokens(inputBuffer input, char* string){
	char* buf = strtok(string, SPLITER_CHR); // convert spliter to \0
	if(buf==NULL)return;
	do{
		while(input->size>=input->capacity){
			char** tp=input->tokens;
			input->tokens=New(char*, input->capacity*2);
			input->capacity*=2;
			Initial(input->tokens, NULL, input->capacity);
			memcpy(input->tokens, tp, sizeof(char*)*input->size);
			free(tp);
		}
		input->tokens[input->size++]=buf;
	}while((buf=strtok(NULL, SPLITER_CHR))!=NULL);
	return;
}

/*
	\ Parser: Do classification, and produce pickled paramaters for db manager.
*/

// Now Supported Database Command:
// use {dbname}
//	-> send message to openDB
// drop {dbname}
//	-> send message to resetDB
// create {dbname}/table{rowname, rowtype}/ref(rowname)
//	-> send message to createTable()
// insert into {table_name} {indexname, value}
//	-> construct a entry object
//	-> send message to insertDB()
// select * from {table_name} where {description}
//	-> construct syntax tree for description and construct lambda function 
//	-> send message to select(table, functionPointerForIndex, functionPointerForValue)
//	-> and store the result in database manager buffer
//	-> call print data routine with pointer to manager buffer
int loadCommand(inputBuffer input){}

typedef enum{META_COMMAND_SUCCESS, META_COMMAND_NOT_FOUND} metaCommandResult;
metaCommandResult runMetaCommand(char* cmdString){

}

int main(int argc, char* argv[]){

	// print database system log
	putSystemLog();

	inputBuffer input=newInputBuffer();

	// enter prompt loop
	while(true){
		printPrompt();
		readInput(input);
		/*
		if(input->tokens[0][0]=='.'){
			metaCommand(input->tokens);
		}
		else switch(){

		}
		switch(fetchDatabase()){
			case(DB_NOT_ACCESSIBLE):Error(DB_NOT_ACCESSIBLE);
			case(DB_ACCESSIBLE):continue;
		}
		*/
		clearInput(input);
	}
	

	return 0;
}