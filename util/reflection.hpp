#ifndef SBASE_UTIL_REFLECTION_HPP_
#define SBASE_UTIL_REFLECTION_HPP_

#include <vector>
#include <string>
#include <sstream>

#include ".\util\blob.hpp"

#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <iostream>
using std::istream;
using std::ostream;

// NOTICE //
// unsafe constructor using naked char*
// causing explicit difference between char* and std::string
// only use string to construct by literal

// Custom Type and util //

struct FixChar{
	uint32_t length;
	mutable char* fixchar; // len = length+1
	FixChar(int len = 0):length(len),fixchar(nullptr){
		if(length < 0)length = 0;
		// lazy initialization
		// if(length > 0)fixchar = new char[length];
	}
	FixChar(int len, std::string in):length(len),fixchar(nullptr){
		if(length < 0)length = 0;
		fixchar = new char[length + 1]();
		memcpy(fixchar, in.c_str(), ((in.size()<length)?in.size():length) );
	}
	FixChar(const FixChar& rhs):length(rhs.length),fixchar(nullptr){
		if(rhs.fixchar){
			fixchar = new char[length + 1]();
			memcpy(fixchar, rhs.fixchar, length); // cut down overflow
		}
	}
	~FixChar(){
		if(fixchar)delete [] fixchar;
	}
	friend istream& operator>>(istream& is, FixChar& rhs){
		if(rhs.length <= 0) return is;
		if(!rhs.fixchar && rhs.length > 0)rhs.fixchar = new char[rhs.length + 1]();
		std::string tmp;
		is >> tmp;
		memcpy(rhs.fixchar, tmp.c_str(), ((tmp.size()<rhs.length)?tmp.size():rhs.length));
		rhs.fixchar[rhs.length] = '\0'; // in case of overflow
		return is;
	}
	friend ostream& operator<<(ostream& os, const FixChar& rhs){
		if(rhs.length <= 0 || !rhs.fixchar)return os;
		os << rhs.fixchar;
		return os;
	}
	bool operator<(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return false;
		if(!fixchar)return true;
		if(!rhs.fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) < 0;
	}
	bool operator>(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return false;
		if(!fixchar)return false;
		if(!rhs.fixchar)return true;
		return strcmp(fixchar, rhs.fixchar) > 0;
	}
	bool operator==(const FixChar& rhs) const{
		if(!fixchar && !rhs.fixchar)return true;
		if(!fixchar || !rhs.fixchar)return false;
		return strcmp(fixchar, rhs.fixchar) == 0;
	}
	bool operator==(const std::string rhs) const{
		return strcmp(fixchar, rhs.c_str()) == 0;
	}
	friend bool operator==(const std::string lhs, const FixChar& rhs){
		return rhs == lhs;
	}
};

template<typename T>
struct TypeLen{ 
  size_t operator()(const T& key) const{
    return sizeof(key);
  }
};
// special for string
template<> struct TypeLen<FixChar>{
  size_t operator()(const FixChar& fixchar) const{
  	return fixchar.length;
  }
};

template <typename T>
struct TypeBlob{
	const char* operator()(const T& key){
		return reinterpret_cast<const char*>(&key);
	}
	char* operator()(T& key){
		return reinterpret_cast<char*>(&key);
	}
};
template<> struct TypeBlob<FixChar>{
	char* operator()(const FixChar& key){
		if(!key.fixchar)key.fixchar = new char[key.length+1]();
		return key.fixchar;
	}
};


// Type and Value //

class Value;

// holder of real value //
// convertion: 
// string <->	Value (public interface) 		 <-> blob
// 							|
// string <-> RealValue (<->explicit value)<-> blob
// 					 		|
// 					 BaseValue(internal usage)
class BaseValue{
 protected:
 	static std::stringstream ConvertHelper_;
 public:
 	// template constructor
 	virtual ~BaseValue(){ }
 	virtual BaseValue* clone() const = 0;
 	virtual std::string ToString() const = 0;
 	virtual void FromString(const std::string) = 0;
 	virtual void set(BaseValue const*) = 0;
 	// unsafe approach
 	virtual void FromNakedPtr(const char*) = 0;
 	virtual void ToNakedPtr(char*) const = 0;
 	virtual void FromBlob(const Blob) = 0;
 	virtual Blob ToBlob() const = 0;
 	virtual bool operator<(const BaseValue& rhs) const = 0;
 	virtual bool operator>(const BaseValue& rhs) const = 0;
 	virtual bool operator==(const BaseValue& rhs) const = 0;
 	virtual size_t length(void) = 0;
};

