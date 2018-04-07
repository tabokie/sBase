#include <cstring>
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

// class Treer{

//   Status Insert(char* key){
//     page1,page2;
//     while(true){
//       bplus.Descend(page2);
//       if(bplus.notfound){
//         bplus.Shift(page2);
//       }
//       else if(bplus.found)break;
//       UnlockRead(page1);
//       page1 = page2;
//       LockRead(page1);
//     }
//     // insert to table //
//     if(split){
//       SplitBPlus(bplus);
//     }
//   }
//   Status SplitBPlus(PageHandle page);

// }

// B Tree
// Page Layout :: [1byte size] + [2byte page] + [x'b key | 2byte no | 2byte ptr(no)]*
// class BCursor{
//   Status Insert
// };

// B Flow Table
// Page Layout :: 
// [1]: [1byte size][x'b key | record]*
// [2]: [1byte head| 1byte tail|2byte next_no][x'b key | record]*
// [3]: [1]
// insert

class BFlowCursor{
  enum BFlowStatus{
    kFragValued = 0,
    kSliceValued = 1
  }
  PageManager* page_;
  PageHandle page_no_;
  char* page_data_;
  Fragment key_;
  Slice record_;
  inline void MoveTo(PageHandle page){
    assert(page_->type(page) == kBFlowTablePage);
    page_no_ = page;
    page_data_ = nullptr;
    return ;
  }
  Status Get(Frag key, Slice& ret){
    if(!page_->type(page_no_) == kBFlowTablePage)MoveTo(head_);
    // open in read mode
    // make local cursor
    Fragment cur(key_);
    // initial header data in section A
    char* a_data = page_data_;
    PageSizeType asize = *(reinterpret_cast<PageSizeType*>(a_data));
    a_data += kPageSizeWid;
    size_t offset = record_.length();
    // initial header data in section B
    char* b_data = page_data_ + kSectionASize;
    PageSizeType bhead = *(reinterpret_cast<PageSizeType*>(b_data));
    PageSizeType btail = *(reinterpret_cast<PageSizeType*>(b_data+kPageSizeWid));
    PageHandle next_page = *(reinterpret_cast<PageSizeType*>(b_data+2*kPageSizeWid))
    b_data += kPageSizeWid*2+kPageHandleWid;
    // check which side
    char* cur_record = b_data + offset * bhead;
    cur_record >> cur;
    if(cur > key_){ // section B probing
      if(bhead > btail){
        btail += kSectionBSize;
      }
      // binary searching
      size_t hi = btail+1, mid, lo = bhead;
      while(hi - lo >= 2){
        mid = (lo+(hi-lo)/2);
        char* cur_record = b_data+offset*(mid%kSectionBSize);
        cur_record >> cur;
        if(cur == key_){
          lo = mid;
          break;
        }
        else if(cur < key_)lo = mid;
        else hi = mid;
      }
      b_data = b_data+offset*(lo%kSectionBSize);
    }
    else{
      size_t hi = asize+1, mid, lo = 0;
      while(hi - lo >= 2){
        mid = lo+(hi-lo)/2;
        char* cur_record = a_data+offset*mid;
        cur_record >> cur;
        if(cur == key_){
          lo = mid;
          break;
        }
        else if(cur < key_)lo = mid;
        else hi = mid;
      }
      a_data = a_data+offset*lo;
    }

  }
  Status Insert(){
    if(!page_->type(page_no_) == kBFlowTablePage)MoveTo(head_);


  }
};


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
    // if full
    // split new
    // insert
    // insert to pather
  // Split
    // make new page
    // narrow the old(override)
