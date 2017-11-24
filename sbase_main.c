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
	printf("sBase 0.0.1 (Nov 24 2017, 11:40) on win32.\n");

	// meta command
	printf("Type .exit to exit prompt interface.\n");
	printf("Type .help, .copyright, .credits, .license for more information.\n");
	return;
}

void printPrompt(void){
	printf(">>> ");
}



typedef struct INPUT_t{
	char** tokens;
	int size;
	int capacity;
}* inputBuffer;

#define INIT_BUFFER_s		(20)

inputBuffer newInputBuffer(void){
	inputBuffer new=New(struct INPUT_t,1);
	new->tokens=New(char*, INIT_BUFFER_s);
	new->size=0;
	new->capacity=INIT_BUFFER_s;
	
	return new;
}

/*
typedef struct BUFFER_t{
	char* buffer;
	int capacity;
}* BUFFER;

typedef enum{EXPAND_BUFFER_SUCCESS, EXPAND_BUFFER_FAILURE} expandBufferResult;

expandBufferResult expandBuffer(BUFFER buffer_like_obj){
	char* previous_buffer=buffer_like_obj->buffer;
	buffer_like_obj->buffer=New(char, buffer_like_obj->capacity*2+1);
	buffer_like_obj->capacity*=2;

	memcpy(buffer_like_obj->buffer, previous_buffer, sizeof());

	return EXPAND_BUFFER_SUCCESS;
}
*/
#define CMD_END_CHR		";"
void readInput(inputBuffer input){
	char first_buffer[INIT_BUFFER_s+1]={0};
	int first_size=INIT_BUFFER_s;
	char second_buffer[INIT_BUFFER_s+1]={0};
	int second_size=INIT_BUFFER_s;
	// if havenot read end
	while(strlen(first_buffer)==strcspn(first_buffer, CMD_END_CHR)){
		Initial(first_buffer, 0, first_size);
		scanf_s("%s",first_buffer,first_size);
		while(strlen(first_buffer)+strlen(second_buffer)>second_size){
			char* tp=second_buffer;
			second_buffer=New(char,second_size*2+1);
			Initial(second_buffer, 0, second_size*2+1);
			second_size*=2;
			memcpy(second_buffer, tp, sizeof(char)*strlen(tp));
		}
		strcat(second_buffer, first_buffer);
	}

	// tokenize
	toTokens(input, second_buffer);
	return input;
}

#define SPLITER_CHR		" \n"
int toTokens(inputBuffer input, char* string){
	char* buf;
	while((buf=strtok(string, SPLITER_CHR))!=NULL){
		while(input->size>=input->capacity){
			char** tp=input->tokens;
			input->tokens=New(char*, input->capacity*2);
			input->capacity*=2;
			Initial(input->tokens, NULL, input->capacity);
			memcpy(input->tokens, tp, sizeof(char*)*input->size);
		}
		input->tokens[input->size++]=buf;
	}

}

void clearInput(inputBuffer input){
	free(input->tokens);
	input->tokens=New(char*, INIT_BUFFER_s);
	input->size=0;
	input->capacity=INIT_BUFFER_s;
	return;
}

// Now Supported Command:
// use {dbname}
// drop {dbname}
// create
// insert into {table_name} ({field}) ({values})
// select * from {table_name} where {description}
int loadCommand(inputBuffer input){}

typedef enum{META_COMMAND_SUCCESS, META_COMMAND_NOT_FOUND} metaComandResult;
metaComandResult runMetaCommand(char* cmdString){

}

int main(int argc, char* argv[]){

	// print database system log
	putSystemLog();

	inputBuffer input=newInputBuffer();

	// enter prompt loop
	while(true){
		printPrompt();
		readInput(input);
		if(input->tokens[0][0]=='.'){
			metaCommand(input->tokens);
		}
		else switch(){

		}
		switch(fetchDatabase()){
			case(DB_NOT_ACCESSIBLE):Error(DB_NOT_ACCESSIBLE);
			case(DB_ACCESSIBLE):continue;
		}
		clearInput(input);
	}
	

	return 0;
}