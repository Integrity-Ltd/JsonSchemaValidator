#ifndef JSONSCHEMAVALIDATOR_HH_
#define JSONSCHEMAVALIDATOR_HH_

#include <string>
#include <sstream>
#include <stdexcept>

#include <jsoncpp/json/json.h>
struct Reference {
private:
	const Json::Value * _ptr;
public:
	std::string _position;
	const Json::Value & _value;
	explicit Reference(const Json::Value &);
	explicit Reference(const Json::Value &, const std::string &);
	explicit Reference(const Reference & r, const std::string &);
	explicit Reference(const Reference & r, const int &);
};


class JsonSchemaValidator {
private:
	std::stringstream _error;
	bool _result;
	const bool _goThroughErrors;
	const Json::Value &_root, &_rootSchema;
	static std::map<const std::string, const std::string> _format;
	std::stringstream & error();
	Reference getReferece(const Json::Value &, const std::string &, const std::string &);
	void validateArray(const Reference &, const Json::Value &);
	void validateObject(const Reference &, const Json::Value &);
	void validateString(const Reference &, const Json::Value &);
	void validateNumber(const Reference &, const Json::Value &);
	void validateInteger(const Reference &, const Json::Value &);
	void validateBoolean(const Reference &, const Json::Value &);
	void validateEnum(const Reference &, const Json::Value &);
	bool validateRegex(const std::string &, const std::string &);
	bool validateAllOf(const Reference &, const Json::Value &);
	bool validateAnyOf(const Reference &, const Json::Value &);
	bool validateOneOf(const Reference &, const Json::Value &);
	bool validateNot(const Reference &, const Json::Value &);
	void validate(const Reference &, const Json::Value &, bool = true);
	JsonSchemaValidator(const Json::Value &, const Reference &, const Json::Value &schema, bool = false);
public:
	static void init();
	JsonSchemaValidator(const Json::Value &, const Json::Value &, bool = false);
	bool getResult() const;
	std::string getError() const;
};

#endif /* JSONSCHEMAVALIDATOR_HH_ */
