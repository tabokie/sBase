#ifndef SBASE_STORAGE_MEMPOOL_HPP_
#define SBASE_STORAGE_MEMPOOL_HPP_

#include "status.h"
#include "hash.hpp"

#include <cassert>
#include <vector>
#include <queue>

namespace sbase {

using std::vector;
using std::queue;

const size_t kMempoolDefaultBlockSize = 4096;
const size_t kFullSize = 2;
const size_t kInitBlock = 10;

template <typename _ET>
class MemPool{
  using HandleType = _ET;
 private:
  HashMap<HandleType, size_t> pool_;
  vector<char*> blocks_;
  queue<size_t> free_;
  size_t block_size_;

 public:
  MemPool(size_t block = kMempoolDefaultBlockSize):block_size_(block){AllocBlocks(kInitBlock);};
  ~MemPool(){
    for(size_t i = 0; i < blocks_.size(); i++){
      delete [] blocks_[i];
    }
  }
  Status New(HandleType page, size_t size, char*& ret_ptr){
    char* new_ptr = nullptr;
    if(free_.empty()){
      new_ptr = AllocNewBlock();
    }
    size_t idx = free_.front();
    free_.pop();
    pool_.Insert(page, idx);
    new_ptr = blocks_[idx];
    ret_ptr = new_ptr;
    if(!ret_ptr)return Status::Corruption("Cannot Apply for New Block");
    return Status::OK();
  }
  Status Free(HandleType page){
    size_t res = 0;
    if(!pool_.Get(page, res))return Status::NotFound("Hash Get Failed");
    if( res >= blocks_.size() )return Status::Corruption("Hash Return Erroneous Index");
    char* new_ptr = blocks_[res];
    free_.push(res);
    pool_.Delete(page);
    return Status::OK();
  }

  // Accessor //
  inline bool pooling(HandleType page){
    size_t res;
    return pool_.Get(page, res);
  }
  inline char* get_ptr(HandleType page){
    size_t res = blocks_.size();
    if(!pool_.Get(page, res) || res >= blocks_.size()) return nullptr;
    return blocks_[res];
  }
  inline bool full(void){
    return (free_.size() <= kFullSize);
  }
 private:
  inline char* AllocNewBlock(void){
    assert(free_.empty());
    char* new_ptr = new char[block_size_];
    size_t new_page = blocks_.size();
    blocks_.push_back(new_ptr);
    free_.push(new_page);
    return new_ptr;
  }
  inline bool AllocBlocks(size_t size){
    for(int i = 0; i < size; i++){
      blocks_.push_back(new char[block_size_]);
      free_.push(i);
    }
    return true;
  }


};


} // namespace sbase

#endif // SBASE_STORAGE_MEMPOOL_HPP_