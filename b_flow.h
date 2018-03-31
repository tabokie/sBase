
#include "storage\page_manager.h"
#include "slice.hpp"
#include "storage\status.h"
#include "utility.hpp"
#include <functional>
#include <iostream>

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

// Page Layout :: [1byte size][x'b key | 4byte no | 4byte ptr(no)]
class BPlusCursor{
 private:
  PageManager* page_;
  Fragment key_; // store data, as base type template
  PageHandle page_no_;
  char* page_data_;
 public:
  BPlusCursor(PageManager* page, Type* type):page_(page), page_data_(nullptr), key_(type){}
  ~BPlusCursor(){ }
  inline void MoveTo(PageHandle phandle){page_no_ = phandle; page_data_ = nullptr; return;}
  Status Descend(char* key);
  Status Find(char* key, char*&, PageHandle&);
  // Accessors
  PageHandle current(void){return page_no_;}
 private:
};

Status BPlusCursor::Descend(char* key){
  char* key_ptr = nullptr;
  PageHandle page_ptr;
  Find(key, key_ptr, page_ptr );
  page_no_ = page_ptr;
  page_data_ = nullptr; // safe to mempool ?
  return Status::OK();
}
Status BPlusCursor::Find(char* key, char*& key_ptr, PageHandle& page_ptr){
  if(!page_data_ && !page_->Read(page_no_, page_data_).ok())return Status::IOError("Cannot Fetch Page");
  // make key
  key >> key_;
  Fragment cur(key_);
  // get offset and size
  char* cur_data = page_data_;
  size_t offset = key_.length() + kPageHandleWid*2;
  size_t cur_no = 0;
  PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
  PageHandle lower_bound = *(reinterpret_cast<PageHandle*>(cur_data+kPageSizeWid));
  cur_data = page_data_+kPageHandleWid+kPageSizeWid;
  cur_data >> cur ;
  if(key_ < cur){ // match lower bound
    key_ptr = nullptr;
    page_ptr = lower_bound;
    return Status::OK();
  }
  // binary searching
  size_t hi = size, mid, lo = 0;
  while(hi - lo >= 2){
    mid = lo+(hi-lo)/2;
    cur_data = page_data_+kPageSizeWid+kPageHandleWid+offset*mid;
    cur_data >> cur;
    if(cur == key_){
      key_ptr = cur_data;
      page_ptr = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()));
      return Status::OK();
    }
    else if(cur < key_)lo = mid;
    else hi = mid;
  }
  cur_data = page_data_+kPageSizeWid+kPageHandleWid+offset*lo;
  key_ptr = nullptr;
  page_ptr = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()+kPageHandleWid));
  return Status::OK();
}

// template <typename _ET>
// class BCursor{
//   Status 
// }
/*
bpluscursor
modify(key, new_key)
find(key)

bcursor
modify
find 
shift

bflowcursor **
if(not slot A empty){
  memmove
}
if(slot B full){
  if(slot A empty){
    write to slot A
  }
  else{
    write to buffer
  }
}
if(slot C empty){
  copy, flow
}

*/

// class BFlowCursor{

// }





} // namespace sbase