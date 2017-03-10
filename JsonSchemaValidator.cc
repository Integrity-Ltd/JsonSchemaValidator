#include "JsonSchemaValidator.hh"

#include <list>
#include <algorithm>
#include <regex>

#define TERMINATE if (! _goThroughErrors) return
#define IF_TERMINATE  if (! _result && ! _goThroughErrors) return

Reference::Reference(const Reference & r)
		: _ptr(r._ptr), _position(r._position), _value(*_ptr) {
}

Reference::Reference(const Json::Value & in)
		: _ptr(&in), _position("#"), _value(*_ptr) {
}

Reference::Reference(const Json::Value & in, const std::string &pos)
		: _ptr(&in), _position(pos), _value(*_ptr) {
}

Reference::Reference(const Reference & r, const std::string &pos)
		: _ptr(&(r._value[pos])), _position(r._position + "/" + pos), _value(*_ptr) {
}

Reference::Reference(const Reference & r, const int &i)
		: _ptr(&(r._value[i])), _position(r._position + "/" + std::to_string(i)), _value(*_ptr) {
}

void Reference::remap(const Reference & r) {
	new (this) Reference(r);
}

void Reference::forward(const std::string &pos) {
	if (pos != "") {
		std::string element = "";
		for (unsigned i = 0; i < pos.length(); i++) {
			if (pos[i] == '/') {
				if (element.length() > 0) {
					if (_value.isArray()) {
						auto int_postion = std::stoi(element);
						remap(Reference(*this, int_postion));
					} else {
						remap(Reference(*this, element));
					}
				}
				element = "";
			} else {
				element += pos[i];
			}
		}
		if (element.length() > 0) {
			if (_value.isArray()) {
				auto int_postion = std::stoi(element);
				remap(Reference(*this, int_postion));
			} else {
				remap(Reference(*this, element));
			}
		}
	}
}

Reference JsonSchemaValidator::getReferece(const Json::Value & in, const std::string & ref) {
	try {
		if (ref[0] != '#') {
			throw std::invalid_argument(ref + " is not a correct reference!");
		}
		unsigned i = 1;
		Reference rtn(in);
		if (ref[1] == '/') {
			rtn.remap(Reference(_root, "#"));
			i++;
		}
		rtn.forward(ref.substr(i));
		return Reference(rtn);
	} catch (...) {
		throw std::invalid_argument(ref + " is not a correct reference!");
	}
}

std::stringstream & JsonSchemaValidator::error() {
	_result = false;
	return _error;
}

void JsonSchemaValidator::validateArray(const Reference & in, const Json::Value & schema) {
	if (!in._value.isArray()) {
		error() << in._position << " is not an array!" << "\n";
		TERMINATE;
	}
	int size = in._value.size();
	if (schema["minItems"].isInt()) {
		int min = schema["minItems"].asInt();
		if (schema["exclusiveMinimum"].isBool()
				&& schema["exclusiveMinimum"].asBool()) {
			if (size <= min) {
				error() << in._position << " is below minimum!" << "\n";
				TERMINATE;
			}
		} else if (size < min) {
			error() << in._position << " is below minimum!" << "\n";
			TERMINATE;
		}
	}
	if (schema["maxItems"].isInt()) {
		int max = schema["minItems"].asInt();
		if (schema["exclusiveMaximum"].isBool()
				&& schema["exclusiveMaximum"].asBool()) {
			if (size >= max) {
				error() << in._position << " is exceeds maximum!" << "\n";
				TERMINATE;
			}
		}if (size > max) {
			error() << in._position << " is exceeds maximum!" << "\n";
			TERMINATE;
		}
	}
	if (schema["uniqueItems"].isBool() && schema["uniqueItems"].asBool()) {
		for (int i = 0; i < size; i++) {
			for (int j = i + 1; j < size;j ++) {
				if(in._value[i] == in._value[j]) {
					error() << in._position << " has not unique items!"<< "\n";
					TERMINATE;
				}
			}
		}
	}
	for (int i = 0; i < size; i++) {
		validate(Reference(in, i), schema["items"]);
		IF_TERMINATE;
	}
}

