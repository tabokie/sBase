#include "memmap.h"

namespace sbase{

// Memory Map encapsulation for OSs //

// for windows
#if defined(__WIN64) || defined(__WIN32)

MemMap::MemMap():
	file_opened(false),
	file_mapped(false),
	ptr_(nullptr){ }

MemMap::MemMap(const char* file_name):
	file_opened(false),
	file_mapped(false),
	ptr_(nullptr)
	{Open(file_name);}

MemMap::~MemMap(){
	Close();
}

bool MemMap::Open(const char* file_name){
	if(file_opened)return false;
	file_handle_ = CreateFile(file_name ,GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	file_opened = true;
	return true;
}
bool MemMap::Map(const char* map_name){
	if(file_mapped)return false;
	if(!file_opened)return false;
	map_handle_ = CreateFileMapping(file_handle_, NULL, PAGE_READONLY, 0, 0, map_name);
	if(map_handle_ == INVALID_HANDLE_VALUE){
		Error();
	}
	ptr_ = (char*)MapViewOfFile(map_handle_, FILE_MAP_READ, 0, 0, 0);
	if(ptr_ == NULL){
		Error();
	}
	file_mapped = true;
	return true;
}
bool MemMap::Share(const char* map_name){
	if(file_mapped)return false;
	map_handle_ = OpenFileMapping(PAGE_READONLY, FALSE, map_name);
	if(map_handle_ == INVALID_HANDLE_VALUE){
		Error();
	}
	ptr_ = (char*)MapViewOfFile(map_handle_, FILE_MAP_READ, 0, 0, 0);
	if(ptr_ == NULL){
		Error();
	}
	return true;
}
bool MemMap::Close(void){
	if(file_mapped)
		Unmap();
	if(file_opened)
		Unfile();
	return true;
}
char* MemMap::get_ptr(){
	if(ptr_ == NULL)return nullptr;
	return ptr_;
}

// handles and flags
HANDLE file_handle_;
bool file_opened;
HANDLE map_handle_;
bool file_mapped;
char* ptr_;

void MemMap::Unmap(){
	if(!UnmapViewOfFile(ptr_) || !CloseHandle(map_handle_)){
		Error();
	}
	file_mapped = false;
}
void MemMap::Unfile(){
	if(!CloseHandle(file_handle_)){
		Error();
	}
	file_opened = false;
}
void MemMap::Error(void){
	std::cout << GetLastError() << std::endl << std::flush;
	exit(0);
}


}; /* class MemMap */

#endif /* __WINXX */


// for linux
#if defined(__linux)

MemMap::MemMap():file_opened(false),file_mapped(false),ptr_(nullptr){}
MemMap::MemMap(const char* file_name):file_opened(false),file_mapped(false),ptr_(nullptr){Open(file_name);}
MemMap::~MemMap(){
	Close();
}

bool MemMap::Open(const char* file_name){
	if(file_opened)return false;
	struct stat sb;
	f_id = open(file_name, O_RDONLY | O_CREAT, S_IRUSR);
	if(f_id==-1 || fstat(f_id, &sb)==-1)return false;
	file_size = sb.st_size;
	file_opened = true;
	return true;
}
bool MemMap::Map(const char* map_name){
	if(file_mapped)return false;
	if(!file_opened)return false;
	ptr_ = (char*)mmap(NULL, file_size,PROT_READ, MAP_SHARED, f_id, 0);
	if(ptr_ == NULL){
		Error();
	}
	file_mapped = true;
	return true;
}
bool MemMap::Close(void){
	if(file_mapped)
		Unmap();
	if(file_opened)
		Unfile();
	return true;
}
char* MemMap::get_ptr(){
	if(ptr_ == NULL)return nullptr;
	return ptr_;
}

int f_id;
bool file_opened;
bool file_mapped;
size_t file_size;
char* ptr_;
void MemMap::Unmap(){
	if((munmap(ptr_, file_size*sizeof(char))) == -1){
		Error();
	}
	file_mapped = false;
}
void MemMap::Unfile(){
	if(close(f_id) == -1){
		Error();
	}
	file_opened = false;
}
void MemMap::Error(void){
	exit(0);
}

#endif /* __linux */


} /* namespace sbase */


