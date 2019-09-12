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
#include <map>
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

    using std::string;
    using std::string_view;
    using std::unique_ptr;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    // This will be thrown if anything goes wrong during the decoding process
    // `position` is the index into the string where the error occured (roughly)
    struct JsonReadError : public JsonError {
        size_t position;
        JsonReadError(const char * msg, size_t position) : JsonError(msg), position(position) {}
    };

    // This will be thrown when attempting to access as value as the wrong type
    struct JsonTypeError : public JsonError {};

    enum class Type : uint32_t { none, object, array, string, integer, floating, boolean, null }; // CAN ONLY HAVE 8 VALUES

    class Value {

        using Pair = std::pair<string, Value>;

        public:

        Value(Type type) : m_typeCapacity(uint32_t(type)), m_size(0), m_data(0) {}
        Value(const string & str) : m_typeCapacity(uint32_t(Type::string)), m_size(uint32_t(str.length())) { // TODO: find a way to avoid this copy
            if (m_size <= 8) {
                std::copy_n(str.c_str(), m_size, m_strData);
            }
            else {
                m_strPtr = static_cast<char *>(::operator new(m_size));
                std::copy_n(str.c_str(), m_size, m_strPtr);
            }
        }
        Value(int64_t val) : m_typeCapacity(uint32_t(Type::integer)), m_size(0), m_integer(val) {}
        Value(double val) : m_typeCapacity(uint32_t(Type::floating)), m_size(0), m_floating(val) {}
        Value(bool val) : m_typeCapacity(uint32_t(Type::boolean)), m_size(0), m_boolean(val) {}
        Value(nullptr_t) : m_typeCapacity(uint32_t(Type::null)), m_size(0), m_data(0) {}

        Value(const Value &) = delete;
        Value(Value && other) : m_typeCapacity(other.m_typeCapacity), m_size(other.m_size), m_data(other.m_data) { other.m_typeCapacity = 0; other.m_size = 0; other.m_data = 0; }

        Value & operator=(const Value &) = delete;
        Value & operator=(Value &&) = delete;

        ~Value() {
            switch (type()) {
                case Type::object: for (uint32_t i(0); i < m_size; ++i) m_objPtr[i].~pair(); ::operator delete(m_objPtr); break;
                case Type::array: for (uint32_t i(0); i < m_size; ++i) m_arrPtr[i].~Value(); ::operator delete(m_arrPtr); break;
                case Type::string: if (m_size > 8) ::operator delete(m_strPtr); break;
            }
        }

        Type type() const { return Type(m_typeCapacity & 7u); }

        uint32_t size() const { return m_size; }

        uint32_t capacity() const { return m_typeCapacity & 0xFFFFFFF8u; }

        void reallocateArray() {
            uint32_t currCapacity(capacity());
            uint32_t newCapacity(currCapacity << 1);
            Value * newArrData(static_cast<Value *>(::operator new(newCapacity * sizeof(Value))));
            std::memcpy(newArrData, m_arrPtr, currCapacity * sizeof(Value));
            ::operator delete(m_arrPtr);
            m_typeCapacity = newCapacity | (m_typeCapacity & 7u);
            m_arrPtr = newArrData;
        }

        void reallocateObject() {
            uint32_t currCapacity(capacity());
            uint32_t newCapacity(currCapacity << 1);
            Pair * newObjData(static_cast<Pair *>(::operator new(newCapacity * sizeof(Pair))));
            std::memcpy(newObjData, m_objPtr, currCapacity * sizeof(Pair));
            ::operator delete(m_objPtr);
            m_typeCapacity = newCapacity | (m_typeCapacity & 7u);
            m_objPtr = newObjData;
        }

        Pair * find(string_view key) const {
            // TODO: this needs to return one past end in case of key being largest
            Pair * low(m_objPtr), * high(low + m_size);
            while (low < high) {
                Pair * mid(((high - low) >> 1) + low);
                int c(std::strcmp(key.data(), mid->first.c_str()));
                if (c < 0) {
                    high = mid;
                }
                else if (c > 0) {
                    low = mid + 1;
                }
                else {
                    return mid;
                }
            }
            return low;
        }

        const Value & operator[](string_view key) const {
            if (type() != Type::object) {
                throw JsonTypeError();
            }
            if (m_size == 0) {
                throw std::out_of_range("Key not found");
            }

            const Pair * pos(find(key));
            if (pos->first == key) {
                return pos->second;
            }
            else {
                throw std::out_of_range("Key not found");
            }
        }

        const Value & operator[](uint32_t i) const {
            if (type() != Type::array) {
                throw JsonTypeError();
            }
            if (i >= m_size) {
                throw std::out_of_range("Index is out of bounds");
            }

            return m_arrPtr[i];
        }

        void add(Value && val) {
            if (type() != Type::array) throw JsonTypeError();

            if (capacity() == 0) {
                m_arrPtr = static_cast<Value *>(::operator new(8 * sizeof(Value)));
                m_typeCapacity |= 8;
            }
            else if (m_size == capacity()) {
                reallocateArray();
            }

            new (m_arrPtr + m_size) Value(std::move(val));
            ++m_size;
        }

        void add(string && key, Value && val) {
            if (type() != Type::object) throw JsonTypeError();

            if (capacity() == 0) {
                m_objPtr = static_cast<Pair *>(::operator new(8 * sizeof(Pair)));
                m_typeCapacity |= 8;
            }

            if (m_size == 0) {
                new (&m_objPtr->first) string(std::move(key));
                new (&m_objPtr->second) Value(std::move(val));
                m_size = 1;
                return;
            }

            Pair * pos(find(key));

            if (pos > m_objPtr && (pos - 1)->first == key) {
                pos->second.~Value();
                new (&pos->second) Value(std::move(val));
                ++m_size;
                return;
            }

            if (m_size == capacity()) reallocateObject();

            std::memmove(pos + 1, pos, (m_size - (pos - m_objPtr)) * sizeof(Pair)); // TODO: possibly optimize by doing a backwards copy using 32 or 64 bit data chunks

            new (&pos->first) string(std::move(key));
            new (&pos->second) Value(std::move(val));
            ++m_size;
        }

        template <typename T> T as() const { return qjson_decode<T>()(*this); }

        template <> string_view as<string_view>() const { if (type() != Type::string) throw JsonTypeError(); return {m_size <= 8 ? m_strData : m_strPtr, m_size}; }
        template <>        char as<       char>() const { if (type() != Type::string || m_size > 1) throw JsonTypeError(); return *m_strData; };
        template <>     int64_t as<    int64_t>() const { if (type() != Type::integer) throw JsonTypeError(); return  int64_t(m_integer); }
        template <>     int32_t as<    int32_t>() const { if (type() != Type::integer) throw JsonTypeError(); return  int32_t(m_integer); }
        template <>     int16_t as<    int16_t>() const { if (type() != Type::integer) throw JsonTypeError(); return  int16_t(m_integer); }
        template <>      int8_t as<     int8_t>() const { if (type() != Type::integer) throw JsonTypeError(); return   int8_t(m_integer); }
        template <>    uint64_t as<   uint64_t>() const { if (type() != Type::integer) throw JsonTypeError(); return uint64_t(m_integer); }
        template <>    uint32_t as<   uint32_t>() const { if (type() != Type::integer) throw JsonTypeError(); return uint32_t(m_integer); }
        template <>    uint16_t as<   uint16_t>() const { if (type() != Type::integer) throw JsonTypeError(); return uint16_t(m_integer); }
        template <>     uint8_t as<    uint8_t>() const { if (type() != Type::integer) throw JsonTypeError(); return  uint8_t(m_integer); }
        template <>      double as<     double>() const { if (type() != Type::floating) throw JsonTypeError(); return m_floating; }
        template <>       float as<      float>() const { return float(as<double>()); }
        template <>        bool as<       bool>() const { if (type() != Type::boolean) throw JsonTypeError(); return m_boolean; }

        private:

        uint32_t m_typeCapacity;
        uint32_t m_size;
        union {
            uint64_t m_data;
            Pair * m_objPtr;
            Value * m_arrPtr;
            char * m_strPtr;
            char m_strData[8];
            int64_t m_integer;
            double m_floating;
            bool m_boolean;
        };

    };

    Value read(string_view str);

}

