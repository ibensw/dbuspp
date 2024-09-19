#pragma once

#include <cstdint>
#include <dbus-1.0/dbus/dbus.h>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

// #include <iostream>
// using std::cout;
// using std::endl;

namespace wibens::dbuspp {
inline namespace v1 {
namespace type {

using Ignore = std::monostate;
using U8 = uint8_t;
using Boolean = bool;
using I16 = int16_t;
using U16 = uint16_t;
using I32 = int32_t;
using U32 = uint32_t;
using I64 = int64_t;
using U64 = uint64_t;
using Double = double;
using String = std::string;

struct UnixFileDescriptor {
    uint32_t fd;
};
class ObjectPath : public std::string {
  public:
    using std::string::string;
};

template <typename... T> using Variant = std::variant<T...>;
template <typename T> using Array = std::vector<T>;
template <typename... T> using DictEntry = std::pair<String, Variant<T...>>;
template <typename... T> using Dict = std::map<String, Variant<T...>>;

} // namespace type

namespace parsers {

struct ParseException : public std::runtime_error {
    static inline char dbusTypeToChar(int type) { return static_cast<char>(type); }

    using std::runtime_error::runtime_error;
    ParseException(int typeReceived, const std::string &expected)
        : runtime_error(std::string{"Expected type one of ["} + expected + "] but received " +
                        dbusTypeToChar(typeReceived)) {}
    ParseException(int typeReceived, char expected)
        : runtime_error(std::string{"Expected type one of ["} + expected + "] but received " +
                        dbusTypeToChar(typeReceived)) {}
};

template <typename Type, int DBUS_TYPE> struct ParserHelper {
    static constexpr inline bool accept(int dbusType) { return dbusType == DBUS_TYPE; }
};

template <typename Type, int DBUS_TYPE> struct BasicTypeParserHelper : ParserHelper<Type, DBUS_TYPE> {
    using ParserHelper<Type, DBUS_TYPE>::accept;
    static void addArgument(DBusMessage *msg, Type val) {
        dbus_message_append_args(msg, DBUS_TYPE, &val, DBUS_TYPE_INVALID);
    }
    static Type parse(DBusMessageIter *iter) {
        // cout << "Parsing buildin" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, DBUS_TYPE);
        }
        Type val{};
        dbus_message_iter_get_basic(iter, &val);
        // cout << val << endl;
        return val;
    }
};

template <typename Type, int DBUS_TYPE> struct StringParserHelper : ParserHelper<Type, DBUS_TYPE> {
    using ParserHelper<Type, DBUS_TYPE>::accept;
    static void addArgument(DBusMessage *msg, std::string_view val) {
        auto addr = val.data();
        dbus_message_append_args(msg, DBUS_TYPE, &addr, DBUS_TYPE_INVALID);
    }
    static Type parse(DBusMessageIter *iter) {
        // cout << "Parsing string-like" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, DBUS_TYPE);
        }
        const char *str;
        dbus_message_iter_get_basic(iter, &str);
        // cout << str << endl;
        return str;
    }
};

template <typename T> struct Parser;
template <> struct Parser<type::U8> : BasicTypeParserHelper<type::U8, DBUS_TYPE_BYTE> {};
template <> struct Parser<type::Boolean> : BasicTypeParserHelper<type::Boolean, DBUS_TYPE_BOOLEAN> {};
template <> struct Parser<type::I16> : BasicTypeParserHelper<type::I16, DBUS_TYPE_INT16> {};
template <> struct Parser<type::U16> : BasicTypeParserHelper<type::U16, DBUS_TYPE_UINT16> {};
template <> struct Parser<type::I32> : BasicTypeParserHelper<type::I32, DBUS_TYPE_INT32> {};
template <> struct Parser<type::U32> : BasicTypeParserHelper<type::U32, DBUS_TYPE_UINT32> {};
template <> struct Parser<type::I64> : BasicTypeParserHelper<type::I64, DBUS_TYPE_INT64> {};
template <> struct Parser<type::U64> : BasicTypeParserHelper<type::U64, DBUS_TYPE_UINT64> {};
template <> struct Parser<type::Double> : BasicTypeParserHelper<type::Double, DBUS_TYPE_DOUBLE> {};
template <>
struct Parser<type::UnixFileDescriptor> : BasicTypeParserHelper<type::UnixFileDescriptor, DBUS_TYPE_UNIX_FD> {};

template <> struct Parser<type::Ignore> {
    static constexpr bool accept([[maybe_unused]] int) { return true; }
    static type::Ignore parse(DBusMessageIter *) { return {}; }
};

template <> struct Parser<type::String> : StringParserHelper<type::String, DBUS_TYPE_STRING> {};
template <> struct Parser<const char *> : StringParserHelper<type::String, DBUS_TYPE_STRING> {};
template <> struct Parser<std::string_view> : StringParserHelper<type::String, DBUS_TYPE_STRING> {};
template <> struct Parser<type::ObjectPath> : StringParserHelper<type::ObjectPath, DBUS_TYPE_OBJECT_PATH> {};

// template <typename T> struct Parser<type::String> : StringParserHelper<type::String, DBUS_TYPE_STRING> {};

template <typename... TYPES>
struct Parser<type::Variant<TYPES...>> : ParserHelper<type::Variant<TYPES...>, DBUS_TYPE_VARIANT> {
    using Type = type::Variant<TYPES...>;
    using ParserHelper<Type, DBUS_TYPE_VARIANT>::accept;
    template <typename T, typename... REST> static Type parseInner(int type, DBusMessageIter *iter) {
        if (Parser<T>::accept(type)) {
            return Parser<T>::parse(iter);
        }
        if constexpr (sizeof...(REST) > 0) {
            return parseInner<REST...>(type, iter);
        }
        throw ParseException(std::string{"No matching variant type found for "} + ParseException::dbusTypeToChar(type));
    }

