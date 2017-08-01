#ifndef JSON_HELPER_HH_
#define JSON_HELPER_HH_

#include "schema_validator.hh"
typedef Json_Schema_Validator::JSON JSON;

namespace algorithm {

template<typename Tp_>
struct LESS {
	static bool compare(const Tp_ &a, const Tp_ &b) {
		return a <= b;
	}
	static bool compare_exclusive(const Tp_ &a, const Tp_ &b) {
		return a < b;
	}
};

template<typename Tp_>
struct MORE {
	static bool compare(const Tp_ &a, const Tp_ &b) {
		return a >= b;
	}
	static bool compare_exclusive(const Tp_ &a, const Tp_ &b) {
		return a > b;
	}
};

}

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

template<typename Array_, typename Element_>
decltype(auto) find_element(const Array_ & array, const Element_ & element) {
	return std::find(array.begin(), array.end(), element);
}

template<typename Array_, typename Element_>
bool find_bool(const Array_ & array, const Element_ & element) {
	return find_element(array, element) != array.end();
}

template<template<typename > class alg_, typename Tp_>
bool test_value(Tp_ test_value, const JSON &schema, const std::string & get,
		const std::string & exclusive) {
	if (get::is<Tp_>(schema[get])) {
		auto max = get::as<Tp_>(schema[get]);
		if (get::is_as_bool(schema[exclusive])) {
			if (alg_<Tp_>::compare(test_value, max)) {
				return true;
			}
		}
		if (alg_<Tp_>::compare_exclusive(test_value, max)) {
			return true;
		}
	}
	return false;
}

#endif /* JSON_HELPER_HH_ */
