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

    enum class Type { object, array, string, integer, hex, floating, boolean, null };

    struct Value;

    using Object = std::unordered_map<std::string, std::unique_ptr<Value>>;
    using Array = std::vector<std::unique_ptr<Value>>;

    struct Value {

        virtual Type type() const = 0;

        virtual const      Object &   asObject() const { throw JsonTypeError(); }
        virtual const       Array &    asArray() const { throw JsonTypeError(); }
        virtual const std::string &   asString() const { throw JsonTypeError(); }
        virtual           int64_t    asInteger() const { throw JsonTypeError(); }
        virtual          uint64_t        asHex() const { throw JsonTypeError(); }
        virtual            double   asFloating() const { throw JsonTypeError(); }
        virtual              bool    asBoolean() const { throw JsonTypeError(); }

    };

    // A helper class to keep track of state, only the `read` function is important
    class Reader {

        public:

        // This is the sole function the user invokes directly
        friend Object read(std::string_view str);

        private:

        const char * const m_start, * const m_end, * m_pos;

        Reader(std::string_view str);

        Object operator()();

        bool m_isMore() const;

        size_t m_remaining() const;

        size_t m_position() const;

        void m_skipWhitespace();

        void m_readChar(char c);

        bool m_checkChar(char c);

        bool m_checkString(std::string_view str);

        std::unique_ptr<Value> m_readValue();

        std::unique_ptr<Value> m_readObject();

        std::unique_ptr<Value> m_readArray();

        void m_readEscaped(std::string & str);

        char m_readUnicode();

        std::string m_readString();

        uint64_t m_readHex();

        int64_t m_readInteger(bool negative);

        std::unique_ptr<Value> m_readNumber();

    };

}

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

        struct HexWrapper : public Value {
            uint64_t val;
            HexWrapper(uint64_t val) : val(val) {}
            virtual Type type() const override { return Type::hex; }
            virtual uint64_t asHex() const override { return val; }
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

        inline double fastPow(double v, uint64_t e) {
            double r(1.0);

            do {
                if (e & 1) r *= v; // exponent is odd
                e >>= 1;
                v *= v;
            } while (e);

            return r;
        }

        inline double fastPow(double v, int64_t e) {
            if (e >= 0) {
                return pow(v, uint64_t(e));
            }
            else {
                return pow(1.0 / v, uint64_t(-e));
            }
        }

    }

    using namespace detail;

    inline Object read(std::string_view str) {
        return Reader(str)();
    }

    inline Reader::Reader(std::string_view str) :
        m_start(str.data()),
        m_end(m_start + str.length()),
        m_pos(m_start)
    {}

    inline Object Reader::operator()() {
        m_skipWhitespace();
        m_readChar('{');

        std::unique_ptr<Value> val(m_readObject());

        m_skipWhitespace();
        if (m_pos != m_end) {
            throw JsonReadError("Content in string after root object", m_position());
        }

        return std::move(static_cast<ObjectWrapper*>(val.get())->val);
    }

    inline bool Reader::m_isMore() const {
        return m_pos < m_end;
    }

    inline size_t Reader::m_remaining() const {
        return m_end - m_pos;
    }

    inline size_t Reader::m_position() const {
        return m_pos - m_start;
    }

    inline void Reader::m_skipWhitespace() {
        while (m_isMore() && std::isspace(*m_pos)) ++m_pos;
    }

    inline void Reader::m_readChar(char c) {
        if (!m_checkChar(c)) {
            throw JsonReadError(("Expected character `"s + c + "`"s).c_str(), m_position());
        }
    }

    inline bool Reader::m_checkChar(char c) {
        if (m_isMore() && *m_pos == c) {
            ++m_pos;
            return true;
        }
        else {
            return false;
        }
    }

    inline bool Reader::m_checkString(std::string_view str) {
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

    inline std::unique_ptr<Value> Reader::m_readValue() {
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

    inline std::unique_ptr<Value> Reader::m_readObject() {
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

    inline std::unique_ptr<Value> Reader::m_readArray() {
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

    inline void Reader::m_readEscaped(std::string & str) {
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

    inline char Reader::m_readUnicode() {
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

    inline std::string Reader::m_readString() {
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

    inline uint64_t Reader::m_readHex() {
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

    inline int64_t Reader::m_readInteger(bool negative) {
        if (!m_isMore()) {
            throw JsonReadError("Expected integer sequence", m_position());
        }

        if (!std::isdigit(*m_pos)) {
            throw JsonReadError("Invalid digit", m_position());
        }

        uint64_t uval(*m_pos - '0');
        ++m_pos;

        for (int digits(1); digits < 19 && m_isMore() && std::isdigit(*m_pos); ++digits, ++m_pos) {
            uval = uval * 10 + (*m_pos - '0');
        }

        bool inRange(uval < 0x8000000000000000ull || negative && uval == 0x8000000000000000ull);
        if (!inRange || m_isMore() && std::isdigit(*m_pos)) {
            throw JsonReadError("Integer value is too large", m_position());
        }

        int64_t sval{int64_t(uval)};
        return negative ? -sval : sval;
    }

    inline std::unique_ptr<Value> Reader::m_readNumber() {
        if (!m_isMore()) {
            throw JsonReadError("Expected number", m_position());
        }

        // Hexadecimal
        if (m_remaining() >= 3 && m_pos[0] == '0' && m_pos[1] == 'x') {
            m_pos += 2;
            return std::make_unique<HexWrapper>(m_readHex());
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
        int64_t integer(m_readInteger(negative));

        // Parse fractional part
        int64_t fraction(0);
        int64_t fractionDigits(0);
        if (m_isMore() && *m_pos == '.') {
            ++m_pos;
            if (!m_isMore()) {
                throw JsonReadError("Expected fractional component", m_position());
            }

            const char * start(m_pos);
            fraction = m_readInteger(negative);
            fractionDigits = int64_t(m_pos - start);
        }

        // Parse exponent part
        bool hasExponent(false);
        int64_t exponent(0);
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

            exponent = m_readInteger(exponentNegative);
        }

        // Assemble floating
        if (fractionDigits || hasExponent) {
            double val(double(integer) + double(fraction) * fastPow(10.0, -fractionDigits));
            if (hasExponent) val *= fastPow(10.0, exponent);
            return std::make_unique<FloatingWrapper>(val);
        }
        // Assemble integral
        else {
            return std::make_unique<IntegerWrapper>(integer);
        }

    }

}
