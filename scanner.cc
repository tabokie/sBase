#include "./storage/hash.hpp"
#include "dict.hpp"

#include <string>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;


/*
Syntax Rule /18-4-7
select_clause = 'select' + column_list + 'from' + table_list + where_clause + ';'

column_list = '*'
column_list = column + column_list_tail
column_list_tail = none
column_list_tail = column + column_list_tail

table_list = table + table_list_tail
table_list_tail = none
table_list_tail = ',' + table + table_list_tail

where_clause = none
where_clause = 'where' + condition_clause

condition_clause = single_condition + condition_tail
condition_tail = none
condition_tail = logic_op + single_condition + condition_tail

single_condition = value_expr + bool_op + value_expr

bool_op = 'like'
bool_op = '<>=='.split()
bool_op = in

value_expr = term + term_tail
value_expr = select_clause
value_expr = table
term_binary_op = '+-'
term_tail = term_binary_op + term + term_tail

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

class Scanner{

  Scanner() = default;
  ~Scanner(){ }

};

} // namespace sbase

using namespace sbase;

int main(void){

  AutoDict<std::string> dict(
  	"SELECT","COLUMN","FROM","TABLE","WHERE","SEMICOLON",
  	"NONE","COMMA","NUMBER","STRING",
  	"LOGIC_OP","BOOL_OP",
  	"MUTIPLY_OP","FACTOR_BINARY_OP","TERM_BINARY_OP","UNARY_OP"
  	);

  return 0;
}