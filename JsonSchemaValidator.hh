#ifndef JSON_SCHEMA_VALIDATOR_HH_
#define JSON_SCHEMA_VALIDATOR_HH_

#include <string>
#include <sstream>
#include <stdexcept>

#include <jsoncpp/json/json.h>

class Json_Schema_Validator {
public:
	typedef Json::Value JSON;
private:
	std::stringstream sserror_;
	bool result_;
	const bool go_through_errors_;
	const JSON &root_, &root_schema_;
	class Ref_ {
		const JSON * ptr_;
		std::string position_;
	public:
		explicit Ref_(const JSON &);
		explicit Ref_(const Ref_ &);
		explicit Ref_(const JSON &, const std::string &);
		explicit Ref_(const Ref_ & r, const std::string &);
		explicit Ref_(const Ref_ & r, const int &);
		void step_(const std::string &pos);
		void step_(const int &i);
		void forward_(const std::string &pos);
		const std::string & get_position_() const;
		const JSON & get_json_() const;
	};
	// static std::map<const std::string, const std::string> format_;
	std::stringstream & error_();
	std::stringstream & error_(const Ref_ &);
	void error_(const std::string &);
	void error_(const Ref_ &, const std::string &);
	Ref_ get_referece_(const JSON &, const std::string &);
	void validate_array_(const Ref_ &, const JSON &);
	void validate_object_(const Ref_ &, const JSON &);
	void validate_string_(const Ref_ &, const JSON &);
	void validate_number_(const Ref_ &, const JSON &);
	void validate_integer_(const Ref_ &, const JSON &);
	void validate_boolean_(const Ref_ &);
	void validate_enum_(const Ref_ &, const JSON &);
	bool validate_regex_(const std::string &, const std::string &);
	bool validate_all_of_(const Ref_ &, const JSON &);
	bool validate_any_of_(const Ref_ &, const JSON &);
	bool validate_one_of_(const Ref_ &, const JSON &);
	bool validate_not_(const Ref_ &, const JSON &);
	void validate_(const Ref_ &, const JSON &, bool = true);
	Json_Schema_Validator(const JSON & json, const Ref_ &, const JSON &schema,
			bool = false);
public:
	Json_Schema_Validator(const JSON & json, const JSON & schema, bool = false);
	bool get_result() const;
	std::string get_error() const;
};

#endif /* JSON_SCHEMA_VALIDATOR_HH_ */
