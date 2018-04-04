
#include "storage\page_manager.h"
#include "slice.hpp"
#include "storage\status.h"
#include "utility.hpp"
#include <functional>
#include <iostream>
#include <cassert>

using namespace std;

namespace sbase{

// class Tabler : public NoCopy{
//  private:
//   PageManager* page_;
//   BPlusCursor* table_idx_;
//   BFlowCursor* table_;
//   vector<BCursor*> idx_;
//  public:
//   Tabler();
//   ~Tabler();
//   Status Get(Slice key);
//   Status Put(Slice key);
//   Status Free(void);
// }

// // primary
// Status Tabler::Get(Slice key){
//   table_idx_->Reset()
//   while(table_idx_->Descend(key))
//   table_->Get(table_idx_->no);
// }

class Treer{

  Status Insert(char* key){
    page1,page2;
    while(true){
      bplus.Descend(page2);
      if(bplus.notfound){
        bplus.Shift(page2);
      }
      else if(bplus.found)break;
      UnlockRead(page1);
      page1 = page2;
      LockRead(page1);
    }
    // insert to table //
    if(split){
      SplitBPlus(bplus);
    }
  }
  Status SplitBPlus(PageHandle page);

}

// B Tree
// Page Layout :: [1byte size] + [2byte page] + [x'b key | 2byte no | 2byte ptr(no)]*
class BCursor{

}


// B Plus Tree (ommit pivot)
// Page Layout :: [1byte size] + [x'b key | 2 byte ptr] + [x'b key | 2byte ptr]*
// One Step only
// Move //
  // Right Shift
  // Descend
  // Ascend
  // MoveTop
// Update //
  // Insert
  // Split
class BPlusCursor{
  size_t kBPlusStackSize = 8;
  enum BPlusStatus{
    kNormal = 0,
    kMatch = 1,
    kDescended = 2,
    kShifted = 3,
    kNotFound = 4,
    kAtBottom = 5,
    kNilTree = 6
  };
 private:
  // constant
  PageManager* page_;
  Fragment key_; // store data, as base type template
  // backtracking
  PageHandle stack_[kBPlusStackSize];
  int stack_top_;
  size_t level_;
  // page handle
  char* page_data_;
  // cursor status
  BPlusStatus status_;
  // split handler
  char* halted_key_;
  PageHandle halted_page_;
  PageHandle new_page_;
  BPlusStatus status_restore_;
 public:
  BPlusCursor(PageManager* page, Type* type, PageHandle root = 0):
    page_(page), 
    page_data_(nullptr), 
    key_(type), 
    stack_top_(0),
    level_(0){
    stack_[0] = root;
  }
  ~BPlusCursor(){ }
  inline Status SetRoot(PageHandle root){
    stack_top_ = 0;
    stack_[0] = root;
    level_ = 0;
    return Status::OK();
  }
  // Accessors //
  inline BPlusStatus status(void){return status_;}
  inline PageHandle current(void){assert(stack_top_>=0);return stack_[stack_top_];}
  inline Status pop_table(void){assert(status_==kAtBottom);level_ --;return status_[stack_top_--];}
  // Move //
  inline void Assign(char* key){
    key >> key_;
    status_ = kNormal;
    return ;
  }
  inline Status MoveTop(void){
    stack_top_ = 0;
    if(stack_[0] == 0)status_ = kNilTree;
    level_ = 0;
    return Status::OK();
  }
  Status Descend(void){
    if(status_!=kNormal)return Status::InvalidArgument("Not a normal cursor");
    PageHandle page_no = stack_[stack_top_]
    // open page
    assert(page->type(page_no) == kBPlusIndexPage);
    if(!page_data_ && !page_->Read(page_no,page_data_).ok())return Status::IOError("Page failed");
    // make local cursor
    Fragment cur(key_);
    // initial header data
    char* cur_data = page_data_;
    PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
    cur_data += kPageSizeWid;
    cur_data >> cur; // forward ptr
    // should shift right
    if(key_ >= cur){
      PageHandle right = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()));
      return Shift(right);
    }
    size_t offset = key_.length() + kPageHandleWid;
    cur_data += offset;
    // binary searching
    size_t hi = size, mid, lo = 0;
    while(hi - lo >= 2){
      mid = lo+(hi-lo)/2;
      char* frag = cur_data+offset*mid;
      frag >> cur;
      if(cur == key_){
        lo = mid;
        break;
      }
      else if(cur < key_)lo = mid;
      else hi = mid;
    }
    cur_data = cur_data+offset*lo;
    page_no = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()));
    if(page_->type(page_no) == kBPlusIndexPage){
      status_ = kDescended;
    }
    else {
      status_ = kAtBottom;
    }
    stack_[++stack_top_] = page_no;
    level_ ++;
    return Status::OK();
  }
  Status Ascend(void){
    if(stack_top_ == 0)return Status::InvalidArgument("At Root Already, Ascend Denied");
    stack_top_--;
    level_ --;
    page_data_ = nullptr;
    return Status::OK();
  }
  // Update //
  // insert only apply on current level
  Status Insert(PageHandle page){
    if(page == 0)assert(status_ == kAtBottom); // only delete bottom ptr

  }
  // delete at higher level ?
  Status Delete(void){
    return Insert(0); // 0 for nil page no
  }
  // For Split
  // return new node's head key
  Status SplitNew(PageHandle new_page, char*& new_key){
    assert(status_ == kInsertFull);

    status_ = kSplitNew;
  }
  Status SplitOverride(void){
    assert(status_ == kSplitNew);

    status_ = status_restore_;
  }

 private:
  inline Status Shift(PageHandle right){
    stack_[stack_top_] = right;
    page_data_ = nullptr;
    return Status::OK();
  }

}; // class BPlusCursor



} // namespace sbase