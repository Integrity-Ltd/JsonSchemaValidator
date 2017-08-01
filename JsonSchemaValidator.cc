#include <list>
#include <algorithm>
#include <regex>

#include "schema_validator.hh"
#include "helper.hpp"

#define TERMINATE if (! go_through_errors_) return
#define IF_TERMINATE  if (! result_ && ! go_through_errors_) return

typedef Json_Schema_Validator::JSON JSON;

Json_Schema_Validator::Ref_::Ref_(const Ref_ & r) :
		ptr_(r.ptr_), position_(r.position_) {
}

Json_Schema_Validator::Ref_::Ref_(const JSON & in) :
		ptr_(&in), position_("#") {
}

Json_Schema_Validator::Ref_::Ref_(const JSON & in, const std::string &pos) :
		ptr_(&in), position_(pos) {
}

Json_Schema_Validator::Ref_::Ref_(const Ref_ & r, const std::string &pos) :
		ptr_(&(r.get_json_()[pos])), position_(r.position_ + "/" + pos) {
}

Json_Schema_Validator::Ref_::Ref_(const Ref_ & r, const int &i) :
		ptr_(&(r.get_json_()[i])), position_(
				r.position_ + "/" + std::to_string(i)) {
}
void Json_Schema_Validator::Ref_::step_(const std::string &pos) {
	const JSON & ref = *ptr_;
	if (ref.isArray()) {
		ptr_ = &(ref[std::stoi(pos)]);
	} else {
		ptr_ = &(ref[pos]);
	}
	position_ += "/" + pos;

}

void Json_Schema_Validator::Ref_::step_(const int &i) {
	ptr_ = &(get_json_()[i]);
	position_ += "/" + std::to_string(i);

}

void Json_Schema_Validator::Ref_::forward_(const std::string &pos) {
	if (pos != "") {
		std::string element = "";
		for (unsigned i = 0; i < pos.length(); i++) {
			if (pos[i] == '/') {
				if (element.length() > 0) {
					step_(element);
				}
				element = "";
			} else {
				element += pos[i];
			}
		}
		if (element.length() > 0) {
			step_(element);
		}
	}
}

const std::string & Json_Schema_Validator::Ref_::get_position_() const {
	return position_;
}

const JSON & Json_Schema_Validator::Ref_::get_json_() const {
	return *ptr_;
}

Json_Schema_Validator::Ref_ Json_Schema_Validator::get_referece_(
		const JSON & in, const std::string & ref) {
	try {
		if (ref[0] != '#') {
			throw std::invalid_argument(ref + " is not a correct reference!");
		}
		unsigned i = 1;
		Ref_ rtn(in);
		if (ref[1] == '/') {
			rtn = Ref_(root_, "#");
			i++;
		}
		rtn.forward_(ref.substr(i));
		return Ref_(rtn);
	} catch (...) {
		throw std::invalid_argument(ref + " is not a correct reference!");
	}
}

std::stringstream & Json_Schema_Validator::error_() {
	result_ = false;
	return sserror_;
}

std::stringstream & Json_Schema_Validator::error_(const Ref_ &in) {
	result_ = false;
	sserror_ << in.get_position_();
	return sserror_;
}

void Json_Schema_Validator::error_(const std::string &str) {
	error_() << " " << str << "\n";
}

void Json_Schema_Validator::error_(const Ref_ &in, const std::string &str) {
	error_(in) << " " << str << "\n";
}

void Json_Schema_Validator::validate_array_(const Ref_ & in,
		const JSON & schema) {
	if (!in.get_json_().isArray()) {
		error_(in) << " is not an array!" << '\n';
		TERMINATE;
	}
	int size = in.get_json_().size();
	if(test_value<algorithm::LESS>(size, schema, "minItems", "exclusiveMinimum")) {
		error_(in) << " is below minimum!" << '\n';
		TERMINATE;
	}
	if(test_value<algorithm::MORE>(size, schema, "maxItems", "exclusiveMaximum")) {
		error_(in) << " is exceeds maximum!" << '\n';
		TERMINATE;
	}
	if (get::is_as_bool(schema["uniqueItems"])) {
		const auto end_ = in.get_json_().end();
		for (auto it_1 = in.get_json_().begin(); it_1 != end_; ++it_1) {
			for (auto it_2 = it_1; ++it_2 != end_; ) {
				if (*it_1 == *it_2) {
					error_(in) << " has not unique items!"<< '\n';
					TERMINATE;
				}
			}
		}
	}
	if (schema["items"].isObject()) {
		for (int i = 0; i < size; i++) {
			validate_(Ref_(in, i), schema["items"]);
			IF_TERMINATE;
		}
	}
}