std::stringstream BaseValue::ConvertHelper_("");

// real value holder

// PlainType pre-requisite
// PlainType(const PlainType&)
// operator<< >>
// operotr <=>
template <typename PlainType>
class RealValue: public BaseValue{
	friend Value;
	PlainType val;
 public: 
 	// explicit define
 	RealValue(PlainType v):val(v){ }
 	RealValue(const RealValue<PlainType>& rhs):val(rhs.val){ }
 	BaseValue* clone(void) const{
 		return new RealValue<PlainType>(*this);
 	}
 	// string convert
 	std::string ToString(void) const{
 		ConvertHelper_.str("");
 		ConvertHelper_.clear();
 		ConvertHelper_ << val;
 		return ConvertHelper_.str();
 	}
 	void FromString(const std::string str){
 		ConvertHelper_.str("");
 		ConvertHelper_.clear();
 		ConvertHelper_.str(str);
 		ConvertHelper_ >> val;
 		return ;
 	}
 	// blob convert
 	void FromBlob(const Blob bob){
 		if(TypeLen<PlainType>{}(val) != bob.len){
 			return ;
 		}
 		memcpy(TypeBlob<PlainType>{}(val), bob.data, TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	Blob ToBlob(void) const{
 		return Blob( TypeBlob<PlainType>{}(val),TypeLen<PlainType>{}(val));
 	}
 	// unsafe approach
 	void FromNakedPtr(const char* src){
 		memcpy(TypeBlob<PlainType>{}(val), src, TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	void ToNakedPtr(char* des) const{
 		memcpy(des, TypeBlob<PlainType>{}(val), TypeLen<PlainType>{}(val) );
 		return ;
 	}
 	// set
 	void set(BaseValue const* v){
 		if(v){
 			val = (static_cast<const RealValue<PlainType>*>(v)->val);
 		}
 		else{
 			val = PlainType();
 		}
 		return ;
 	}
 	bool operator<(const BaseValue& rhs) const{
 		return val < (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator>(const BaseValue& rhs) const{
 		return val > (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	bool operator==(const BaseValue& rhs) const{
 		return val == (static_cast<const RealValue<PlainType>&>(rhs).val);
 	}
 	size_t length(void){return TypeLen<PlainType>{}(val);}

};

enum TypeT{
	tinyintT = 0, 
	intT = 1, 
	uintT = 2,
	bigintT = 3, 
	doubleT = 4, 
	// varchar = 0,
	fixchar8T = 5,
	fixchar16T = 6,
	fixchar32T = 7,
	fixchar64T = 8,
	fixchar128T = 9,
	fixchar256T = 10,
	unknownT = 11
};

class Type{
	friend class Value;
 public:
	explicit Type(TypeT type_id):typeId(type_id){ }
	BaseValue* newValue() const{
		if(typeId < 0 || typeId >= unknownT)return nullptr;
		return prototypes[typeId]->clone();
	}
	TypeT type() const{
		return typeId;
	}
	static size_t getLength(TypeT type_id){
		if(type_id < unknownT)return prototype_length[type_id];
		return 0;
	}
 protected: 
	static BaseValue* prototypes[unknownT];
	static size_t prototype_length[unknownT];
	TypeT typeId;
};

BaseValue* Type::prototypes[unknownT] = {
	new RealValue<int8_t>(0),
	new RealValue<int32_t>(0),
	new RealValue<uint32_t>(0),
	new RealValue<int64_t>(0),
	new RealValue<double>(0),
	new RealValue<FixChar>(FixChar(8)),
	new RealValue<FixChar>(FixChar(16)),
	new RealValue<FixChar>(FixChar(32)),
	new RealValue<FixChar>(FixChar(64)),
	new RealValue<FixChar>(FixChar(128)),
	new RealValue<FixChar>(FixChar(256))
};

// compromise
size_t Type::prototype_length[unknownT] = {
	1, // new RealValue<int8_t>(0),
	4, // new RealValue<int32_t>(0),
	4,
	8, // new RealValue<int64_t>(0),
	4, // new RealValue<double>(0),
	8,
	16, // new RealValue<FixChar>(FixChar(16)),
	32, // new RealValue<FixChar>(FixChar(32)),
	64, // new RealValue<FixChar>(FixChar(64)),
	128, // new RealValue<FixChar>(FixChar(128))
	256
};

// uniform interface
// exist because BaseValue is pure abstract class(no data, no ctor)
// need delegate
class Value{
	BaseValue* val;
	TypeT type_;
 public:
	// Value(BaseValue const& v):val(v.clone()){ }
	Value(Value const& rhs):val(rhs.val ? rhs.val->clone() : nullptr),type_(rhs.type_){ }
	explicit Value(TypeT typeId, BaseValue* pv):val(pv),type_(typeId){ }
	~Value(){if(val)delete val;}
	Value& operator=(const Value& rhs){
		if(val){
			if(rhs.val){
				val->set(rhs.val);
			}
			else{
				delete val;
				val = nullptr;
			}
		}
		else{
			val = (rhs.val ? rhs.val->clone() : nullptr);
		}
		return *this;
	}
	template <typename PlainType>
	PlainType get()const{
		if(val){
			RealValue<PlainType> const & converted = dynamic_cast<RealValue<PlainType>const&>(*val);
			return converted.val;
		}
		return PlainType();
	}
	// inherit interface
	Value(TypeT typeId = unknownT):val(nullptr),type_(typeId){
		if(typeId < unknownT){
			val = Type::prototypes[type_]->clone();
		}
	}
	Value(TypeT typeId, const std::string str):val(nullptr),type_(typeId){
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromString(str);
		}
	}	
	// unsafe interface
	Value(TypeT typeId, const char* ptr):val(nullptr),type_(typeId){
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromNakedPtr(ptr);
		}
	}	
	Value(TypeT typeId, const Blob bob):val(nullptr),type_(typeId){
		if(typeId < unknownT){
			val = Type::prototypes[typeId]->clone();
			val->FromBlob(bob);
		}
	}
	void Read(const std::string str){
		if(!val)return ;
		val->FromString(str);
		return ;
	}
	void Read(const char* ptr){ // unsafe
		if(!val)return;
		val->FromNakedPtr(ptr);
		return ;
	}
	void Read(const Blob bob){
		if(!val)return;
		val->FromBlob(bob);
		return;
	}
	void Write(char* ptr){
		if(val)val->ToNakedPtr(ptr);
		return ;
	}
	operator std::string()const{
		return val->ToString();
		// if(!val)return std::string("");
		// return val->ToString();
	}
	operator Blob()const{
		if(!val)return Blob();
		return val->ToBlob();
	}
	bool operator<(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return false;
		else if(!val)return false;
		else if(!rhs.val)return true;
		return (*val) < (*(rhs.val));
	}
	bool operator>(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return false;
		else if(!val)return false;
		else if(!rhs.val)return true;
		return (*val) > (*(rhs.val));
	}
	bool operator==(const Value& rhs) const{
		// always push null to last
		if(!val && !rhs.val)return true;
		else if(!val)return false;
		else if(!rhs.val)return false;
		return (*val) == (*(rhs.val));
	}
	size_t length(void) const{
		if(!val)return 0;
		return val->length();
	}
	TypeT type(void) const{
		return type_;
	}
};

// Class and Object //
class Attribute{
	std::string attr_name_;
	TypeT type_;
 public:
 	Attribute(const std::string& name, TypeT typeId):
 	attr_name_(name),
 	type_(typeId){ }
 	Attribute():attr_name_("null"),type_(unknownT){ }
 	~Attribute(){ }
 	const std::string& name() const{
 		return attr_name_;
 	}
 	TypeT type(void) const{
 		return type_;
 	}
 	bool operator==(const Attribute& rhs) const{
 		return attr_name_ == rhs.attr_name_ && type_ == rhs.type_;
 	}
 	bool operator==(const std::string& name) const{
 		return attr_name_ == name;
 	}
};

// forward declaration
class Object;

class ClassDef{
 protected:
	using AttributeContainer = std::vector<Attribute>;
	using AttributeIterator = std::vector<Attribute>::const_iterator;
	// attributes
	std::vector<Attribute> attr_;
	std::vector<Attribute> effective_attr_; // include full attr
	// base inheritence
	ClassDef const* base_;
	// flag
	std::string name_;
	mutable bool definition_fix_;
 public:
 	template<typename attr_iterator>
	ClassDef(ClassDef const* base, std::string name, attr_iterator attrBegin, attr_iterator attrEnd):		
		base_(base),
		name_(name),
		definition_fix_(false){ 
		std::copy(attrBegin, attrEnd, std::back_inserter<AttributeContainer>(attr_));
		BaseInit();
		effective_attr_.insert(effective_attr_.end(), attr_.begin(), attr_.end());
	}
	ClassDef(ClassDef const* base, std::string name):
		base_(base),
		name_(name),
		definition_fix_(false){ 
		BaseInit();
	}
	bool AddAttribute(const Attribute& newAttr){
		if(!definition_fix_){
			attr_.push_back(newAttr);
			effective_attr_.push_back(newAttr);
			return true;
		}
		return false;
	}
	bool SubAttribute(size_t idx){
		if(!definition_fix_ && idx < attr_.size()){
			attr_.erase(attr_.begin()+idx);
			return true;
		}
		return false;
	}
	bool SubAttribute(std::string name){
		if(definition_fix_)return false;
		auto found = std::find(attr_.begin(), attr_.end(), name);
		if(found != attr_.end()){
			attr_.erase(found);
			return true;
		}
		return false;
	}
	Attribute GetAttribute(size_t idx)const{
		assert(idx < effective_attr_.size());
		return effective_attr_[idx];
	}
	size_t GetAttributeIndex(std::string name)const{
		for(int idx = 0; idx < effective_attr_.size(); idx++){
			auto attr = effective_attr_[idx];
			if(attr.name() == name){
				return idx;
			}
		}
		return 0;
	}
	size_t length(void) const{ // byte
		size_t ret = 0;
		for(auto& attr : effective_attr_){
			ret += Type::getLength(attr.type());
		}
		return ret;
	}
	size_t attributeCount(void)const{return effective_attr_.size();}
	AttributeIterator attributeBegin(void)const{return effective_attr_.begin();}
	AttributeIterator attributeEnd(void)const{return effective_attr_.end();}
	std::string name(void)const{return name_;}
	void setUnfix(void) const{definition_fix_ = true;}
	Object* NewObject(void)const;

 protected:
 	void BaseInit(void){
 		if(base_){
 			base_->setUnfix();
 			std::copy(base_->attributeBegin(), 
 				base_->attributeEnd(), 
 				std::back_inserter<AttributeContainer>(effective_attr_));
 		}
 	}
};

class Object{
	ClassDef const* const class_;
	std::vector<Value> values_;
 public:
 	Object(ClassDef const* cls):class_(cls){
 		InitAttributeValue();
 	}
 	template <class ...Args>
 	Object(ClassDef const* cls, Args... args):class_(cls){
 		InitAttributeValue(args...);
 	}
 	Object(const Object& rhs):class_(rhs.class_),values_(rhs.values_){	}
 	~Object(){ }
 	Object* clone(void){return new Object(class_);};
 	ClassDef const* instanceOf(void) const{return class_;}
 	Value GetValue(size_t idx){
 		if(idx >= values_.size())return Value();
 		return values_[idx];
 	}
 	Value GetValue(const std::string& name){
 		size_t index = class_->GetAttributeIndex(name);
 		if(index >= 0)return values_[index];
 		else return Value();
 	}
 	bool SetValue(size_t idx, const Value& val){
 		if(idx >= values_.size())return false;
 		values_[idx] = val;
 		return true;
 	}
 	bool SetValue(std::string name, const Value& val){
 		size_t index = class_->GetAttributeIndex(name);
 		if(index >= 0){
 			values_[index] = val;
 			return true;
 		}
 		return false;
 	}
 	// unsafe
 	void Read(char* ptr){
 		for(auto& val: values_){
 			val.Read(ptr);
 			ptr += val.length();
 		}
 		return ;
 	}
 	size_t length(void) const{if(!class_)return 0; return class_->length();}
 private:
 	void InitAttributeValue(void){
 		if(class_)
 		for(auto attr = class_->attributeBegin(); attr < class_->attributeEnd(); attr++){
 			values_.push_back(Value((*attr).type()));
 		}
 	}
 	template <class ...Args>
 	void InitAttributeValue(Args... args){
 		assert(sizeof...(args) <= class_->attributeCount());
 		auto attr = class_->attributeBegin();
 		int arr[] = { ( values_.push_back(Value((*attr).type(), args)),attr++,0)... };
 		while(attr < class_->attributeEnd()){
 			values_.push_back(Value((*attr).type()));
 			attr++;
 		}
 	}

};


// Afterward definition for ClassDef
Object* ClassDef::NewObject(void)const{
	return new Object(this);
}



#endif // SBASE_UTIL_REFLECTION_HPP_