#ifndef SBASE_STORAGE_SLICE_HPP_
#define SBASE_STORAGE_SLICE_HPP_

/*
// interface:
// slice : support convertion from/to Blob, string, runtime type
// record : reference to field definition, contain value 

*/

#include "util\reflection.hpp"
#include "util\status.h"

#include <iostream>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <cassert>

using std::ostream;
using std::istream;
using std::vector;
using std::function;
using std::string;

namespace sbase{

// Field //
class Schema: public ClassDef{
	 template<typename attr_iterator>
	Schema(ClassDef const* base, std::string name, attr_iterator attrBegin, attr_iterator attrEnd):
		ClassDef(base,name,attrBegin,attrEnd){ } 
	Schema(ClassDef const* base, std::string name):ClassDef(base,name){ }
	bool GetPrimary(AttributeContainer& ret){
		bool flag = false;
		for(auto& attr : effective_attr_){
			if(attr.tag_.isPrimary){
				ret.push_back(attr);
				flag = true;
			}
		}
		return flag;
	}
}
typedef Field ClassDef;
typedef Slice Object;

}



#endif // SBASE_STORAGE_SLICE_HPP_