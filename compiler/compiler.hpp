#ifndef SBASE_COMPILER_COMPILER_HPP_
#define SBASE_COMPILER_COMPILER_HPP_
#include "./compiler/scanner.hpp"
#include "./compiler/parser.hpp"
#include "./db/slice.hpp"
// #include "./db/engine.h"
#include "./util/hash.hpp"
#include <sstream>
#include <functional>
using namespace std::placeholders;

namespace sbase{

// Util Meta Function //
bool sql_string_match(std::string a, std::string b){
	std::istringstream tmp(b);
	std::string token;
	size_t idx;
	size_t cur = 0;
	while(getline(tmp,token,'%')){
		idx = a.find(token, cur);
		if(idx != std::string::npos && idx < a.length()){
			cur = idx;
		}
		else return false;
	}
	return true;
}
bool SliceFilter_like(const Slice* slice, int index, std::string format, bool inverse){
	auto value = slice->GetValue(index);
	if(value.type() < fixchar8T)return false;
	std::string strVal = std::string(value);
	return sql_string_match(strVal, format)^inverse;
}
bool SliceFilter_eq(const Slice* slice, int index, std::string rhs, bool inverse){
	auto value = slice->GetValue(index);
	Value rhsVal(value.type(), rhs);
	return (value==rhsVal)^inverse;
}
bool SliceFilter_gt(const Slice* slice, int index, std::string rhs, bool inverse){
	auto value = slice->GetValue(index);
	Value rhsVal(value.type(), rhs);
	return (value>rhsVal)^inverse;
}
bool SliceFilter_lt(const Slice* slice, int index, std::string rhs, bool inverse){
	auto value = slice->GetValue(index);
	Value rhsVal(value.type(), rhs);
	return (value<rhsVal)^inverse;
}

using SliceFilterType = std::function<bool(const Slice*)>;

class Compiler{
	Scanner scanner_;
	Parser parser_;
	std::vector<Token> tokens_;
	// std::vector<ByteCode> bytecodes_;
	// for interaction with db
	std::vector<std::string> tables_;
	std::vector<std::string> columns_;
	std::vector<SliceFilterType> filters_;
	// std::vector<Slice> slices_;
	// reference to engine
	// Engine* engine;
 public:

