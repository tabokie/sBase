#ifndef SBASE_STORAGE_MEMPOOL_HPP_
#define SBASE_STORAGE_MEMPOOL_HPP_

#include "./util/status.h"
#include "./util/hash.hpp"

#include <cassert>
#include <vector>
#include <queue>

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
    bool alloc; // true for heap allocated
    // for LRU
    MinTimeType last_decr;
    uint8_t count;
    // for consistency logging
    MinTimeType last_commit;
    MinTimeType last_modify;
    PtrWrapper(char* p, size_t s):
      ptr(p),size(s),alloc(false)
      last_decr(Time::Now_min()),count(1),
      last_commit(0),last_modify(0){
      if(!ptr){
        alloc = true;
        ptr = new char[size];
      }
      assert(ptr);
    }
    // only one unique master responsible for memory free
    PtrWrapper(const PtrWrapper& that):ptr(that.ptr),size(that.size),
      last_decr(that.last_decr),count(that.count),
      last_commit(that.last_commit),last_modify(that.last_modify){ }
    PtrWrapper(const PtrWrapper const* thatp) = delete; // non ptr copy
    inline void Incr(void){
      Update();
      if(count == 255)return ;
      double random_checker = Random::getDoubleRand();
      double base = count - 1;
      if(base < 0)base = 0;
      double p = 1.0 / (base * 2.0 + 1);
      if(r < p) count ++;
    }
    inline void Update(void){
      int addition_time = Time::Now_min() - last_decr;
      while(addition_time -- && count > 0)count /= 2;
    }
    ~PtrWrapper(){if(alloc && ptr) delete [] ptr;}
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
  Status Add(HandleType handle, char*& ptr, size_t size){
    latch.WeakWriteLock();
    if(!ptr){
      for(int i = 0; i< free_.size(; i++)){
        if(blocks_[idx].size >= size){
          ptr = blocks_[idx].ptr;
          free_.erase(free.begin()+i);
          goto MEMPOOL_ADD_DONE_;
        }
      }
      approximate_blocks_ += static_cast<double>(size) / kMempoolDefaultBlockSize;
      while(approximate_blocks_ > kMaxSolidMemory){
        if(!DumpFree())return Status::IOError("Not enough space.");
      }
      pool_.Insert(handle, blocks_.size());
      blocks_.push_back(make_shared<PtrWrapperPtr>(nullptr, size));
      ptr = (*blocks_.end()).ptr;
    }
    else{
      pool_.Insert(handle, blocks_.size());
      blocks_.push_back(make_shared<PtrWrapper>(ptr, size));
    }
    MEMPOOL_ADD_DONE_:
    latch.ReleaseWeakWriteLock();
    return Status::OK();
  }
  char* Get(HandleType handle){
    size_t idx;
    auto bool_ret = pool_.Get(handle, idx);
    if(!bool_ret)return nullptr;
    else{
      return blocks_[idx].ptr;
    }
  }
  Status Delete(HandleType handle){
    size_t idx;
    auto bool_ret = pool_.DeleteOnGet(handle, idx);
    if(bool_ret){
      if(!blocks_.alloc) // map
        blocks_.erase(blocks_.begin()+idx);
      else
        free_.push_back(idx);
    }
    return Status::OK();
  }
  // Accessor //
  inline bool pooling(HandleType page){
    size_t res;
    return pool_.Get(page, res);
  }
  inline size_t estimateSpace(void){
    return approximate_blocks_ * kMempoolDefaultBlockSize;
  }
 private:
  bool DumpFree(void){
    size_t freeSize = free_.size();
    size_t idx = 0;
    if(freeSize == 0)return false;
    else if(freeSize >= 2){
      auto idx0 = Random::getIntRandom(freeSize);
      do{auto idx1 = Random::getIntRandom(freeSize);}while(idx1==idx0);
      idx = idx0;
      if(blocks_[free_[idx0]].size > blocks_[free[idx1]].size)idx = idx1;
    }
    size_t pageIdx = free_[idx];
    free_.erase(free_.begin() + idx);
    blocks_.erase(blocks_.begin() + pageIdx);
    return true;
  }
}; // class MemPool


} // namespace sbase

#endif // SBASE_STORAGE_MEMPOOL_HPP_