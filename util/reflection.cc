#include ".\util\reflection.hpp"


std::stringstream BaseValue::ConvertHelper_("");

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

// Afterward definition for ClassDef
Object* ClassDef::NewObject(void)const{
	return new Object(this);
}