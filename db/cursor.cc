#include ".\db\cursor.h"


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
  if(!data) return ;
  BFlowHeader* header = reinterpret_cast<BFlowHeader*>(data);
  set_.nSize = header->nSize;
  set_.hPri = header->hPri;
  set_.hNext = header->hNext;
  return ;
}
// Shift
Status BFlowCursor::ShiftRight(void){
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr);
  if(set_.hNext == 0)return Status::Corruption("Invalid handle.");
  set_.hPage = set_.hNext;
  return Status::OK();
}
Status BFlowCursor::ShiftLeft(void){
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr);
  if(set_.hPri == 0)return Status::Corruption("Invalid handle.");
  set_.hPage = set_.hPri;
  return Status::OK();
}
Status BFlowCursor::Rewind(void){
  if(root_ == 0)return Status::Corruption("Invalid handle.");
  set_.hPage = root_;
  return Status::OK();
}
// Query
Status BFlowCursor::GetMatch(Value* key, SliceContainer& ret){
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  assert(!key || schema_->GetPrimaryAttr().type() == key->type());
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr);
  char* basePtr = ref.ptr + kBFlowHeaderLen;
  Slice* slice = schema_->NewObject();
  int stripe = schema_->length();
  if(key == nullptr){
    for(int i = 0; i < set_.nSize; i++){
      slice->Read(basePtr + stripe * i);
      ret.push_back((*slice));
    }
    return Status::OK();
  }
  char* match = lower_close_bound(basePtr, set_.nSize, key, stripe);
  if(match == nullptr)return Status::OK();
  Value cur(key->type(), match);
  while(cur == *key){
    slice->Read(match);
    ret.push_back((*slice));
    match += stripe;
    cur.Read(match);
  }
  return Status::OK();
}
Status BFlowCursor::GetInRange(Value* min, Value* max, tuple<bool,bool>& query, SliceContainer& ret){
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  assert(!min || schema_->GetPrimaryAttr().type() == min->type());
  assert(!max || schema_->GetPrimaryAttr().type() == max->type());
  PageRef ref(page_, set_.hPage, kReadOnly); 
  read_page(ref.ptr);
  int stripe = schema_->length();
  char* basePtr = ref.ptr + kBFlowHeaderLen;
  char* endPtr = basePtr + set_.nSize * stripe;
  Slice* slice = schema_->NewObject();
  if(min == nullptr && max == nullptr){
    for(int i = 0; i < set_.nSize; i++){
      slice->Read(basePtr + stripe * i);
      ret.push_back((*slice));
    }
    std::get<0>(query) = true;
    std::get<1>(query) = true;
    return Status::OK();
  }
  if(min == nullptr){
    char* upper ;
    if(std::get<1>(query))upper = upper_close_bound(basePtr, set_.nSize, max,stripe);
    else upper = upper_open_bound(basePtr, set_.nSize, max, stripe);
    char* curSlice = basePtr;
    for(curSlice = basePtr; curSlice <endPtr; curSlice += stripe ){
      slice->Read(curSlice);
      ret.push_back((*slice));
    }
    if(upper >= basePtr + set_.nSize * stripe)std::get<1>(query) = true;
    else std::get<1>(query) = false;
    std::get<0>(query) = true;
    return Status::OK();
  }
  if(max == nullptr){
    char* lower ;
    if(std::get<0>(query))lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
    else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
    char* curSlice = lower;
    for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
      slice->Read(curSlice);
      ret.push_back((*slice));
    }
    if(lower <= basePtr)std::get<0>(query) = true;
    else std::get<0>(query) = false;
    std::get<1>(query) = true;
    return Status::OK();   
  }
  char* lower ;
  if(std::get<0>(query))lower = lower_close_bound(basePtr, set_.nSize, min, stripe);
  else lower = lower_open_bound(basePtr, set_.nSize, min, stripe);
  if(lower <= basePtr)std::get<0>(query) = true;
  else std::get<0>(query) = true;
  char* curSlice = lower;
  Value cur(min->type());
  for(curSlice = lower; curSlice < endPtr; curSlice += stripe){
    cur.Read(curSlice);
    if(std::get<1>(query) && cur > (*max)){ // allow equal
      std::get<1>(query) = false;
      return Status::OK();
    }
    if(!std::get<1>(query) && cur >= (*max)){ // not equal
      std::get<1>(query) = false;
      return Status::OK();
    }
    slice->Read(curSlice);
    ret.push_back((*slice));
  }
  std::get<1>(query) = true;
  return Status::OK();
}
// Modify
Status BFlowCursor::Insert(Slice* record){
  assert(page_->GetPageType(set_.hPage) == kBFlowPage);
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr);
  if(!record)return Status::InvalidArgument("Invalid slice.");
  if(set_.nSize >= kBlockLen / record->length()){
    return Status::IOError("Full page.");
  }
  Value pivot = record->GetValue(0);
  assert(pivot.type() == schema_->GetPrimaryAttr().type());
  int stripe = schema_->length();
  char* start = lower_close_bound(ref.ptr+kBFlowHeaderLen, set_.nSize, &pivot, stripe);
  if(!start)start = ref.ptr + set_.nSize*stripe;
  else{
    Value first(pivot.type(), start);
    if(first == pivot)return Status::InvalidArgument("Duplicate key");
    for(char* top = ref.ptr + kBFlowHeaderLen + set_.nSize*stripe; top > start; top -= stripe){
      memcpy(top, top-stripe, stripe);
    }  
  }
  record->Write(start);
  return Status::OK();
}
Status BFlowCursor::Delete(Value* key){
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr);
  int stripe = schema_->length();
  char* start = lower_close_bound(ref.ptr+kBFlowHeaderLen, set_.nSize, key, stripe);
  if(!start)return Status::OK();
  Value first(key->type(), start);
  if(!(first == *key))return Status::OK(); // no match
  if(start+stripe < ref.ptr + kBFlowHeaderLen +set_.nSize*stripe){
    first.Read(start + stripe);
    if(first == *key)return Status::Corruption("Duplicate key");
  }
  for(char* top = start; top < ref.ptr + kBFlowHeaderLen + set_.nSize*stripe; top += stripe){
    memcpy(top, top+stripe, stripe);
  }
  return Status::OK();
}
Status BFlowCursor::Merge(void){
  return Status::Corruption("Function not supported");
}
Status BFlowCursor::Clear(void){
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr);
  if(set_.nSize > 0)return Status::Corruption("Page not empty.");
  PageRef nextRef(page_, set_.hNext, kLazyModify);
  BFlowHeader* nextHeader = reinterpret_cast<BFlowHeader*>(nextRef.ptr);
  nextHeader->hPri = set_.hPri;
  PageRef priRef(page_, set_.hPri, kLazyModify);
  BFlowHeader* priHeader = reinterpret_cast<BFlowHeader*>(priRef.ptr);
  priHeader->hNext = set_.hNext; 
  auto status = page_->DeletePage(set_.hPage);
  return status;
}
Status BFlowCursor::Split(Value* vRet, PageHandle& hRet){ // return new splited page and pilot key
  // access old page
  PageRef oldRef(page_, set_.hPage, kReadOnly);
  read_page(oldRef.ptr);
  // access new page
  PageHandle hNew;
  auto status = page_->NewPage(set_.hFile, kBFlowPage, hNew);
  if(!status.ok())return status;
  PageRef newRef(page_, hNew, kLazyModify);
  // build new header struct, and prepare header to override
  BFlowHeader leftHeader;
  BFlowHeader* rightHeader = reinterpret_cast<BFlowHeader*>(newRef.ptr);
  leftHeader.nSize = set_.nSize/2+1; // left more
  rightHeader->nSize = set_.nSize - leftHeader.nSize;
  leftHeader.hPri = set_.hPri;
  rightHeader->hPri = set_.hPage;
  leftHeader.hNext = hNew;
  rightHeader->hNext = set_.hNext;
  // copy tail to new page
  char* leftData = oldRef.ptr;
  char* rightData = newRef.ptr;
  memcpy(rightData + kBFlowHeaderLen,\
   leftData + kBFlowHeaderLen + leftHeader.nSize * schema_->length(),\
   rightHeader->nSize * schema_->length() );
  vRet = new Value(vRet->type(), rightData + kBFlowHeaderLen);
  hRet = hNew;
  // now ready to override old page
  if(!oldRef.LiftToWrite()){
    page_->DeletePage(hNew);
    return Status::Corruption("Error locking old page.");
  }
  memcpy(oldRef.ptr, reinterpret_cast<char*>(&leftHeader),kBFlowHeaderLen );
  return Status::OK();
}




