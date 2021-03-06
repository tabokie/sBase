#ifndef SBASE_COMPILER_SCANNER_HPP_
#define SBASE_COMPILER_SCANNER_HPP_


#include "./util/hash.hpp"
#include "./util/dict.hpp"
#include "./util/status.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <regex>

namespace sbase{

const size_t kWordSize = 19+3+7+1+4;
static const AutoDict<std::string> kWordDict(
  "NUMBER","STRING",
  "SEMICOLON","COMMA","LBRACKET","RBRACKET","DOT",
  "SELECT","FROM", "WHERE", 
  "INSERT","INTO","VALUES",
  "CREATE","TABLE","PRIMARY","KEY","CHAR","INT","FLOAT","UNIQUE",
  "DELETE", // ERROR
  "DROP","INDEX","ON", // priority ERROR
  "UNARY_LOGIC_OP","LOGIC_OP","BOOL_OP",
  "TERM_OP","MULTIPLY_OP","FACTOR_BINARY_OP","FACTOR_UNARY_OP", 
  "NAME"
  );
static const char* kWordLex[kWordSize] = {
  "^[\r\t\n ]",  // none
  "^([+-]*[0-9]+|[+-]*[0-9]+[.][0-9]*)",
  "^\'[^\']+\'",
  "^;",
  "^,",
  "^[(]",
  "^[)]",
  "^[.]",
  "^(select|SELECT)[\r\t\n ]",
  "^(from|FROM)[\r\t\n ]",
  "^(where|WHERE)[\r\t\n ]",
  "^(insert|INSERT)[\r\t\n ]",
  "^(into|INTO)[\r\t\n ]",
  "^(values|VALUES)",
  "^(CREATE|create)[\r\t\n ]",
  "^(TABLE|table)[\r\t\n ]",
  "^(PRIMARY|primary)[\r\t\n ]",
  "^(KEY|key)",
  "^(CHAR|char)",
  "^(INT|int)",
  "^(FLOAT|float)",
  "^(UNIQUE|unique)",
  "^(DELETE|delete)",
  "^(DROP|drop)[\r\t\n ]",
  "^(INDEX|index)[\r\t\n ]",
  "^(ON|on)[\r\t\n ]",
  "^(not|NOT)[\r\t\n ]",
  "^(and|AND|or|OR)[\r\t\n ]",
  "^(<>|<=|<|>=|>|=|!=|like|LIKE|in|IN)",
  "^[+-]",
  "^[*]",
  "^[/]",
  "^(sqrt)",
  "^[a-zA-Z_][a-zA-Z0-9_]*"
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

#endif // SBASE_COMPILER_SCANNER_HPP_