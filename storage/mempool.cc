#include "mempool.h"

namespace sbase {

MemPool::~MemPool(void){
  for(size_t i = 0; i < blocks_.size(); i++){
    delete [] blocks_[i];
  }
}

Status MemPool::New(PageHandle page, size_t size, char*& ret_ptr){
  char* new_ptr = nullptr;
  if(free_.empty()){
    new_ptr = AllocNewBlock();
  }
  else{
    size_t page = free_.front();
    free_.pop();
    new_ptr = blocks_[page];  
  }
  ret_ptr = new_ptr;
  if(!ret_ptr)return Status::Corruption("Cannot Apply for New Block");
  return Status::OK();
}

Status MemPool::Free(PageHandle page){
  size_t res = 0;
  if(!pool_.Get(page, res))return Status::NotFound("Hash Get Failed");
  if( res >= blocks_.size() )return Status::Corruption("Hash Return Erroneous Index");
  char* new_ptr = blocks_[res];
  free_.push(res);
  pool_.Delete(page);
  return Status::OK();
}




} // namespace sbase

