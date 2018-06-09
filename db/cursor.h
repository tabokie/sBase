#ifndef SBASE_DB_CURSOR_H_
#define SBASE_DB_CURSOR_H_

#include <cstring>
#include ".\storage\file_format.hpp"
#include ".\storage\page_manager.h"
#include ".\db\slice.hpp"
#include ".\util\status.h"
#include ".\util\utility.hpp"
#include <functional>
#include <iostream>
#include <cassert>

using namespace std;

namespace sbase{

// B Flow Table
// Page Layout :: 
// Common Header + BFlowHeader(oTop,hNext) + Slice*
class BFlowCursor{
  // maintenence
  PageManager* page_; // only access for page type assertion
  PageHandle root_page_;
  Schema* record_schema_;
  // runtime
  struct BFlowPageInfo{
    FileHandle hFile; // for self initialize page
    char* pPage; // for long term possess
    PageHandle hPage;
    // current load, invalidate if pPage is null
    PageHandle hPri;
    PageHandle hNext;
    uint16_t oTop;
  } set_;
 private:
  void read_page(char*);
 public:
  using SliceIterator = arena::SliceIterator;
  // Shift
  Status Shift(void);
  Status ShiftBack(void);
  Status ToRoot(void);
  // Modify
  Status Insert(Slice* record);
  Status Split(void);
  Status Merge(void);
  Status ClearPage(void);
  // Query
  Status GetMatch(Value* key, SliceIterator& ret);
  // in: lequal/requal specify wheather the range is open
  // out: lequal.requal return prediction of neighbor pages
  Status GetInRange(Value* min, Value* max, tuple<bool,bool>& query, SliceIterator& ret);
};

/*
// B Plus Tree //
// For primary index, have forward pointer
// Page Layout :: 
// Common Header [1byte size] + [x'b key | 2 byte ptr] + [x'b key | 2byte ptr]*
class BPlusCursor{
  static const size_t kBPlusStackSize = 8;
  enum BPlusMoveStatus{
    kRoot = 0,
    kNil = 1,
    kDescendNodePage = 2,
    kShiftNodePage = 3,
    kDescendLeafPage = 4
  };
  enum BPlusModifyStatus{
    kEmpty = 0,
    kToOverride = 1,
    kToInsertReady = 2,
    kToInsertFull = 3
  };
 private:
  struct BPlusPageInfo{
    size_t bSize;
    // history handles
    PageHandle hTrace[kBPlusStackSize];
    size_t nStackTop; // stack helper
    // current handle
    PageHandle hRight;
    PageHandle hPage;
    PageHandle hLeft;
    PageHandle hDown; // for descend
    size_t nLevel; // 1 for first level
  } set_;
  // maintenance
  PageManager* page_;
  Value* key;
 public:
  BPlusCursor();
  ~BPlusCursor(){ }
  
  // Accessors //
  inline PageHandle current(void){assert(stack_top_>=0);return stack_[stack_top_];}
 public:
  inline void SetRoot(PageHandle root){
    stack_top_ = 0;
    stack_[0] = root;
    level_ = 0;
    status_ = kRoot;
    return ;
  }
  // Movement //
  Status Descend(void);
  Status Ascend(void);
  Status Shift(PageHandle right);
  // Modify //
  // insert only apply on current level
  // if full, split new, insert, insert to father
  Status Insert(PageHandle page);
  Status Delete(void){return Insert(0);}
  // return new node's head key
  // make new page, narrow the old(override)
  Status Split(PageHandle new_page, char*& new_key);
  // Query //
  // get sequence by getting min(continuity in BFlow), if no, shift
  Status Get(PageHandle& ret);
}; // class BPlusCursor
*/




} // namespace sbase

#endif // SBASE_DB_CURSOR_H_