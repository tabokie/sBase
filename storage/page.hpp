#ifndef SBASE_STORAGE_PAGE_HPP_
#define SBASE_STORAGE_PAGE_HPP_

#include "./util/utility.hpp"
#include "./storage/latch.hpp"
#include "./util/timer.hpp"
#include "./storage/file.h"

#include <memory>


namespace sbase{

enum PageType{
  kEmptyPage = 0,
  kBFlowTablePage = 1,
  kBPlusIndexPage = 2,
  kBIndexPage = 3
};

class Page: public NonCopy{
  PageHandle handle;
  shared_ptr<File> file;
  Latch latch;
  PageType type;
  Timestamp modified;
  Timestamp commited;
 public:
  Page(const File& f, PageHandle h):file(f), handle(h){ }
  ~Page(){ }
  bool Referenced(void){
    return latch.occupied();
  }
};

} // namespace sbase

#endif // SBASE_STORAGE_PAGE_HPP_