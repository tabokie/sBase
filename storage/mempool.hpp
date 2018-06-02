#ifndef SBASE_STORAGE_MEMPOOL_HPP_
#define SBASE_STORAGE_MEMPOOL_HPP_

#include "./util/status.hpp"
#include "./util/hash.hpp"
#include "./util/latch.hpp"
#include "./util/error.hpp"

#include <cassert>
#include <vector>
#include <queue>
#include <iostream>

namespace sbase {

using std::vector;
using std::queue;

const size_t kMempoolDefaultBlockSize = 4096;
const size_t kMaxSolidMemory = 100; // blocks

// maybe big block ?
template <typename _ET>
class MemPool{
  using HandleType = _ET;
 private:
  struct PtrWrapper{
    char* ptr;
    size_t size;
    PtrWrapper(size_t s):ptr(nullptr),size(s){
      if(s<0)size = 0;
      if(size > 0){
        ptr = new char[size]();
        if(ptr == NULL || ptr == nullptr){
          ptr = nullptr;
          throw MemoryOutofBounds{};
        }
      }
    }
    ~PtrWrapper(){if(ptr)delete [] ptr;}
  };
  using PtrWrapperPtr = shared_ptr<PtrWrapper>;
  Latch latch; // for now
  vector<PtrWrapperPtr> blocks_;
  vector<size_t> free_;
  HashMap<HandleType, size_t> pool_;
  double approximate_blocks_;
 public:
  MemPool() = default;
  ~MemPool(){ }
  Status Add(HandleType handle, size_t size, char*& ptr){
    // LOG_FUNC();
    latch.WeakWriteLock();
    for(int i = 0; i< free_.size(); i++){
      if(blocks_[free_[i]]->size >= size){
        ptr = blocks_[free_[i]]->ptr;
        free_.erase(free_.begin()+i);
        latch.ReleaseWeakWriteLock();
        return Status::OK();
      }
    }
    if(approximate_blocks_ + static_cast<double>(size) / kMempoolDefaultBlockSize > kMaxSolidMemory){
      latch.ReleaseWeakWriteLock();
      return Status::IOError("Not enough space.");
    }
    approximate_blocks_ += static_cast<double>(size) / kMempoolDefaultBlockSize;
    pool_.Insert(handle, blocks_.size());
    blocks_.push_back(make_shared<PtrWrapper>(size));
    ptr = (*(blocks_.end()-1))->ptr;

    latch.ReleaseWeakWriteLock();
    return Status::OK();
  }
  char* Get(HandleType handle){
    size_t idx;
    auto bool_ret = pool_.Get(handle, idx);
    if(!bool_ret)return nullptr;
    else{
      return blocks_[idx]->ptr;
    }
  }
  Status Delete(HandleType handle){
    // LOG_FUNC();
    latch.WeakWriteLock();
    size_t idx;
    auto bool_ret = pool_.DeleteOnGet(handle, idx);
    if(bool_ret){
      free_.push_back(idx);
    }
    latch.ReleaseWeakWriteLock();
    return Status::OK();
  }
  Status Switch(HandleType before, HandleType after){
    // LOG_FUNC();
    latch.WeakWriteLock();
    size_t idx;
    auto bool_ret = pool_.DeleteOnGet(before, idx);
    if(bool_ret){
      bool_ret = pool_.Insert(after, idx);
    }
    latch.ReleaseWeakWriteLock();
    if(!bool_ret)return Status::Corruption("");
    return Status::OK();
  }
  HandleType IterateHandleNext(void){
    return pool_.IterateKeyNext();
  }
  HandleType SampleHandle(void){
    return pool_.RandomSampleKey();
  }
  // Accessor //
  inline bool pooling(HandleType page){
    size_t res;
    return pool_.Get(page, res);
  }
  inline size_t estimateSpace(void){
    return approximate_blocks_ * kMempoolDefaultBlockSize;
  }

}; // class MemPool


} // namespace sbase

#endif // SBASE_STORAGE_MEMPOOL_HPP_