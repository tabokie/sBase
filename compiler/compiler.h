#ifndef SBASE_COMPILER_COMPILER_HPP_
#define SBASE_COMPILER_COMPILER_HPP_
#include "./compiler/scanner.hpp"
#include "./compiler/parser.hpp"
#include "./db/slice.hpp"
#include "./storage/file_format.hpp"
#include "./db/engine.h"
#include "./util/hash.hpp"
#include "./util/error.hpp"
#include <sstream>
#include <functional>
#include <iomanip>
#include <memory>
const int kDisplayWidth = 12;
using namespace std::placeholders;

namespace sbase{

enum ByteCode{
	// kCreateDatabase, // path
	// kLoadDatabase, // path
	// kDropDatabase, // -
	// kCloseDatabase, // -
	// kCreateTable, // schema
	// kLoadTable, // tablename
	// kDropTable, // tablename
	// kCloseTable, // tablename
	kMakeIndex, // tablename, fieldname(not shared)
	kTransaction,
	kOpenRecordCursor, // tablename
	kOpenIndexCursor, // tablename, fieldname(not shared)
	kPrepareMatch, // value(not shared)
	kPrepareSequence, // value*2(not shared)
	kNextSlice, // return ptr
	kIf, // filter, assert not nil
	kIfNot,
	kIfNil,
	kJump, // need index, relative offset
	kInsertSlice, // slice at runtime(need index)
	kDeleteSlice, // slice at runtime
	kPrintSlice // fields, slice at runtime
};

struct Instruction{
	ByteCode code;
	std::string str_holder;
	std::shared_ptr<Value> value_holder_1 = nullptr;
	std::shared_ptr<Value> value_holder_2 = nullptr;
	bool left;
	bool right;
	int offset;
	Instruction(ByteCode _code):code(_code){ }
	Instruction(ByteCode _code, std::string str):code(_code),str_holder(str){ }
	Instruction(ByteCode _code, std::shared_ptr<Value> value):code(_code),value_holder_1(value){ }
	Instruction(ByteCode _code, std::shared_ptr<Value> value1, std::shared_ptr<Value> value2, bool a, bool b):
		code(_code),value_holder_1(value1),value_holder_2(value2), left(a),right(b){ }
	Instruction(ByteCode _code, int index):code(_code),offset(index){ }
	~Instruction(){ }
};


// Util Meta Function //
bool sql_string_match(std::string a, std::string b);
bool SliceFilter_like(const Slice& slice, int index, std::string format, bool inverse);
bool SliceFilter_eq(const Slice& slice, int index, std::string rhs, bool inverse);
bool SliceFilter_gt(const Slice& slice, int index, std::string rhs, bool inverse);
bool SliceFilter_lt(const Slice& slice, int index, std::string rhs, bool inverse);

using SliceFilterType = std::function<bool(const Slice&)>;

class Compiler{
	// front pass
	Scanner scanner_;
	Parser parser_;
	std::vector<Token> tokens_;
	// decode pass
	struct Resource{
		std::string database = kDatabaseRootPath;
		std::vector<std::string> tables; // though only use one table here
		Schema* schema;
		std::vector<std::string> displayColumns;
		bool headerDisplayed;
		// shared used for insertion and query
		std::vector<Slice> slices;
		std::shared_ptr<Slice> shared_slice;
		// used for iteration filter
		std::vector<SliceFilterType> filters;
		Resource():schema(nullptr), shared_slice(nullptr),headerDisplayed(false){ }
		void Clear(){
			tables.clear();
			schema = nullptr;
			shared_slice = nullptr;
			displayColumns.clear();
			headerDisplayed = false;
			filters.clear();
			slices.clear();
		}
	} resource_;
	// optimize pass
	Engine engine;
	std::vector<Instruction> bytecodes_;
	// run pass
	std::vector<Slice> slices_;
 public:
 	Compiler(){ }
 	~Compiler(){ }
 	// Interface //
 	void RunInterface(istream& is, ostream& os, bool prompt = true);
 private:
 	// for interface
 	inline void ClearPreviousPass(void){
 		if(!engine.CheckDatabase()){
 			auto status = engine.LoadDatabase(resource_.database);
 			if(status.IsIOError()){
 				ErrorLog::Warning(status.ToString().c_str());
 				// std::cout << status.ToString() << std::endl;
 				status = engine.CreateDatabase(resource_.database);
 			}
 			if(!status.ok()){
 				ErrorLog::Fatal(status.ToString().c_str());
 				std::cout << status.ToString() << std::endl;
 			}
 		}
 		tokens_.clear();
 		parser_.Clear();
 		bytecodes_.clear();
 		resource_.Clear();
 		return ;
 	}
 	inline void Terminate(void){
 		if(engine.CheckDatabase()){
 			auto status = engine.CloseDatabase();
 			if(!status.ok()){
 				ErrorLog::Fatal(status.ToString().c_str());
 			}
 		}
 		return ;
 	}
 	// decode pass
 	inline Status Compile(void){
 		// if(!engine)return Status::Corruption("Error connecting to database engine.");
 		auto status = parser_.ValidateStatus();
 		if(!status.ok())return status;
 		// to byte code
 		auto deduced = parser_.SymbolBegin();
 		if(deduced->using_rule == 0){
 			return CompileSelect();
 		}
 		else if(deduced->using_rule == 1){
 			return CompileInsert();
 		}
 		else if(deduced->using_rule == 2){
 			return CompileCreate();
 		}
 		else if(deduced->using_rule == 3){
 			return CompileDelete();
 		}
 		else if(deduced->using_rule == 4){
 			deduced += 3;
 			if(deduced >= parser_.SymbolEnd())return Status::Corruption("Cannot resolve drop clause.");
 			if(deduced->symbol == kSymbolDict["INDEX"])return Status::OK();
 			if(deduced->symbol == kSymbolDict["TABLE"] && deduced+1<parser_.SymbolEnd())return engine.DropTable((deduced+1)->text);
 			return Status::Corruption("Cannot resolve drop clause.");
 		}
 		else if(deduced->using_rule == 5){
 			auto end = parser_.SymbolEnd();
 			deduced += 4;
 			if(deduced >= end)return Status::Corruption("Cannot resolve index clause.");
 			std::string name = deduced->text;
 			deduced += 2;
 			if(deduced >= end)return Status::Corruption("Cannot resolve index clause.");
 			std::string table = deduced->text;
 			deduced += 2;
 			if(deduced >= end)return Status::Corruption("Cannot resolve index clause.");
 			std::string field = deduced->text;
 			return engine.MakeIndex(table, field, name);
 		}
 		return Status::Corruption("Unresolved type of clause.");
 	}
 	Status CompileSelect(void);
 	Status CompileInsert(void);
 	Status CompileCreate(void);
 	Status CompileDelete(void);
 	// execute
 	Status RunInstructions(ostream& os);
 	// auxilary
 	Status ParseSimpleExpr(std::vector<DeducedSymbol>::iterator& begin, std::string& ret, bool& isText);
 	Status ParseWhereClause(std::vector<DeducedSymbol>::iterator& begin);
 	bool CheckSlice(void);


}; // class Compiler


} // namespace sbase

#endif // SBASE_COMPILER_COMPILER_HPP_


