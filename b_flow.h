
#include "storage\page_manager.h"
#include "slice.hpp"
#include "storage\status.h"
#include "utility.hpp"
#include <functional>

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

class BPlusCursor{
 private:
  PageManager* page_;
  Fragment key_; // store data, as base type template
  PageHandle cur_page_;
 public:
  FileMeta
  Status Fetch();
  Status Split();
  Status Insert();
  Status Descend();
  Status Climb();
  Status Shift();
  PageHandle& current(void);
 private:
}

Status BPlusCursor::Descend(char* key){
  key >> key_;
  Fragment cur(key_);
  size_t offset = key_.len + kPageHandleWid;
  size_t no = 0;
  char* page;
  page_->Read(cur_page_, page);
  PageSizeType size = *(reinterpret_cast<PageSizeType*>(page));
  page+= kPageSizeWid;
  while(no < size){
    page = page + no*offset;
    no++;
    page >> cur;
    if(cur == key_){
      cur_page_ = *(reinterpret_cast<PageHandle*>(page));
      break;
    }
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