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
    PageHandle hPage;
    // current load, invalidate if pPage is null
    PageHandle hPri;
    PageHandle hNext;
    uint16_t nSize;
  } set_;
 private:
  inline void read_page(char* data){
    if(!data)std::cout << "nil pointer" << std::endl;
    if(!data) return ;
    BFlowHeader* header = reinterpret_cast<BFlowHeader*>(data);
    set_.nSize = header->nSize;
    set_.hPri = header->hPri;
    set_.hNext = header->hNext;
    // std::cout << "nSize: " << set_.nSize << std::endl;
    // std::cout << "hPri: " << set_.hPri << std::endl;
    // std::cout << "hNext: " << set_.hNext << std::endl;
    return ;
  }
 public:
  BFlowCursor(PageManager* m):page_(m),schema_(nullptr),root_(0){ }
  using SliceContainer = sbase::SliceContainer;
  inline void Set(Schema* schema, PageHandle root){
    schema_ = schema;
    root_ = root;
    set_.hFile = GetFileHandle(root_);
    set_.hPage = root_;
    return ;
  }
  inline Status MoveTo(PageHandle hP){
    if(page_->GetPageType(hP) != kBFlowPage)return Status::InvalidArgument("Not a b flow page handle.");
    set_.hPage = hP;
    return Status::OK();
  }
  // Shift //
  inline Status ShiftRight(void){
    // LOGFUNC();
    assert(page_->GetPageType(set_.hPage) == kBFlowPage);
    PageRef ref(page_, set_.hPage, kReadOnly); 
    read_page(ref.ptr + sizeof(BlockHeader));
    if(set_.hNext == 0)return Status::Corruption("Invalid handle.");
    set_.hPage = set_.hNext;
    return Status::OK();
  }
  inline Status ShiftLeft(void){
    // LOGFUNC();
    assert(page_->GetPageType(set_.hPage) == kBFlowPage);
    PageRef ref(page_, set_.hPage, kReadOnly); 
    read_page(ref.ptr + sizeof(BlockHeader));
    if(set_.hPri == 0)return Status::Corruption("Invalid handle.");
    set_.hPage = set_.hPri;
    return Status::OK();
  }
  inline Status Rewind(void){
    // LOGFUNC();
    if(root_ == 0)return Status::Corruption("Invalid handle.");
    set_.hPage = root_;
    return Status::OK();
  }
  inline PageHandle rightHandle(void){
    assert(page_->GetPageType(set_.hPage) == kBFlowPage);
    PageRef ref(page_, set_.hPage, kReadOnly);
    read_page(ref.ptr+sizeof(BlockHeader));
    return set_.hNext;
  }
  inline PageHandle leftHandle(void){
    assert(page_->GetPageType(set_.hPage) == kBFlowPage);
    PageRef ref(page_, set_.hPage, kReadOnly);
    read_page(ref.ptr+sizeof(BlockHeader));
    return set_.hPri;
  }
  inline PageHandle currentHandle(void){
    return set_.hPage;
  }
  // Query //
  Status Get(Value* min, Value* max, bool& left, bool& right, SliceContainer& ret);
  // Modify //
  Status InsertOnSplit(Slice* slice, PageHandle& ret);
  Status Delete(Value* val);
  Status Insert(Slice* record);
  void Plot(void);
};

// B Plus Tree //
// For primary index, have forward pointer
// Page Layout :: 
// Common Header [1byte size] + [2 byte ptr] + [x'b key | 2byte ptr]*
class BPlusCursor{
  static const size_t kBPlusStackSize = 8;
 private:
  struct BPlusPageInfo{
    FileHandle hFile;
    // history handles
    PageHandle hTrace[kBPlusStackSize];
    int nStackTop; // stack helper
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

  inline void read_page(char* pPage){
    // LOGFUNC();
    if(!pPage)return ;
    BPlusHeader* header = reinterpret_cast<BPlusHeader*>(pPage);
    set_.nSize = header->nSize;
    set_.hRight = header->hRight;
    // std::cout << "nSize: " << set_.nSize << std::endl;
    // std::cout << "hRight: " << set_.hRight << std::endl;
    return ;
  }
 public:
  using HandleContainer = std::vector<PageHandle>;
  BPlusCursor(PageManager* m):page_(m),schema_(nullptr){ }
  ~BPlusCursor(){ if(schema_)delete schema_; }

  inline Status Set(TypeT keyType, PageHandle root){
    // LOGFUNC();
    if(schema_)delete schema_;
    key_len_ =  Type::getLength(keyType);
    std::vector<Attribute> v{Attribute("Key",keyType),Attribute("Handle",uintT)};
    schema_ = new Schema("", v.begin(), v.end());
    root_ = root;
    set_.hPage = root;
    set_.hTrace[set_.nStackTop=0]=root_;
    set_.hFile = GetFileHandle(root_);
    return Status::OK();
  }
  inline Status Rewind(void){
    // LOGFUNC();
    set_.nStackTop = 0;
    set_.hTrace[0] = root_;
    set_.hPage = root_; // ERROR
    return Status::OK();
  }
  inline PageHandle protrude(void){return set_.hDown;}
  inline Status ShiftRight(void){
    // LOGFUNC();
    if(set_.hRight == 0)return Status::Corruption("Invalid handle.");
    set_.hPage = set_.hRight;
    return Status::OK();
  }
  Status Descend(Value* key);
  Status Ascend(void);
  Status MakeRoot(Value* key, PageHandle& page);
  Status Insert(Value* key, PageHandle handle);
  Status InsertOnSplit(Value* key, PageHandle& page );
  void Plot(void);

}; // class BPlusCursor





} // namespace sbase

#endif // SBASE_DB_CURSOR_H_