#include <sstream>
#include <string>
#include <iostream>

using namespace std;

class base{
 public:
 	static stringstream kStr;
	base(){}
	~base(){ }
};
template<class T>
class derived: public base{
	T val;
 public: 
 	derived(T v):val(v){ }
 	~derived(){ }
 	T get(void){
 		cout << derived<T>::kStr; 
 		return val;}
};
stringstream base::kStr("Hii");

int main(void){
	// base* p = new derived<int>(7);
	// static_cast<derived<int>*>(p)->init();
	// static_cast<derived<int>*>(p)->get();
	string str = "hello world";
	cout << sizeof(str) << endl;
	return 0;
}