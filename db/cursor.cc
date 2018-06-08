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

// Common for cursor query
// ret_data >= key
inline char* lower_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur;
  size_t hi = top, mid, lo = 0;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)>>1; // mid > lo
    char* curSlice = data + stripe * mid;
    cur = Value(key->type(), curSlice);
    if( cur == *key){
      return cur;
    }
    else if(cur < *key){
      lo = mid;
    }
    else{
      hi = mid;
    }
  }
  return data + stripe * hi;
}

// BFlowCursor //
inline void BFlowCursor::read_page(char* data){
  BFlowHeader* header = reinterpret_cast<BFlowHeader*>(data);
  set_.oTop = header->oTop;
  set_.hNext = header->hNext;
  pPage = data;
}
// Shift
Status BFlowCursor::Shift(void){
  set_.hPri = set_.hPage;
  set_.hPage = set_.hNext;
  assert(page_->type(set_.hPage) == kBFlowTablePage);
  PageRef ref(page_, set.hPage, kReadOnly); 
  read_page(ref.ptr);
  return Status::OK();
}
Status BFlowCursor::ShiftBack(void){
  set_.hNext = set_.hPage;
  set_.hPage = set_.hPri;
}
// Modify
Status Insert(Slice* record){
  
}
// Query
Status BFlowCursor::GetMatch(Value* key, SliceIterator& ret){
  assert(page_->type(set_.hPage) == kBFlowTablePage);
  PageRef ref(page_, set.hPage, kReadOnly); 
  read_page(ref.ptr);
  char* basePtr = ref.ptr + kBFlowHeaderLen;
  Slice* slice = record_schema_->NewObject();
  int stripe = record_schema_->length();
  if(key == nullptr){
    for(int i = 0; i < set_.oTop; i++){
      slice->Read(basePtr + stripe * i);
      ret.push_back((*slice));
    }
    return Status::OK();
  }
  char* match = lower_bound(basePtr, set_.oTop, key, stripe);
  if(match == nullptr)return Status::OK();
  Value cur = Value(key->type(), match);
  while(cur == key){
    slice->Read(match);
    ret.push_back((*slice));
    match += stripe;
    cur.Read(match);
  }
  return Status::OK();
}
Status BFlowCursor::GetInRange(Value* min, Value* max, tuple<bool,bool>& query, SliceIterator& ret){
  assert(page_->type(set_.hPage) == kBFlowTablePage);
  PageRef ref(page_, set.hPage, kReadOnly); 
  read_page(ref.ptr);
  char* basePtr = ref.ptr + kBFlowHeaderLen;
  Slice* slice = record_schema_->NewObject();
  int stripe = record_schema_->length();
  if(min == nullptr && max == nullptr){
    for(int i = 0; i < set_.oTop; i++){
      slice->Read(basePtr + stripe * i);
      ret.push_back((*slice));
    }
    query[0] = true;
    query[1] = true;
    return Status::OK();
  }
  if(min == nullptr){
    char* upper = lower_bound(basePtr, set_.oTop, max, stripe) - stripe;
    if(query[1]){
      Value cur = Value(key->type());
      while(upper < basePtr + set_.oTop * stripe){
        cur.Read(upper);
        if(cur == key)upper += stripe;
        else break;
      }
    }
    char* curSlice = basePtr;
    while(curSlice < upper){
      slice->Read(curSlice);
      ret.push_back((*slice));      
    }
    if(upper >= basePtr + set_.oTop * stripe)query[1] = true;
    else query[1] = false;
    query[0] = true;
    return Status::OK();
  }
  if(max == nullptr){
    char* lower = lower_bound(basePtr, set_.oTop, max, stripe);
    if(!query[0]){ // left not equal
      Value cur = Value(key->type());
      while(lower < basePtr + set_.oTop * stripe){
        cur.Read(lower);
        if(cur == key)lower += stripe;
        else break;
      }
    }
    char* curSlice = lower;
    while(curSlice <= basePtr + set_.oTop * stripe){
      slice->Read(curSlice);
      ret.push_back((*slice));      
    }
    if(lower <= basePtr)query[0] = true;
    else query[0] = false;
    query[1] = true;
    return Status::OK();   
  }
  char* lower = lower_bound(basePtr, set_.oTop, min, stripe);
  if(!query[0]){ // left not equal
    Value cur = Value(key->type());
    while(lower < basePtr + set_.oTop * stripe){
      cur.Read(lower);
      if(cur == key)lower += stripe;
      else break;
    }
  }
  if(lower <= basePtr)query[0] = true;
  else query[0] = true;
  char* curSlice = lower;
  Value cur = Value(key->type(), lower);
  while(curSlice <= basePtr  + set_.oTop * stripe){
    if(query[1] && cur > (*max)){ // allow equal
      query[1] = false;
      return Status::OK();
    }
    if(!query[1] && cur >= (*max)){ // not equal
      query[1] = false;
      return Status::OK();
    }
    slice->Read(curSlice);
    ret.push_back((*slice));
    curSlice += stripe;
    cur.Read(curSlice);
  }
  query[1] = true;
  return Status::OK();
}



// BPlusCursor //
// Move //
inline Status BPlusCursor::CurseBack(void){
  stack_top_ = 0;

  if(stack_[0] == 0){
    status_ = kNil;
    last_error_ = "Nil Tree";
    stack_top_ = -1;
    level_ = -1;
    return Status::Corruption("Nil Tree");
  }
  level_ = 0;
  return Status::OK();
}
Status BPlusCursor::Descend(void){
  if(!descendable())return Status::InvalidArgument("Not Descendable");
  if(status_ == kRoot && protrude_ == 0){
    // pass
  }
  else if(protrude_ == 0){
    return Status::Corruption("Protrude Not Found");
  }
  else {
    stack_[++stack_top_] = protrude_;
    protrude_ = 0;
    level_ ++;
  }
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
    status_ = kDescendNodePage;
  }
  else {
    status_ = kDescendLeafPage;
  }

  return Status::OK();
}
Status BPlusCursor::Ascend(void){
  if(stack_top_ == 0)return Status::InvalidArgument("At Root Already, Ascend Denied");
  stack_top_--;
  level_ --;
  protrude_ = 0; // clear prodrude
  page_data_ = nullptr;
  assert(page_->type(stack_[stack_top_]) == kBPlusIndexPage);
  if(level_ == 0)status_ = kRoot;
  else status_ = kDescendNodePage;
  return Status::OK();
}
// Update //
// insert only apply on current level
Status BPlusCursor::Insert(PageHandle page){
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
// For Split
// return new node's head key
Status BPlusCursor::Split(PageHandle new_page, char*& new_key){
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
inline Status BPlusCursor::Shift(PageHandle right){
  stack_[stack_top_] = right;
  page_data_ = nullptr;
  return Status::OK();
}



} // namespace sbase