// BPlusCursor //
void BPlusCursor::read_page(char* pPage){
  BPlusHeader* header = reinterpret_cast<BPlusHeader*>(pPage);
  set_.nSize = header->nSize;
  set_.hRight = header->hRight;
  return ;
}
Status BPlusCursor::Set(TypeT keyType, PageHandle root){
  if(schema_)delete schema_;
  key_len_ =  Type::getLength(keyType);
  std::vector<Attribute> v{Attribute("Key",keyType),Attribute("Handle",uintT)};
  schema_ = new Schema("", v.begin(), v.end());
  root_ = root;
  set_.hPage = root;
  return Status::OK();
}
Status BPlusCursor::Rewind(void){
  set_.nStackTop = 0;
  set_.hTrace[0] = root_;
  set_.nLevel = 1;
  return Status::OK();
}


Status BPlusCursor::Descend(Value* key){
  PageRef ref(page_, set_.hPage, kReadOnly);
  read_page(ref.ptr);
  if(!key){ // descend at first branch
    auto slice = schema_->NewObject();
    slice->Read(ref.ptr + sizeof(BPlusHeader) + key_len_);
    PageHandle hDown = ( slice->GetValue(1).get<uint32_t>());
    if(hDown == 0)return Status::Corruption("Invalid descend handle.");
    if(page_->GetPageType(hDown) == kBFlowPage){
      // status = kAtBottom
      set_.hDown = hDown;
      return Status::InvalidArgument("Already at bottom");
    }
    set_.hPage = hDown;
    return Status::OK();
  }
  return Status::OK();
}
Status BPlusCursor::Ascend(void){
  if(set_.nStackTop <= 0)return Status::InvalidArgument("No more to ascend.");
  set_.nStackTop--;
  set_.nLevel--;
  set_.hPage = set_.hTrace[set_.nStackTop];
  return Status::OK();
}
Status BPlusCursor::ShiftRight(void){
  if(set_.hRight == 0)return Status::Corruption("Invalid handle.");
  set_.hPage = set_.hRight;
  return Status::OK();
}
Status BPlusCursor::Insert(Value* key, PageHandle handle){
  if(!key)return Status::InvalidArgument("Nil key.");
  assert(page_->GetPageType(set_.hPage) == kBPlusPage);
  PageRef ref(page_, set_.hPage, kLazyModify);
  read_page(ref.ptr);
  int stripe = key_len_ + sizeof(PageHandle);
  if(set_.nSize + 1 >= (kBlockLen-sizeof(BPlusHeader)) / stripe)return Status::IOError("Full page.");
  char* lower = lower_open_bound(ref.ptr+sizeof(BPlusHeader), set_.nSize, key, stripe);
  for(char* cur = ref.ptr + sizeof(BPlusHeader) + stripe*set_.nSize; cur > lower; cur-=stripe){
    memcpy(cur, cur-stripe, stripe);
  }
  auto slice = schema_->NewObject();
  slice->SetValue(0, *key);
  slice->SetValue(1, Value(uintT, new RealValue<uint32_t>(handle)));
  slice->Write(lower);
  return Status::OK();
}
Status BPlusCursor::Delete(Value* key){
  return Status::OK();
}




} // namespace sbase