void JsonSchemaValidator::validateObject(const Reference & in, const Json::Value & schema) {
	if (!in._value.isObject()) {
		error() << in._position << " is not an object!" << "\n";
		TERMINATE;
	}
	std::list<std::string> inNames, schemaNames, requiedNames, additionalNames;
	for (const std::string & member : in._value.getMemberNames()) {
		inNames.push_back(member);
	}
	if (schema["properties"].isObject()) {
		for (const std::string & member : schema["properties"].getMemberNames()) {
			schemaNames.push_back(member);
		}
	}
	bool req = false;
	if (schema["required"].isArray()) {
		for (auto & member : schema["required"]) {
			requiedNames.push_back(member.asString());
		}
		req = true;
		if (inNames.size() < requiedNames.size()) {
			error() << in._position << "/" << " does not have enough element!"<< "\n";
			TERMINATE;
		}
	}
	unsigned add = 0;
	if (schema["additionalItems"].isBool() && schema["additionalItems"].asBool()) {
		add = 1;
	} else if (schema["additionalItems"].isArray()) {
		for (auto & member : schema["additionalItems"]) {
			additionalNames.push_back(member.asString());
		}
		add = 2;
		if (inNames.size() > additionalNames.size() + requiedNames.size()) {
			error() << in._position << "/" << " has too many element!"<< "\n";
			TERMINATE;
		}
	}
	for(const std::string & member : inNames) {
		if (auto it = std::find(schemaNames.begin(), schemaNames.end(), member); it != schemaNames.end()) {
			validate(Reference(in, *it), schema["properties"][*it]);
			requiedNames.remove(*it);
		} else if (add == 2) {
			if (auto it = std::find(additionalNames.begin(),additionalNames.end(), member); it == additionalNames.end()) {
				error() << in._position << "/" << member <<" is not expected!"<< "\n";
				TERMINATE;
			}
		} else if (add == 0) {
			error() << in._position << "/" << member <<" is not expected!"<< "\n";
			TERMINATE;
		}
	}
	if (req && requiedNames.size() > 0) {
		error() << in._position << "/" <<" not enough element, expected: "<< "\n";
		for (const auto & member : requiedNames) {
			_error << member << ", ";
		}
		TERMINATE;
	}
}

void JsonSchemaValidator::validateString(const Reference & in, const Json::Value & schema) {
	if (!in._value.isString()) {
		error() << in._position << " is not a string!" << "\n";
		TERMINATE;
	} else {
		std::string str = in._value.asString();
		if (schema["minLength"].isInt()) {
			if (str.length() < (unsigned) schema["minLength"].asInt()) {
				error() << in._position << " is too short!"<< "\n";
				TERMINATE;
			}
		}
		if (schema["minLength"].isInt()) {
			if (str.length() > (unsigned) schema["minLength"].asInt()) {
				error() << in._position << " is too long!"<< "\n";
				TERMINATE;
			}
		}
		if (schema["pattern"].isString()) {
			if (!validateRegex(str, schema["pattern"].asString())) {
				error() << in._position << " does not match regex pattern!"<< "\n";
				TERMINATE;
			}
		}
		if (schema["format"].isString()) {
//		auto it = _format.find(schema["format"].asString());
//		if (!validateRegex(str, it->second)) {
//			error() << in._position << "does not match regex pattern!"<< "\n";
//			TERMINATE;
//		}
		}
	}
}

void JsonSchemaValidator::validateNumber(const Reference & in, const Json::Value & schema) {
	if (!(in._value.isInt() || in._value.isDouble())) {
		error() << in._position << " is not a number!";
		TERMINATE;
	} else {
		double value = in._value.asDouble();
		if (!schema["maximum"].isNull()) {
			if (double max = schema["maximum"].asDouble(); schema["exclusiveMaximum"].isBool()
					&& schema["exclusiveMaximum"].asBool()) {
				if (value >= max) {
					error() << in._position << " is exceeds maximum!"<< "\n";
					TERMINATE;
				}
			} else if (value > max) {
				error() << in._position << " is exceeds maximum!"<< "\n";
				TERMINATE;
			}
		}
		if (!schema["minimum"].isNull()) {
			if (double min = schema["minimum"].asDouble(); schema["exclusiveMinimum"].isBool()
					&& schema["exclusiveMinimum"].asBool()) {
				if (value <= min) {
					error() << in._position << " is below minimum!"<< "\n";
					TERMINATE;
				}
			} else if (value < min) {
				error() << in._position << " is below minimum!"<< "\n";
				TERMINATE;
			}
		}
	}
}

void JsonSchemaValidator::validateInteger(const Reference & in, const Json::Value & schema) {
	if (!in._value.isInt()) {
		error() << in._position << " is not an integer!" << "\n";
		TERMINATE;
	} else {
		if (auto value = in._value.asInt64(); !schema["multipleOf"].isNull()) {
			auto multi = schema["multipleOf"].asInt64();
			if (value % multi !=0) {
				error() << in._position << " is not a multiple!" << "\n";
				TERMINATE;
			}
		}
		validateNumber(in, schema);
	}
}

