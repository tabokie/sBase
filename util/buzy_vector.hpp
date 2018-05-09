#include <vector>

template <class T>
class BuzyVector: std::vector<T>{
	std::vector<bool> flags_;
	BuzyVector(void){}
	~BuzyVector(){ }
	void push_back (const value_type& val){
		
	}
};