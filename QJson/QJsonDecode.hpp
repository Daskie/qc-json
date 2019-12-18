#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//
// Basic, lightweight JSON decoder.
//
// Example:
//      qjson::Object root(qjson::decode(myJsonString));
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


namespace qjson {

#ifndef QJSON_COMMON
#define QJSON_COMMON

    // If anything goes wrong, this exception will be thrown
    struct Error : public std::exception {
        Error() = default;
        Error(const char * msg) : std::exception(msg) {}
        virtual ~Error() = default;
    };

#endif

    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    // This will be thrown if anything goes wrong during the decoding process
    // `position` is the index into the string where the error occured (roughly)
    struct DecodeError : public Error {
        size_t position;
        DecodeError(const char * msg, size_t position) : Error(msg), position(position) {}
    };

    template <typename Decoder, typename State> void decode(string_view json, Decoder & decoder, State initialState);

}

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
        template <typename Decoder, typename State>
        class DecodeHelper {

            public:

            DecodeHelper(string_view str, Decoder & decoder) :
                m_start(str.data()),
                m_end(m_start + str.length()),
                m_pos(m_start),
                m_decoder(decoder)
            {}

            void operator()(State & initialState) {
                m_skipWhitespace();
                m_ingestValue(initialState);
                m_skipWhitespace();
                if (m_pos != m_end) {
                    throw DecodeError("Extraneous content", m_position());
                }
            }

            private:

            const char * const m_start, * const m_end, * m_pos;
            Decoder & m_decoder;
            std::string m_stringBuffer;

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

            void m_decodeChar(char c) {
                if (!m_checkChar(c)) {
                    throw DecodeError(("Expected character `"s + c + "`"s).c_str(), m_position());
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

            void m_ingestValue(State & state) {
                if (!m_isMore()) {
                    throw DecodeError("Expected value", m_position());
                }

                char c(*m_pos);
                if (c == '"') {
                    ++m_pos;
                    m_decoder.val(m_ingestString(), state);
                }
                else if (std::isdigit(c) || c == '-') {
                    m_ingestNumber(state);
                }
                else if (c == '{') {
                    ++m_pos;
                    m_ingestObject(state);
                }
                else if (c == '[') {
                    ++m_pos;
                    m_ingestArray(state);
                }
                else if (m_checkString("true"sv)) {
                    m_decoder.val(true, state);
                }
                else if (m_checkString("false"sv)) {
                    m_decoder.val(false, state);
                }
                else if (m_checkString("null"sv)) {
                    m_decoder.val(nullptr, state);
                }
                else {
                    throw DecodeError("Unknown value", m_position());
                }
            }

            void m_ingestObject(State & outerState) {
                State innerState(m_decoder.object(outerState));

                m_skipWhitespace();
                if (m_checkChar('}')) {
                    m_decoder.end(std::move(innerState), outerState);
                    return;
                }

                m_decodeChar('"');
                string_view key(m_ingestString());
                if (key.empty()) {
                    throw DecodeError("Key is empty", m_position()); // TODO: add test for this
                }
                m_decoder.key(string(key), innerState);
                m_skipWhitespace();
                m_decodeChar(':');
                m_skipWhitespace();
                m_ingestValue(innerState);
                m_skipWhitespace();

                while (!m_checkChar('}')) {
                    m_decodeChar(',');
                    m_skipWhitespace();
                    m_decodeChar('"');
                    m_decoder.key(string(m_ingestString()), innerState);
                    m_skipWhitespace();
                    m_decodeChar(':');
                    m_skipWhitespace();
                    m_ingestValue(innerState);
                    m_skipWhitespace();
                }

                m_decoder.end(std::move(innerState), outerState);
            }

            void m_ingestArray(State & outerState) {
                State innerState(m_decoder.array(outerState));

                m_skipWhitespace();
                if (m_checkChar(']')) {
                    m_decoder.end(std::move(innerState), outerState);
                    return;
                }

                m_skipWhitespace();
                m_ingestValue(innerState);
                m_skipWhitespace();

                while (!m_checkChar(']')) {
                    m_decodeChar(',');
                    m_skipWhitespace();
                    m_ingestValue(innerState);
                    m_skipWhitespace();
                }

                m_decoder.end(std::move(innerState), outerState);
            }

            char m_ingestEscaped() {
                if (!m_isMore()) {
                    throw DecodeError("Expected escape sequence", m_position());
                }

                char c(*m_pos);
                ++m_pos;

                switch (c) {
                    case  '"': return  '"';
                    case '\\': return '\\';
                    case  '/': return  '/';
                    case  'b': return '\b';
                    case  'f': return '\f';
                    case  'n': return '\n';
                    case  'r': return '\r';
                    case  't': return '\t';
                    case  'u': return m_ingestUnicode();
                    default:
                        throw DecodeError("Unknown escape sequence", m_position() - 1);
                }
            }

            char m_ingestUnicode() {
                if (m_remaining() < 4) {
                    throw DecodeError("Expected unicode", m_position());
                }

                unsigned char v3(k_hexTable[m_pos[2]]), v4(k_hexTable[m_pos[3]]);
                if (m_pos[0] != '0' || m_pos[1] != '0' || (v3 & 0xF8) || (v4 & 0xF0)) {
                    throw DecodeError("Non-ASCII unicode is unsupported", m_position());
                }

                m_pos += 4;

                return (v3 << 4) | v4;
            }

            string_view m_ingestString() {
                m_stringBuffer.clear();

                while (true) {
                    if (!m_isMore()) {
                        throw DecodeError("Expected more string content", m_position());
                    }

                    char c(*m_pos);
                    ++m_pos;

                    if (c == '"') {
                        return m_stringBuffer;
                    }
                    else if (c == '\\') {
                       m_stringBuffer.push_back(m_ingestEscaped());
                    }
                    else if (std::isprint(c)) {
                        m_stringBuffer.push_back(c);
                    }
                    else {
                        throw DecodeError("Unknown string content", m_position() - 1);
                    }
                }
            }

            void m_ingestHex(State & state) {
                if (!m_isMore()) {
                    throw DecodeError("Expected hex sequence", m_position());
                }

                unsigned char hexVal(k_hexTable[*m_pos]);
                if (hexVal & 0xF0) {
                    throw DecodeError("Invalid hex digit", m_position());
                }

                // TODO: this totally doesn't check if the other digits are valid hex???

                uint64_t val(hexVal);
                ++m_pos;

                for (int digits(1); digits < 16 && m_isMore() && (hexVal = k_hexTable[*m_pos]) < 16; ++digits, ++m_pos) {
                    val = (val << 4) | hexVal;
                }

                if (m_isMore() && std::isxdigit(*m_pos)) {
                    throw DecodeError("Hex value is too large", m_position());
                }

                m_decoder.val(val, state);
            }

            uint64_t m_decodeUnsignedInteger() {
                if (!m_isMore()) {
                    throw DecodeError("Expected integer sequence", m_position());
                }

                if (!std::isdigit(*m_pos)) {
                    throw DecodeError("Invalid digit", m_position());
                }

                uint64_t val(*m_pos - '0');
                ++m_pos;

                for (int digits(1); digits < 19 && m_isMore() && std::isdigit(*m_pos); ++digits, ++m_pos) {
                    val = val * 10 + (*m_pos - '0');
                }

                if (m_isMore() && std::isdigit(*m_pos)) {
                    uint64_t potentialVal(val * 10 + (*m_pos - '0'));
                    if (potentialVal < val) {
                        throw DecodeError("Integer too large", m_position());
                    }
                    val = potentialVal;
                    ++m_pos;

                    if (m_isMore() && std::isdigit(*m_pos)) {
                        throw DecodeError("Too many digits", m_position());
                    }
                }

                return val;
            }

            void m_ingestNumber(State & state) {
                if (!m_isMore()) {
                    throw DecodeError("Expected number", m_position());
                }

                // Hexadecimal
                if (m_remaining() >= 3 && m_pos[0] == '0' && m_pos[1] == 'x') {
                    m_pos += 2;
                    m_ingestHex(state);
                }

                // Parse sign
                bool negative(false);
                if (*m_pos == '-') {
                    negative = true;
                    ++m_pos;
                    if (!m_isMore()) {
                        throw DecodeError("Expected number", m_position());
                    }
                }

                // Parse whole part
                uint64_t integer(m_decodeUnsignedInteger());

                // Parse fractional part
                uint64_t fraction(0);
                int fractionDigits(0);
                if (m_isMore() && *m_pos == '.') {
                    ++m_pos;
                    if (!m_isMore()) {
                        throw DecodeError("Expected fractional component", m_position());
                    }

                    const char * start(m_pos);
                    fraction = m_decodeUnsignedInteger();
                    fractionDigits = int(m_pos - start);
                }

                // Parse exponent part
                bool hasExponent(false);
                int exponent(0);
                if (m_isMore() && (*m_pos == 'e' || *m_pos == 'E')) {
                    hasExponent = true;

                    ++m_pos;
                    if (!m_isMore()) {
                        throw DecodeError("Expected exponent", m_position());
                    }

                    bool exponentNegative(false);
                    if (*m_pos == '-') {
                        exponentNegative = true;
                        ++m_pos;
                        if (!m_isMore()) {
                            throw DecodeError("Expected exponent", m_position());
                        }
                    }
                    else if (*m_pos == '+') {
                        ++m_pos;
                        if (!m_isMore()) {
                            throw DecodeError("Expected exponent", m_position());
                        }
                    }

                    uint64_t tempExponent(m_decodeUnsignedInteger());
                    if (tempExponent > 1000) {
                        throw DecodeError("Exponent is too large", m_position());
                    }
                    exponent = exponentNegative ? -int(tempExponent) : int(tempExponent);
                }

                // Assemble floater
                if (fractionDigits || hasExponent) {
                    double val(double(integer) + double(fraction) * fastPow(10.0, -fractionDigits));
                    if (negative) val = -val;
                    if (hasExponent) val *= fastPow(10.0, exponent);
                    m_decoder.val(val, state);
                }
                // Assemble integral
                else {
                    m_decoder.val(negative ? -int64_t(integer) : int64_t(integer), state);
                }
            }

        };

    }

    using namespace detail;

    template <typename Decoder, typename State>
    inline void decode(string_view json, Decoder & decoder, State initialState) {
        return DecodeHelper<Decoder, State>(json, decoder)(initialState);
    }

}
