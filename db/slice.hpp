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
using SliceIterator = std::vector<Object>::iterator;

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
		if(stuffed){
			for(int i = 0; i < effective_attr_.size(); i++){
				if(primary[i]){
					ret.push_back(effective_attr_[i]);
				}
			}
		}
		if(!stuffed)return false;
		return true;
	}
	size_t GetKeyIndexWithName(std::string name){
		return GetAttributeIndex(name);
	}
	Attribute GetKeyWithIndex(size_t idx){
		return GetAttribute(idx);
	}
	bool isPrimary(size_t idx){
		if(!stuffed)return false;
	}
	bool allowNull(size_t idx){
		if(!stuffed)return false;
		return null[idx];
	}
	Value defaultValue(size_t idx){
		auto str = defaultV[idx];
		return Value(effective_attr_[idx].getType(), str);
	}
	// effective accessors //
	size_t length(void){
		// restructure type //
	}
};

#endif // SBASE_STORAGE_SLICE_HPP_