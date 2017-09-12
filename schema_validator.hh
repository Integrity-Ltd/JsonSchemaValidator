#ifndef JSON_SCHEMA_VALIDATOR_HH_
#define JSON_SCHEMA_VALIDATOR_HH_

#include <string>
#include <sstream>
#include <stdexcept>

#include <jsoncpp/json/json.h>

class Schema_Validator {
public:
	typedef Json::Value JSON;
private:
	std::stringstream m_error;
	bool m_result;
	const bool mc_go_through_errors;
	const JSON & mcr_root, & mcr_root_schema;
	// static std::map<const std::string, const std::string> ms_format;
	class Ref {
		const JSON * mcp_ptr;
		std::string m_position;
	public:
		explicit Ref(const JSON &);
        	Ref(const Ref &);
		explicit Ref(const JSON &, const std::string &);
		explicit Ref(const Ref &, const std::string &);
		explicit Ref(const Ref &, const int &);
		void step(const std::string &);
		void step(const int &);
		void forward(const std::string &);
		const std::string & get_position() const;
		const JSON & get_json() const;
	};
	std::stringstream & m_report();
	std::stringstream & m_report(const Ref &);
	void m_report(const std::string &);
	void m_report(const Ref &, const std::string &);
	Ref mc_get_referece(const JSON &, const std::string &) const;
	void m_validate_array(const Ref &, const JSON &);
	void m_validate_object(const Ref &, const JSON &);
	void m_validate_string(const Ref &, const JSON &);
	void m_validate_number(const Ref &, const JSON &);
	void m_validate_integer(const Ref &, const JSON &);
	void m_validate_boolean(const Ref &);
	void m_validate_enum(const Ref &, const JSON &);
	bool mc_validate_regex(const std::string &, const std::string &) const;
	bool m_validate_all_of(const Ref &, const JSON &);
	bool m_validate_any_of(const Ref &, const JSON &);
	bool m_validate_one_of(const Ref &, const JSON &);
	bool m_validate_not(const Ref &, const JSON &);
	void m_validate(const Ref &, const JSON &, bool = true);
	Schema_Validator(const JSON & json, const Ref_ &, const JSON &schema, bool = false);
public:
	Schema_Validator(const JSON & json, const JSON & schema, bool = false);
	bool get_result() const;
	std::string get_error() const;
};

#endif /* JSON_SCHEMA_VALIDATOR_HH_ */
