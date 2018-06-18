#include ".\db\cursor.h"
#include <cstdlib>

#ifndef LOGFUNC
#define LOGFUNC()       do{std::cout<<__func__<<std::endl;}while(0)
#endif

namespace sbase{
// Common for modify
inline void shift_right(char* begin, char* end, size_t stripe){
  char* cur;
  for(cur = end-stripe; cur>=begin; cur-=stripe){
    memcpy(cur+stripe, cur, stripe);
  }
  cur += stripe;
  if(cur-begin>0)memcpy(begin+stripe, begin, cur-begin);
  return ;
}
inline void shift_left(char* begin, char* end, size_t stripe){
  char* cur;
  for(cur = begin-stripe;cur+2*stripe<=end; cur+= stripe){
    memcpy(cur, cur+stripe, stripe);
  }
  if(end-cur-stripe>0)memcpy(cur, cur+stripe, end-cur-stripe);
  return ;
}
// Common for cursor query
// ret_data >= key
inline char* lower_close_bound(char* data, size_t top, Value* key, size_t stripe){
  Value cur(key->type());
  int hi = top-1, mid, lo = -1;
  if(lo>=hi)return nullptr;
  cur.Read(data + hi*stripe);
  if(cur < *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)/2; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if( cur == *key){
      while((curSlice-stripe) >= data){
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
  size_t hi = top, mid=0, lo = 0;
  if(top<=0)return nullptr;
  cur.Read(data);
  if(cur > *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)/2; // mid > lo
    char* curSlice = data + stripe * mid;
    cur.Read(curSlice);
    if( cur == *key){
      while((curSlice+stripe) < data + top*stripe){
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
  size_t hi = top-1, mid, lo = -1;
  cur.Read(data + (top-1)*stripe);
  if(cur <= *key)return nullptr;
  while(hi-lo >= 2){
    mid = lo + (hi-lo)/2; // mid > lo
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
    mid = lo + (hi-lo)/2; // mid > lo
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
// Query
Status BFlowCursor::Get(Value* min, Value* max, bool& left, bool& right, SliceContainer& ret){
  // LOGFUNC();
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  assert(!min || schema_->GetPrimaryAttr().type() == min->type());
  assert(!max || schema_->GetPrimaryAttr().type() == max->type());
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr + sizeof(BlockHeader));
  int stripe = schema_->length();
  char* basePtr = ref.ptr + sizeof(BlockHeader) + kBFlowHeaderLen;
  char* endPtr = basePtr + set_.nSize * stripe;
  Slice slice = schema_->NewObject();
  if(min == nullptr && max == nullptr){
    for(int i = 0; i < set_.nSize; i++){
      slice.Read(basePtr + stripe * i);
      ret.push_back(slice);
    }
    left = true;
    right = true;
    return Status::OK();
  }
  if(min == nullptr){ // query x<max
    char* upper ;
    if(right)upper = upper_close_bound(basePtr, set_.nSize, max,stripe);
    else upper = upper_open_bound(basePtr, set_.nSize, max, stripe);
    if(!upper){ // no in this page
      left = true;
      right = true;
      return Status::OK(); 
    }
    char* curSlice = basePtr;
    if(upper>=endPtr)upper = endPtr-stripe;
    for(curSlice = basePtr; curSlice <= upper; curSlice += stripe ){
      slice.Read(curSlice);
      ret.push_back(slice);
    }
    if(upper >= endPtr)left = true;
    else left = false;
    right = true;
    return Status::OK();
  }
  if(max == nullptr){ // query x>min
    char* lower ;
    if(left)lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
    else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
    if(!lower){ // no in this page
      left = false;
      right = true;
      return Status::OK();
    }
    char* curSlice = lower;
    for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
      slice.Read(curSlice);
      ret.push_back(slice);
    }
    if(lower <= basePtr)right = true;
    else right = false;
    left = true;
    return Status::OK();   
  }
  char* lower ;
  if(left)lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
  else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
  if(!lower){ // all smaller that this
    left = false;
    right = true;
    return Status::OK();
  }
  if(lower <= basePtr)left = true;
  else left = true;
  char* curSlice = lower;
  Value cur(min->type());
  for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
    cur.Read(curSlice);
    if(right && cur > (*max)){ // allow equal
      right = false;
      return Status::OK();
    }
    if(!right && cur >= (*max)){ // not equal
      right = false;
      return Status::OK();
    }
    slice.Read(curSlice);
    ret.push_back(slice);
  }
  right = true;
  return Status::OK();
}

// Modify
Status BFlowCursor::Insert(Slice* record){
  // LOGFUNC();
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr + sizeof(BlockHeader));
  if(!record)return Status::InvalidArgument("Invalid slice.");
  if(set_.nSize+1 >= (kBlockLen-sizeof(BlockHeader)-sizeof(BFlowHeader)) / record->length()){
    return Status::IOError("Full page.");
  }
  int stripe = schema_->length();
  char* basePtr = ref.ptr + sizeof(BlockHeader) + sizeof(BFlowHeader);
  char* endPtr = basePtr + stripe*set_.nSize;
  Value pivot = record->GetValue(0);
  assert(pivot.type() == schema_->GetPrimaryAttr().type());
  char* start = lower_close_bound(basePtr, set_.nSize, &pivot, stripe);
  if(!start)start = endPtr;
  else{
    Value first(pivot.type(), start);
    if(first == pivot)return Status::InvalidArgument("Duplicate key");
    shift_right(start, endPtr, stripe);
  }
  BFlowHeader* header = reinterpret_cast<BFlowHeader*>(ref.ptr + sizeof(BlockHeader));
  record->Write(start);
  header->nSize ++;
  return Status::OK();
}
Status BFlowCursor::Delete(Slice* slice){
  // LOGFUNC();
  Value key = slice->GetValue(0);
  assert(schema_->GetAttribute(0).type() == key.type());
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr + sizeof(BlockHeader));
  int stripe = schema_->length();
  char* basePtr = ref.ptr + sizeof(BlockHeader) + sizeof(BFlowHeader);
  char* endPtr = basePtr + stripe * set_.nSize;
  char* start = lower_close_bound(basePtr, set_.nSize, &key, stripe);
  if(!start)return Status::OK();
  Value first(key.type(), start);
  if(!(first == key))return Status::OK(); // no match // ERROR
  int size = 1;
  char* end;
  for(end = start+stripe;end<endPtr;end+= stripe ){
    first.Read(end);
    if(! (first== key))break;
    size++;
  }
  shift_left(end, endPtr, end-start);
  BFlowHeader* header = reinterpret_cast<BFlowHeader*>(ref.ptr+sizeof(BlockHeader));
  header->nSize -= size;

  return Status::OK();
}
Status BFlowCursor::InsertOnSplit(Slice* slice, PageHandle& hRet){ // return new splited page and pilot key
  // LOGFUNC();
  // access old page
  PageRef oldRef(page_, set_.hPage, kIncrementalModify);
  read_page(oldRef.ptr + sizeof(BlockHeader));
  // access new page
  PageHandle hNew;
  auto status = page_->NewPage(set_.hFile, kBFlowPage, hNew);
  if(!status.ok())return status;
  PageRef newRef(page_, hNew, kLazyModify);
  // build new header struct, and prepare header to override
  BFlowHeader leftHeader;
  BFlowHeader* rightHeader = reinterpret_cast<BFlowHeader*>(newRef.ptr+sizeof(BlockHeader));
  leftHeader.nSize = set_.nSize/2+1; // left more
  rightHeader->nSize = set_.nSize - leftHeader.nSize;
  leftHeader.hPri = set_.hPri;
  rightHeader->hPri = set_.hPage;
  leftHeader.hNext = hNew;
  rightHeader->hNext = set_.hNext;
  // copy tail to new page
  int stripe = schema_->length();
  char* leftData = oldRef.ptr+sizeof(BlockHeader) + sizeof(BFlowHeader);
  char* leftEnd = leftData + leftHeader.nSize * stripe;
  char* rightData = newRef.ptr + sizeof(BlockHeader) +sizeof(BFlowHeader);
  char* rightEnd = rightData + rightHeader->nSize * stripe;
  memcpy(rightData, leftEnd, rightHeader->nSize * stripe); // copy tail of left to right

  Value key = slice->GetValue(0);
  Value leftMax(key.type(), leftEnd - stripe );
  if(leftMax<=key){ // put to right   
    char* cur = lower_close_bound(rightData, rightHeader->nSize, &key, stripe);
    if(!cur){ // append to end
      cur = rightEnd;
    }
    else{
      leftMax.Read(cur);
      if(leftMax == key)return Status::InvalidArgument("Duplicate key.");
      shift_right(cur, rightEnd, stripe);
    }
    slice->Write(cur);
    rightHeader->nSize ++;
    slice->Read(rightData); // read pivot  
  }
  else{   
    if(!oldRef.LiftToWrite()){
      page_->DeletePage(hNew);
      return Status::Corruption("Error locking old page.");
    }
    char* cur = lower_close_bound(leftData, leftHeader.nSize, &key, stripe);
    if(!cur){
      cur = leftEnd;
    }
    else{
      leftMax.Read(cur);
      if(leftMax == key)return Status::InvalidArgument("Duplicate key.");
      shift_right(cur, leftEnd, stripe);
    }
    slice->Write(cur);
    leftHeader.nSize ++;
    slice->Read(rightData);
  }
  memcpy(oldRef.ptr+sizeof(BlockHeader), reinterpret_cast<char*>(&leftHeader),sizeof(BFlowHeader) );
  hRet = hNew;
  return Status::OK();
}
void BFlowCursor::Plot(void){
  PageHandle backup = set_.hPage;
  set_.hPage = root_;
  std::cout << "============ BFlow ==============" << std::endl;
  while(true){
    if(set_.hPage == 0 || page_->GetPageType(set_.hPage) != kBFlowPage)break;
    PageRef ref(page_, set_.hPage, kReadOnly);
    BFlowHeader* header = reinterpret_cast<BFlowHeader*>(ref.ptr+sizeof(BlockHeader));
    std::cout << "(Header)" << header->nSize << "(size),(cur) " << set_.hPage << std::endl;
    auto slice = schema_->NewObject();
    char* cur = ref.ptr + sizeof(BlockHeader) + sizeof(BFlowHeader);
    int stripe = schema_->length();
    slice.Read(cur);
    std::cout << std::string(slice.GetValue(0)) << ", " << std::string(slice.GetValue(1)) << " => ";
    slice.Read(cur + stripe * (header->nSize-1));
    std::cout << std::string(slice.GetValue(0)) << ", " << std::string(slice.GetValue(1)) << std::endl;
    set_.hPage = header->hNext;
  }
  set_.hPage = backup;
  return ;
}


// BPlusCursor //
Status BPlusCursor::Descend(Value* key){
  assert(page_->GetPageType(set_.hPage) == kBPlusPage);
  PageRef ref(page_, set_.hPage, kReadOnly);
  read_page(ref.ptr+sizeof(BlockHeader));
  char* basePtr = ref.ptr + sizeof(BlockHeader) + sizeof(BPlusHeader);


  if(!key){ // descend at first branch
    auto slice = schema_->NewObject();
    slice.Read(basePtr);
    PageHandle hDown = ( slice.GetValue(1).get<uint32_t>());
    if(hDown == 0)return Status::Corruption("Invalid descend handle.");
    if(page_->GetPageType(hDown) == kBFlowPage){
      // status = kAtBottom
      set_.hDown = hDown;
      return Status::IOError("Already at bottom");
    }
    set_.hTrace[++set_.nStackTop] = hDown;
    set_.hPage = hDown;
    return Status::OK();
  }
  else{
    int stripe = schema_->length();
    char* match = upper_close_bound(basePtr, set_.nSize, key, stripe);
    auto slice = schema_->NewObject();

    char* cur = basePtr;
    if(!match)return Status::Corruption("No branch to descend");
    slice.Read(match);
    PageHandle hDown = slice.GetValue(1).get<uint32_t>();
    if(hDown == 0)return Status::Corruption("Invalid descend handle.");
    if(page_->GetPageType(hDown) == kBFlowPage){

      set_.hDown = hDown;
      return Status::IOError("Already at bottom");
    }
    set_.hTrace[++set_.nStackTop] = hDown;
    set_.hPage = hDown;
    return Status::OK();
  }
}
Status BPlusCursor::Ascend(void){
  // LOGFUNC();
  if(set_.nStackTop <= 0)return Status::IOError("No more to ascend.");
  set_.nStackTop--;
  set_.hPage = set_.hTrace[set_.nStackTop];
  return Status::OK();
}
Status BPlusCursor::Insert(Value* key, PageHandle handle){
  // LOGFUNC();
  if(!key)return Status::InvalidArgument("Nil key.");
  assert(page_->GetPageType(set_.hPage) == kBPlusPage);
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr+sizeof(BlockHeader));
  int stripe = schema_->length();
  if(set_.nSize + 1 >= (kBlockLen-sizeof(BPlusHeader)-sizeof(BlockHeader)) / stripe)return Status::IOError("Full page.");
  BPlusHeader* header = reinterpret_cast<BPlusHeader*>(ref.ptr + sizeof(BlockHeader));
  char* basePtr = ref.ptr + sizeof(BlockHeader) + sizeof(BPlusHeader);
  char* endPtr = basePtr + set_.nSize * stripe;
  char* lower = lower_close_bound(basePtr, set_.nSize, key, stripe);
  auto slice = schema_->NewObject();
  slice.SetValue(0, *key);
  slice.SetValue(1, Value(uintT, new RealValue<uint32_t>(handle)));  
  if(!lower){
    slice.Write(endPtr);
    header->nSize++;
    return Status::OK();
  }
  Value tmp(key->type(), lower);
  if(tmp == *key)return Status::InvalidArgument("Duplicate key.");
  shift_right(lower, endPtr, stripe);
  slice.Write(lower);
  header->nSize++;
  return Status::OK();
}
Status BPlusCursor::MakeRoot(Value* key, PageHandle& page){
  // LOGFUNC();
  assert(key && key->type() == schema_->GetAttribute(0).type() );
  auto status = Rewind();
  if(!status.ok())return status;
  PageHandle hRoot;
  status = page_->NewPage(set_.hFile, kBPlusPage, hRoot);
  // std::cout << "root page: " << hRoot << std::endl;
  if(hRoot==0||!status.ok())return status;

  PageRef rootRef(page_, hRoot, kLazyModify);
  BPlusHeader* rootHeader = reinterpret_cast<BPlusHeader*>(rootRef.ptr + sizeof(BlockHeader));
  rootHeader->nSize = 2;
  rootHeader->hRight = 0;
  char* rootBase = rootRef.ptr + sizeof(BlockHeader) + sizeof(BPlusHeader);
  auto slice = schema_->NewObject();
  slice.SetValue(0, Value(key->type(), Type::InfinityValue(key->type())));
  slice.SetValue(1, Value(uintT, new RealValue<uint32_t>(root_)));  
  slice.Write(rootBase);
  slice.SetValue(0, *key);
  slice.SetValue(1, Value(uintT, new RealValue<uint32_t>(page)));
  slice.Write(rootBase + schema_->length() );
  // all done
  root_ = hRoot;
  set_.nStackTop = 0;
  set_.hTrace[0] = root_;
  page = root_;
  return Status::OK();
}

Status BPlusCursor::InsertOnSplit(Value* key, PageHandle& page ){
  // LOGFUNC();
  assert(page_->GetPageType(set_.hPage) == kBPlusPage);
  // access old page
  // PageRef oldRef(page_, set_.hPage, kIncrementalModify);
  PageRef oldRef(page_, set_.hPage, kLazyModify);
  read_page(oldRef.ptr + sizeof(BlockHeader));
  // access new page
  PageHandle hNew;
  auto status = page_->NewPage(set_.hFile, kBPlusPage, hNew);
  if(!status.ok())return status;
  PageRef newRef(page_, hNew, kLazyModify);
  // build new header struct, and prepare header to override
  BPlusHeader leftHeader;
  BPlusHeader* rightHeader = reinterpret_cast<BPlusHeader*>(newRef.ptr+sizeof(BlockHeader));
  // std::cout << "before splitting index page: " << set_.hPage << "->" << set_.hRight << std::endl;
  leftHeader.nSize = set_.nSize/2+1; // left more
  rightHeader->nSize = set_.nSize - leftHeader.nSize;
  leftHeader.hRight = hNew;
  rightHeader->hRight = set_.hRight;
  // std::cout << "after: " << leftHeader.hRight << "->" << hNew << "->" << rightHeader->hRight << std::endl;
  // Make rec
  auto slice = schema_->NewObject();
  slice.SetValue(0, *key);
  slice.SetValue(1, Value(uintT, new RealValue<uint32_t>(page)));  
  // copy tail to new page
  char* leftData = oldRef.ptr+sizeof(BlockHeader) + sizeof(BPlusHeader);
  char* rightData = newRef.ptr + sizeof(BlockHeader) +sizeof(BPlusHeader);
  int stripe = schema_->length();

  Value leftEnd(key->type(), leftData + (leftHeader.nSize-1)*stripe );
  if(leftEnd<=(*key)){ // put to right
    memcpy(rightData,\
     leftData + leftHeader.nSize * stripe,\
     rightHeader->nSize * stripe );   
    char* cur = lower_close_bound(rightData, rightHeader->nSize, key, stripe);
    if(!cur){
      cur = rightData + rightHeader->nSize * stripe;
    }
    else shift_right(cur, rightData + rightHeader->nSize * stripe, stripe);
    slice.Write(cur);
    rightHeader->nSize ++;
    key->Read(rightData);
  }
  else{
    memcpy(rightData,\
     leftData + leftHeader.nSize * stripe,\
     rightHeader->nSize * stripe );    
    char* cur = lower_close_bound(leftData, leftHeader.nSize, key, stripe);
    if(!cur){
      cur = leftData + leftHeader.nSize * stripe;
    }
    else shift_right(cur, leftData + leftHeader.nSize * stripe, stripe);
    slice.Write(cur);
    leftHeader.nSize ++;
    key->Read(rightData);
  }
  memcpy(oldRef.ptr+sizeof(BlockHeader), reinterpret_cast<char*>(&leftHeader),sizeof(BPlusHeader) );
  // page = slice.GetValue(1).get<uint32_t>(); // ERROR
  page = hNew;
  return Status::OK();
}
void BPlusCursor::Plot(void){
  PageHandle backup = set_.hPage;
  set_.hPage = root_;
  if(set_.hPage==0 || page_->GetPageType(set_.hPage) != kBPlusPage){
    return ;
  }
  std::cout << "============ Index ==============" << std::endl;
  PageRef ref(page_, set_.hPage, kReadOnly);
  BPlusHeader* header = reinterpret_cast<BPlusHeader*>(ref.ptr + sizeof(BlockHeader));
  std::cout << "(Header)" << header->nSize << "(size),(next) " << header->hRight << std::endl;
  auto slice=  schema_->NewObject();
  char* cur = ref.ptr + sizeof(BlockHeader) +sizeof(BPlusHeader);
  int stripe = schema_->length();
  for(int i = 0;i < header->nSize; i++, cur+=stripe){
    slice.Read(cur);
    std::cout << std::string(slice.GetValue(0)) << ", " << std::string(slice.GetValue(1)) << std::endl;
  }
  set_.hPage = backup;
  return ;
}
Status BPlusCursor::Get(Value* min, Value* max, bool& left, bool& right, std::deque<PageHandle>& ret){
  // LOGFUNC();
  assert(page_->GetPageType(set_.hPage) == kBPlusPage);
  assert(!min || schema_->GetPrimaryAttr().type() == min->type());
  assert(!max || schema_->GetPrimaryAttr().type() == max->type());
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr + sizeof(BlockHeader));
  int stripe = schema_->length();
  char* basePtr = ref.ptr + sizeof(BlockHeader) + kBPlusHeaderLen;
  char* endPtr = basePtr + set_.nSize * stripe;
  Slice slice = schema_->NewObject();
  PageHandle handle;
  if(min == nullptr && max == nullptr){
    for(int i = 0; i < set_.nSize; i++){
      slice.Read(basePtr + stripe * i);
      handle = slice.GetValue(1).get<uint32_t>();
      ret.push_back(handle);
    }
    left = true;
    right = true;
    return Status::OK();
  }
  if(min == nullptr){ // query x<max
    char* upper ;
    if(right)upper = upper_close_bound(basePtr, set_.nSize, max,stripe);
    else upper = upper_open_bound(basePtr, set_.nSize, max, stripe);
    if(!upper){ // no in this page
      left = true;
      right = true;
      return Status::OK(); 
    }
    char* curSlice = basePtr;
    if(upper>=endPtr)upper = endPtr-stripe;
    for(curSlice = basePtr; curSlice <= upper; curSlice += stripe ){
      slice.Read(curSlice);
      handle = slice.GetValue(1).get<uint32_t>();
      ret.push_back(handle);
    }
    if(upper >= endPtr)left = true;
    else left = false;
    right = true;
    return Status::OK();
  }
  if(max == nullptr){ // query x>min
    char* lower ;
    if(left)lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
    else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
    if(!lower){ // no in this page
      left = false;
      right = true;
      return Status::OK();
    }
    char* curSlice = lower;
    for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
      slice.Read(curSlice);
      handle = slice.GetValue(1).get<uint32_t>();
      ret.push_back(handle);
    }
    if(lower <= basePtr)right = true;
    else right = false;
    left = true;
    return Status::OK();   
  }
  char* lower ;
  if(left)lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
  else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
  if(!lower){ // all smaller that this
    left = false;
    right = true;
    return Status::OK();
  }
  if(lower <= basePtr)left = true;
  else left = true;
  char* curSlice = lower;
  Value cur(min->type());
  for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
    cur.Read(curSlice);
    if(right && cur > (*max)){ // allow equal
      right = false;
      return Status::OK();
    }
    if(!right && cur >= (*max)){ // not equal
      right = false;
      return Status::OK();
    }
    slice.Read(curSlice);
    handle = slice.GetValue(1).get<uint32_t>();
    ret.push_back(handle);
  }
  right = true;
  return Status::OK();
}
Status BPlusCursor::Delete(Value* key){
  assert(schema_->GetAttribute(0).type() == key->type());
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr + sizeof(BlockHeader));
  int stripe = schema_->length();
  char* basePtr = ref.ptr + sizeof(BlockHeader) + sizeof(BPlusHeader);
  char* endPtr = basePtr + stripe * set_.nSize;
  char* start = lower_close_bound(basePtr, set_.nSize, key, stripe);
  if(!start)return Status::OK();
  Value first(key->type(), start);
  if(!(first == *key))return Status::OK();
  int size = 1;
  char* end;
  for(end = start+stripe; end<endPtr;end+=stripe){
    first.Read(end);
    if(!(first == *key))break;
    size++;
  }
  shift_left(end, endPtr, end-start);
  BPlusHeader* header = reinterpret_cast<BPlusHeader*>(ref.ptr + sizeof(BlockHeader));
  header->nSize -= size;
  return Status::OK();
}

Status BPlusCursor::DeleteAllPage(void){
  auto status = Rewind();
  if(!status.ok())return status;
  // descend to leaf
  while(true){
    status = Descend(nullptr);
    if(status.IsIOError())break;
    if(!status.ok())return status;
  }
  // delete level by level
  PageRef* ref;
  while(true){
    // delete current level
    while(true){
      ref = new PageRef(page_, set_.hPage, kReadOnly);
      if(!ref->ptr)break;
      read_page(ref->ptr + sizeof(BlockHeader));
      PageHandle right = set_.hRight;
      delete ref;
      status = page_->DeletePage(set_.hPage);
      if(!status.ok())return status;
      if(right == 0)break;
      set_.hPage = right;
    }
    status = Ascend();
    if(status.IsIOError())break; // at top
  }

  return Status::OK();
}

} // namespace sbase
