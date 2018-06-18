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
  size_t  curMax = 0;
  bool matchHead = false;
  if(b[0] != '%')matchHead =true;
  while(getline(tmp,token,'%')){
    if(token.length()==0)continue;
    if(cur >= a.length())return false;
    idx = a.find(token, cur);
    if(idx > cur && matchHead)return false;
    matchHead = false;
    if(idx != std::string::npos && idx < a.length()){
      cur = idx + token.length();
      while(idx != std::string::npos){
        curMax = idx + token.length();
        idx = a.find(token,curMax);
      }
    }
    else return false;
  }
  // cur not end
  if(curMax < a.length() && b[b.length()-1] != '%')return false;
  return true;
}
bool SliceFilter_like(const Slice& slice, int index, std::string format, bool inverse){
  auto value = slice.GetValue(index);
  if(value.type() < fixchar8T)return false;
  std::string strVal = std::string(value);
  return sql_string_match(strVal, format)^inverse;
}
bool SliceFilter_eq(const Slice& slice, int index, std::string rhs, bool inverse){
  auto value = slice.GetValue(index);
  Value rhsVal(value.type(), rhs);
  return (value==rhsVal)^inverse;
}
bool SliceFilter_gt(const Slice& slice, int index, std::string rhs, bool inverse){
  auto value = slice.GetValue(index);
  Value rhsVal(value.type(), rhs);
  return (value>rhsVal)^inverse;
}
bool SliceFilter_lt(const Slice& slice, int index, std::string rhs, bool inverse){
  auto value = slice.GetValue(index);
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
      Terminate();
      break;
    }
    if(buffer.find("execfile") < std::string::npos){
      int head = buffer.find("execfile")+8;
      int end = buffer.find(";");
      if(end == std::string::npos){
        os << "Cannot find file path terminator." << std::endl;
        continue;
      }
      while(buffer[head] == ' ')head++;
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
        status = RunInstructions(os);
        if(prompt || !status.ok())os << status.ToString() << std::endl;
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
  bool selectAll = false; // ERROR
  for(deduced; deduced < end; deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["MULTIPLY_OP"])selectAll = true;
    if(deduced->symbol == kSymbolDict["NAME"]){
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
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table // now only support one table query
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to select.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in selection.");
  auto pSchema = engine.GetSchema(resource_.database, resource_.tables[0]);
  if(!pSchema)return Status::Corruption("Can not find table.");
  resource_.schema = pSchema;

  if(selectAll){
    resource_.displayColumns.resize(pSchema->attributeCount());
    for(int i = 0; i < pSchema->attributeCount(); i++){
      resource_.displayColumns[pSchema->GetUserIndex(i)] = pSchema->GetAttribute(i).name();
    }
  }  

  // parse condition
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["where_clause"]); deduced++);
  if(deduced > end)return Status::OK(); // no other condition
  bytecodes_.push_back(kTransaction);
  bytecodes_.push_back(kOpenRecordCursor);
  auto status = ParseWhereClause(deduced); // encode open index cursor and prepare
  if(!status.ok())return status;
  bytecodes_.push_back(kNextSlice);
  bytecodes_.push_back(Instruction(kIfNil, 3));
  bytecodes_.push_back(Instruction(kIfNot, -3));
  bytecodes_.push_back(kPrintSlice);
  bytecodes_.push_back(Instruction(kJump, -5));
  return Status::OK();
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
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table // now only support one table query
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to select.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in selection.");
  auto pSchema = engine.GetSchema(resource_.database, resource_.tables[0]);
  if(!pSchema)return Status::Corruption("Can not find table.");
  resource_.schema = pSchema;
  // parse condition
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["where_clause"]); deduced++);
  if(deduced > end)return Status::OK(); // no other condition
  bytecodes_.push_back(kTransaction);
  bytecodes_.push_back(kOpenRecordCursor);
  auto status = ParseWhereClause(deduced); // encode open index cursor and prepare
  if(!status.ok())return status;
  bytecodes_.push_back(kNextSlice);
  bytecodes_.push_back(Instruction(kIfNil, 3));
  bytecodes_.push_back(Instruction(kIfNot, -3));
  bytecodes_.push_back(kDeleteSlice);
  bytecodes_.push_back(Instruction(kJump, -5));
  return Status::OK();
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
      resource_.tables.push_back(deduced->text);
    }
  }
  deduced ++; // end parsing tables
  // check table
  if(resource_.tables.size() <= 0)return Status::Corruption("Can not resolve table to insert.");
  if(resource_.tables.size() > 1)return Status::Corruption("More than one table participated in insertion.");
  auto pSchema = engine.GetSchema(resource_.database, resource_.tables[0]);
  if(!pSchema)return Status::Corruption("Can not find table.");
  resource_.schema = pSchema;
  // parse values
  for(deduced;deduced < end &&(deduced->symbol!=kSymbolDict["package_list"]); deduced++);
  if(deduced >= end)return Status::Corruption("Can not find symbol package_list");
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["package"]){
      // new slice, parse values
      for(deduced;deduced<end&&deduced->symbol!= kSymbolDict["value_list"];deduced++);
      if(deduced >= end)return Status::Corruption("Can not find symbol value_list");
      int idxField = 0;
      Slice cur = pSchema->NewObject();
      for(deduced; deduced < end; deduced++){
        // parse factor
        if(deduced->symbol == kSymbolDict["NONE"])break;
        if(deduced->symbol == kSymbolDict["value_expr"]){
          std::string value;
          bool isText = false;
          auto status = ParseSimpleExpr(deduced, value, isText);
          if(!status.ok())return status;
          // encode to slice here
          int tmp = resource_.schema->GetRealIndex(idxField);
          if(tmp < 0)return Status::InvalidArgument("Cannot find field");
          cur.SetValue(tmp, Value(cur.GetValue(tmp).type(), value));
          idxField ++;
        }
      }
      resource_.slices.push_back(cur);
    }
  }
  // bytecodes_.push_back(kLoadDatabase);
  // bytecodes_.push_back(kLoadTable);
  bytecodes_.push_back(kTransaction);
  bytecodes_.push_back(kOpenRecordCursor);
  bytecodes_.push_back( Instruction(kOpenIndexCursor, resource_.schema->GetAttribute(0).name()) );
  for(int i = 0; i < resource_.slices.size(); i++){
    bytecodes_.push_back( Instruction(kPrepareMatch, make_shared<Value>(resource_.slices[i].GetValue(0)) ) );
    bytecodes_.push_back( Instruction(kInsertSlice, i) );
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
          else if(deduced->symbol == kSymbolDict["FLOAT"])types.push_back(doubleT);
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
            deduced++; // ERROR
          }
          else return Status::Corruption("Cannot resolve field type.");
          if(deduced+1<end && (deduced+1)->symbol == kSymbolDict["UNIQUE"])isUnique.push_back(true);
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
  Schema schema(table); 
  int indexPrimary = -1;
  // std::cout << primary << std::endl;
  for(int i = 0; i < fields.size(); i++){
    if(fields[i] == primary){
      isUnique[i] = true; // auto set to unique
      // if(!isUnique[i])return Status::InvalidArgument("Cannot build primary index on non-unique field.");
      schema.AppendField(Attribute(fields[i], types[i]), i, isUnique[i]);
      indexPrimary = i;
    }
  }
  if(indexPrimary < 0)return Status::InvalidArgument("Cannot find primary key designation");
  // check name definition
  std::vector<size_t> hash_check;
  for(int i = 0; i < fields.size(); i++){
    size_t hash = _hash<std::string>{}(fields[i]);
    for(auto another: hash_check){
      if(another == hash)return Status::InvalidArgument("Duplicate field definition.");
    }
    hash_check.push_back(hash);
  }
  for(int i = 0; i < fields.size(); i++){
    if(i == indexPrimary)continue;
    schema.AppendField(Attribute(fields[i], types[i]), i, isUnique[i]);
  }

  // directly run // bad_practice()
  // assuming database opened
  return engine.CreateTable(schema);
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
int RankOfAppliant(int competant, std::string op){
  int ret = 0;
  if(op == std::string("!=") || op == std::string("like") || op == std::string("LIKE"))return -1;
  if(op == std::string("=") )ret += 100;
  if(competant == 0)ret += 30;
  return ret;
}
Status Compiler::ParseWhereClause(std::vector<DeducedSymbol>::iterator& deduced){
  // simple implementation, no special for bracket and no OR
  assert(deduced->symbol == kSymbolDict["where_clause"]);
  auto end = parser_.SymbolEnd();
  std::shared_ptr<Value> min = nullptr;bool left;
  std::shared_ptr<Value> match = nullptr;
  std::shared_ptr<Value> max = nullptr;bool right;
  int indexOfAppliant = -1;
  int rank = 0;
  for(deduced;deduced<end;deduced++){
    if(deduced->symbol == kSymbolDict["NONE"])break;
    if(deduced->symbol == kSymbolDict["single_condition"]){
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
          if(indexOfField < 0){ // parse table
            bool findColumn = false;
            for(deduced;deduced<end;deduced++){
              if(deduced->symbol == kSymbolDict["NONE"])break;
              if(deduced->symbol == kSymbolDict["column"])findColumn = true;
              if(deduced->symbol ==kSymbolDict["NAME"]){
                if(findColumn){
                  // assume schema is in position
                  indexOfField = resource_.schema->GetAttributeIndex(deduced->text);
                }
                else return Status::Corruption("Not known name in sql.");
              }
            }
          }
          else{
            bool isText = false;
            auto status = ParseSimpleExpr(deduced, value, isText);
            if(!status.ok())return status;
            break;
          }
        }
        if(deduced->symbol == kSymbolDict["BOOL_OP"]){
          if(indexOfField < 0)return Status::Corruption("Not known bool operand");
          conditionDesciptor = deduced->text;
        }
      }
      if(indexOfField < 0)return Status::InvalidArgument("Not known column in condition clause.");
      if(resource_.schema->GetIndexHandle(indexOfField) != 0){
        int newRank = RankOfAppliant(indexOfField, conditionDesciptor);
        if(newRank > rank){
          rank = newRank;
          indexOfAppliant = indexOfField;
          switch(_hash<std::string>{}(conditionDesciptor)){
            // =
            case 61:match = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);break;
            // <=
            case 7921:right = true;max = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);break;
            // <
            case 60:right = true;max = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);break;
            // >=
            case 8183:left = true;min = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);break;
            // >
            case 62:left = false;min = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);break;
          }          
        }
        else if(newRank == rank && indexOfField == indexOfAppliant){
          switch(_hash<std::string>{}(conditionDesciptor)){
            // <=
            case 7921:if(!max){right = true;max = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);}break;
            // <
            case 60:if(!max){right = false;max = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);}break;
            // >=
            case 8183:if(!min){left = true;min = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);}break;
            // >
            case 62:if(!min){left = false;min = std::make_shared<Value>(resource_.schema->GetAttribute(indexOfField).type(), value);}break;
          }
        }
      }
      SliceFilterType filter;
      switch(_hash<std::string>{}(conditionDesciptor)){
        // !=
        case 4384:
        // <>
        case 7922:inverse = !inverse;
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
        if(deduced->text.substr(0,3) != "AND" && deduced->text.substr(0,3) != "and")return Status::InvalidArgument("Logic op other than AND is not implemented.");
      }
    }
  }
  if(indexOfAppliant < 0){
    indexOfAppliant = 0;
    assert(!min);
    assert(!max);
    assert(!match);
  }
  bytecodes_.push_back(Instruction(kOpenIndexCursor, resource_.schema->GetAttribute(indexOfAppliant).name() ));
  if(match)bytecodes_.push_back(Instruction(kPrepareMatch, match ));
  else bytecodes_.push_back(Instruction(kPrepareSequence, min, max, left, right));
  // left delete for instruction
  return Status::OK();
}
Status Compiler::RunInstructions(ostream& os){
  int selectSize = -1;
  int deleteSize = -1;
  for(int pc = 0; pc < bytecodes_.size(); pc++){
    Status status;
    switch(bytecodes_[pc].code){
      case kTransaction:
      status = engine.Transaction();break;
      case kOpenRecordCursor:
      status = engine.OpenCursor(resource_.tables[0]);break;
      case kOpenIndexCursor:
      status = engine.OpenCursor(resource_.tables[0], bytecodes_[pc].str_holder);break;
      case kInsertSlice:
      status = engine.InsertSlice( &(resource_.slices[bytecodes_[pc].offset]) );break;
      case kPrepareMatch:
      status = engine.PrepareMatch( bytecodes_[pc].value_holder_1.get() );break;
      case kPrepareSequence:
      status = engine.PrepareSequence( bytecodes_[pc].value_holder_1.get(),\
       bytecodes_[pc].value_holder_2.get(),\
        bytecodes_[pc].left, bytecodes_[pc].right );break;
      case kNextSlice:
      status = engine.NextSlice(resource_.shared_slice);break;
      case kIf:
      status = Status::OK();
      if(CheckSlice())pc += bytecodes_[pc].offset;break;
      case kIfNot:
      status = Status::OK();
      if(!CheckSlice())pc += bytecodes_[pc].offset;break;
      case kIfNil:
      status = Status::OK();
      if(!resource_.shared_slice)pc += bytecodes_[pc].offset;break;
      case kJump:
      status = Status::OK();
      pc += bytecodes_[pc].offset;break;
      case kDeleteSlice:
      if(deleteSize < 0)deleteSize = 0;
      status = engine.DeleteSlice( &(*(resource_.shared_slice)) );
      deleteSize++;
      break;
      case kPrintSlice:
      if(selectSize < 0)selectSize = 0;
      if(!resource_.shared_slice){status = Status::Corruption("Nil slice.");break;}
      if(!resource_.headerDisplayed){
        for(auto& col: resource_.displayColumns){
          os << std::setw(kDisplayWidth) << col << " |";
        }
        os << std::endl;
        resource_.headerDisplayed = true;
      }
      for(auto& col: resource_.displayColumns){
        int idx = resource_.schema->GetAttributeIndex(col);
        if(idx < -1){status = Status::Corruption("Cannot find corresponding field to display.");break;}
        os << std::setw(kDisplayWidth) << std::string(resource_.shared_slice->GetValue(idx)) << " |";
      }
      os << std::endl;
      selectSize ++;
      break;
      default: status = Status::InvalidArgument("Instruction not implemented.");break;

    }
    if(!status.ok()){
      ErrorLog::Fatal(status.ToString().c_str());
      return status;
    }
  }
  // bad_practice()
  assert(deleteSize < 0 || selectSize < 0);
  if(deleteSize >= 0)os << deleteSize << " entries deleted." << std::endl;
  else if(selectSize > 0)os << selectSize << " entries in total." << std::endl;
  else if(selectSize == 0)os << "Empty set." << std::endl;
  return Status::OK();
}
bool Compiler::CheckSlice(void){
  if(!resource_.shared_slice)return false;
  for(auto& filter: resource_.filters){
    if(!filter(*(resource_.shared_slice)))return false;
  }
  return true;
}

} // namespace sbase

