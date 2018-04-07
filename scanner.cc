#include "./storage/hash.hpp"
#include "dict.hpp"

#include <string>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;


namespace sbase{

class Scanner{

  Scanner() = default;
  ~Scanner(){ }

};

} // namespace sbase

using namespace sbase;

int main(void){

  AutoDict<std::string> dict("SELECT", "COLUMN_LIST","FROM","TABLE_LIST","WHERE_CLAUSE","END");



  return 0;
}