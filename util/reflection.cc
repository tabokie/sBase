#include ".\util\reflection.hpp"

// char* FixChar::debugPtr = nullptr;
std::stringstream BaseValue::ConvertHelper_("");

BaseValue* Type::prototypes[unknownT] = {
	new RealValue<int8_t>(-128),
	new RealValue<int32_t>(0x80000000LL),
	new RealValue<uint32_t>(0),
	new RealValue<int64_t>(0x8000000000000000LL),
	// new RealValue<int64_t>(-9223372036854775808LL),
	new RealValue<double>(-1.0/0.0), // -inf
	new RealValue<FixChar8>(FixChar8()),
	new RealValue<FixChar16>(FixChar16()),
	new RealValue<FixChar32>(FixChar32()),
	new RealValue<FixChar64>(FixChar64()),
	new RealValue<FixChar128>(FixChar128()),
	new RealValue<FixChar256>(FixChar256())
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
Object ClassDef::NewObject(void)const{
	return Object(this);
}