void JsonSchemaValidator::validateBoolean(const Reference & in) {
	if (!in._value.isBool()) {
		error() << in._position << " is not a boolean!" << "\n";
		TERMINATE;
	}
}

void JsonSchemaValidator::validateEnum(const Reference & in, const Json::Value & schema) {
	if (schema["enum"].isArray()) {
		if (std::find(schema["enum"].begin(), schema["enum"].end(), in._value) == schema["enum"].end()) {
			error() << in._position << " is not a correct enum!" << "\n";
			TERMINATE;
		}
	}
}

bool JsonSchemaValidator::validateRegex(const std::string &str, const std::string &regex) {
	std::regex re(regex, std::regex::ECMAScript);
	std::smatch sm;
	if (!std::regex_search(str, sm, re)) {
		return false;
	}
	return true;
}

bool JsonSchemaValidator::validateAllOf(const Reference &in, const Json::Value &schema) {
	for (const Json::Value & j : schema) {
		if (JsonSchemaValidator jv(_root, in, j); ! jv.getResult()) {
			_error << "allOf fail: "<< jv.getError() << "\n";
			return false;
		}
	}
	return true;
}

bool JsonSchemaValidator::validateAnyOf(const Reference &in, const Json::Value &schema) {
	for (const Json::Value & j : schema) {
		if (JsonSchemaValidator jv(_root, in, j); jv.getResult()) {
			_error << "anyOf success" << "\n";
			return true;
		} else {
			_error << "anyOf fail: "<< jv.getError() << "\n";
		}
	}
	return false;
}

bool JsonSchemaValidator::validateOneOf(const Reference &in, const Json::Value &schema) {
	int ctr = 0;
	for (const Json::Value & j : schema) {
		if (JsonSchemaValidator jv(_root, in, j); jv.getResult()) {
			ctr++;
			if (ctr > 1) break;
		} else {
			_error << "oneOf fail: "<< jv.getError() << std::endl;
		}
	}
	return ctr == 1;
}

bool JsonSchemaValidator::validateNot(const Reference &in, const Json::Value &schema) {
	if (JsonSchemaValidator jv(_root, in, schema); ! jv.getResult()) {
		_error << in._position << "is ok to a \"NOT\" but should not";
		return false;
	}
	return true;
}

void JsonSchemaValidator::validate(const Reference & in, const Json::Value & schema, bool allowRef) {
	if (allowRef && schema["$ref"].isString()) {
		Reference r(getReferece(in._value, schema["$ref"].asString()));
		validate(Reference(getReferece(in._value, schema["$ref"].asString())), schema, false);
		IF_TERMINATE;
		return;
	}
	std::string saveError = _error.str();
	if (schema["allOF"].isArray() && !validateAllOf(in, schema["allOf"])) {
		error() << in._position << " ALL_OF!" << "\n";
		TERMINATE;
	}
	if (schema["anyOf"].isArray() && !validateAnyOf(in, schema["anyOf"])) {
		error() << in._position << " ANY_OF!" << "\n";
		TERMINATE;
	}
	if (schema["oneOf"].isArray() && !validateOneOf(in, schema["oneOf"])) {
		error() << in._position << " ONE_OF!" << "\n";
		TERMINATE;
	}
	if (schema["not"].isArray() && !validateNot(in, schema["not"])) {
		error() << in._position << " NOT!" << "\n";
		TERMINATE;
	}
	if (_result) {
		_error.str("");
		_error << saveError;
	}
	if (schema["type"].isString()) {
		std::string type = schema["type"].asString();
		if (type == "object") {
			validateObject(in, schema);
		} else if (type == "array") {
			validateArray(in, schema);
		} else if (type == "string") {
			validateString(in, schema);
		} else if (type == "number") {
			validateNumber(in, schema);
		} else if (type == "integer") {
			validateInteger(in, schema);
		} else if (type == "boolean") {
			validateBoolean(in);
		} else if (type == "null" && !in._value.isNull()) {
			error() << in._position << "is not a null!";
		}
	}
	validateEnum(in, schema);
}

JsonSchemaValidator::JsonSchemaValidator(const Json::Value &root, const Reference & in, const Json::Value &schema, bool error)
	: _result(true), _goThroughErrors(error), _root(root), _rootSchema(schema) {
	validate(in, _rootSchema);
}

JsonSchemaValidator::JsonSchemaValidator(const Json::Value &root, const Json::Value &schema, bool error)
	: _result(true), _goThroughErrors(error), _root(root), _rootSchema(schema) {
	validate(Reference(_root), _rootSchema);
}

bool JsonSchemaValidator::getResult() const {
	return _result;
}

std::string JsonSchemaValidator::getError() const {
	return _error.str();
}
