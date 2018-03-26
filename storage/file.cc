#include "status.h"
#include "file.h"

#include <string>

namespace sbase{

#if defined(__WIN32) || defined(__WIN64)

Status WritableFile::Open(void){
  fhandle_ = CreateFile(file_.filename, 
    GENERIC_READ | GENERIC_WRITE, 
    0,  // share mode
    NULL,  // security
    OPEN_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL, 
    NULL); // template file handle
  file_end_ = GetFileSize(fhandle_, NULL);
  return Status::OK();
}

Status WritableFile::Close(void){
  if(!CloseHandle(fhandle_))return Status::IOError("Close File Failed");
  return Status::OK();
}

Status SequentialFile::Read(size_t offset, char* alloc_ptr) {
  DWORD dwPtr = SetFilePointer(fhandle_, 
    offset, 
    NULL, 
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  bool rfRes = ReadFile(fhandle_, 
    alloc_ptr, 
    file_.block_size, 
    NULL, // num of bytes read
    NULL); // overlapped structure
  if(!rfRes)return Status::IOError("Read File Failed");
  return Status::OK();
}
Status SequentialFile::Flush(size_t offset, char* data_ptr) {
  DWORD dwPtr = SetFilePointer(fhandle_, 
    offset, 
    NULL, 
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  bool rfRes = WriteFile(fhandle_, 
    data_ptr, 
    file_.block_size, 
    NULL,  // num of bytes read
    NULL); // overlapped structure
  if(!rfRes)return Status::IOError("Write File Failed");
  return Status::OK();
}
Status SequentialFile::Append(size_t offset) {
  DWORD dwPtr = SetFilePointer(fhandle_, \
    offset+file_.block_size, \
    NULL, \
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  SetEndOfFile(fhandle_);
  file_end_ = offset + file_.block_size;
  // SetFileValidData(fhandle_, offset+block_size_)
  return Status::OK();
}


Status RandomAccessFile::Append(size_t offset, size_t size){
  DWORD dwPtr = SetFilePointer(fhandle_, \
    offset+file_.block_size, \
    NULL, \
    0); // 0 for starting from beginning
  DWORD dwError;
  if(dwPtr == INVALID_SET_FILE_POINTER \
    && (dwError = GetLastError())!=NO_ERROR)return Status::IOError("Set File Pointer Failed");
  SetEndOfFile(fhandle_);
  file_end_ = offset + size;
  return Status::OK();
}
Status RandomAccessFile::Read(size_t offset, size_t size, char* alloc_ptr) {
  mhandle_ = CreateFileMapping(fhandle_, 
    NULL,  // security
    PAGE_READWRITE, 
    0,  // higher dword
    offset,  // lower dword
    NULL); // map name
  if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
  char* map_ptr_ = (char*)MapViewOfFile(mhandle_, 
    FILE_MAP_ALL_ACCESS, 
    0,  // higher dword
    offset,  // lower dword
    size);
  if(map_ptr_ == NULL)return Status::IOError("Map View Invalid");
  memcpy(alloc_ptr, map_ptr_, size);
  if(!UnmapViewOfFile(map_ptr_))return Status::IOError("Map View Close Failed");
  if(!CloseHandle(mhandle_))return Status::IOError("Map Handle Close Failed");
  return Status::OK();
}
Status RandomAccessFile::Flush(size_t offset, size_t size, char* data_ptr) {
  mhandle_ = CreateFileMapping(fhandle_, 
    NULL,  // security
    PAGE_READWRITE, 
    0,  // higher dword
    offset+size,  // lower dword
    NULL); // map name
  if(mhandle_ == INVALID_HANDLE_VALUE)return Status::IOError("Map Invalid");
  char* map_ptr_ = (char*)MapViewOfFile(mhandle_, 
    FILE_MAP_ALL_ACCESS,
    0,  // higher dword
    offset, // lower dword
    size);
  if(map_ptr_ == NULL)return Status::IOError("Map View Invalid");
  memcpy(map_ptr_, data_ptr, size);
  if(!UnmapViewOfFile(map_ptr_))return Status::IOError("Map View Close Failed");
  if(!CloseHandle(mhandle_))return Status::IOError("Map Handle Close Failed");
  return Status::OK();
}


#endif


#if defined(__linux)

class SequentialFile : public WritableFile{

};

class RandomAccessFile : public WritableFile{

};

#endif

}