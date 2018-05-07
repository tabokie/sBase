#include <vector>
#include <string>
#include <sstream>

#include "util\blob.hpp"

// Type and Value //

class Value;

// holder of real value //
// convertion: string <-> realvalue(<-explicit value) <-> blob
class BaseValue{
 protected:
 	static std::stringstream ConvertHelper;
 public:
 	// template constructor
 	virtual ~BaseValue(){ }
 	virtual BaseValue* clone() const = 0;
 	virtual std::string asString() const = 0;
 	virtual void fromString(std::string) = 0;
 	virtual void set(BaseValue const*) = 0;
 	virtual void fromBlob(Blob) = 0;
 	virtual Blob toBlob() const = 0;
 	virtual bool operator<(const BaseValue& rhs) const = 0;
 	virtual bool operator>(const BaseValue& rhs) const = 0;
 	virtual bool operator==(const BaseValue& rhs) const = 0;
};

std::stringstream BaseValue::ConvertHelper("");



template<typename T>
struct TypeLen{ 
  size_t operator()(const T& key) const{
    return sizeof(key);
  }
};
// special for string
template<> struct TypeLen<std::string>{
  size_t operator()(const std::string& str) const{
    return str.size();
  }
};

// real value holder
template <typename PlainType>
class RealValue: public BaseValue{
	PlainType val;
 public: 
 	// explicit define
 	RealValue(PlainType v):val(v){ }
 	RealValue* clone(void) const{
 		return new RealValue(*this);
 	}
 	// string convert
 	std::string asString(void) const{
 		ConvertHelper.str("");
 		ConvertHelper.clear();
 		ConvertHelper << val;
 		return ConvertHelper.str();
 	}
 	void fromString(std::string str){
 		ConvertHelper.str("");
 		ConvertHelper.clear();
 		ConvertHelper.str(str);
 		ConvertHelper >> val;
 		return ;
 	}
 	// blob convert
 	void fromBlob(Blob bob){
 		if(TypeLen<PlainType>(val) != bob.len){
 			return ;
 		}
 		val = *(reinterpret_cast<PlainType*>(bob.data));
 		return ;
 	}
 	Blob toBlob(void) const{
 		return Blob((static_cast<char*>(&val)),TypeLen<PlainType>(val));
 	}
 	// set
 	void set(BaseValue const* v){
 		if(v){
 			val = (static_cast<RealValue<PlainType>*const>(v).val);
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

};


// uniform interface
class Value
{
	BaseValue* val;
 public:
	Value(BaseValue const& v):val(v.clone()){ }
	Value(Value const& rhs):val(rhs.val ? rhs.val->clone() : nullptr){ }
	explicit Value(BaseValue* pv = nullptr):val(pv){ }
	~Value(){delete val;}
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
	}

};


class Type{
 public:
	enum TypeT{
		stringT = 0, 
		intT = 1, 
		bigintT = 2, 
		tinyintT = 3, 
		doubleT = 4, 
		unknownT = 5
	};
	explicit Type(TypeT type_id):typeId(type_id){ }
	BaseValue* newValue() const{
		if(typeId < 0 || typeId >= unknownT)return nullptr;
		return prototypes[typeId]->clone();
	}
	TypeT getType() const{
		return typeId;
	}
 private: 
	static BaseValue* prototypes[unknownT];
	TypeT typeId;
};

BaseValue* Type::prototypes[unknownT] = {
	new RealValue<std::string>(""),
	new RealValue<int>(0),
	new RealValue<uint64_t>(0),
	new RealValue<uint8_t>(0),
	new RealValue<double>(0) };


// class and Object //
class Attibute{
	std::string attrName_;
	Type::TypeT type_;
 public:
 	Attibute(const std::string& name, Type::TypeT typeId):
 	attrName_(name),
 	type_(typeId){ }
 	~Attibute(){ }
 	const std::string& getName() const{
 		return attrName_;
 	}
 	Type getType(void) const{
 		return type_;
 	}
}

class ClassDef{
	std::vector<Attibute> attributes_;
	ClassDef* base_;
	std::string name_;
	bool definitionFix_;
	ClassDef(ClassDef const* base, )
};

class Object{

};