    static auto parse(DBusMessageIter *iter) {
        // cout << "Parsing variant" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, DBUS_TYPE_VARIANT);
        }
        DBusMessageIter subIter;
        dbus_message_iter_recurse(iter, &subIter);
        return parseInner<TYPES...>(dbus_message_iter_get_arg_type(&subIter), &subIter);
    }
};

template <typename T> struct Parser<type::Array<T>> : ParserHelper<type::Array<T>, DBUS_TYPE_ARRAY> {
    using Type = type::Array<T>;
    using ParserHelper<Type, DBUS_TYPE_ARRAY>::accept;
    static Type parse(DBusMessageIter *iter) {
        // cout << "Parsing array" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, DBUS_TYPE_ARRAY);
        }
        Type response;
        DBusMessageIter subIter;
        dbus_message_iter_recurse(iter, &subIter);
        while (true) {
            int type = dbus_message_iter_get_arg_type(&subIter);
            if (type == DBUS_TYPE_INVALID) {
                break;
            }
            response.emplace_back(Parser<T>::parse(&subIter));
            dbus_message_iter_next(&subIter);
        }
        return response;
    }
};

template <typename... TYPES> struct Parser<std::tuple<TYPES...>> {
    using Type = std::tuple<TYPES...>;
    static constexpr bool accept(int type) { return type == DBUS_TYPE_DICT_ENTRY || type == DBUS_TYPE_ARRAY; }
    template <typename T, typename... REST> static auto parseInner(DBusMessageIter *iter) {
        auto first = std::make_tuple<T>(Parser<T>::parse(iter));
        if constexpr (sizeof...(REST) > 0) {
            dbus_message_iter_next(iter);
            return std::tuple_cat(first, parseInner<REST...>(iter));
        } else {
            return first;
        }
    }

    static auto parse(DBusMessageIter *iter) {
        // cout << "Parsing tuple" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, "ea");
        }
        DBusMessageIter subIter;
        dbus_message_iter_recurse(iter, &subIter);
        return parseInner<TYPES...>(&subIter);
    }
};

template <typename... TYPES>
struct Parser<type::DictEntry<TYPES...>> : ParserHelper<type::DictEntry<TYPES...>, DBUS_TYPE_DICT_ENTRY> {
    using Type = type::DictEntry<TYPES...>;
    using ParserHelper<Type, DBUS_TYPE_DICT_ENTRY>::accept;
    template <typename T, typename... REST> static auto parseInner(DBusMessageIter *iter) {
        auto first = std::make_tuple<T>(Parser<T>::parse(iter));
        if constexpr (sizeof...(REST) > 0) {
            dbus_message_iter_next(iter);
            return std::tuple_cat(first, parseInner<REST...>(iter));
        } else {
            return first;
        }
    }

    static auto parse(DBusMessageIter *iter) {
        // cout << "Parsing dictentry" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, "ea");
        }
        DBusMessageIter subIter;
        dbus_message_iter_recurse(iter, &subIter);
        auto key = Parser<typename Type::first_type>::parse(&subIter);
        dbus_message_iter_next(&subIter);
        auto value = Parser<typename Type::second_type>::parse(&subIter);
        return std::make_pair(key, value);
    }
};

template <typename... T> struct Parser<type::Dict<T...>> : ParserHelper<type::Dict<T...>, DBUS_TYPE_ARRAY> {
    using Type = type::Dict<T...>;
    using ParserHelper<Type, DBUS_TYPE_ARRAY>::accept;
    static Type parse(DBusMessageIter *iter) {
        // cout << "Parsing dict" << endl;
        if (auto type = dbus_message_iter_get_arg_type(iter); !accept(type)) {
            throw ParseException(type, DBUS_TYPE_ARRAY);
        }
        Type response;
        DBusMessageIter subIter;
        dbus_message_iter_recurse(iter, &subIter);
        while (true) {
            int type = dbus_message_iter_get_arg_type(&subIter);
            if (type == DBUS_TYPE_INVALID) {
                break;
            }
            response.insert(Parser<type::DictEntry<T...>>::parse(&subIter));
            dbus_message_iter_next(&subIter);
        }
        return response;
    }
};

} // namespace parsers

} // namespace v1
} // namespace wibens::dbuspp
