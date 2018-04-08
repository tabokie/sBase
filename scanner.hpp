#ifndef SBASE_SCANNER_HPP_
#define SBASE_SCANNER_HPP_


#include "./storage/hash.hpp"
#include "dict.hpp"
#include "./storage/status.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <regex>

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