void Json_Schema_Validator::validate_object_(const Ref_ & in,
		const JSON & schema) {
	if (!in.get_json_().isObject()) {
		error_(in) << " is not an object!" << '\n';
		TERMINATE;
	}
	std::list<std::string> inNames, schemaNames, requiedNames, additionalNames;
	for (auto member : in.get_json_().getMemberNames()) {
		inNames.push_back(member);
	}
	if (schema["properties"].isObject()) {
		for (auto member : schema["properties"].getMemberNames()) {
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
			error_(in) << "/" << " does not have enough element!"<< '\n';
			TERMINATE;
		}
	}
	unsigned add = 0;
	if (get::is_as_bool(schema["additionalItems"]) || !schema["properties"].isObject()) {
		add = 1;
	} else if (schema["additionalItems"].isArray()) {
		for (auto & member : schema["additionalItems"]) {
			additionalNames.push_back(member.asString());
		}
		add = 2;
		if (inNames.size() > additionalNames.size() + requiedNames.size()) {
			error_(in) << "/" << " has too many element!"<< '\n';
			TERMINATE;
		}
	}
	for(auto member : inNames) {
		auto it = find_element(schemaNames, member);
		if (it != schemaNames.end()) {
			validate_(Ref_(in, *it), schema["properties"][*it]);
			requiedNames.remove(*it);
		} else if (add == 2) {
			if (!find_bool(additionalNames, member)) {
				error_(in) << "/" << member <<" is not expected!"<< '\n';
				TERMINATE;
			}
		} else if (add == 0) {
			error_(in) << "/" << member <<" is not expected!"<< '\n';
			TERMINATE;
		}
	}
	if (req && requiedNames.size() > 0) {
		error_(in) << "/" <<" not enough element, expected: "<< '\n';
		for (const auto & member : requiedNames) {
			sserror_ << member << ", ";
		}
		TERMINATE;
	}
}

void Json_Schema_Validator::validate_string_(const Ref_ & in,
		const JSON & schema) {
	if (!in.get_json_().isString()) {
		error_(in) << " is not a string!" << '\n';
		TERMINATE;
	} else {
		std::string str = in.get_json_().asString();
		if (schema["minLength"].isInt()) {
			if (str.length() < (unsigned) schema["minLength"].asInt()) {
				error_(in) << " is too short!"<< '\n';
				TERMINATE;
			}
		}
		if (schema["minLength"].isInt()) {
			if (str.length() > (unsigned) schema["minLength"].asInt()) {
				error_(in) << " is too long!"<< '\n';
				TERMINATE;
			}
		}
		if (schema["pattern"].isString()) {
			if (!validate_regex_(str, schema["pattern"].asString())) {
				error_(in) << " does not match regex pattern!"<< '\n';
				TERMINATE;
			}
		}
		if (schema["format"].isString()) {
//		auto it = _format.find(schema["format"].asString());
//		if (!validateRegex(str, it->second)) {
//			error_(in) << "does not match regex pattern!"<< '\n';
//			TERMINATE;
//		}
		}
	}
}

void Json_Schema_Validator::validate_number_(const Ref_ & in,
		const JSON & schema) {
	if (!(in.get_json_().isInt64() || in.get_json_().isDouble())) {
		error_(in) << " is not a number!";
		TERMINATE;
	} else {
		double value = in.get_json_().asDouble();
		if(test_value<algorithm::MORE>(value, schema, "maximum", "exclusiveMaximum")) {
			error_(in) << " is exceeds maximum!" << '\n';
			TERMINATE;
		}
		if(test_value<algorithm::LESS>(value, schema, "minimum", "exclusiveMinimum")) {
			error_(in) << " is below minimum!" << '\n';
			TERMINATE;
		}
	}
}

void Json_Schema_Validator::validate_integer_(const Ref_ & in,
		const JSON & schema) {
	if (!in.get_json_().isInt()) {
		error_(in) << " is not an integer!" << '\n';
		TERMINATE;
	} else {
		auto value = in.get_json_().asInt64();
		if ( !schema["multipleOf"].isNull()) {
			auto multi = schema["multipleOf"].asInt64();
			if (value % multi !=0) {
				error_(in) << " is not a multiple!" << '\n';
				TERMINATE;
			}
		}
		validate_number_(in, schema);
	}
}

void Json_Schema_Validator::validate_boolean_(const Ref_ & in) {
	if (!in.get_json_().isBool()) {
		error_(in) << " is not a boolean!" << '\n';
	}
}

