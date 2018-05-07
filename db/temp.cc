#include <iostream>

using namespace std;

#include <string> 
#include <iostream> 

// template<typename T> 
// std::string f(T) 
// { 
//     return "Template"; 
// } 

// std::string f(int&) 
// { 
//     return "Nontemplate"; 
// } 

// template<typename T> 
// std::string f(T*) 
// { 
//     return "Nontemplate"; 
// } 

// int main() 
// { 
//     int x = 7; 
//     std::cout << f(x) << std::endl; 
// }


// class base{
// public:
// 	virtual void func(void) = 0;
// };

// template <typename T>
// class son: public base{
// 	T val;
// 	void func(void){
// 		val = 2;
// 		cout << "son" << endl;
// 		cout << val;
// 	}
// };

// int main(void){
// 	base* ptr = new son<int>();
// 	ptr->func();
// 	cout << ptr->val;
// }

template <typename T>
struct derived{
	T val;
	derived(T n):val(n){ }
	~derived(){ }
};

#define getType(n)		

int main(void){

	using type0 = derived<int>;
	using type1 = derived<string>;



	return 0;
}