class BPlusCursor{
  static const size_t kBPlusStackSize = 8;
  enum BPlusStatus{
    kNormal = 0,
    kMatch = 1,
    kDescended = 2,
    kShifted = 3,
    kFull = 4,
    kSplit = 5,
    kNotFound = 6,
    kNilTree = 7
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
  inline Status pop_table(void){assert(status_==kMatch);level_ --;stack_top_--;return Status::OK();}
  // assign key //
  inline resetStatus(void){status_ = kNormal;return ;}
  friend istream& operator>>(istream& is, BPlusCursor& cursor){
    is >> cursor.key_;
    cursor.resetStatus();
    return is;
  }
  friend char* operator>>(char* in, BPlusCursor& cursor){
    cursor.resetStatus();
    return (in >> cursor.key_());
  }
  // Move //
  inline Status MoveTop(void){
    stack_top_ = 0;
    if(stack_[0] == 0)status_ = kNilTree;
    level_ = 0;
    return Status::OK();
  }
  Status Descend(void){
    if(status_==kMatch)return Status::InvalidArgument("Already Match");
    if(status_==kNotFound)return Status::InvalidArgument("Location not found");
    if(status_==kNilTree)return Status::InvalidArgument("Nil tree");
    PageHandle page_no = stack_[stack_top_];
    // open page
    assert(page_->type(page_no) == kBPlusIndexPage);
    if(!page_data_ && !page_->Read(page_no,page_data_).ok())return Status::IOError("Page failed");
    // make local cursor
    Fragment cur(key_);
    // initial header data
    char* cur_data = page_data_;
    PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
    cur_data += kPageSizeWid;
    cur_data >> cur; // forward ptr
    // should shift right
    if(!(key_ < cur)){ // >=
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
      status_ = kMatch;
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
    if(page == 0)assert(status_ == kMatch); // only delete bottom ptr
    if(status_==kNotFound)return Status::InvalidArgument("Location not found");
    if(status_==kNilTree)return Status::InvalidArgument("Nil tree");

    PageHandle page_no = stack_[stack_top_];
    // open page
    assert(page_->type(page_no) == kBPlusIndexPage);
    if(!page_data_ && !page_->Read(page_no,page_data_).ok())return Status::IOError("Page failed");
    // make local cursor
    Fragment cur(key_);
    // initial header data
    char* cur_data = page_data_;
    PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
    cur_data += kPageSizeWid;
    cur_data >> cur; // forward ptr
    if(key_ < cur){
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
      cur_data >> cur;
      if(cur == key_){ // override
        memcpy(cur_data+key_.length(), (reinterpret_cast<char*>(&page)), kPageHandleWid);
        return Status::OK();
      }
      else if(size < kFullSize){
        status_ = kFull;
        return Status::Corruption("Full Node");
      }
      else{
        cur_data += offset;
        for(int i = size-lo; i >= 1; i--){
          memcpy(cur_data + i*offset, cur_data + (i-1)*offset, offset);
        }
        char* frag_char;
        frag_char << key_;
        memcpy(cur_data, frag_char, key_.length());
        memcpy(cur_data+key_.length(), (reinterpret_cast<char*>(&page)), kPageHandleWid);
        return Status::OK();
      }
    }
    // should shift right // tail recursive
    PageHandle right = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()));
    if(!Shift(right).ok())return Status::Corruption("Shift Failed");
    return Insert(page);    
  }
  // delete at higher level ?
  Status Delete(void){
    return Insert(0); // 0 for nil page no
  }
  // For Split
  // return new node's head key
  Status Split(PageHandle new_page, char*& new_key){
    assert(status_ == kFull);

    // First : make new page //
    PageHandle page_no = stack_[stack_top_];
    // open page
    char* newpage_data = nullptr;
    char* newpage_cur = nullptr;
    assert(page_->type(new_page) == kBPlusIndexPage);
    if(!page_->Read(page_no,newpage_data).ok())return Status::IOError("Page failed");
    // make local cursor
    Fragment cur(key_);
    // initial header data
    char* cur_data = page_data_;
    PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
    size_t init_idx = size / 2 ;
    size_t newpage_size = size - init_idx;
    PageSizeType writable_size = static_cast<PageSizeType>(newpage_size);
    memcpy(newpage_cur, (reinterpret_cast<char*>(&writable_size)), kPageSizeWid);
    newpage_cur += kPageSizeWid;
    cur_data += kPageSizeWid;
    size_t offset = key_.length() + kPageHandleWid;
    memcpy(newpage_cur, cur_data, offset); // forward ptr value
    newpage_cur += offset;
    cur_data += (init_idx+1)*offset;
    memcpy(newpage_cur, cur_data, newpage_size*offset);
    memcpy(new_key, cur_data, offset);

    // Second : override old page //
    cur_data = page_data_;
    size_t oldpage_size = init_idx;
    writable_size = static_cast<PageSizeType>(oldpage_size);
    memcpy(cur_data, (reinterpret_cast<char*>(&writable_size)), kPageSizeWid);
    cur_data += kPageSizeWid;
    memcpy(cur_data, new_key, key_.length());
    cur_data += key_.length();
    memcpy(cur_data, (reinterpret_cast<char*>(&new_page)), kPageHandleWid);

    status_ = kSplit;
    return Status::OK();
  }
 private:
  inline Status Shift(PageHandle right){
    stack_[stack_top_] = right;
    page_data_ = nullptr;
    return Status::OK();
  }

}; // class BPlusCursor



} // namespace sbase