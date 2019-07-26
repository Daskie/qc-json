#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

namespace qjson {

    using namespace std::string_literals;
    using namespace std::string_view_literals;

    struct JsonReadError : public std::runtime_error {
        size_t position;
        JsonReadError(const std::string & msg, size_t position) : std::runtime_error(msg), position(position) {}
    };

    struct Value;

    using Object = std::unordered_map<std::string, std::unique_ptr<Value>>;
    using Array = std::vector<std::unique_ptr<Value>>;

    struct Value {

        virtual      const Object * asObject() const { return nullptr; }
        virtual       const Array *  asArray() const { return nullptr; }
        virtual const std::string * asString() const { return nullptr; }
        virtual     const int64_t *    asInt() const { return nullptr; }
        virtual    const uint64_t *   asUInt() const { return nullptr; }
        virtual      const double *  asFloat() const { return nullptr; }
        virtual        const bool *   asBool() const { return nullptr; }

    };    

    class Reader {

      public:

        std::unique_ptr<Value> operator()(std::string_view str);

      private:

        const char * m_start, * m_end, *m_pos;

        bool m_isMore() const;

        int64_t m_remaining() const;

        size_t m_position() const;

        void m_skipWhitespace();

        void m_readChar(char c);

        bool m_checkChar(char c);

        bool m_checkString(std::string_view str);

        std::unique_ptr<Value> m_readValue();

        std::unique_ptr<ObjectWrapper> m_readObject();

        std::unique_ptr<ArrayWrapper> m_readArray();

        void m_readEscaped(std::string & str);

        char m_readUnicode();

        std::string m_readString();

        uint64_t m_readHex();

        uint64_t m_readUInt();

        std::unique_ptr<Value> m_readNumber();

    };

    // IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////

    namespace detail {

        struct ObjectWrapper : public Value, public Object {
            virtual const Object * asObject() const override { return this; }
        };

        struct ArrayWrapper : public Value, public Array {
            virtual const Array * asArray() const override { return this; }
        };

        struct StringWrapper : public Value, public std::string {
            StringWrapper(std::string && str) : std::string(move(str)) {}
            virtual const std::string * asString() const override { return this; }
        };

        struct IntWrapper : public Value {
            int64_t val;
            IntWrapper(int64_t val) : val(val) {}
            virtual const int64_t * asInt() const override { return &val; }
        };

        struct UIntWrapper : public Value {
            uint64_t val;
            UIntWrapper(uint64_t val) : val(val) {}
            virtual const uint64_t * asUInt() const override { return &val; }
        };

        struct FloatWrapper : public Value {
            double val;
            FloatWrapper(double val) : val(val) {}
            virtual const double * asFloat() const override { return &val; }
        };

        struct BoolWrapper : public Value {
            bool val;
            BoolWrapper(bool val) : val(val) {}
            virtual const bool * asBool() const override { return &val; }
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

    }

    using namespace detail;

    inline std::unique_ptr<Value> Reader::operator()(std::string_view str) {
        m_start = str.data();
        m_end = m_start + str.length();
        m_pos = m_start;

        m_skipWhitespace();
        m_readChar('{');

        std::unique_ptr<Value> val(m_readObject());

        m_skipWhitespace();
        if (m_pos != m_end) {
            throw JsonReadError("Content in string after root object", m_position());
        }

        return move(val);
    }

    inline bool Reader::m_isMore() const {
        return m_pos < m_end;
    }

    inline int64_t Reader::m_remaining() const {
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
            throw JsonReadError("Expected character `"s + c + "`"s, m_position());
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
            return std::unique_ptr<StringWrapper>(new StringWrapper(m_readString()));
        }
        else if (std::isdigit(c)) {
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
            return std::unique_ptr<BoolWrapper>(new BoolWrapper(true));
        }
        else if (m_checkString("false"sv)) {
            return std::unique_ptr<BoolWrapper>(new BoolWrapper(false));
        }
        else if (m_checkString("null"sv)) {
            return {};
        }
        else {
            throw JsonReadError("Unknown value", m_position());
        }
    }

