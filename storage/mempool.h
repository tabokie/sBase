
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

class MemPool{
 private:
  HashMap<PageHandle, size_t> pool_;
  vector<char*> blocks_;
  queue<size_t> free_;

 public:
  MemPool() = default;
  ~MemPool();
  Status New(PageHandle page, size_t size);
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
    if(res < 0 || res >= blocks_.size()) return nullptr;
    return blocks_[res];
  }
 private:
  char* AllocNewBlock(void);

};


} // namespace sbase

