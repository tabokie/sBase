
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
  // Accessors
  PageHandle current(void){return page_no_;}
 private:
};

Status BPlusCursor::Descend(char* key){
  // read page data
  if(!page_data_)cout << page_->Read(page_no_, page_data_).ToString() << endl;
  char* cur_data = page_data_;
  // make fragment
  key >> key_;
  Fragment cur(key_);
  // get offset and size
  size_t offset = key_.length() + kPageHandleWid*2;
  size_t cur_no = 0;
  PageSizeType size = *(reinterpret_cast<PageSizeType*>(cur_data));
  // begin searching
  cur_data += kPageSizeWid;
  while(cur_no < size){
    cur_no ++;
    cur_data >> cur;

    if(cur == key_){
      page_no_ = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()));
      page_data_ = nullptr;
      break;
    }
    else if(cur < key_){
      page_no_ = *(reinterpret_cast<PageHandle*>(cur_data+key_.length()+kPageHandleWid));
      page_data_ = nullptr;
    }
    else break;
    cur_data += offset;
  }
  return Status::OK();
}

// template <typename _ET>
// class BCursor{
//   Status 
// }
/*

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