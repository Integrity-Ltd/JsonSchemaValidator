#ifndef JSON_SCHEMA_H_INCLUDED
#define JSON_SCHEMA_H_INCLUDED

#include <sstream>

#include <jsoncpp/json/value.h>

namespace Json {

class Schema {
  class Ref {
    const Value * mcp_ptr;
    std::string m_position;
  public:
    explicit Ref();
    explicit Ref(const Value &);
    Ref(const Ref &);
    explicit Ref(const Value &, const std::string &);
    explicit Ref(const Ref &, const std::string &);
    explicit Ref(const Ref &, const int &);
    Ref & operator=(const Ref &);
    void step(const std::string &);
    void step(const int &);
    void forward(const std::string &);
    const std::string & get_position() const;
    const Value & get_json() const;
  };
  Value m_schema;
  bool m_go_through_errors;
  std::stringstream m_error;
  bool m_result;
  Ref m_root;
  std::stringstream & m_report();
  std::stringstream & m_report(const Ref &);
  void m_report(const std::string &);
  void m_report(const Ref &, const std::string &);
  Ref mc_get_referece(const Value &, const std::string &) const;
  void m_validate_array(const Ref &, const Value &);
  void m_validate_object(const Ref &, const Value &);
  void m_validate_string(const Ref &, const Value &);
  void m_validate_number(const Ref &, const Value &);
  void m_validate_integer(const Ref &, const Value &);
  void m_validate_boolean(const Ref &);
  void m_validate_enum(const Ref &, const Value &);
  bool mc_validate_regex(const std::string &, const std::string &) const;
  bool m_validate_all_of(const Ref &, const Value &);
  bool m_validate_any_of(const Ref &, const Value &);
  bool m_validate_one_of(const Ref &, const Value &);
  bool m_validate_not(const Ref &, const Value &);
  void m_validate(const Ref &, const Value &, bool = true);

  Schema(const Value & json, const Ref &, const Value & schema, bool = false);

public:
  Schema(const Value & schema);
  Schema(Value && schema);

  void set_error_type(bool go_through);

  Value & get_schema();
  const Value & get_schema() const;

  bool validate(const Value & json);

  bool get_result() const;
  std::string get_error() const;
};

} // namespace Json

#endif // JSON_SCHEMA_H_INCLUDED
