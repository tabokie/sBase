#ifndef SBASE_STORAGE_PAGE_MANAGER_H_
#define SBASE_STORAGE_PAGE_MANAGER_H_

#include "status.h"
#include "file.h"
#include "mempool.hpp"
#include "buzy_queue.hpp"

#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

namespace sbase{

using std::vector;

typedef uint8_t FileHandle;
const size_t kFileHandleWid = 2 ;
typedef uint16_t PageHandle;
const size_t kPageHandleWid = 4;
typedef char PageSizeType;
const size_t kPageSizeWid = 1;


struct PageMeta{
  FileHandle file;
  size_t offset;
  size_t size;
  PageMeta(FileHandle f, size_t o, size_t s = 0):file(f),offset(o),size(s){ }
  PageMeta(const PageMeta& that):file(that.file),offset(that.offset),size(that.size){ }
  ~PageMeta(){ }
};

// unique page handle & file handle

class PageManager{
  using FilePtr = shared_ptr<WritableFile>;

  vector<FilePtr> file_;
  vector<PageMeta> page_;
  MemPool<PageHandle> pool_;
  BuzyQueue<PageHandle> buzy_;

 public:
  PageManager() = default;
  ~PageManager(){ };
  // on file
  Status NewFile(FileMeta file, FileHandle& handle);
  Status CloseFile(FileHandle file);
  Status DeleteFile(FileHandle file);
  // on page
  Status New(FileHandle file, PageHandle& handle);
  Status New(FileHandle file, size_t size, PageHandle& handle);
  Status Read(PageHandle page, char*& ret_ptr);
  Status Read(PageHandle page, size_t page_offset, size_t size, char*& ret_ptr);
  Status Write(PageHandle page, char* data_ptr);
  Status Write(PageHandle page, char* data_ptr, size_t size);
  Status Flush(PageHandle page){return FlushPage(page);}
 private:
  // mem pool interface
  Status FlushPage(PageHandle page);
  Status FlushPage(void);
  // disk interface
  Status DirectWritePage(PageHandle page, char* data_ptr);
  Status DirectWritePage(PageHandle page, char* data_ptr, size_t size);
};

} // namespace sbase



#endif // SBASE_STORAGE_PAGE_MANAGER_H_