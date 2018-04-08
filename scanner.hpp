#ifndef SBASE_SCANNER_HPP_
#define SBASE_SCANNER_HPP_


#include "./storage/hash.hpp"
#include "dict.hpp"
#include "./storage/status.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <regex>

/*
Syntax Rule /18-4-8
select_clause = 'select' + column_list + 'from' + table_list + where_clause + ';'

column_list = '*'
column_list = column + column_list_tail
column = name + '.' + name
column = name
column_list_tail = none
column_list_tail = column + column_list_tail

table_list = name + table_list_tail
table_list_tail = none
table_list_tail = ',' + name + table_list_tail

where_clause = none
where_clause = 'where' + condition_clause

condition_clause = single_condition + condition_tail
condition_tail = none
condition_tail = logic_op + single_condition + condition_tail

single_condition = unary_logic_op + single_condition
single_condition = value_expr + bool_op + value_expr

bool_op = 'like'
bool_op = '<>=='.split()
bool_op = in

value_expr = term + term_tail
value_expr = select_clause
value_expr = name
value_expr = '(' + value_expr + ')'
term_op = '+-'
term_tail = term_op + term + term_tail

term = factor + factor_tail
factor_binary_op = '\/'
factor_tail = none
factor_tail = factor_binary_op + factor + factor_tail
factor_tail = "*" + factor + factor_tail // for multiply take different meaning

factor = '(' + value_expr + ')'
factor = column,number,string
factor = unary_op + factor
*/

namespace sbase{

const size_t kWordSize = 19;
static const AutoDict<std::string> kWordDict(
  "NUMBER","STRING",
  "SEMICOLON","COMMA","LBRACKET","RBRACKET","DOT",
  "SELECT","FROM", "WHERE", 
  "UNARY_LOGIC_OP","LOGIC_OP","BOOL_OP",
  "TERM_OP","MULTIPLY_OP","FACTOR_BINARY_OP","FACTOR_UNARY_OP", 
  "NAME"
  );
static const char* kWordLex[kWordSize] = {
  "^[\r\t\n ]",  // none
  "^[0-9]+",
  "^\'[^\']+\'",
  "^;",
  "^,",
  "^[(]",
  "^[)]",
  "^[.]",
  "^(select|SELECT)",
  "^(from|FROM)",
  "^(where|WHERE)",
  "^(not|NOT)",
  "^(and|AND|or|OR)",
  "^(<|<=|>|>=|=|like|LIKE|in|IN)",
  "^[+-]",
  "^[*]",
  "^[/]",
  "^(sqrt)",
  "^[a-zA-Z][a-zA-Z0-9]*"
};

struct Token{
  int id;
  std::string text;
  Token(int a, std::string str):id(a),text(str){ }
  Token(const Token& that):id(that.id),text(that.text){ }
  ~Token(){ }
};

class Scanner{
  std::vector<std::regex> regs_;
 public:
  
  Scanner(){
    for(int i = 0; i<kWordSize; i++){
      regs_.push_back(std::regex(kWordLex[i]));
    }
  }
  ~Scanner(){ }
  
  Status Scan(std::string text, std::vector<Token>& ret){
    std::string str = text;
    while(str.size()){
      int i = 0;
      for(i = 0; i<kWordSize; i++){
        std::smatch match;
        if(std::regex_search(str, match, regs_[i])){
          if(i>0){
            ret.push_back(Token(i, match[0]));
          }
          str = match.suffix().str();
          break;
        }
      }
      if(i >= kWordSize){
        return Status::Corruption("Error parsing");
      }
    }
    return Status::OK();
  }
};

} // namespace sbase

#endif // SBASE_SCANNER_HPP_