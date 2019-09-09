#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//
// Basic, lightweight JSON decoder.
//
// Example:
//      qjson::Object root(qjson::read(myJsonString));
//      const std::string & name(root["Price"]->asString());
//      const qjson::Array & favoriteBooks(root["Favorite Books"]->asArray());
//      const std::string & bookTitle(favoriteBooks[0]->asString());
//      ...
//------------------------------------------------------------------------------

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

#ifndef QJSON_COMMON
#define QJSON_COMMON
namespace qjson {

    // If anything goes wrong, this exception will be thrown
    struct JsonError : public std::exception {
        JsonError() = default;
        JsonError(const char * msg) : std::exception(msg) {}
        virtual ~JsonError() = default;
    };

}
#endif

namespace qjson {

    // This will be thrown if anything goes wrong during the decoding process
    // `position` is the index into the string where the error occured (roughly)
    struct JsonReadError : public JsonError {
        size_t position;
        JsonReadError(const char * msg, size_t position) : JsonError(msg), position(position) {}
    };

    // This will be thrown when attempting to access as value as the wrong type
    struct JsonTypeError : public JsonError {};

    enum class Type { object, array, string, integer, floating, boolean, null };

    struct Value;

    using Object = std::unordered_map<std::string, std::unique_ptr<Value>>;
    using Array = std::vector<std::unique_ptr<Value>>;

    struct Value {

        virtual Type type() const = 0;

        virtual const      Object &   asObject() const { throw JsonTypeError(); }
        virtual const       Array &    asArray() const { throw JsonTypeError(); }
        virtual const std::string &   asString() const { throw JsonTypeError(); }
        virtual           int64_t    asInteger() const { throw JsonTypeError(); }
        virtual            double   asFloating() const { throw JsonTypeError(); }
        virtual              bool    asBoolean() const { throw JsonTypeError(); }
        template <typename T> T as() const {
            return qjson_decode<T>(*this);
        }

    };

    Object read(std::string_view str);

}

