#ifndef JSON_HELPER_HH_
#define JSON_HELPER_HH_

#include "schema_validator.hh"
typedef Json_Schema_Validator::JSON JSON;

namespace get {
bool is_as_bool(const JSON & val) {
	return val.isBool() && val.asBool();
}

template<typename Tp_>
bool is(const JSON & val) {
	return false;
}

template<>
bool is<int>(const JSON & val) {
	return val.isInt();
}

template<>
bool is<double>(const JSON & val) {
	return val.isDouble();
}

template<typename Tp_>
Tp_ as(const JSON & val) {
	return Tp_();
}

template<>
int as<int>(const JSON & val) {
	return val.asInt();
}

template<>
double as<double>(const JSON & val) {
	return val.asDouble();
}
}

#endif /* JSON_HELPER_HH_ */
