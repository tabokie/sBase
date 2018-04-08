# sBase


## front end

### sentence lexer

Syntax Rule /18-4-8

`select_clause = 'select' + column_list + 'from' + table_list + where_clause + ';'
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
`