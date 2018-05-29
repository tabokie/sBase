#ifndef SBASE_STORAGE_PAGE_HPP_
#define SBASE_STORAGE_PAGE_HPP_

#include "./util/utility.hpp"
#include "./storage/latch.hpp"
#include "./util/time.hpp"
#include "./storage/file.h"

#include <memory>


namespace sbase{

enum PageType{
  kEmptyPage = 0,
  kBFlowTablePage = 1,
  kBPlusIndexPage = 2,
  kBIndexPage = 3
};

double kLRU_decr_factor = 10;
struct Page: public NonCopy{
  PageHandle handle;
  FileMeta* file;
  Latch latch;
  PageType type;
  Timestamp modified;
  Timestamp commited;
  Page(const FileMeta& f, PageHandle h = 0):file(f), handle(h){ }
  ~Page(){ }
  inline bool Referenced(void){
    return latch.occupied();
  }
};

} // namespace sbase

#endif // SBASE_STORAGE_PAGE_HPP_