namespace sbase{

static const AutoDict<std::string> kWordDict(
  // word at first
  "NUMBER","STRING",
  "SEMICOLON","COMMA","LBRACKET","RBRACKET","DOT",
  "SELECT","FROM", "WHERE", 
  "UNARY_LOGIC_OP","LOGIC_OP","BOOL_OP",
  "TERM_OP","MULTIPLY_OP","FACTOR_BINARY_OP","FACTOR_UNARY_OP", 
  "NAME",
  // non-terminals
  "select_clause", "column_list", "table_list", "where_clause",
  "column", "column_list_tail", "table_list_tail", "condition_clause",
  "single_condition", "condition_tail", "value_expr", "term", "term_tail",
  "factor", "factor_tail"
  );

static const 


  class Parser{
    Parser() = default;
    ~Parser(){ }

  }


} // namespace sbase