#include <iostream>
#include <functional>

using namespace std;

template <typename T>
int func(int a){return a;}


int main(void){
INTEGER.Make<int>();
	double a = 1.8;
	// char* conv = static_cast<char*>(&a);
	// string 
	char* p = reinterpret_cast<char*>(&a);
	char c = 'c';
	cout << c <<  p[0] << p[1] << p[2] << p[3] << endl;
	return 0;
}