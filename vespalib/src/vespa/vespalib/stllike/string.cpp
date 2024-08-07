// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "string.h"
#include "small_string.hpp"
#include <algorithm>
#include <cctype>
#include <istream>
#include <ostream>

namespace vespalib {

template<uint32_t SS>
std::ostream & operator << (std::ostream & os, const small_string<SS> & v)
{
     return os << v.buffer();
}

template<uint32_t SS>
std::istream & operator >> (std::istream & is, small_string<SS> & v)
{
    std::string s;
    is >> s;
    v = s;
    return is;
}

template std::ostream & operator << (std::ostream & os, const vespa_string & v);
template std::istream & operator >> (std::istream & is, vespa_string & v);

template class small_string<48>;

template vespa_string operator + (const vespa_string & a, const vespa_string & b) noexcept;
template vespa_string operator + (const vespa_string & a, std::string_view b) noexcept;
template vespa_string operator + (std::string_view a, const vespa_string & b) noexcept;
template vespa_string operator + (const vespa_string & a, const char * b) noexcept;
template vespa_string operator + (const  char * a, const vespa_string & b) noexcept;

const string &empty_string() noexcept {
    static string empty;
    return empty;
}

inline namespace waiting_for_godot {

void ltrim(vespalib::string &s) noexcept;
void rtrim(vespalib::string &s) noexcept;

void
ltrim(std::string &s) noexcept {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
}

void
rtrim(std::string &s) noexcept {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}
}

void
chomp(vespalib::string & s) noexcept {
    ltrim(s);
    rtrim(s);
}

vespalib::string
safe_char_2_string(const char * p) {
    return (p != nullptr) ? vespalib::string(p) : vespalib::string("");
}

}

namespace std {

vespalib::string
operator + (std::string_view a, const char * b) noexcept
{
    vespalib::string t(a);
    t += b;
    return t;
}

vespalib::string
operator + (const char * a, std::string_view b) noexcept
{
    vespalib::string t(a);
    t += b;
    return t;
}

vespalib::string
operator + (std::string_view a, std::string_view b) noexcept {
    vespalib::string t(a);
    t += b;
    return t;
}

}
