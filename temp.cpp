#include <iostream>

using namespace std;


int main(void){

	double a = 1.8;
	// char* conv = static_cast<char*>(&a);
	// string 
	char* p = reinterpret_cast<char*>(&a);
	char c = 'c';
	cout << c <<  p[0] << p[1] << p[2] << p[3] << endl;
	return 0;
}