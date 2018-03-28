
#include "page_manager.h"
#include "status.h"




class Tabler : public NoCopy{
 private:
 	PageManager* page_;
 	BPlusCursor* table_idx_;
 	BFlowCursor* table_;
 	vector<BCursor*> idx_;
 public:
  Tabler();
  ~Tabler();
  Status Get(Slice key);
  Status Put(Slice key);
  Status Free(void);
}

class BPlusCursor{
 private:
 	PageHandle* page_;
 	vector<Slice>;
 	vector<PageHandle>;
 public:
	FileMeta
	Status Fetch();
	Status Split();
	Status Insert();
	Status Descend();
	Status Climb();
	Status Shift();
	PageHandle& current(void);
 private:

}

template <typename _ET>
class BCursor{
	Status 
}
class BFlowCursor{
	
}