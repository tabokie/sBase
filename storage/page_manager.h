#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "status.h"
#include "file.h"
#include "mempool.hpp"
#include "buzy_queue.hpp"

#include <vector>
#include <cassert>

namespace sbase{

using std::vector;

typedef uint8_t FileHandle;
typedef uint8_t PageHandle;

struct PageMeta{
  FileHandle file;
  size_t offset;
  size_t size;
  PageMeta(FileHandle f, size_t o, size_t s = -1):
    file(f),offset(o),size(s){ }
  PageMeta(PageMeta& that):file(that.file),offset(that.offset),size(that.size){ }
  ~PageMeta(){ }

};

// unique page handle & file handle

class PageManager{
  vector<WritableFile> file_;
  vector<PageMeta> page_;
  MemPool<PageHandle> pool_;
  BuzyQueue<size_t> buzy_;

 public:
  PageManager();
  ~PageManager();
  // on file
  Status NewFile(FileMeta file, FileHandle& handle);
  Status CloseFile(FileHandle file);
  // on page
  Status New(FileHandle file, PageHandle& handle);
  Status New(FileHandle file, size_t size, PageHandle& handle);
  Status Read(PageHandle page, char*& ret_ptr);
  Status Read(PageHandle page, size_t size, char*& ret_ptr);
  Status Write(PageHandle page, char* data_ptr);
  Status Write(PageHandle page, size_t size, char* data_ptr);
 private:
  // mem pool interface
  Status FlushPage(PageHandle page);
  // disk interface
  Status DirectWritePage(PageHandle page, char* data_ptr);

};

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_