    inline std::unique_ptr<ObjectWrapper> Reader::m_readObject() {
        std::unique_ptr<ObjectWrapper> obj(new ObjectWrapper());

        m_skipWhitespace();
        if (m_checkChar('}')) {
            return std::move(obj);
        }

        m_readChar('"');
        std::string key(m_readString());
        m_skipWhitespace();
        m_readChar(':');
        m_skipWhitespace();
        (*obj)[std::move(key)] = m_readValue();
        m_skipWhitespace();

        while (!m_checkChar('}')) {
            m_readChar(',');
            m_skipWhitespace();
            m_readChar('"');
            key = m_readString();
            m_skipWhitespace();
            m_readChar(':');
            m_skipWhitespace();
            (*obj)[std::move(key)] = m_readValue();
            m_skipWhitespace();
        }

        return std::move(obj);
    }

    inline std::unique_ptr<ArrayWrapper> Reader::m_readArray() {
        std::unique_ptr<ArrayWrapper> arr(new ArrayWrapper());

        m_skipWhitespace();
        if (m_checkChar(']')) {
            return std::move(arr);
        }

        m_skipWhitespace();
        arr->push_back(m_readValue());
        m_skipWhitespace();

        while (!m_checkChar('}')) {
            m_readChar(',');
            m_skipWhitespace();
            arr->push_back(m_readValue());
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

    inline uint64_t Reader::m_readUInt() {
        if (!m_isMore()) {
            throw JsonReadError("Expected integer sequence", m_position());
        }

        if (!std::isdigit(*m_pos)) {
            throw JsonReadError("Invalid digit", m_position());
        }

        uint64_t val(*m_pos - '0');
        ++m_pos;

        for (int digits(1); digits < 18 && m_isMore() && std::isdigit(*m_pos); ++digits) {
            val = val * 10 + (*m_pos - '0');
        }

        if (m_isMore() && std::isdigit(*m_pos)) {
            uint64_t potential(val * 10 + (*m_pos - '0'));
            if (potential < val) {
                throw JsonReadError("Integer value is too large", m_position());
            }
            val = potential;
            ++m_pos;
        }

        if (m_isMore() && std::isdigit(*m_pos)) {
            throw JsonReadError("Integer value is too large", m_position());
        }

        return val;
    }

    inline std::unique_ptr<Value> Reader::m_readNumber() {
        if (!m_isMore()) {
            throw JsonReadError("Expected number", m_position());
        }

        // Hexadecimal
        if (m_remaining() >= 3 && m_pos[0] == '0' && m_pos[1] == 'x') {
            m_pos += 2;
            return std::unique_ptr<UIntWrapper>(new UIntWrapper(m_readHex()));
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
        uint64_t integer(m_readUInt());

        // Parse fractional part
        uint64_t fraction(0);
        int fractionDigits(0);
        if (m_isMore() && *m_pos == '.') {
            ++m_pos;
            if (!m_isMore()) {
                throw JsonReadError("Expected fractional component", m_position());
            }

            const char * start(m_pos);
            fraction = m_readUInt();
            fractionDigits = m_pos - start;
        }

        // Parse exponent part
        bool hasExponent(false);
        int exponent(0);
        bool exponentNegative(false);
        if (m_isMore() && (*m_pos == 'e' || *m_pos == 'E')) {
            hasExponent = true;

            ++m_pos;
            if (!m_isMore()) {
                throw JsonReadError("Expected exponent", m_position());
            }

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

            uint64_t exponent64(m_readUInt());
            if (exponent64 > 0x7FFFFFFF) {
                throw JsonReadError("Exponent is too large", m_position());
            }
            exponent = int(exponent64);
        }

        // Assemble floating
        if (fractionDigits || hasExponent) {
            double val(double(integer) + double(fraction) * fastPow(10.0, -fractionDigits));
            if (hasExponent) val *= fastPow(10.0, exponent);
            return std::unique_ptr<FloatWrapper>(new FloatWrapper(negative ? -val : val));
        }
        // Assemble integral
        else {
            int64_t sval(int64_t{integer});
            if (sval < 0 && sval != std::numeric_limits<int64_t>::min()) {
                throw JsonReadError("Integer is too large", m_position());
            }
            return std::unique_ptr<IntWrapper>(new IntWrapper(negative ? -sval : sval));
        }

    }

}