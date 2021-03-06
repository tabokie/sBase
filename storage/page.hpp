#ifndef SBASE_STORAGE_PAGE_HPP_
#define SBASE_STORAGE_PAGE_HPP_

#include "./util/utility.hpp"
#include "./util/latch.hpp"
#include "./util/time.hpp"
#include "./storage/file.h"
#include "./util/time.hpp"
#include "./storage/file_format.hpp" // PageType

#include <memory>
#include <cstdint>
#include <memory>
#include <iostream>

namespace sbase{

struct Page: public NoCopy{
  using PagePtr = std::shared_ptr<Page>;
  PageHandle handle;
  const WritableFile* file;
  Latch latch;
  PageType type;
  TimeType modified;
  TimeType commited;
  Page(const WritableFile& f, PageHandle h = 0):file(&f), handle(h), type(kUnknownPage){
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