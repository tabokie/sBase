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
inline char* lower_close_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur(key->type());
  size_t hi = top, mid, lo = 0;
  cur.Read(data + (top-1)*stripe);
  if(cur < *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)>>1; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if( cur == *key){
      while(curSlice >= data){
        curSlice -= stripe;
        cur.Read(curSlice);
        if(cur < *key)return curSlice + stripe;
      }
      return curSlice;
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
// ret_data <= key
inline char* upper_close_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur(key->type());
  size_t hi = top, mid, lo = 0;
  cur.Read(data);
  if(cur > *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)>>1; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if( cur == *key){
      while(curSlice < data + top*stripe){
        curSlice += stripe;
        cur.Read(curSlice);
        if(cur > *key)return curSlice - stripe;
      }
      return curSlice;
    }
    else if(cur < *key){
      lo = mid;
    }
    else{
      hi = mid;
    }
  }
  return data + stripe * lo;
}
// ret_data > key
inline char* lower_open_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur(key->type());
  size_t hi = top, mid, lo = 0;
  cur.Read(data + (top-1)*stripe);
  if(cur <= *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)>>1; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if(cur <= *key){
      lo = mid;
    }
    else{
      hi = mid;
    }
  }
  return data + stripe * hi;
}
// ret_data < key
inline char* upper_open_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur(key->type());
  size_t hi = top, mid, lo = 0;
  cur.Read(data);
  if(cur >= *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)>>1; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if(cur < *key){
      lo = mid;
    }
    else{
      hi = mid;
    }
  }
  return data + stripe * lo;
}


// BFlowCursor //
inline void BFlowCursor::read_page(char* data){
  BFlowHeader* header = reinterpret_cast<BFlowHeader*>(data);
  set_.oTop = header->oTop;
  set_.hPri = header->hPri;
  set_.hNext = header->hNext;
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
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page();
  if(set_.oTop >= kMaxSliceNum){
    return Status::IOError("Full page.");
  }
  Value* pivot = record.get_primary();
  int stripe = record_schema_->length();
  char* start = lower_close_bound(ref.ptr+kBFlowHeaderLen, set_.oTop, pivot, stripe);
  Value first(pivot->type(), start);
  if(start == *pivot)return Status::InvalidArgument("Duplicate key");
  if(start)
  for(char* top = ref.ptr + kBFlowHeaderLen + set_.oTop*stripe; top > start; top -= stripe){
    memcpy(top, top-stripe, stripe);
  }
  else start = ref.ptr + top*stripe;
  record->Write(start);
  return Status::OK();
}
Status Delete(Value* key){
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page();
  int stripe = record_schema_->length();
  char* start = lower_close_bound(ref.ptr+kBFlowHeaderLen, set_.oTop, key, stripe);
  if(!start)return Status::OK();
  Value first(pivot->type(), start);
  if(start != *pivot)return Status::OK(); // no match
  if(start+stripe < ref.ptr + kBFlowHeaderLen +set_.oTop*stripe){
    first.Read(start + stripe);
    if(first == key)return Status::Corruption("Duplicate key");
  }
  for(char* top = start; top < ref.ptr + kBFlowHeaderLen + set_.oTop*stripe; top += stripe){
    memcpy(top, top+stripe, stripe);
  }
  return Status::OK();
}
Status Merge(void){
  return Status::Corruption("Function not supported");
}
Status ClearPage(void){
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr);
  if(set_.oTop > 0)return Status::Corruption("Page not empty.");
  PageRef nextRef(page_, set_.hNext, kLazyModify);
  BFlowHeader* nextHeader = reinterpret_cast<BFlowHeader*>(nextRef.ptr);
  nextHeader->hPri = set_.hPri;
  PageRef priRef(page_, set_.hPri, kLazyModify);
  BFlowHeader* priHeader = reinterpret_cast<BFlowHeader*>(priRef.ptr);
  priHeader->hNext = set_.hNext; 
  auto status = page_->DeletePage(set_.hPage);
  return status;
}
Status Split(Value* pilot, PageHandle& hRet){ // return new splited page and pilot key
  // access old page
  PageRef oldRef(page_, set_.hPage, kReadOnly);
  read_page();
  // access new page
  PageHandle hNew;
  auto status = page_->NewPage(set_.hFile, kBFlowTablePage, hNew);
  if(!status.ok())return status;
  PageRef newRef(page_, hNew, kLazyModify);
  // build new header struct, and prepare header to override
  BFlowHeader leftHeader;
  BFlowHeader* rightHeader = reinterpret_cast<BFlowHeader*>(newRef.ptr);
  leftHeader.oTop = set_.oTop/2+1; // left more
  rightHeader->oTop = set_.oTop - left->oTop;
  leftHeader.hPri = set_.hPri;
  rightHeader->hPri = set_.hPage;
  leftHeader.hNext = hNew;
  rightHeader->hNext = set_.hNext;
  // copy tail to new page
  char* leftData = oldRef->ptr;
  char* rightData = newRef->ptr;
  memcpy(rightData + kBFlowHeaderLen,\
   leftData + kBFlowHeaderLen + leftHeader.oTop * record_schema_->length(),\
   rightHeader->oTop * record_schema_->length() );
  pilot = new Value(key->type(), rightData + kBFlowHeaderLen);
  hRet = hNew;
  // now ready to override old page
  if(!oldRef.LiftToWrite()){
    DeletePage(hNew);
    return Status::Corruption("Error locking old page.");
  }
  memcpy(oldRef.ptr, reinterpret_cast<char*>(&leftHeader),kBFlowHeaderLen );
  return Status::OK();
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
  int stripe = record_schema_->length();
  char* basePtr = ref.ptr + kBFlowHeaderLen;
  char* endPtr = basePtr + set_.oTop * stripe;
  Slice* slice = record_schema_->NewObject();
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
    char* upper ;
    if(query[1])upper = upper_close_bound(basePtr, set_.oTop, max,stripe);
    else upper = upper_open_bound(basePtr, set_.oTop, max, stripe);
    char* curSlice = basePtr;
    for(curSlice = basePtr; curSlice <endPtr; curSlice += stripe ){
      slice->Read(curSlice);
      ret.push_back((*slice));
    }
    if(upper >= basePtr + set_.oTop * stripe)query[1] = true;
    else query[1] = false;
    query[0] = true;
    return Status::OK();
  }
  if(max == nullptr){
    char* lower ;
    if(query[0])lower = lower_close_bound(basePtr, set_.oTop, min, stripe);
    else lower = lower_open_bound(basePtr, set_.oTop, min, stripe);
    char* curSlice = lower;
    for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
      slice->Read(curSlice);
      ret.push_back((*slice));
    }
    if(lower <= basePtr)query[0] = true;
    else query[0] = false;
    query[1] = true;
    return Status::OK();   
  }
  char* lower ;
  if(query[0])lower = lower_close_bound(basePtr, set_.oTop, min, stripe);
  else lower = lower_open_bound(basePtr, set_.oTop, min, stripe);
  if(lower <= basePtr)query[0] = true;
  else query[0] = true;
  char* curSlice = lower;
  Value cur(key->type());
  for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
    cur.Read(curSlice);
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
  }
  query[1] = true;
  return Status::OK();
}



// BPlusCursor //




} // namespace sbase