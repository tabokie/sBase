#include ".\compiler\compiler.h"

#include <iostream>

using namespace sbase;
using namespace std;

int main(void){

	Compiler compiler;

	compiler.RunInterface(cin, cout);

	return 0;
}