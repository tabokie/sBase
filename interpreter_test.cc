#include "scanner.hpp"
#include "parser.hpp"

using namespace sbase;
using namespace std;

int main(void){

	Scanner scanner;
	string str = "select * from A where C > 100 and B < (select * from B) or C like '%df';";
	vector<Token> v;
	scanner.Scan(str,v);

	Parser parser;
	for(auto token: v){
		cout << kSymbolDict(token.id) << endl;
		parser.NextPredict(token);
		cout << parser;
	}

	return 0;
}