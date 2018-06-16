#ifndef SBASE_COMPILER_COMPILER_HPP_
#define SBASE_COMPILER_COMPILER_HPP_
#include "./compiler/scanner.hpp"
#include "./compiler/parser.hpp"
#include "./db/slice.hpp"
#include "./db/engine.h"
#include "./util/hash.hpp"
#include <sstream>
#include <functional>
using namespace std::placeholders;

namespace sbase{

enum ByteCode{
	kCreateDatabase, // path
	kLoadDatabase, // path
	kDropDatabase, // -
	kCloseDatabase, // -
	kCreateTable, // schema
	kLoadTable, // tablename
	kDropTable, // tablename
	kCloseTable, // tablename
	kMakeIndex, // tablename, fieldname
	kTransaction,
	kOpenRecordCursor, // tablename
	kOpenIndexCursor, // tablename, fieldname
	kPrepareMatch, // value
	kPrepareSequence, // value*2
	kNextSlice, // return ptr
	kIf, // filter
	kInsertSlice, // slice at runtime
	kDeleteSlice, // slice at runtime
	kPrintSlice // fields, slice at runtime
};


// Util Meta Function //
bool sql_string_match(std::string a, std::string b);
bool SliceFilter_like(const Slice* slice, int index, std::string format, bool inverse);
bool SliceFilter_eq(const Slice* slice, int index, std::string rhs, bool inverse);
bool SliceFilter_gt(const Slice* slice, int index, std::string rhs, bool inverse);
bool SliceFilter_lt(const Slice* slice, int index, std::string rhs, bool inverse);

using SliceFilterType = std::function<bool(const Slice*)>;

class Compiler{
	// front pass
	Scanner scanner_;
	Parser parser_;
	std::vector<Token> tokens_;
	// decode pass
	struct Resource{
		std::string database;
		std::vector<std::string> tables; // though only use one table here
		Schema* schema;
		std::vector<std::string> names; // build index
		std::vector<std::string> columns; // display or build index
		// used for insertion and query
		std::vector<Slice*> slices;
		// used for main query
		Value* min;
		Value* max;
		std::string queryField;
		// used for iteration filter
		std::vector<SliceFilterType> filters;
		Resource():schema(nullptr),min(nullptr),max(nullptr){ }
		void Clear(){
			database = "";
			tables.clear();
			schema = nullptr;
			columns.clear();
			filters.clear();
			names.clear();
			slices.clear();
		}
	} resource_;
	// optimize pass
	Engine engine;
	std::vector<ByteCode> bytecodes_;
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
 		tokens_.clear();
 		parser_.Clear();
 		bytecodes_.clear();
 		resource_.Clear();
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
 		return Status::Corruption("Unresolved type of clause.");
 	}
 	Status CompileSelect(void);
 	Status CompileInsert(void);
 	Status CompileCreate(void);
 	Status CompileDelete(void);
 	// auxilary
 	Status ParseSimpleExpr(std::vector<DeducedSymbol>::iterator& begin, std::string& ret, bool& isText);
 	Status ParseWhereClause(std::vector<DeducedSymbol>::iterator& begin);

}; // class Compiler


} // namespace sbase

#endif // SBASE_COMPILER_COMPILER_HPP_


