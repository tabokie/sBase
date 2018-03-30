#ifndef SBASE_STORAGE_FILE_H_
#define SBASE_STORAGE_FILE_H_

#include "status.h"
#include <string>
#include <iostream>


namespace sbase{

const size_t kBlockSize = 4096; // 4 kb

enum Access {
  kSequential = 0,
  kRandom = 1
};

struct FileMeta{
  const char* filename;
  size_t block_size;
  Access access;

  FileMeta(const char* name, size_t block, Access acs):
    filename(name),block_size(block),access(acs){ 
      if(access == kSequential){
        block_size = kBlockSize;
      }
  }
  // copy allowed
  FileMeta(const FileMeta& that){
    filename = that.filename;
    block_size = that.block_size;
    access = that.access;
  }
  ~FileMeta(){ }
};

#if defined(__WIN32) || defined(__WIN64)


#include <windows.h>

typedef HANDLE OsFileHandle;
typedef HANDLE OsMapHandle;

class WritableFile{
 protected:
  FileMeta file_;
  OsFileHandle fhandle_;
  size_t file_end_;
 public:
  WritableFile(FileMeta file):file_(file),file_end_(0){ }
  virtual ~WritableFile(){ };
  virtual Status Open(void);
  virtual Status Close(void);
  virtual Status Delete(void);
  virtual Status Read(size_t offset, char* alloc_ptr) = 0;
  virtual Status Read(size_t offset, char* alloc_ptr, size_t size) = 0;
  virtual Status Flush(size_t offset, char* data_ptr) = 0;
  virtual Status Flush(size_t offset, char* data_ptr, size_t size) = 0;
  virtual Status Append(size_t offset) = 0 ;
  inline Access access(void){return file_.access;}
  inline const char* name(void){return file_.filename;}
  inline size_t size(void){return file_end_;}
};

class SequentialFile : public WritableFile{
 public:
  SequentialFile(FileMeta file):WritableFile(file){ }
  ~SequentialFile(){ }
  Status Read(size_t offset, char* alloc_ptr);
  Status Read(size_t offset, char* alloc_ptr, size_t size);
  Status Flush(size_t offset, char* data_ptr);
  Status Flush(size_t offset, char* data_ptr, size_t size);
  Status Append(size_t offset);
 private:
  Status Read_(size_t offset, char* alloc, size_t size);
  Status Flush_(size_t offset, char* alloc, size_t size);
};

class RandomAccessFile : public WritableFile{
 private:
  OsMapHandle mhandle_;
 public:
  RandomAccessFile(FileMeta file):WritableFile(file){ }
  ~RandomAccessFile(){ }
  Status Read(size_t offset, char* alloc_ptr);
  Status Read(size_t offset, char* alloc_ptr, size_t size);
  Status Flush(size_t offset, char* data_ptr, size_t size);
  Status Flush(size_t offset, char* data_ptr);
  Status Append(size_t offset);
 private:
  Status Read_(size_t offset, char* alloc, size_t size);
  Status Flush_(size_t offset, char* alloc, size_t size);
};

#endif // __WINXX


}


#endif // SBASE_STORAGE_FILE_H_