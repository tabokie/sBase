#ifndef SBASE_FRONT_PARSER_HPP_
#define SBASE_FRONT_PARSER_HPP_

#include "scanner.hpp" // struct Token

#include "stack.hpp"
#include "dict.hpp"

#include ".\storage\status.hpp"

#include <list>
#include <iostream>

namespace sbase{

static const AutoDict<std::string> kSymbolDict(
  // word at first
  "NUMBER","STRING", // 1,2
  "SEMICOLON","COMMA","LBRACKET","RBRACKET","DOT", // 3,4,5,6,7
  "SELECT","FROM", "WHERE", // 8,9,10
  "UNARY_LOGIC_OP","LOGIC_OP","BOOL_OP", // 11,12,13
  "TERM_OP","MULTIPLY_OP","FACTOR_BINARY_OP","FACTOR_UNARY_OP", // 14,15,16,17
  "NAME", // 18
  // non-terminals
  "select_clause", "column_list", "table_list", "where_clause", // 19,20,21,22
  "column", "column_list_tail", "table_list_tail", "condition_clause", // 23,24,25,26
  "single_condition", "condition_tail", "value_expr", "term", "term_tail", // 27,28,29,30,31
  "factor", "factor_tail", // 32,33
  "NONE", // 34
  "sql"
  );

// initializer: having init(Args... args)
// dict: [] to get item id
// Args: key of dict
template <class _initializer, class _dict, class ...Args>
_initializer InitByDict(_dict dict, Args... args){
  return _initializer(dict[args]...);
}

#define _from_(_str)   kSymbolDict[_str]
#define _to_(...)   InitByDict<Stack<int>, AutoDict<string>>(kSymbolDict, __VA_ARGS__)

// auto init_func_ = InitByDict<Stack<int>, AutoDict<string>,class ...Args>;
LayeredDict<int, Stack<int>> kRuleDict(
  _from_("sql"),_to_("select_clause","SEMICOLON"),
  _from_("select_clause"),_to_("SELECT", "column_list","FROM","table_list","where_clause"),
  _from_("column_list"),_to_("MULTIPLY_OP"),
  _from_("column_list"),_to_("column","column_list_tail"),
  _from_("column"),_to_("NAME", "DOT", "NAME"),
  _from_("column"),_to_("NAME"),
  _from_("column_list_tail"),_to_("NONE"),
  _from_("column_list_tail"),_to_("column","column_list_tail"),
  _from_("table_list"),_to_("NAME", "table_list_tail"),
  _from_("table_list_tail"),_to_("NONE"),
  _from_("table_list_tail"),_to_("COMMA","NAME","table_list_tail"),
  _from_("where_clause"),_to_("NONE"),
  _from_("where_clause"),_to_("WHERE","condition_clause"),
  _from_("condition_clause"),_to_("single_condition","condition_tail"),
  _from_("condition_tail"),_to_("NONE"),
  _from_("condition_tail"),_to_("LOGIC_OP","single_condition","condition_tail"),
  _from_("single_condition"),_to_("UNARY_LOGIC_OP","single_condition"),
  _from_("single_condition"),_to_("value_expr","BOOL_OP","value_expr"),
  _from_("value_expr"),_to_("term","term_tail"),
  _from_("value_expr"),_to_("select_clause"),
  _from_("term_tail"),_to_("TERM_OP","term","term_tail"),
  _from_("term_tail"),_to_("NONE"),
  _from_("term"),_to_("factor","factor_tail"),
  _from_("factor_tail"),_to_("NONE"),
  _from_("factor_tail"),_to_("FACTOR_BINARY_OP","factor","factor_tail"),
  _from_("factor_tail"),_to_("MULTIPLY_OP","factor","factor_tail"),
  _from_("factor"),_to_("LBRACKET","value_expr","RBRACKET"),
  _from_("factor"),_to_("FACTOR_UNARY_OP","factor"),
  _from_("factor"),_to_("column"),
  _from_("factor"),_to_("NUMBER"),
  _from_("factor"),_to_("STRING")
  );

#undef _to_
#undef _from_

typedef int RuleType;
typedef int CommonSymbol;

struct DeducedSymbol{
  RuleType using_rule; // -1 for terminal
  CommonSymbol symbol;
  std::string text; // "" for non-terminal
  DeducedSymbol(CommonSymbol s, RuleType r):using_rule(r), symbol(s), text(""){ }
  DeducedSymbol(CommonSymbol s, std::string t):using_rule(-1), symbol(s), text(t){ }
  ~DeducedSymbol(){ }
};


// prefix compress
struct ParsingProcess{
  vector<DeducedSymbol> analyzed;
  Stack<CommonSymbol> predict;
  ParsingProcess(const Stack<CommonSymbol>& pred){
    predict += pred;
  }
  ParsingProcess(DeducedSymbol a, const Stack<CommonSymbol>& pred ){
    analyzed.push_back(a);
    predict += pred;
  }
  ParsingProcess(vector<DeducedSymbol> anal, const Stack<CommonSymbol>& pred ){
    analyzed = anal;
    predict += pred;
  }
  CommonSymbol pop(void){
    return predict.pop();
  }
};


class Parser{
  using ProcessPtr = shared_ptr<ParsingProcess>;
  std::list<ProcessPtr> processes_;
 public:
  Parser(){
    processes_.push_back(make_shared<ParsingProcess>(Stack<CommonSymbol>(kSymbolDict["sql"])));
  }
  ~Parser(){ }
  Status NextPredict(Token current){
    CommonSymbol current_symbol = current.id;
    int idx =0;
    for(auto process = processes_.begin(); process != processes_.end(); ){
      // predict
      Stack<CommonSymbol>& predict = (*process)->predict;
      vector<DeducedSymbol>& analyzed =(*process)->analyzed;
      CommonSymbol next = predict.pop();
      // expand with rules
      vector<Stack<CommonSymbol>> rules = kRuleDict[next];
      // if terminal or none
      if(rules.size() == 0 && next == current_symbol ){
        (*process)->analyzed.push_back(DeducedSymbol(next, current.text));
        process ++;
      }
      // donnot match, delete
      else if(rules.size() == 0 && next == 34){
        (*process)->analyzed.push_back(DeducedSymbol(next, current.text));
        // stay here
      }
      else if(rules.size() == 0){
        process = processes_.erase(process);
      }
      // add new fules
      else{
        // copy not ref
        std::list<ProcessPtr>::iterator current_backup(process);
        process ++;
        int rule_id = 0;
        for(auto rule: rules){
          auto inserted = processes_.insert(process,make_shared<ParsingProcess>(analyzed, predict + rule) );
          (*inserted)->analyzed.push_back(DeducedSymbol(next, rule_id));
          rule_id ++;
        }
        process = processes_.erase(current_backup); // delete father
      }
    }

    return Status::OK();
  }
  friend ostream& operator<<(ostream& os, Parser parser){
    for(auto process = parser.processes_.begin(); process != parser.processes_.end(); process ++){
      for(auto analyzed: (*process)->analyzed){
        os << kSymbolDict(analyzed.symbol) << "(" << analyzed.using_rule << ") ";
      }
      os << "|| ";
      (*process)->predict.Put(os);
    }
    return os;
  }

};


} // namespace sbase

/*
  Parser parser;
  // cout << parser;
  parser.NextPredict(8);
  cout << parser;
  parser.NextPredict(15);
  cout << parser;
*/


#endif // SBASE_FRONT_PARSER_HPP_