 	void RunInterface(istream& is, ostream& os){

 		std::string buffer;
 		std::string sentence;
 		ClearPreviousPass();
		while(true){
			os << ">> ";
			getline(is, buffer);
			if(buffer.find("\\q") < std::string::npos){
				os << "Bye";
				break;
			}
			if(buffer.find(";") < std::string::npos){ // one full sentence
				sentence += buffer;
				scanner_.Scan(sentence,tokens_);
				for(auto token: tokens_){
					parser_.NextPredict(token);
				}
				// std::cout << parser_ << std::endl;
				sentence = "";
				auto status = Compile();
				if(!status.ok())os << status.ToString() << std::endl;
				else{
					// os << RunByteCode().ToString() << std::endl;
				}
				ClearPreviousPass();
				continue;
			}
			sentence += buffer;
		}
 	}
 private:
 	void ClearPreviousPass(void){
 		tokens_.clear();
 		parser_.Clear();
 		// bytecodes_.clear();
 		tables_.clear();
 		columns_.clear();
 		// slices_.clear();
 	}
 	Status Compile(void){
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
 		return Status::OK();
 	}
 	Status CompileSelect(void){
 		auto deduced = parser_.SymbolBegin()+1;
 		auto end = parser_.SymbolEnd();
 		assert(deduced->symbol == kSymbolDict["select_clause"]);
 		assert(deduced->using_rule >= 0); // descend
 		deduced++; // main level
 		// find column
 		for(deduced; deduced < end && (deduced->symbol!=kSymbolDict["column_list"]); deduced++);
 		if(deduced >= end)return Status::Corruption("Can not find symbol column_list");
 		bool selectAll = false;
 		for(deduced; deduced < end; deduced++){
 			if(deduced->symbol == kSymbolDict["NONE"])break;
 			if(deduced->symbol == kSymbolDict["MULTIPLY_OP"])selectAll = true;
 			if(deduced->symbol == kSymbolDict["NAME"]){
 				std::cout << "Deducing column name: " << deduced->text << std::endl;
 				if(selectAll)return Status::Corruption("Select colomn list conflicts with *.");
 				columns_.push_back(deduced->text);
 			}
 		}
 		// find table
 		for(deduced; deduced < end&&(deduced->symbol!=kSymbolDict["table_list"]); deduced++);
 		if(deduced >= end)return Status::Corruption("Can not find symbol table_list.");
 		// find table list, encode name
 		for(deduced;deduced<end;deduced++){
 			if(deduced->symbol == kSymbolDict["NONE"])break;
 			if(deduced->symbol == kSymbolDict["NAME"]){
 				std::cout << "Deducing table name: " << deduced->text << std::endl;
 				tables_.push_back(deduced->text);
 			}
 		}
 		deduced ++; // end parsing tables
 		// check table // now only support one table query
 		if(tables_.size() <= 0)return Status::Corruption("Can not resolve table to select.");
 		if(tables_.size() > 1)return Status::Corruption("More than one table participated in selection.");
 		// auto pSchema = engine->GetTableSchema();
 		// if(!pSchema)return Statu::Corruption("Can not find table.");
 		// parse condition
 		for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["condition_clause"]); deduced++);
 		if(deduced > end)return Status::OK(); // no other condition
 		// simple implementation, no special for bracket and no OR
 		for(deduced;deduced<end;deduced++){
 			if(deduced->symbol == kSymbolDict["NONE"])break;
 			if(deduced->symbol == kSymbolDict["single_condition"]){
 				std::cout << "Deducing single condition" << std::endl;
 				bool inverse = false;
 				int indexOfField = -1;
 				std::string value;
 				std::string conditionDesciptor;
 				for(deduced;deduced<end;deduced++){
 					if(deduced->symbol == kSymbolDict["LBRACKET"])return Status::Corruption("Bracket not implemented.");
 					if(deduced->symbol == kSymbolDict["UNARY_LOGIC_OP"] &&\
 					 (deduced->text == "NOT" || deduced->text == "not")){
 						inverse = !inverse;
 					}
 					if(deduced->symbol == kSymbolDict["value_expr"]){
 						std::cout << "Deducing value expr" << std::endl;
 						if(indexOfField < 0){ // parse table
 							bool findColumn = false;
 							for(deduced;deduced<end;deduced++){
 								if(deduced->symbol == kSymbolDict["NONE"])break;
 								if(deduced->symbol == kSymbolDict["column"])findColumn = true;
 								if(deduced->symbol ==kSymbolDict["NAME"]){
 									if(findColumn)indexOfField = 1; // ERROR
 									else return Status::Corruption("Not known name in sql.");
 								}
 							}
 						}
 						else{
	 						std::cout << "Deducing values: ";
	 						bool isText = false;
	 						auto status = ParseSimpleExpr(deduced, value, isText);
	 						if(!status.ok())return status;
	 						std::cout << value << std::endl;
	 						break;
 						}
 					}
 					if(deduced->symbol == kSymbolDict["BOOL_OP"]){
 						if(indexOfField < 0)return Status::Corruption("Not known bool operand");
 						conditionDesciptor = deduced->text;
 					}
 				}
 				SliceFilterType filter;
 				// switch(_hash<std::string>{}(conditionDesciptor)){
 				// 	case _hash<std::string>{}("!="):inverse = !inverse;
 				// 	case _hash<std::string>{}("="):filter = std::bind(SliceFilter_eq, indexOfField, value, inverse);break;
 				// 	case _hash<std::string>{}("<="):inverse = !inverse;
 				// 	case _hash<std::string>{}(">"):filter = std::bind(SliceFilter_gt, indexOfField, value, inverse);break;
 				// 	case _hash<std::string>{}(">="):inverse = !inverse;
 				// 	case _hash<std::string>{}("<"):filter = std::bind(SliceFilter_lt, indexOfField, value, inverse);break;
 				// 	case _hash<std::string>{}("like"):
 				// 	case _hash<std::string>{}("LIKE"):filter = std::bind(SliceFilter_like, indexOfField, value, inverse);break;
 				// 	default: return Status::InvalidArgument("Operator not implemented.");
 				// }
 				switch(_hash<std::string>{}(conditionDesciptor)){
 					case 4384:inverse = !inverse;
 					case 61:filter = std::bind(SliceFilter_eq,_1, indexOfField, value, inverse);break;
 					case 7921:inverse = !inverse;
 					case 62:filter = std::bind(SliceFilter_gt,_1, indexOfField, value, inverse);break;
 					case 8183:inverse = !inverse;
 					case 60:filter = std::bind(SliceFilter_lt,_1, indexOfField, value, inverse);break;
 					case 244609851:
 					case 172117563:filter = std::bind(SliceFilter_like,_1, indexOfField, value, inverse);break;
 					default: return Status::InvalidArgument("Operator not implemented.");
 				}
 				filters_.push_back(filter);
 			}
 			else if(deduced->symbol == kSymbolDict["condition_tail"]){
 				deduced ++;
 				if(deduced > end)return Status::Corruption("Can not resolve symbol condition_tail");
 				if(deduced->symbol == kSymbolDict["NONE"])break;
 				if(deduced->symbol == kSymbolDict["LOGIC_OP"]){
 					if(deduced->text != "AND" && deduced->text != "and")return Status::InvalidArgument("Logic op other than AND is not implemented.");
 				}
 			}
 		}
 		return Status::OK();
 	}
 	Status CompileInsert(void){
 		auto deduced = parser_.SymbolBegin()+1;
 		auto end = parser_.SymbolEnd();
 		assert(deduced->symbol == kSymbolDict["insert_clause"]);
 		assert(deduced->using_rule >= 0); // descend
 		deduced++; // main level
 		// find table
 		for(deduced; deduced < end&&(deduced->symbol!=kSymbolDict["table_list"]); deduced++);
 		if(deduced >= end)return Status::Corruption("Can not find symbol table_list.");
 		// find table list, encode name
 		for(deduced;deduced<end;deduced++){
 			if(deduced->symbol == kSymbolDict["NONE"])break;
 			if(deduced->symbol == kSymbolDict["NAME"]){
 				std::cout << "Deducing table name: " << deduced->text << std::endl;
 				tables_.push_back(deduced->text);
 			}
 		}
 		deduced ++; // end parsing tables
 		// check table
 		if(tables_.size() <= 0)return Status::Corruption("Can not resolve table to insert.");
 		if(tables_.size() > 1)return Status::Corruption("More than one table participated in insertion.");
 		// auto pSchema = engine->GetTableSchema();
 		// if(!pSchema)return Statu::Corruption("Can not find table.");
 		// parse values
 		for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["package_list"]); deduced++);
 		if(deduced >= end)return Status::Corruption("Can not find symbol package_list");
 		for(deduced;deduced<end;deduced++){
 			if(deduced->symbol == kSymbolDict["NONE"])break;
 			if(deduced->symbol == kSymbolDict["package"]){
 				std::cout << "Deducing package: " << std::endl;
 				// new slice, parse values
 				for(deduced;deduced<end&&deduced->symbol!= kSymbolDict["value_list"];deduced++);
 				if(deduced >= end)return Status::Corruption("Can not find symbol value_list");
 				int idxField = 0;
 				for(deduced; deduced < end; deduced++){
 					// parse factor
 					if(deduced->symbol == kSymbolDict["NONE"])break;
 					if(deduced->symbol == kSymbolDict["value_expr"]){
 						std::cout << "Deducing values: ";
 						std::string value;
 						bool isText = false;
 						auto status = ParseSimpleExpr(deduced, value, isText);
 						if(!status.ok())return status;
 						std::cout << value << std::endl;
 						// encode to slice here
 					}
 				}
 			}
 		}
 		return Status::OK();
 	}
 	// auxilary
 	Status ParseSimpleExpr(std::vector<DeducedSymbol>::iterator& begin, std::string& ret, bool& isText){
 		// only parse single string / number
 		// return one after none or undefined if failed
 		assert(begin->symbol == kSymbolDict["value_expr"]);
 		auto end = parser_.SymbolEnd();
 		ret = "";
 		double numeric = 0.0;
 		double factor = 1.0;
 		std::string text;
 		begin++;
 		if(begin>=end || (begin)->symbol == kSymbolDict["select_clause"]){
 			return Status::Corruption("Can not resolve const expr frrom select clause.");
 		}
 		int termOp = 1; // 1 for +, 2 for -
 		int factorOp = 1; // 1 for *, 2 for /
 		int factorUnary = 0; // 1 for sqrt
 		for(begin; begin<end; begin++){
 			if(begin->symbol == kSymbolDict["NONE"]){
 				break;
 			}
 			if(begin->symbol == kSymbolDict["term"]){
 				begin++;
 				for(begin; begin < end; begin++){
 					if(begin->symbol == kSymbolDict["NONE"]){
 						break;
 					}
 					if(begin->symbol == kSymbolDict["factor"]){
 						begin ++;
 						if(begin >= end)return Status::Corruption("Can not resolve factor");
 						if(begin->symbol == kSymbolDict["LBRACKET"]){
 							begin++;
 							std::string tmp;
 							auto status = ParseSimpleExpr(begin, tmp, isText);
 							if(isText){
 								text += tmp;
 							}
 							else{
 							 	double ntmp = atof(tmp.c_str());
	 							if(factorUnary != 0){
	 								if(factorUnary == 1)ntmp = sqrt(ntmp);
	 								factorUnary = 0;
	 							}
	 							if(termOp == 0){
	 								return Status::Corruption("Missing ");
	 							}
	 							else if(factorOp == 0){
	 								factor = ntmp;
	 							}
	 							else if(factorOp != 0){
	 								if(factorOp == 1)factor *= ntmp;
	 								else if(factorOp == 2)factor /= ntmp;
	 								factorOp = 0;
	 							}
 							}
 						}
 						else if(begin->symbol == kSymbolDict["FACTOR_UNARY_OP"]){
 							if(begin->text == "sqrt")factorUnary = 1;
 							else return Status::Corruption("Can not resolve factor unary operation");
 						}
 						else if(begin->symbol == kSymbolDict["NUMBER"]){
 							if(isText)return Status::InvalidArgument("Mixture of plain text and numeric.");
 							double tmp = atof(begin->text.c_str());
 							if(factorUnary != 0){
 								if(factorUnary == 1)tmp = sqrt(tmp);
 								factorUnary = 0;
 							}
 							if(termOp == 0){
 								return Status::Corruption("Missing ");
 							}
 							else if(factorOp == 0){
 								factor = tmp;
 							}
 							else if(factorOp != 0){
 								if(factorOp == 1)factor *= tmp;
 								else if(factorOp == 2)factor /= tmp;
 								factorOp = 0;
 							}
 						}
 						else if(begin->symbol == kSymbolDict["STRING"]){
 							isText = true;
 							text = begin->text;
 						}
 						else return Status::Corruption("Can not resolve const expr.");
 					}
 					else if(begin->symbol == kSymbolDict["factor_tail"]){
 						begin++;
 						if(begin >= end)return Status::Corruption("Can not resolve factor_tail.");
 						if(begin->symbol == kSymbolDict["NONE"]){
 							break;
 						}
 						if(isText)return Status::InvalidArgument("Mixture of plain text and numeric.");
 						else if(begin->symbol == kSymbolDict["FACTOR_BINARY_OP"]){
 							if(begin->text == "/")factorOp = 2;
 							else return Status::Corruption("Can not resolve factor operation");
 						}
 						else if(begin->symbol == kSymbolDict["MULTIPLY_OP"]){
 							factorOp = 1;
 						}
 						else return Status::Corruption("Can not resolve factor_tail.");
 					}
 				}

 			}
 			else if(begin->symbol == kSymbolDict["term_tail"]){
 				begin ++;
 				if(begin >= end)return Status::Corruption("Can not resolve term_tail.");
 				if(begin->symbol == kSymbolDict["NONE"])break; // end
 				if(isText)return Status::InvalidArgument("Mixture of plain text and numeric.");
 				if(begin->symbol == kSymbolDict["TERM_OP"]){
 					if(termOp == 0)return Status::Corruption("Missing term operation.");
					else if(termOp == 1)numeric += factor;
					else if(termOp == 2)numeric -= factor;

 					if(begin->text == "+")termOp = 1;
 					else if(begin->text == "-")termOp = 2;
 					else return Status::Corruption("Can not resolve term operation.");
 				}
 				else return Status::Corruption("Can not resolve term_tail");
 			} 				

 		}
		if(isText){
			ret = text;
			return Status::OK();
		}
		if(factorOp != 0)return Status::Corruption("Missing factor.");
		if(termOp == 0)return Status::Corruption("Missing term operation.");
		else if(termOp == 1)numeric += factor;
		else if(termOp == 2)numeric -= factor;
		ret = to_string(numeric);
		return Status::OK();
 	}


}; // class Compiler


} // namespace sbase

#endif // SBASE_COMPILER_COMPILER_HPP_