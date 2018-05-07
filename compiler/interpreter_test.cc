#include "./compiler/scanner.hpp"
#include "./compiler/parser.hpp"

using namespace sbase;
using namespace std;

int main(void){

	Scanner scanner;
	Parser parser;
	std::string sentence;
	std::string buffer;
	vector<Token> v;
	while(true){
		cout << ">> ";
		getline(cin, buffer);
		if(buffer.find("\\q") < std::string::npos){
			cout << "Bye";
			break;
		}
		if(buffer.find(";")){
			sentence += buffer;
			scanner.Scan(sentence,v);
			for(auto token: v){
				cout << kSymbolDict(token.id) << endl;
				parser.NextPredict(token);
			}
			cout << parser;
			parser.Clear();
			v.clear();
			sentence = "";
			continue;
		}
		sentence += buffer;
	}


	return 0;
}