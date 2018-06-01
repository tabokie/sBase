#ifndef SBASE_STORAGE_FILE_H_
#define SBASE_STORAGE_FILE_H_

#include "./util/status.hpp"
#include "./storage/file_format.hpp"
#include <string>
#include <iostream>
#include <memory>


namespace sbase{

// Define Handle Type & Header // 
#if defined(__WIN32) || defined(__WIN64)

#include <windows.h>
typedef HANDLE OsFileHandle;
typedef HANDLE OsMapHandle;
inline void CaptureError(void){std::cout << "OS raise error code: " << GetLastError() << std::endl;}

#endif // __WINXX

#ifdef __linux
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
typedef int OsFileHandle;
typedef bool OsMapHandle; // custom
inline void CaptureError(void){std::cout << "OS raise unknown error." << std::endl;}

#endif // __linux


class WritableFile{
 protected:
  OsFileHandle fhandle_;
  size_t file_end_;
  // extern data
  const std::string fileName;
 public:
  using WritableFilePtr = std::shared_ptr<WritableFile>;
  WritableFile(const char* name):fileName(name),file_end_(0){ }
  WritableFile(const std::string name):fileName(name),file_end_(0){ }
  WritableFile():file_end_(0){ }
  bool Empty(void){return file_end_ == 0;}
  virtual ~WritableFile(){ };
  virtual Status Open(void);
  virtual Status Close(void);
  virtual Status Delete(void);
  virtual Status Read(size_t offset, size_t size, char* alloc_ptr) = 0;
  virtual Status Write(size_t offset, size_t size, char* data_ptr) = 0;
  virtual Status SetEnd(size_t offset) = 0;
  inline const std::string name(void) const{return fileName;}
  inline size_t size(void) const{return file_end_;}
};

class SequentialFile : public WritableFile{
 public:
  SequentialFile(const char* name):WritableFile(name){ }
  SequentialFile(const std::string name):WritableFile(name){ }
  SequentialFile():WritableFile(){ }
  ~SequentialFile(){ }
  Status Read(size_t offset, size_t size, char* alloc_ptr);
  Status Write(size_t offset, size_t size, char* data_ptr);
  Status SetEnd(size_t offset);
};

}


#endif // SBASE_STORAGE_FILE_H_