// Specialize `qjson_decode` to enable Value::as for custom types
// Example:
//      template <>
//      struct qjson_decode<std::pair<int, int>> {
//          std::pair<int, int> operator()(const qjson::Value & v) {
//              const Array & arr(v.asArray());
//              return {arr.at(0)->asInteger(), arr.at(1)->asInteger()};
//          }
//      };
//
template <typename T> struct qjson_decode;

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    namespace detail {

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

            Reader(string_view str) :
                m_start(str.data()),
                m_end(m_start + str.length()),
                m_pos(m_start)
            {}

            Value operator()() {
                m_skipWhitespace();
                m_readChar('{');

                Value val(m_readObject());

                m_skipWhitespace();
                if (m_pos != m_end) {
                    throw JsonReadError("Content in string after root object", m_position());
                }

                return val;
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

            bool m_checkString(string_view str) {
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

            Value m_readValue() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected value", m_position());
                }

                char c(*m_pos);
                if (c == '"') {
                    ++m_pos;
                    return Value(m_readString());
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
                    return true;
                }
                else if (m_checkString("false"sv)) {
                    return false;
                }
                else if (m_checkString("null"sv)) {
                    return nullptr;
                }
                else {
                    throw JsonReadError("Unknown value", m_position());
                }
            }

            Value m_readObject() {
                Value obj(Type::object);

                m_skipWhitespace();
                if (m_checkChar('}')) {
                    return obj;
                }

                m_readChar('"');
                string key(m_readString());
                m_skipWhitespace();
                m_readChar(':');
                m_skipWhitespace();
                obj.add(std::move(key), m_readValue());
                m_skipWhitespace();

                while (!m_checkChar('}')) {
                    m_readChar(',');
                    m_skipWhitespace();
                    m_readChar('"');
                    key = m_readString();
                    m_skipWhitespace();
                    m_readChar(':');
                    m_skipWhitespace();
                    obj.add(std::move(key), m_readValue());
                    m_skipWhitespace();
                }

                return obj;
            }

            Value m_readArray() {
                Value arr(Type::array);

                m_skipWhitespace();
                if (m_checkChar(']')) {
                    return arr;
                }

                m_skipWhitespace();
                arr.add(m_readValue());
                m_skipWhitespace();

                while (!m_checkChar(']')) {
                    m_readChar(',');
                    m_skipWhitespace();
                    arr.add(m_readValue());
                    m_skipWhitespace();
                }

                return arr;
            }

            void m_readEscaped(string & str) {
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

            string m_readString() {
                string str;

                while (true) {
                    if (!m_isMore()) {
                        throw JsonReadError("Expected more string content", m_position());
                    }

                    char c(*m_pos);
                    if (c == '"') {
                        ++m_pos;
                        return str;
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

            Value m_readNumber() {
                if (!m_isMore()) {
                    throw JsonReadError("Expected number", m_position());
                }

                // Hexadecimal
                if (m_remaining() >= 3 && m_pos[0] == '0' && m_pos[1] == 'x') {
                    m_pos += 2;
                    return int64_t(m_readHex());
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
                    return val;
                }
                // Assemble integral
                else {
                    return negative ? -int64_t(integer) : int64_t(integer);
                }
            }

        };

    }

    using namespace detail;

    inline Value read(string_view str) {
        return Reader(str)();
    }

}
