#include "page_manager.h"

namespace sbase{

Status PageManager::NewFile(FileMeta file, FileHandle& handle){
  FileHandle handle = file_.size();

  if(file.access == kSequential ){
    SequentialFile f(file);
    f.Open();
    file_.push_back(f);
  }
  else if(file.access == kRandom){
    RandomAccessFile f(file);
    f.Open();
    file_.push_back(f);
  }
  else{
    return Status::InvalidArgument("Invalid Access Mode");
  }
  return Status::OK();
}
Status PageManager::CloseFile(FileHandle file){
  if(file < 0 || file >= file_.size())return Status::InvalidArgument("File Handle Flow");
  for(FileHandle i = 0; i < page_.size(); i++){
    if(page_[i].file == file){
      FlushPage(i); // !assert(OK())
    }
  }
  auto f = vector[file]
  return f.Close();
}

Status PageManager::New(FileHandle file, PageHandle& page){
  PageHandle handle = page_.size();
  assert(file >= 0 && file < file_.size());
  WritableFile f = file_[file];
  assert(f.access() == kSequential);
  page_.push_back(PageMeta(file, f.size()));
  page = handle;
  return f.Append(f.size());
}
Status PageManager::New(FileHandle file, size_t size, PageHandle& page){
  PageHandle handle = page_.size();
  assert(file >= 0 && file < file_.size());
  WritableFile f = file_[file];
  assert(f.access() == kRandom);
  page_.push_back(PageMeta(file, f.size(), size));
  page = handle;
  return f.Append(f,size(), size);
}

Status PageManager::Read(PageHandle page, char*& ret_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  if(pool_.pooling(page)){
    size_t size = page_[page].size;
    return pool_.Read(page, size, ret_ptr);
  }
  else{
    char* alloc_ptr;
    size_t offset = page_[page].offset;
    size_t size = page_[page].size;
    FileHandle fhandle = page_[page].file_;
    auto ret = pool_.New(page, kBlockSize, alloc_ptr);
    if(!ret.OK())return ret;
    WritableFile& file = file_[fhandle];
    file.Read(offset, alloc_ptr);
    ret_ptr = alloc_ptr;
    return Status::OK();
  }
}
Status PageManager::Read(PageHandle page, size_t page_offset, size_t read_size, char*& ret_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  if(page_offset + read_size > page_[page].size)return Status::InvalidArgument("Read Out of Page");
  if(pool_.pooling(page)){
    pool_.Read(page, ret_ptr);
    ret_ptr += page_offset;
    return Status::OK();
  }
  else{
    char* alloc_ptr;
    size_t file_offset = page_[page].offset;
    FileHandle fhandle = page_[page].file_;
    auto ret = pool_.New(page, kBlockSize, alloc_ptr);
    if(!ret.OK())return ret;
    WritableFile& file = file_[fhandle];
    file.Read(file_offset, alloc_ptr);
    ret_ptr = alloc_ptr + page_offset;
    return Status::OK();
  }
}

Status PageManager::Write(PageHandle page, char* data_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  auto page_meta = page_[page];
  if(pool_.pooling(page)){
    char* pool_ptr = pool_.get_ptr(page);
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else if(!pool_.full()){
    char* pool_ptr;
    if(!Read(page, pool_ptr))return Status::IOError("Reading File from Disk Failed");
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else{
    DirectWritePage(page, data_ptr);
  }
  if(pool_.full()){
    FlushPage(page);
  }
}
Status PageManager::Write(PageHandle page, size_t size, char* data_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  auto page_meta = page_[page];
  if(pool_.pooling(page)){
    char* pool_ptr = pool_.get_ptr(page);
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else if(!pool_.full()){
    char* pool_ptr;
    if(!Read(page, pool_ptr))return Status::IOError("Reading File from Disk Failed");
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else{
    DirectWritePage(page, size, data_ptr);
  }
  if(pool_.full()){
    FlushPage(page);
  }
}

Status PageManager::FlushPage(PageHandle page){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  char* pool_ptr = pool_.get_ptr(page);
  if(pool_ptr){
    auto status = DirectWritePage(page, pool_ptr);
    if(!status.OK())return status;
    return pool_.Free(page);  
  }
  return Status::InvalidArgument("Cannot Find Page in Mem Pool");
}

Status PageManager::FlushPage(void){
  PageHandle last;
  if(!buzy_.GetLast(last).OK())return Status::Corruption("Buzy List Corruption");
  return FlushPage(last);
}

Status PageManager::DirectWritePage(PageHandle page, char* data_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  PageHandle page_handle = page_[page];
  FileHandle file = page_handle.file;
  return file.Flush(page_handle.offset, data_ptr);
}

Status PageManager::DirectWritePage(PageHandle page, size_t size, char* data_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  PageHandle page_handle = page_[page];
  FileHandle file = page_handle.file;
  return file.Flush(page_handle.offset, size, data_ptr);
}



} // namespace sbase