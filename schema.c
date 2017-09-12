#include <list>
#include <algorithm>
#include <regex>
#include <vector>

#include "schema.h"

using std::find;
using std::find_if;
using std::all_of;
using std::any_of;
using std::none_of;
using std::count_if;
using std::sort;
using std::adjacent_find;
using std::string;
using std::stringstream;
using std::to_string;
using std::list;
using std::vector;
using std::invalid_argument;
using std::stoi;
using std::regex;
using std::smatch;
using std::regex_search;
using std::remove_const_t;
using std::move;

#define TERMINATE if (! m_go_through_errors) return

#define IF_TERMINATE  if (! m_result && ! m_go_through_errors) return

#define COND_TERMINATE(cond, message) \
    if (cond) {\
        m_report(in) << " " << message << '\n';\
        TERMINATE;\
    }

namespace Json {

namespace get
{

template<typename Tp_>
bool is(const Value & val) {
  return false;
}

template<>
bool is<int>(const Value & val) {
  return val.isInt();
}

template<>
bool is<unsigned int>(const Value & val) {
  return val.isUInt();
}

template<>
bool is<double>(const Value & val) {
  return val.isDouble();
}

template<typename Tp_>
Tp_ as(const Value & val) {
  return Tp_();
}

template<>
int as<int>(const Value & val) {
  return val.asInt();
}

template<>
unsigned int as<unsigned int>(const Value & val) {
  return val.asUInt();
}

template<>
double as<double>(const Value & val) {
  return val.asDouble();
}

bool is_as_bool(const Value & json) {
  return json.isBool() && json.asBool();
}

} // namespace get

template<typename Tp_>
struct LESS {
  static bool compare(const Tp_ & a, const Tp_ & b) {
    return a <= b;
  }
  static bool compare_exclusive(const Tp_ & a, const Tp_ & b) {
    return a < b;
  }
};

template<typename Tp_>
struct MORE {
  static bool compare(const Tp_ & a, const Tp_ & b) {
    return a >= b;
  }
  static bool compare_exclusive(const Tp_ & a, const Tp_ & b) {
    return a > b;
  }
};

// return std::find(array.begin(), array.end(), element);
template<typename Array_, typename Element_>
decltype(auto) find_element(const Array_ & array, const Element_ & element) {
  return find(array.begin(), array.end(), element);
}

// return std::find(array.begin(), array.end(), element) != array.end();
template<typename Array_, typename Element_>
bool find_bool(const Array_ & array, const Element_ & element) {
  return find_element(array, element) != array.end();
}

bool is_unique(const Value & json) {
  vector<const Value *> vec;
  vec.reserve(json.size());
  for (const auto & item : json) {
    vec.push_back(&item);
  }
  sort(vec.begin(), vec.end(), [](const Value * l, const Value * r) { return *l < *r; });
  return adjacent_find(vec.begin(), vec.end(), [](const Value * l, const Value * r) { return *l == *r; }) == vec.end();
}

Schema::Ref::Ref() : mcp_ptr(nullptr), m_position() {}

Schema::Ref::Ref(const Ref & r)
    : mcp_ptr(r.mcp_ptr), m_position(r.m_position) {}

Schema::Ref::Ref(const Value & in)
    : mcp_ptr(&in), m_position("#") {}

Schema::Ref::Ref(const Value & in, const string & pos)
    : mcp_ptr(&in), m_position(pos) {}

Schema::Ref::Ref(const Ref & r, const string & pos)
    : mcp_ptr(&(r.get_json()[pos])), m_position(r.m_position + "/" + pos) {}

Schema::Ref::Ref(const Ref & r, const int & i)
    : mcp_ptr(&(r.get_json()[i])), m_position(r.m_position + "/" + to_string(i)) {}

Schema::Ref & Schema::Ref::operator=(const Ref & other) {
    mcp_ptr = other.mcp_ptr;
    m_position = other.m_position;
    return *this;
}

void Schema::Ref::step(const string & pos) {
    const auto & ref = *mcp_ptr;
    if (ref.isArray()) {
        mcp_ptr = &(ref[stoi(pos)]);
    } else {
        mcp_ptr = &(ref[pos]);
    }
    m_position += "/" + pos;
}

void Schema::Ref::step(const int & i) {
    mcp_ptr = &(get_json()[i]);
    m_position += "/" + to_string(i);
}

void Schema::Ref::forward(const string & pos) {
    if (pos != "") {
        string element = "";
        for (unsigned i = 0; i < pos.length(); i++) {
            if (pos[i] == '/') {
                if (element.length() > 0) {
                    step(element);
                }
                element = "";
            } else {
                element += pos[i];
            }
        }
        if (element.length() > 0) {
            step(element);
        }
    }
}

const string & Schema::Ref::get_position() const {
    return m_position;
}

const Value & Schema::Ref::get_json() const {
    return *mcp_ptr;
}

Schema::Ref Schema::mc_get_referece(const Value & in, const string & ref) const {
    try {
        if (ref[0] != '#') {
            throw invalid_argument(ref + " is not a correct reference!");
        }
        unsigned i = 1;
        Ref rtn_ref(in);
        if (ref[1] == '/') {
            rtn_ref = Ref(m_root, "#");
            i++;
        }
        rtn_ref.forward(ref.substr(i));
        return rtn_ref;
    } catch (...) {
        throw invalid_argument(ref + " is not a correct reference!");
    }
}

stringstream & Schema::m_report() {
    m_result = false;
    return m_error;
}

stringstream & Schema::m_report(const Ref & in) {
    m_result = false;
    m_error << in.get_position();
    return m_error;
}

void Schema::m_report(const string & str) {
    m_report() << " " << str << "\n";
}

void Schema::m_report(const Ref & in, const string & str) {
    m_report(in) << " " << str << "\n";
}

template<template<typename > class alg_, typename Tp_>
bool test_value(Tp_ test_value, const Value & schema, const string & get, const string & exclusive) {
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

void Schema::m_validate_array(const Ref & in, const Value & schema) {
    COND_TERMINATE(!in.get_json().isArray(), "is not an array!");
    const auto size = in.get_json().size();
    COND_TERMINATE(test_value<LESS>(size, schema, "minItems", "exclusiveMinimum"), "is below minimum!");
    COND_TERMINATE(test_value<MORE>(size, schema, "maxItems", "exclusiveMaximum"), "is exceeds maximum!");
    if (get::is_as_bool(schema["uniqueItems"])) {
        COND_TERMINATE(!is_unique(in.get_json()), "has not unique items!");
    }
    if (schema["items"].isObject()) {
        for (remove_const_t<decltype(size)> i = 0; i < size; i++) {
            m_validate(Ref(in, i), schema["items"][i]);
            IF_TERMINATE;
        }
    }
}

void Schema::m_validate_object(const Ref & in, const Value & schema) {
    COND_TERMINATE(!in.get_json().isObject(), "is not an object!");
    list<string> in_names, schema_names, requied_names, additional_names;
    for (const auto & member : in.get_json().getMemberNames()) {
        in_names.push_back(member);
    }
    if (schema["properties"].isObject()) {
        for (const auto & member : schema["properties"].getMemberNames()) {
            schema_names.push_back(member);
        }
    }
    auto req = false;
    if (schema["required"].isArray()) {
        for (const auto & member : schema["required"]) {
            requied_names.push_back(member.asString());
        }
        COND_TERMINATE(in_names.size() < requied_names.size(), "does not have enough element!");
        req = true;
    }
    unsigned add = 0;
    if (get::is_as_bool(schema["additionalItems"]) || !schema["properties"].isObject()) {
        add = 1;
    } else if (schema["additionalItems"].isArray()) {
        for (const auto & member : schema["additionalItems"]) {
            additional_names.push_back(member.asString());
        }
        COND_TERMINATE(in_names.size() > (additional_names.size() + requied_names.size()), "has too many element!");
        add = 2;
    }
    if (req && requied_names.size() > 0) {
        if (requied_names.size() > in_names.size()) {
            m_report(in) << "/" << " not enough element, expected: " << '\n';
            for (const auto & member : requied_names) {
                m_error << member << ", ";
            }
            m_error.seekp(-1, m_error.cur);
            m_error << ".";
            TERMINATE;
        } else {
            const auto it = find_if(requied_names.begin(), requied_names.end(), [&in_names](const string & name) {
                return !find_bool(in_names, name);
            });
            if (it != requied_names.end()) {
                m_report(in) << "/" << " some requied element(s) is/are missing, expected:" << '\n';
                for (const auto & member : requied_names) {
                    m_error << member << ", ";
                }
                m_error.seekp(-1, m_error.cur);
                m_error << ".";
                TERMINATE;
            }
        }
    }
    for (const auto & member : in_names) {
        const auto it = find_element(schema_names, member);
        if (it != schema_names.end()) {
            if (schema["properties"].isObject()) {
                m_validate(Ref(in, *it), schema["properties"][*it]);
                IF_TERMINATE;
            }
        } else if (add == 2) {
            COND_TERMINATE(!find_bool(additional_names, member), member << "is not expected!");
        } else COND_TERMINATE(add == 0, member << " is not expected!");
    }
}

void Schema::m_validate_string(const Ref & in, const Value & schema) {
    COND_TERMINATE(!in.get_json().isString(), "is not an string!")
    else {
        const auto str = in.get_json().asString();
        COND_TERMINATE(schema["minLength"].isInt() && str.length() < schema["minLength"].asUInt(), "is too short!");
        COND_TERMINATE(schema["maxLength"].isInt() && str.length() > schema["maxLength"].asUInt(), "is too long!");
        COND_TERMINATE(schema["pattern"].isString() && !mc_validate_regex(str, schema["pattern"].asString()), " does not match regex pattern!");
        // COND_TERMINATE(schema["format"].isString() && !validateRegex(str, _format.find(schema["format"].asString())->second), "does not match regex pattern!");
    }
}

void Schema::m_validate_number(const Ref & in, const Value & schema) {
    COND_TERMINATE(!(in.get_json().isInt64() || in.get_json().isDouble()), "is not a number!")
    else {
        const auto value = in.get_json().asDouble();
        COND_TERMINATE(test_value<MORE>(value, schema, "maximum", "exclusiveMaximum"), "is exceeds maximum!");
        COND_TERMINATE(test_value<LESS>(value, schema, "minimum", "exclusiveMinimum"), "is below minimum!");
    }
}

void Schema::m_validate_integer(const Ref & in, const Value & schema) {
    COND_TERMINATE(!in.get_json().isInt(), "is not an integer!")
    else {
        const auto value = in.get_json().asInt64();
        if (!schema["multipleOf"].isNull()) {
            const auto multi = schema["multipleOf"].asInt64();
            COND_TERMINATE(value % multi != 0, "is not multiple of " << to_string(multi) << "!");
        }
        m_validate_number(in, schema);
    }
}

void Schema::m_validate_boolean(const Ref & in) {
    if (!in.get_json().isBool()) {
        m_report(in) << " is not a boolean!" << '\n';
    }
}

void Schema::m_validate_enum(const Ref & in, const Value & schema) {
    if (schema["enum"].isArray()) {
        if (!find_bool(schema["enum"], in.get_json())) {
            m_report(in) << " is not a correct enum!" << '\n';
        }
    }
}

bool Schema::mc_validate_regex(const string & str, const string & regex_str) const {
    const regex re(regex_str, regex::ECMAScript);
    smatch sm;
    if (!regex_search(str, sm, re)) {
        return false;
    }
    return true;
}

bool Schema::m_validate_all_of(const Ref & in, const Value & schema) {
    return all_of(schema.begin(), schema.end(), [this, &in](const Value & sch) {
        Schema jsv(m_root.get_json(), in, sch);
        if (!jsv.get_result()) {
            m_error << "allOf fail: " << jsv.get_error() << '\n';
            return false;
        }
        return true;
    });
}

bool Schema::m_validate_any_of(const Ref & in, const Value & schema) {
    return any_of(schema.begin(), schema.end(), [this, &in](const Value & sch) {
        Schema jsv(m_root.get_json(), in, sch);
        if (jsv.get_result()) {
            m_error << "anyOf success" << '\n';
            return true;
        } else {
            m_error << "anyOf fail: " << jsv.get_error() << '\n';
            return false;
        }
    });
}

bool Schema::m_validate_one_of(const Ref & in, const Value & schema) {
    return 1 == count_if(schema.begin(), schema.end(), [&in, this](const Value & in_schema) {
        Schema jv(m_root.get_json(), in, in_schema);
        if (jv.get_result()) {
            return true;
        } else {
            m_error << "oneOf fail: " << jv.get_error() << '\n';
            return false;
        }
    });
}

bool Schema::m_validate_not(const Ref & in, const Value & schema) {
    return none_of(schema.begin(), schema.end(), [this, &in](const Value & sch) {
        Schema jsv(m_root.get_json(), in, sch);
        if (jsv.get_result()) {
            m_error << in.get_position() << " validate not fail\n";
            return true;
        }
        return false;
    });
}

void Schema::m_validate(const Ref & in, const Value & schema, bool allow_ref) {
    if (allow_ref && schema["$ref"].isString()) {
        m_validate(Ref(mc_get_referece(in.get_json(), schema["$ref"].asString())), schema, false);
        IF_TERMINATE;
        return;
    }
    const auto save_error = m_error.str();
    COND_TERMINATE(schema["allOF"].isArray() && !m_validate_all_of(in, schema["allOf"]), "ALL_OF!");
    COND_TERMINATE(schema["anyOF"].isArray() && !m_validate_any_of(in, schema["anyOf"]), "ANY_OF!");
    COND_TERMINATE(schema["oneOF"].isArray() && !m_validate_one_of(in, schema["oneOf"]), "ONE_OF!");
    COND_TERMINATE(schema["not"].isArray() && !m_validate_not(in, schema["not"]), "NOT!");
    if (m_result) {
        m_error.str("");
        m_error << save_error;
    }
    if (schema["type"].isString()) {
        const auto type = schema["type"].asString();
        if (type == "object") {
            m_validate_object(in, schema);
        } else if (type == "array") {
            m_validate_array(in, schema);
        } else if (type == "string") {
            m_validate_string(in, schema);
        } else if (type == "number") {
            m_validate_number(in, schema);
        } else if (type == "integer") {
            m_validate_integer(in, schema);
        } else if (type == "boolean") {
            m_validate_boolean(in);
        } else if (type == "null" && !in.get_json().isNull()) {
            m_report(in) << "is not a null!";
        } else if (type == "not_null" && in.get_json().isNull()) {
            m_report(in) << "is a null!";
        }
    }
    IF_TERMINATE;
    m_validate_enum(in, schema);
}

Schema::Schema(const Value & root, const Ref & in, const Value & schema, bool go_through)
    : m_schema(schema), m_go_through_errors(go_through), m_result(true), m_root(root) {
    m_validate(in, m_schema);
}

Schema::Schema(const Value & schema)
    : m_schema(schema), m_go_through_errors(false), m_result(true) {}

Schema::Schema(Value && schema)
    : m_schema(move(schema)), m_go_through_errors(false), m_result(true) {}

void Schema::set_error_type(bool go_through) {
    const_cast<bool &>(m_go_through_errors) = go_through;
}

Value & Schema::get_schema() { return m_schema; }

const Value & Schema::get_schema() const { return m_schema; }

bool Schema::validate(const Value & json) {
    m_root = Ref(json);
    m_result = true;
    m_error.clear();
    m_validate(m_root, m_schema);
    return m_result;
}

bool Schema::get_result() const {
    return m_result;
}

string Schema::get_error() const {
    return m_error.str();
}

} // namespace Json
