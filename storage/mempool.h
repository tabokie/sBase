
#include "page_manager.h"
#include "status.h"
#include "hash.hpp"

#include <cassert>
#include <vector>
#include <queue>

namespace sbase {

using std::vector;
using std::queue;

const size_t kBlockSize = 4000;
const size_t kFullSize = 2;
const size_t kInitBlock = 10;

class MemPool{
 private:
  HashMap<PageHandle, size_t> pool_;
  vector<char*> blocks_;
  queue<size_t> free_;

 public:
  MemPool(){AllocBlocks(kInitBlock);};
  ~MemPool();
  Status New(PageHandle page, size_t size, char*& ret_ptr);
  Status Free(PageHandle page);

  // Accessor //
  inline bool pooling(PageHandle page){
    size_t res;
    return pool_.Get(page, res);
  }
  inline char* get_ptr(PageHandle page){
    size_t res;
    pool_.Get(page, res);
    if(res >= blocks_.size()) return nullptr;
    return blocks_[res];
  }
  inline bool full(void){
    return (free_.size() <= kFullSize);
  }
 private:
  inline char* MemPool::AllocNewBlock(void){
    assert(free_.empty());
    char* new_ptr = new char[kBlockSize];
    size_t new_page = blocks_.size();
    blocks_.push_back(new_ptr);
    free_.push(new_page);
    return new_ptr;
  }
  inline bool AllocBlocks(size_t size){
    for(int i = 0; i < size; i++){
      blocks_.push_back(new char[kBlockSize]);
      free_.push(i);
    }
    return true;
  }


};


} // namespace sbase

