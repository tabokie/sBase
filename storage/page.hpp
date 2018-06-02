#ifndef SBASE_STORAGE_PAGE_HPP_
#define SBASE_STORAGE_PAGE_HPP_

#include "./util/utility.hpp"
#include "./util/latch.hpp"
#include "./util/time.hpp"
#include "./storage/file.h"
#include "./util/time.hpp"

#include <memory>
#include <cstdint>
#include <memory>

namespace sbase{

enum PageType{
  kEmptyPage = 0,
  kBFlowTablePage = 1,
  kBPlusIndexPage = 2,
  kBIndexPage = 3
};

struct Page: public NonCopy{
  using PagePtr = std::shared_ptr<Page>;
  PageHandle handle;
  const WritableFile* file;
  Latch latch;
  PageType type;
  TimeType modified;
  TimeType commited;
  Page(const WritableFile& f, PageHandle h = 0):file(&f), handle(h){
    commited = Time::Now();
    modified = Time::Now();
  }
  ~Page(){ }
  // 0 for referenced
  inline uint64_t Rank(void){return (Referenced()) ? 0 : Time::Now()-modified ;}
  inline bool Referenced(void){return latch.occupied();}
  // modify <= GetDataPtr, FlushFromDisk
  inline void Modify(void){modified = Time::Now();}
  // commit <= WriteFile, FlushFromMem
  inline void Commit(void){commited = Time::Now();}
};

} // namespace sbase

#endif // SBASE_STORAGE_PAGE_HPP_