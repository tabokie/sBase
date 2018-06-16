#include ".\compiler\compiler.h"

#include <iostream>
#include <fstream>

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

// Interface //
void Compiler::RunInterface(std::istream& is, std::ostream& os, bool prompt){
  std::string buffer;
  std::string sentence;
  ClearPreviousPass();
  while(!is.eof()){
    if(prompt)os << ">> ";
    getline(is, buffer);
    if(buffer.find("\\q") < std::string::npos){
      os << "Bye";
      break;
    }
    if(buffer.find("execfile") < std::string::npos){
      int head = buffer.find("execfile")+8;
      int end = buffer.find(";");
      if(end == std::string::npos){
        os << "Cannot find file path terminator." << std::endl;
        continue;
      }
      if(end-head <= 0)continue;
      std::fstream f(buffer.substr(head,end-head).c_str(), std::ios::in);
      if(!f.good()){
        os << "Fail to read file" << std::endl;
        continue;
      }
      RunInterface(f, os, false);
      continue;
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
Status Compiler::CompileSelect(void){
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
      // std::cout << "Deducing column name: " << deduced->text << std::endl;
      if(selectAll)return Status::Corruption("Select colomn list conflicts with *.");
      resource_.displayColumns.push_back(deduced->text);
    }
  }
  // find table
  for(deduced; deduced < end&&(deduced->symbol!=kSymbolDict["table_list"]); deduced++);
  if(deduced >= end)return Status::Corruption("Can not find symbol table_list.");
  // find table list, encode name
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["NAME"]){
      // std::cout << "Deducing table name: " << deduced->text << std::endl;
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table // now only support one table query
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to select.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in selection.");
  // auto pSchema = engine->GetTableSchema();
  // if(!pSchema)return Statu::Corruption("Can not find table.");
  // parse condition
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["condition_clause"]); deduced++);
  if(deduced > end)return Status::OK(); // no other condition
  return ParseWhereClause(deduced);
}
Status Compiler::CompileDelete(void){
  auto deduced = parser_.SymbolBegin()+1;
  auto end = parser_.SymbolEnd();
  assert(deduced->symbol == kSymbolDict["delete_clause"]);
  assert(deduced->using_rule >= 0); // descend
  deduced++; // main level
  // find table
  for(deduced; deduced < end&&(deduced->symbol!=kSymbolDict["table_list"]); deduced++);
  if(deduced >= end)return Status::Corruption("Can not find symbol table_list.");
  // find table list, encode name
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["NAME"]){
      // std::cout << "Deducing table name: " << deduced->text << std::endl;
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table // now only support one table query
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to select.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in selection.");
  // auto pSchema = engine->GetTableSchema();
  // if(!pSchema)return Statu::Corruption("Can not find table.");
  // parse condition
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["condition_clause"]); deduced++);
  if(deduced > end)return Status::OK(); // no other condition
  return ParseWhereClause(deduced);
}
Status Compiler::CompileInsert(void){
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
      // std::cout << "Deducing table name: " << deduced->text << std::endl;
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to insert.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in insertion.");
  // auto pSchema = engine->GetTableSchema();
  // if(!pSchema)return Statu::Corruption("Can not find table.");
  // parse values
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["package_list"]); deduced++);
  if(deduced >= end)return Status::Corruption("Can not find symbol package_list");
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["package"]){
      // std::cout << "Deducing package: " << std::endl;
      // new slice, parse values
      for(deduced;deduced<end&&deduced->symbol!= kSymbolDict["value_list"];deduced++);
      if(deduced >= end)return Status::Corruption("Can not find symbol value_list");
      int idxField = 0;
      for(deduced; deduced < end; deduced++){
        // parse factor
        if(deduced->symbol == kSymbolDict["NONE"])break;
        if(deduced->symbol == kSymbolDict["value_expr"]){
          // std::cout << "Deducing values: ";
          std::string value;
          bool isText = false;
          auto status = ParseSimpleExpr(deduced, value, isText);
          if(!status.ok())return status;
          // std::cout << value << std::endl;
          // encode to slice here
        }
      }
    }
  }
  return Status::OK();
}
Status Compiler::CompileCreate(void){
  auto deduced = parser_.SymbolBegin() + 1;
  auto end = parser_.SymbolEnd();
  assert(deduced->symbol == kSymbolDict["create_clause"]);
  assert(deduced->using_rule >= 0);
  deduced +=3;
  if(deduced->symbol != kSymbolDict["NAME"])return Status::Corruption("Cannot resolve table name to create.");
  std::string table = deduced->text;
  std::string primary;
  std::vector<std::string> fields;
  std::vector<TypeT> types;
  std::vector<bool> isUnique;
  for(deduced;deduced<end&&deduced->symbol!=kSymbolDict["field_list"];deduced++);
  if(deduced > end)return Status::Corruption("Cannot resolve field list.");
  for(deduced; deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["field_list"] || deduced->symbol==kSymbolDict["field_list_tail"]){
      for(deduced;deduced<end;deduced++){
        if(deduced->symbol == kSymbolDict["type"]){
          if((deduced-1)->symbol != kSymbolDict["NAME"])return Status::Corruption("Cannot resolve field name.");
          fields.push_back((deduced-1)->text );
          deduced ++;
          if(deduced->symbol == kSymbolDict["INT"])types.push_back(intT);
          else if(deduced->symbol == kSymbolDict["CHAR"]){
            deduced += 2;
            int bits = atoi(deduced->text.c_str());
            if(bits <= 0)return Status::InvalidArgument("Cannot have char type shorter than 1.");
            if(bits <= 8)types.push_back(fixchar8T);
            else if(bits <= 16)types.push_back(fixchar16T);
            else if(bits <= 32)types.push_back(fixchar32T);
            else if(bits <= 64)types.push_back(fixchar64T);
            else if(bits <= 128)types.push_back(fixchar128T);
            else if(bits <= 256)types.push_back(fixchar256T);
            else return Status::InvalidArgument("Cannot have char type longer than 256.");
          }
          else return Status::Corruption("Cannot resolve field type.");
          deduced ++;
          if(deduced<end && deduced->symbol == kSymbolDict["UNIQUE"])isUnique.push_back(true);
          else isUnique.push_back(false);
          break;
        }
        if(deduced->symbol == kSymbolDict["PRIMARY"]){
          deduced +=3;
          if(deduced >=end || deduced->symbol != kSymbolDict["column_list"])return Status::Corruption("Cannot resolve primary list.");
          deduced += 2;
          if(primary.length()>0 || deduced>=end || deduced->symbol != kSymbolDict["NAME"])return Status::InvalidArgument("Can only apply one primary key.");
          primary = deduced->text;
          deduced += 2;
          if(deduced->symbol!=kSymbolDict["NONE"])return Status::InvalidArgument("Cannot apply more than one primary key.");
        }
      }
    }
  }
  for(auto& k: fields){
    // std::cout << k << std::endl;
  }
  if(primary.length() <= 0)return Status::InvalidArgument("No primary key detected.");
  return Status::OK();
}
// auxilary
Status Compiler::ParseSimpleExpr(std::vector<DeducedSymbol>::iterator& begin, std::string& ret, bool& isText){
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
    if(text[0] == '\'')ret = text.substr(1,text.length()-2);
    return Status::OK();
  }
  if(factorOp != 0)return Status::Corruption("Missing factor.");
  if(termOp == 0)return Status::Corruption("Missing term operation.");
  else if(termOp == 1)numeric += factor;
  else if(termOp == 2)numeric -= factor;
  ret = to_string(numeric);
  return Status::OK();
}
Status Compiler::ParseWhereClause(std::vector<DeducedSymbol>::iterator& deduced){
  // simple implementation, no special for bracket and no OR
  assert(deduced->symbol == kSymbolDict["condition_clause"]);
  auto end = parser_.SymbolEnd();
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["single_condition"]){
      // std::cout << "Deducing single condition" << std::endl;
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
          // std::cout << "Deducing value expr" << std::endl;
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
            // std::cout << "Deducing values: ";
            bool isText = false;
            auto status = ParseSimpleExpr(deduced, value, isText);
            if(!status.ok())return status;
            // std::cout << value << std::endl;
            break;
          }
        }
        if(deduced->symbol == kSymbolDict["BOOL_OP"]){
          if(indexOfField < 0)return Status::Corruption("Not known bool operand");
          conditionDesciptor = deduced->text;
        }
      }
      SliceFilterType filter;
      switch(_hash<std::string>{}(conditionDesciptor)){
        // !=
        case 4384:inverse = !inverse;
        // =
        case 61:filter = std::bind(SliceFilter_eq,_1, indexOfField, value, inverse);break;
        // <=
        case 7921:inverse = !inverse;
        // >
        case 62:filter = std::bind(SliceFilter_gt,_1, indexOfField, value, inverse);break;
        // >=
        case 8183:inverse = !inverse;
        // <
        case 60:filter = std::bind(SliceFilter_lt,_1, indexOfField, value, inverse);break;
        // like
        case 244609851:
        // LIKE
        case 172117563:filter = std::bind(SliceFilter_like,_1, indexOfField, value, inverse);break;
        default: return Status::InvalidArgument("Operator not implemented.");
      }
      resource_.filters.push_back(filter);
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


} // namespace sbase

