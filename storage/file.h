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
  const char* fileName;
  size_t pageSize;
  Access access;

  FileMeta(const char* name, size_t page, Access acs):
    fileName(name),blockSize(block),access(acs){ }
  // copy allowed
  FileMeta(const FileMeta& that){
    fileName = that.fileName;
    blockSize = that.blockSize;
    access = that.access;
  }
  FileMeta():fileName(nullptr){ }
  ~FileMeta(){ }
};


#if defined(__WIN32) || defined(__WIN64)


#include <windows.h>

typedef HANDLE OsFileHandle;
typedef HANDLE OsMapHandle;

inline void CaptureError(void){std::cout << "Last Error: " << GetLastError() << std::endl;}

#endif // __WINXX


class WritableFile{
 protected:
  FileMeta file_;
  OsFileHandle fhandle_;
  size_t file_end_;
 public:
  WritableFile(FileMeta file):file_(file),file_end_(0){ }
  WritableFile():file_end_(0){ }
  bool Empty(void){return file_end_ == 0;}
  virtual ~WritableFile(){ };
  virtual Status Open(void) = 0;
  virtual Status Close(void) = 0;
  virtual Status Delete(void) = 0;
  virtual Status Read(size_t offset, char* alloc_ptr) = 0;
  virtual Status Read(size_t offset, char* alloc_ptr, size_t size) = 0;
  virtual Status Flush(size_t offset, char* data_ptr) = 0;
  virtual Status Flush(size_t offset, char* data_ptr, size_t size) = 0;
  virtual Status Append(size_t offset) = 0;
  inline Access access(void){return file_.access;}
  inline const char* name(void){return file_.fileName;}
  inline size_t size(void){return file_end_;}
};

class SequentialFile : public WritableFile{
 public:
  SequentialFile(FileMeta file):WritableFile(file){ }
  SequentialFile():WritableFile(){ }
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



}


#endif // SBASE_STORAGE_FILE_H_