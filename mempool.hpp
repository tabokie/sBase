

namespace sbase {

const size_t kBlockSize = kPageSize;

class MemPool : private NoCopy {

 public:
 	MemPool() = default;
 	~MemPool();
 	char* Allocate(void);
 	bool Free(char*);

 private:
 	vector<char*> blocks_;
 	vector<char*> free_;
 	char* AllocateNewBlock(void);

};


} // namespace sbase