void Json_Schema_Validator::validate_enum_(const Ref_ & in,
		const JSON & schema) {
	if (schema["enum"].isArray()) {
		if (!find_bool(schema["enum"], in.get_json_())) {
			error_(in) << " is not a correct enum!" << '\n';
		}
	}
}

bool Json_Schema_Validator::validate_regex_(const std::string &str,
		const std::string &regex) {
	std::regex re(regex, std::regex::ECMAScript);
	std::smatch sm;
	if (!std::regex_search(str, sm, re)) {
		return false;
	}
	return true;
}

bool Json_Schema_Validator::validate_all_of_(const Ref_ &in,
		const JSON &schema) {
	for (const JSON & j : schema) {
		Json_Schema_Validator jv(root_, in, j);
		if (!jv.get_result()) {
			sserror_ << "allOf fail: " << jv.get_error() << '\n';
			return false;
		}
	}
	return true;
}

bool Json_Schema_Validator::validate_any_of_(const Ref_ &in,
		const JSON &schema) {
	for (const JSON & j : schema) {
		Json_Schema_Validator jv(root_, in, j);
		if (jv.get_result()) {
			sserror_ << "anyOf success" << '\n';
			return true;
		} else {
			sserror_ << "anyOf fail: " << jv.get_error() << '\n';
		}
	}
	return false;
}

bool Json_Schema_Validator::validate_one_of_(const Ref_ &in,
		const JSON &schema) {
	int ctr = 0;
	for (const JSON & j : schema) {
		Json_Schema_Validator jv(root_, in, j);
		if (jv.get_result()) {
			ctr++;
			if (ctr > 1)
				break;
		} else {
			sserror_ << "oneOf fail: " << jv.get_error() << '\n';
		}
	}
	return ctr == 1;
}

bool Json_Schema_Validator::validate_not_(const Ref_ &in, const JSON &schema) {
	Json_Schema_Validator jv(root_, in, schema);
	if (!jv.get_result()) {
		sserror_ << in.get_position_() << "is ok to a \"NOT\" but should not";
		return false;
	}
	return true;
}

void Json_Schema_Validator::validate_(const Ref_ & in, const JSON & schema,
		bool allowRef) {
	if (allowRef && schema["$ref"].isString()) {
		Ref_ r(get_referece_(in.get_json_(), schema["$ref"].asString()));
		validate_(
				Ref_(get_referece_(in.get_json_(), schema["$ref"].asString())),
				schema, false);
		IF_TERMINATE;
		return;
	}
	std::string saveError = sserror_.str();
	if (schema["allOF"].isArray() && !validate_all_of_(in, schema["allOf"])) {
		error_(in) << " ALL_OF!" << '\n';
		TERMINATE;
	}
	if (schema["anyOf"].isArray() && !validate_any_of_(in, schema["anyOf"])) {
		error_(in) << " ANY_OF!" << '\n';
		TERMINATE;
	}
	if (schema["oneOf"].isArray() && !validate_one_of_(in, schema["oneOf"])) {
		error_(in) << " ONE_OF!" << '\n';
		TERMINATE;
	}
	if (schema["not"].isArray() && !validate_not_(in, schema["not"])) {
		error_(in) << " NOT!" << '\n';
		TERMINATE;
	}
	if (result_) {
		sserror_.str("");
		sserror_ << saveError;
	}
	if (schema["type"].isString()) {
		std::string type = schema["type"].asString();
		if (type == "object") {
			validate_object_(in, schema);
		} else if (type == "array") {
			validate_array_(in, schema);
		} else if (type == "string") {
			validate_string_(in, schema);
		} else if (type == "number") {
			validate_number_(in, schema);
		} else if (type == "integer") {
			validate_integer_(in, schema);
		} else if (type == "boolean") {
			validate_boolean_(in);
		} else if (type == "null" && !in.get_json_().isNull()) {
			error_(in) << "is not a null!";
		} else if (type == "not_null" && in.get_json_().isNull()) {
			error_(in) << "is a null!";
		}
	}
	validate_enum_(in, schema);
}

Json_Schema_Validator::Json_Schema_Validator(const JSON &root, const Ref_ & in,
		const JSON &schema, bool error) :
		result_(true), go_through_errors_(error), root_(root), root_schema_(
				schema) {
	validate_(in, root_schema_);
}

Json_Schema_Validator::Json_Schema_Validator(const JSON &root,
		const JSON &schema, bool error) :
		result_(true), go_through_errors_(error), root_(root), root_schema_(
				schema) {
	validate_(Ref_(root_), root_schema_);
}

bool Json_Schema_Validator::get_result() const {
	return result_;
}

std::string Json_Schema_Validator::get_error() const {
	return sserror_.str();
}
