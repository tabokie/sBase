#include "page_manager.h"

namespace sbase{

Status PageManager::NewFile(FileMeta file, FileHandle& handle){
  FileHandle fhandle = file_.size();

  if(file.access == kSequential ){
    FilePtr f = make_shared<SequentialFile>(file);
    // SequentialFile f(file);
    if(!f->Open().ok())return Status::IOError("Cannot Open Sequential File");
    file_.push_back(f);
  }
  else if(file.access == kRandom){
    FilePtr f = make_shared<RandomAccessFile>(file);
    if(!f->Open().ok())return Status::IOError("Cannot Open Random File");
    file_.push_back(f);
  }
  else{
    return Status::InvalidArgument("Invalid Access Mode");
  }
  handle = fhandle;
  return Status::OK();
}
Status PageManager::CloseFile(FileHandle file){
  if(file < 0 || file >= file_.size())return Status::InvalidArgument("File Handle Flow");
  for(FileHandle i = 0; i < static_cast<FileHandle>(page_.size()); i++){
    if(page_[i].file == file){
      FlushPage(i); // !assert(OK())
    }
  }
  auto f = file_[file];
  return f->Close();
}
Status PageManager::DeleteFile(FileHandle file){
  if(file < 0 || file >= file_.size())return Status::InvalidArgument("File Handle Flow");
  auto f = file_[file];
  return f->Delete();
}

Status PageManager::New(FileHandle file, PageHandle& page){
  assert(file < file_.size());
  FilePtr f = file_[file];
  assert(f->access() == kSequential);
  page_.push_back( PageMeta(file, f->size()) );
  page = page_.size()-1;
  return f->Append(f->size());
}
Status PageManager::New(FileHandle file, size_t size, PageHandle& page){
  PageHandle handle = page_.size();
  assert(file >= 0 && file < file_.size());
  FilePtr f = file_[file];
  assert(f->access() == kRandom);
  page_.push_back(PageMeta(file, f->size(), size));
  page = handle;
  return f->Append(f->size());
}

Status PageManager::Read(PageHandle page, char*& ret_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  if(pool_.pooling(page)){
    ret_ptr = pool_.get_ptr(page);
    if(!ret_ptr)return Status::IOError("Mem Pool Nil Block");
    return Status::OK();
  }
  else{
    char* alloc_ptr;
    size_t offset = page_[page].offset;
    size_t size = page_[page].size;
    FileHandle fhandle = page_[page].file;
    auto ret = pool_.New(page, kBlockSize, alloc_ptr);
    if(!ret.ok())return ret;
    FilePtr file = file_[fhandle];
    file->Read(offset, alloc_ptr);
    ret_ptr = alloc_ptr;
    return Status::OK();
  }
}
Status PageManager::Read(PageHandle page, size_t page_offset, size_t read_size, char*& ret_ptr){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  if(page_offset + read_size > page_[page].size)return Status::InvalidArgument("Read Out of Page");
  if(pool_.pooling(page)){
    ret_ptr = pool_.get_ptr(page);
    if(!ret_ptr)return Status::IOError("Mem Pool Nil Block");
    ret_ptr += page_offset;
    return Status::OK();
  }
  else{
    char* alloc_ptr;
    size_t file_offset = page_[page].offset;
    FileHandle fhandle = page_[page].file;
    auto ret = pool_.New(page, kBlockSize, alloc_ptr);
    if(!ret.ok())return ret;
    FilePtr file = file_[fhandle];
    file->Read(file_offset, alloc_ptr);
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
    if(!Read(page, pool_ptr).ok())return Status::IOError("Reading File from Disk Failed");
    assert(pool_ptr && data_ptr);
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else{
    DirectWritePage(page, data_ptr);
  }
  if(pool_.full()){
    FlushPage(page);
  }
}
Status PageManager::Write(PageHandle page, char* data_ptr, size_t size){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  buzy_.Visit(page);
  auto page_meta = page_[page];
  if(pool_.pooling(page)){
    char* pool_ptr = pool_.get_ptr(page);
    memcpy(pool_ptr, data_ptr, page_meta.size);
  }
  else if(!pool_.full()){
    char* pool_ptr;
    if(!Read(page, pool_ptr).ok())return Status::IOError("Reading File from Disk Failed");
    memcpy(pool_ptr, data_ptr, size);
  }
  else{
    DirectWritePage(page, data_ptr, size);
  }
  if(pool_.full()){
    FlushPage(page);
  }
  return Status::OK();
}

Status PageManager::FlushPage(PageHandle page){
  if(page < 0 || page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  char* pool_ptr = pool_.get_ptr(page);
  if(pool_ptr){
    auto status = DirectWritePage(page, pool_ptr);
    if(!status.ok())return status;
    return pool_.Free(page);  
  }
  return Status::InvalidArgument("Cannot Find Page in Mem Pool");
}

Status PageManager::FlushPage(void){
  PageHandle last;
  if(!buzy_.Last(last).ok())return Status::Corruption("Buzy List Corruption");
  return FlushPage(last);
}

Status PageManager::DirectWritePage(PageHandle page, char* data_ptr, size_t size){
  if(page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  PageMeta pmeta = page_[page];
  FileHandle fhandle = pmeta.file;
  if(fhandle >= file_.size())return Status::InvalidArgument("File Handle Flow");
  FilePtr f = file_[fhandle];
  return f->Flush(pmeta.offset, data_ptr, size);
}

Status PageManager::DirectWritePage(PageHandle page, char* data_ptr){
  if(page >= page_.size())return Status::InvalidArgument("Page Handle Flow");
  PageMeta pmeta = page_[page];
  FileHandle fhandle = pmeta.file;
  if(fhandle >= file_.size())return Status::InvalidArgument("File Handle Flow");
  FilePtr f = file_[fhandle];
  if(f->access() == kRandom)return f->Flush(pmeta.offset, data_ptr, pmeta.size);
  else if(f->access() == kSequential)return f->Flush(pmeta.offset, data_ptr);
  else return Status::InvalidArgument("Invalid Access Mode");
}


} // namespace sbase