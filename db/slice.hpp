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

// Your Expectations //
// get schema
// inject char* with schema into record
// compare record
// print out record

typedef Object Slice;

// Field //
class Schema: public ClassDef{
 private:
 	// effective auxiliary field
 	bool stuffed;
 	std::vector<bool> primary; // true for primary
 	std::vector<bool> null; // true for allow null
 	std::vector<std::string> defaultV; // store as string
 public:
	template<typename attr_iterator>
	Schema(ClassDef const* base, std::string name, attr_iterator attrBegin, attr_iterator attrEnd):
		ClassDef(base,name,attrBegin,attrEnd),stuffed(false){ } 
	Schema(ClassDef const* base, std::string name):
		ClassDef(base,name),stuffed(false){ }
	// Initialize //
	Slice* NewSlice(void){
		return new Object(this);
	}
	// Get Attribute //
	bool GetPrimary(AttributeIterator& ret){
		if(!stuffed)return false;
	}
	size_t GetKeyIndex(std::string name){
		return GetAttributeIndex(name);
	}
	Attribute GetKey(size_t idx){
		return GetAttribute(idx);
	}
	bool isPrimary(std::string name){
		if(!stuffed)return false;
	}
	bool allowNull(std::string name){
		if(!stuffed)return false;
	}
	Value& defaultValue(std::string name); // don't know if do for template
	// effective accessors //
	size_t length(void){
		return 0;
	}
}

#endif // SBASE_STORAGE_SLICE_HPP_