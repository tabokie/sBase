

namespace sbase {

MemPool::~MemPool(void){
	for(size_t i = 0; i < blocks_.size(); i++){
		delete [] blocks_[i];
	}
}
char* MemPool::Allocate(void){
	while(free_.empty()){
		AllocateNewBlock();
	}
	char* alloc_ptr = free_.front();
	free_.pop();
	return alloc_ptr;
}

bool MemPool::Free(char* alloc_ptr){
	for(auto& p : free_){
		if( p == alloc_ptr ) return false;
	}
	bool in_blocks = false;
	for(auto& p : blocks_){
		if( p == alloc_ptr )in_blocks = true;
	}
	if(in_blocks){
		free_.push(alloc_ptr);
		return true;
	}
	else return false;
}

char* MemPool::AllocateNewBlock(void){
	char* new_ptr = new char[kBlockSize];
	blocks_.push_back(new_ptr);
	free_.push(new_ptr);
	return new_ptr;
}




} // namespace sbase