// Specialize `qjson_dencode` to enable Value::as for custom types
// Example:
//      template <>
//      std::pair<int, int> qjson_dencode<std::pair<int, int>>(qjson::Value & v) {
//          const Array & arr(v.asArray());
//          return {arr.at(0)->asInteger(), arr.at(1)->asInteger()};
//      }
//
template <typename T> T qjson_decode(const qjson::Value & v);

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    using namespace std::string_literals;
    using namespace std::string_view_literals;

    namespace detail {

        struct ObjectWrapper : public Value {
            Object val;
            virtual Type type() const override { return Type::object; }
            virtual const Object & asObject() const override { return val; }
        };

        struct ArrayWrapper : public Value {
            Array val;
            virtual Type type() const override { return Type::array; }
            virtual const Array & asArray() const override { return val; }
        };

        struct StringWrapper : public Value {
            std::string val;
            StringWrapper(std::string && str) : val(move(str)) {}
            virtual Type type() const override { return Type::string; }
            virtual const std::string & asString() const override { return val; }
        };

        struct IntegerWrapper : public Value {
            int64_t val;
            virtual Type type() const override { return Type::integer; }
            IntegerWrapper(int64_t val) : val(val) {}
            virtual int64_t asInteger() const override { return val; }
        };

        struct FloatingWrapper : public Value {
            double val;
            FloatingWrapper(double val) : val(val) {}
            virtual Type type() const override { return Type::floating; }
            virtual double asFloating() const override { return val; }
        };

        struct BooleanWrapper : public Value {
            bool val;
            BooleanWrapper(bool val) : val(val) {}
            virtual Type type() const override { return Type::boolean; }
            virtual bool asBoolean() const override { return val; }
        };

        struct NullWrapper : public Value {
            virtual Type type() const override { return Type::null; }
        };

        constexpr unsigned char k_hexTable[256]{
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
            -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };

        inline double fastPow(double v, unsigned int e) {
            double r(1.0);

            do {
                if (e & 1) r *= v; // exponent is odd
                e >>= 1;
                v *= v;
            } while (e);

            return r;
        }

        inline double fastPow(double v, int e) {
            if (e >= 0) {
                return pow(v, unsigned int(e));
            }
            else {
                return pow(1.0 / v, unsigned int(-e));
            }
        }

        // A helper class to keep track of state
        class Reader {

            public:

            const char * const m_start, * const m_end, * m_pos;

            Reader(std::string_view str) :
                m_start(str.data()),
                m_end(m_start + str.length()),
                m_pos(m_start)
            {}

            Object operator()() {
                m_skipWhitespace();
                m_readChar('{');

                std::unique_ptr<Value> val(m_readObject());

                m_skipWhitespace();
                if (m_pos != m_end) {
                    throw JsonReadError("Content in string after root object", m_position());
                }

                return std::move(static_cast<ObjectWrapper *>(val.get())->val);
            }

            bool m_isMore() const {
                return m_pos < m_end;
            }

            size_t m_remaining() const {
                return m_end - m_pos;
            }

            size_t m_position() const {
                return m_pos - m_start;
            }

            void m_skipWhitespace() {
                while (m_isMore() && std::isspace(*m_pos)) ++m_pos;
            }

            void m_readChar(char c) {
                if (!m_checkChar(c)) {
                    throw JsonReadError(("Expected character `"s + c + "`"s).c_str(), m_position());
                }
            }

            bool m_checkChar(char c) {
                if (m_isMore() && *m_pos == c) {
                    ++m_pos;
                    return true;
                }
                else {
                    return false;
                }
            }

            bool m_checkString(std::string_view str) {
                if (m_remaining() >= str.length()) {
                    for (size_t i(0); i < str.length(); ++i) {
                        if (m_pos[i] != str[i]) {
                            return false;
                        }
                    }
                    m_pos += str.length();
                    return true;
                }
                else {
                    return false;
                }
            }

            std::unique_ptr<Value> m_readValue() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected value", m_position());
                }

                char c(*m_pos);
                if (c == '"') {
                    ++m_pos;
                    return std::make_unique<StringWrapper>(m_readString());
                }
                else if (std::isdigit(c) || c == '-') {
                    return m_readNumber();
                }
                else if (c == '{') {
                    ++m_pos;
                    return m_readObject();
                }
                else if (c == '[') {
                    ++m_pos;
                    return m_readArray();
                }
                else if (m_checkString("true"sv)) {
                    return std::make_unique<BooleanWrapper>(true);
                }
                else if (m_checkString("false"sv)) {
                    return std::make_unique<BooleanWrapper>(false);
                }
                else if (m_checkString("null"sv)) {
                    return std::make_unique<NullWrapper>();
                }
                else {
                    throw JsonReadError("Unknown value", m_position());
                }
            }

            std::unique_ptr<Value> m_readObject() {
                std::unique_ptr<ObjectWrapper> obj(std::make_unique<ObjectWrapper>());

                m_skipWhitespace();
                if (m_checkChar('}')) {
                    return std::move(obj);
                }

                m_readChar('"');
                std::string key(m_readString());
                m_skipWhitespace();
                m_readChar(':');
                m_skipWhitespace();
                obj->val[std::move(key)] = m_readValue();
                m_skipWhitespace();

                while (!m_checkChar('}')) {
                    m_readChar(',');
                    m_skipWhitespace();
                    m_readChar('"');
                    key = m_readString();
                    m_skipWhitespace();
                    m_readChar(':');
                    m_skipWhitespace();
                    obj->val[std::move(key)] = m_readValue();
                    m_skipWhitespace();
                }

                return std::move(obj);
            }

            std::unique_ptr<Value> m_readArray() {
                std::unique_ptr<ArrayWrapper> arr(std::make_unique<ArrayWrapper>());

                m_skipWhitespace();
                if (m_checkChar(']')) {
                    return std::move(arr);
                }

                m_skipWhitespace();
                arr->val.push_back(m_readValue());
                m_skipWhitespace();

                while (!m_checkChar(']')) {
                    m_readChar(',');
                    m_skipWhitespace();
                    arr->val.push_back(m_readValue());
                    m_skipWhitespace();
                }

                return std::move(arr);
            }

            void m_readEscaped(std::string & str) {
                if (!m_isMore()) {
                    throw JsonReadError("Expected escape sequence", m_position());
                }

                char c(*m_pos);
                switch (c) {
                    case '"':
                    case '\\':
                    case '/':
                        str.push_back(c);
                        ++m_pos;
                        return;
                    case 'b':
                        str.push_back('\b');
                        ++m_pos;
                        return;
                    case 'f':
                        str.push_back('\f');
                        ++m_pos;
                        return;
                    case 'n':
                        str.push_back('\n');
                        ++m_pos;
                        return;
                    case 'r':
                        str.push_back('\r');
                        ++m_pos;
                        return;
                    case 't':
                        str.push_back('\t');
                        ++m_pos;
                        return;
                    case 'u':
                        ++m_pos;
                        str.push_back(m_readUnicode());
                        return;
                    default:
                        throw JsonReadError("Unknown escape sequence", m_position());
                }
            }

            char m_readUnicode() {
                if (m_remaining() < 4) {
                    throw JsonReadError("Expected unicode", m_position());
                }

                unsigned char v3(k_hexTable[m_pos[2]]), v4(k_hexTable[m_pos[3]]);
                if (m_pos[0] != '0' || m_pos[1] != '0' || (v3 & 0xF8) || (v4 & 0xF0)) {
                    throw JsonReadError("Non-ASCII unicode is unsupported", m_position());
                }

                m_pos += 4;

                return (v3 << 4) | v4;
            }

            std::string m_readString() {
                const char * start(m_pos);
                std::string str;

                while (true) {
                    if (!m_isMore()) {
                        throw JsonReadError("Expected more string content", m_position());
                    }

                    char c(*m_pos);
                    if (c == '"') {
                        ++m_pos;
                        return std::move(str);
                    }
                    else if (c == '\\') {
                        ++m_pos;
                        m_readEscaped(str);
                    }
                    else if (std::isprint(c)) {
                        str.push_back(c);
                        ++m_pos;
                    }
                    else {
                        throw JsonReadError("Unknown string content", m_position());
                    }
                }
            }

            uint64_t m_readHex() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected hex sequence", m_position());
                }

                unsigned char hexVal(k_hexTable[*m_pos]);
                if (hexVal & 0xF0) {
                    throw JsonReadError("Invalid hex digit", m_position());
                }

                uint64_t val(hexVal);
                ++m_pos;

                for (int digits(1); digits < 16 && m_isMore() && (hexVal = k_hexTable[*m_pos]) < 16; ++digits, ++m_pos) {
                    val = (val << 4) | hexVal;
                }

                if (m_isMore() && std::isxdigit(*m_pos)) {
                    throw JsonReadError("Hex value is too large", m_position());
                }

                return val;
            }

            uint64_t m_readUnsignedInteger() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected integer sequence", m_position());
                }

                if (!std::isdigit(*m_pos)) {
                    throw JsonReadError("Invalid digit", m_position());
                }

                uint64_t val(*m_pos - '0');
                ++m_pos;

                for (int digits(1); digits < 19 && m_isMore() && std::isdigit(*m_pos); ++digits, ++m_pos) {
                    val = val * 10 + (*m_pos - '0');
                }

                if (m_isMore() && std::isdigit(*m_pos)) {
                    uint64_t potentialVal(val * 10 + (*m_pos - '0'));
                    if (potentialVal < val) {
                        throw JsonReadError("Integer too large", m_position());
                    }
                    val = potentialVal;
                    ++m_pos;

                    if (m_isMore() && std::isdigit(*m_pos)) {
                        throw JsonReadError("Too many digits", m_position());
                    }
                }

                return val;
            }

            std::unique_ptr<Value> m_readNumber() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected number", m_position());
                }

                // Hexadecimal
                if (m_remaining() >= 3 && m_pos[0] == '0' && m_pos[1] == 'x') {
                    m_pos += 2;
                    return std::make_unique<IntegerWrapper>(int64_t(m_readHex()));
                }

                // Parse sign
                bool negative(false);
                if (*m_pos == '-') {
                    negative = true;
                    ++m_pos;
                    if (!m_isMore()) {
                        throw JsonReadError("Expected number", m_position());
                    }
                }

                // Parse whole part
                uint64_t integer(m_readUnsignedInteger());

                // Parse fractional part
                uint64_t fraction(0);
                int fractionDigits(0);
                if (m_isMore() && *m_pos == '.') {
                    ++m_pos;
                    if (!m_isMore()) {
                        throw JsonReadError("Expected fractional component", m_position());
                    }

                    const char * start(m_pos);
                    fraction = m_readUnsignedInteger();
                    fractionDigits = int(m_pos - start);
                }

                // Parse exponent part
                bool hasExponent(false);
                int exponent(0);
                if (m_isMore() && (*m_pos == 'e' || *m_pos == 'E')) {
                    hasExponent = true;

                    ++m_pos;
                    if (!m_isMore()) {
                        throw JsonReadError("Expected exponent", m_position());
                    }

                    bool exponentNegative(false);
                    if (*m_pos == '-') {
                        exponentNegative = true;
                        ++m_pos;
                        if (!m_isMore()) {
                            throw JsonReadError("Expected exponent", m_position());
                        }
                    }
                    else if (*m_pos == '+') {
                        ++m_pos;
                        if (!m_isMore()) {
                            throw JsonReadError("Expected exponent", m_position());
                        }
                    }

                    uint64_t tempExponent(m_readUnsignedInteger());
                    if (tempExponent > 1000) {
                        throw JsonReadError("Exponent is too large", m_position());
                    }
                    exponent = exponentNegative ? -int(tempExponent) : int(tempExponent);
                }

                // Assemble floating
                if (fractionDigits || hasExponent) {
                    double val(double(integer) + double(fraction) * fastPow(10.0, -fractionDigits));
                    if (negative) val = -val;
                    if (hasExponent) val *= fastPow(10.0, exponent);
                    return std::make_unique<FloatingWrapper>(val);
                }
                // Assemble integral
                else {
                    return std::make_unique<IntegerWrapper>(negative ? -int64_t(integer) : int64_t(integer));
                }
            }

        };

    }

    using namespace detail;

    inline Object read(std::string_view str) {
        return Reader(str)();
    }

}
