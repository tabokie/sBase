#include "./storage/page_manager.h"

namespace sbase{

// File Interface //
Status PageManager::NewFile(FileMeta file, FileHandle& handle){
  FileHandle fhandle = file_.size()+1;

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
  if(file < 1 || file > file_.size())return Status::InvalidArgument("File Handle Flow");
  for(FileHandle i = 1; i <= static_cast<FileHandle>(page_.size()); i++){
    if(page_[i-1].file == file){
      FlushPage(i); // !assert(OK())
    }
  }
  auto f = file_[file-1];
  return f->Close();
}
Status PageManager::DeleteFile(FileHandle file){
  if(file < 1 || file > file_.size())return Status::InvalidArgument("File Handle Flow");
  auto f = file_[file-1];
  return f->Delete();
}

Status PageManager::NewPage(FileHandle fh, PageHandle& ret){
  FilePtr f = nullptr;
  auto bool_ret = file_.Get(fh, f);
  if(bool_ret && !f){
    return Status::Corruption("File corrupted.");
  }
  else if(!bool_ret){
    return Status::InvalidArgument("File not exist.");
  }
  else{
    PagePtr p = f->GetFreePage();
    if(!p){
      // append
    }
    else{
      ret = p->handle;
    }
    return Status::OK();
  }
}

Status PageManager::Pool(PageHandle ph){
  if(pool_.inPool(ph)){

  }
  else{
    PagePtr
  }
}

Status PageManager::Flush(PageHandle ph){

}

Status PageManager::Expire(PageHandle ph){

}

Latch* PageManager::GetLatch(PageHandle ph){

}

size_t PageManager::GetSize(PageHandle ph){

}

char* PageManager::GetPageDataPtr(PageHandle ph){

}


} // namespace sbase