#ifndef SBASE_DB_INSTRUCTION_HPP_
#define SBASE_DB_INSTRUCTION_HPP_

/*
OpenDatabase
OpenCursor TableName Index
Rewind
ReadSlice Min Max -> Slice
GetKey -> Value
IfNot/If
DeleteSlice
MakeRecord -> Value
Jump
*/
enum Opcode{
	kOpenDatabase,
	kOpenCursor,

};

struct Instruction{
	Opcode code;
	
}


#endif // SBASE_DB_INSTRUCTION_HPP_