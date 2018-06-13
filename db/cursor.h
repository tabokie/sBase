#ifndef SBASE_DB_CURSOR_H_
#define SBASE_DB_CURSOR_H_

#include <cstring>
#include ".\storage\page_ref.hpp"
#include ".\util\reflection.hpp"
#include ".\storage\file_format.hpp"
#include ".\storage\page_manager.h"
#include ".\db\slice.hpp"
#include ".\util\status.hpp"
#include ".\util\utility.hpp"
#include <functional>
#include <iostream>
#include <cassert>
#include <cstdint>

using namespace std;

namespace sbase{

enum QueryStatus{
  kQuerySuccess = 0,
  kAllLargerThan = 1,
  kAllSmallerThen = 2,
  kNotFoundMidst = 3
};

enum MovementStatus{
  kMoveSuccess = 0,
  kInvalidHandle = 1
};

// B Flow Table
// Page Layout :: 
// Common Header + BFlowHeader(oTop,hNext) + Slice*
class BFlowCursor{
  // maintenence
  PageManager* page_; // only access for page type assertion
  PageHandle root_;
  Schema* schema_;
  // runtime
  struct BFlowPageInfo{
    FileHandle hFile; // for self initialize page
    PageRef* refPage = nullptr; // long term procession
    PageHandle hPage;
    // current load, invalidate if pPage is null
    PageHandle hPri;
    PageHandle hNext;
    uint16_t nSize;
  } set_;
 private:
  void read_page(char*);
 public:
  BFlowCursor(PageManager* m):page_(m),schema_(nullptr),root_(0){ }
  using SliceContainer = sbase::SliceContainer;
  void Set(Schema* schema, PageHandle root){
    schema_ = schema;
    root_ = root;
    return ;
  }
  Status MoveTo(PageHandle hP){
    if(page_->GetPageType(hP) != kBFlowPage)return Status::InvalidArgument("Not a b flow page handle.");
    set_.hPage = hP;
    return Status::OK();
  }
  // Shift //
  Status ShiftRight(void);
  Status ShiftLeft(void);
  Status Rewind(void);
  PageHandle leftHandle(void){
    assert(page_->GetPageType(set_.hPage) == kBFlowPage);
    PageRef ref(page_, set_.hPage, kReadOnly);
    read_page(ref.ptr+sizeof(BlockHeader));
    return set_.hNext;
  }
  PageHandle currentHandle(void){
    return set_.hPage;
  }
  // Query //
  Status Get(Value* min, Value* max, bool& left, bool& right, SliceContainer& ret);
  // Status GetMatch(Value* key, SliceContainer& ret);
  // in: lequal/requal specify wheather the range is open
  // out: lequal.requal return prediction of neighbor pages
  // Status GetInRange(Value* min, Value* max, tuple<bool,bool>& query, SliceContainer& ret);
  // Modify //
  // page
  Status Split(Value* val, PageHandle& ret);
  Status Merge(void);
  Status Clear(void); // clear page with no resident
  // slice
  Status Delete(Value* val);
  Status Insert(Slice* record);
};

// B Plus Tree //
// For primary index, have forward pointer
// Page Layout :: 
// Common Header [1byte size] + [2 byte ptr | x'b key] + [x'b key | 2byte ptr]*
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
    // history handles
    PageHandle hTrace[kBPlusStackSize];
    int nStackTop; // stack helper
    int nLevel; // 1 for first level
    // load
    PageHandle hPage;
    uint16_t nSize;
    PageHandle hRight;
    PageHandle hDown;
  } set_;
  // maintenance
  PageManager* page_;
  int key_len_;
  Schema* schema_;
  PageHandle root_;
 public:
  using HandleContainer = std::vector<PageHandle>;
  BPlusCursor(PageManager* m):page_(m),schema_(nullptr){ }
  ~BPlusCursor(){ if(schema_)delete schema_; }
  void read_page(char* ptr);
  Status Set(TypeT keyType, PageHandle root);
  Status Rewind(void);
  PageHandle protrude(void){return set_.hDown;}
  // Movement //
  Status Descend(Value* key);
  Status Ascend(void);
  Status ShiftRight(void);
  // Modify //
  // insert only apply on current level
  // if full, split new, insert, insert to father
  Status Insert(Value* key, PageHandle page);
  // Status Delete(Value* key){return Insert(key, 0);} // lazy delete
  Status Delete(Value* key);
  // return new node's head key
  // make new page, narrow the old(override)
  // Status Split(PageHandle new_page, char*& new_key);
  // Query //
  // get sequence by getting min(continuity in BFlow), if no, shift
  Status GetMatch(Value* key, HandleContainer& ret);
  // Status GetInRange(Value* min, Value* max, tuple<bool,bool>& query, HandleContainer& ret);
  // min prior
  // Status GetFirstInRange(Value* min, Value* max, HandleContainer& ret); // for primary, using continuity of BFlow

}; // class BPlusCursor





} // namespace sbase

#endif // SBASE_DB_CURSOR_H_