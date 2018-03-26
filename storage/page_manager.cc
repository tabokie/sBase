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
  auto f = vector[file];
  return f.Close();
}

Status PageManager::New(FileHandle file, PageHandle& page){
  PageHandle handle = page_.size();
  assert(file >= 0 && file < file_.size());
  WritableFile f = file_[file];
  assert(f.access() == kSequential);
  page = handle;
  return f.Append(f.size(), size);
}
Status PageManager::New(FileHandle file, size_t size, PageHandle& page){
  FileHandle handle = page.size();
  assert(file >= 0 && file < file_.size());
  WritableFile f = file_[file];
  assert(f.access() == kRandom);
  page = handle;
  return f.Append(f,size(), size);
}

Status PageManager::Read(PageHandle page, char*& ret_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  if(pool_.pooling(page)){
    size_t size = page_[page].size;
    return pool_.Read(page, size, ret_ptr);
  }
  else{
    char* alloc_ptr;
    size_t offset = page_[page].offset;
    size_t size = page_[page].size;
    FileHandle fhandle = page_[page].file_;
    auto ret = pool_.New(page, size, alloc_ptr);
    if(!ret.OK())return ret;
    WritableFile& file = file_[fhandle];
    file.Read(offset, alloc_ptr);
    ret_ptr = alloc_ptr;
    return Status::OK();
  }
}
Status PageManager::Read(PageHandle page, size_t size, char*& ret_ptr){

}

Status PageManager::Write(PageHandle page, char* data_ptr){

}
Status PageManager::Write(PageHandle page, size_t size, char* data_ptr){

}



} // namespace sbase