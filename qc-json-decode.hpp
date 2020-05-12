#pragma once

//
// QC Json 1.2.1
// Austin Quick
// July 2019 - May 2020
// https://github.com/Daskie/qc-json
//
// Decodes data from a JSON string and sends it to the provided `Composer`.
//
// See the GitHub link above for more info and examples.
//

#include <cctype>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace qc::json {

    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    //
    // This will be thrown if anything goes wrong during the decoding process.
    // `position` is the index into the string where the error occured.
    //
    struct DecodeError : public std::runtime_error {

        size_t position;

        DecodeError(const string & msg, size_t position);

    };

    //
    // Decodes the JSON string.
    //
    // A note on numbers:
    // A number will be parsed and sent to the composer as either a `int64_t`, a `uint64_t`, or a `double`.
    // - `int64_t` if the number is an integer (a fractional component of zero is okay) and can fit in a `int64_t`
    // - `uint64_t` if the number is a positive integer, can fit in a `uint64_t`, but cannot fit in a `int64_t`
    // - `double` if the number has a non-zero fractional component, has an exponent, or is an integer that is too large to fit in a `int64_t` or `uint64_t`
    //
    // @param json the string to decode
    // @param composer the contents of the JSON are decoded in order and passed to this to do something with
    // @param initialState the initial state object to be passed to the composer
    //
    template <typename Composer, typename State> void decode(string_view json, Composer & composer, State initialState);

}

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json::detail {

    // This functionality is wrapped in a class as a convenient way to keep track of state
    template <typename Composer, typename State>
    class Decoder {

        public:

        Decoder(string_view str, Composer & composer) :
            m_start(str.data()),
            m_end(m_start + str.length()),
            m_pos(m_start),
            m_composer(composer),
            m_stringBuffer()
        {}

        void operator()(State & initialState) {
            m_skipWhitespace();
            m_ingestValue(initialState);
            m_skipWhitespace();

            if (m_pos != m_end) {
                throw DecodeError("Extraneous content", m_pos - m_start);
            }
        }

        private:

        const char * const m_start;
        const char * const m_end;
        const char * m_pos;
        size_t m_line;
        size_t m_column;
        Composer & m_composer;
        string m_stringBuffer;

        void m_skipWhitespace() {
            while (m_pos < m_end && std::isspace(*m_pos)) ++m_pos;
        }

        bool m_tryConsumeChar(char c) {
            if (m_pos < m_end && *m_pos == c) {
                ++m_pos;
                return true;
            }
            else {
                return false;
            }
        }

        void m_consumeChar(char c) {
            if (!m_tryConsumeChar(c)) {
                throw DecodeError(("Expected `"s + c + "`"s).c_str(), m_pos - m_start);
            }
        }

        bool m_tryConsumeChars(string_view str) {
            if (size_t(m_end - m_pos) >= str.length()) {
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
            if (m_pos >= m_end) {
                throw DecodeError("Expected value", m_pos - m_start);
            }

            switch (*m_pos) {
                case '{': {
                    m_ingestObject(state);
                    break;
                }
                case '[': {
                    m_ingestArray(state);
                    break;
                }
                case '"': {
                    m_ingestString(state);
                    break;
                }
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                    m_ingestNumber(state);
                    break;
                }
                case '-': {
                    if (m_end - m_pos > 1 && std::isdigit(m_pos[1])) {
                        m_ingestNumber(state);
                        break;
                    }
                    else if (m_tryConsumeChars("-inf"sv)) {
                        m_composer.val(-std::numeric_limits<double>::infinity(), state);
                        break;
                    }
                    // Intentional fallthrough
                }
                default:
                    if (m_tryConsumeChars("true"sv)) {
                        m_composer.val(true, state);
                    }
                    else if (m_tryConsumeChars("false"sv)) {
                        m_composer.val(false, state);
                    }
                    else if (m_tryConsumeChars("null"sv)) {
                        m_composer.val(nullptr, state);
                    }
                    else if (m_tryConsumeChars("inf"sv)) {
                        m_composer.val(std::numeric_limits<double>::infinity(), state);
                    }
                    else if (m_tryConsumeChars("nan"sv)) {
                        m_composer.val(std::numeric_limits<double>::quiet_NaN(), state);
                    }
                    else {
                        throw DecodeError("Unknown value", m_pos - m_start);
                    }
            }
        }

        void m_ingestObject(State & outerState) {
            State innerState(m_composer.object(outerState));

            ++m_pos; // We already know we have `{`
            m_skipWhitespace();

            if (!m_tryConsumeChar('}')) {
                while (true) {
                    if (*m_pos != '"') {
                        throw DecodeError("Expected key", m_pos - m_start);
                    }
                    string_view key(m_consumeString());
                    if (key.empty()) {
                        throw DecodeError("Key is empty", m_pos - m_start);
                    }
                    m_composer.key(string(key), innerState);
                    m_skipWhitespace();

                    m_consumeChar(':');
                    m_skipWhitespace();

                    m_ingestValue(innerState);
                    m_skipWhitespace();

                    if (m_tryConsumeChar('}')) {
                        break;
                    }
                    else {
                        m_consumeChar(',');
                        m_skipWhitespace();
                    }
                }
            }

            m_composer.end(std::move(innerState), outerState);
        }

        void m_ingestArray(State & outerState) {
            State innerState(m_composer.array(outerState));

            ++m_pos; // We already know we have `[`
            m_skipWhitespace();

            if (!m_tryConsumeChar(']')) {
                while (true) {
                    m_ingestValue(innerState);
                    m_skipWhitespace();

                    if (m_tryConsumeChar(']')) {
                        break;
                    }
                    else {
                        m_consumeChar(',');
                        m_skipWhitespace();
                    }
                }
            }

            m_composer.end(std::move(innerState), outerState);
        }

        void m_ingestString(State & state) {
            m_composer.val(m_consumeString(), state);
        }

        string_view m_consumeString() {
            m_stringBuffer.clear();

            ++m_pos; // We already know we have `"`

            while (true) {
                if (m_pos >= m_end) {
                    throw DecodeError("Expected end quote", m_pos - m_start);
                }

                char c(*m_pos);
                if (c == '"') {
                    ++m_pos;
                    return m_stringBuffer;
                }
                else if (c == '\\') {
                    ++m_pos;
                    m_stringBuffer.push_back(m_consumeEscaped());
                }
                else if (std::isprint(c)) {
                    m_stringBuffer.push_back(c);
                    ++m_pos;
                }
                else {
                    throw DecodeError("Unknown string content", m_pos - m_start);
                }
            }
        }

        char m_consumeEscaped() {
            if (m_pos >= m_end) {
                throw DecodeError("Expected escape sequence", m_pos - m_start);
            }

            switch (*m_pos++) {
                case  '"': return  '"';
                case '\\': return '\\';
                case  '/': return  '/';
                case  'b': return '\b';
                case  'f': return '\f';
                case  'n': return '\n';
                case  'r': return '\r';
                case  't': return '\t';
                case  'u': return m_consumeUnicode();
                default:
                    throw DecodeError("Unknown escape sequence", m_pos - m_start - 1);
            }
        }

        char m_consumeUnicode() {
            if (m_end - m_pos < 4) {
                throw DecodeError("Expected four digits of unicode", m_pos - m_start);
            }

            uint32_t val;
            std::from_chars_result res(std::from_chars(m_pos, m_pos + 4, val, 16));
            if (res.ec != std::errc()) {
                throw DecodeError("Invalid unicode", m_pos - m_start);
            }

            // Is the high bit set?
            if (val & ~0x7Fu) {
                throw DecodeError("Non-ASCII unicode is unsupported", m_pos - m_start);
            }

            m_pos += 4;

            return val;
        }

        bool m_isInteger() const {
            // A precondition is that we know that the first character is either a digit or `-`
            const char * pos(m_pos + 1);
            // Skip all remaining leading digits
            while (pos < m_end && std::isdigit(*pos)) ++pos;
            // If that's it, we're an integer
            if (pos >= m_end) {
                return true;
            }
            // If instead there is a decimal point...
            else if (*pos == '.') {
                ++pos;
                // Skip all zeroes
                while (pos < m_end && *pos == '0') ++pos;
                // If there's a digit or an exponent, we must be a floater
                if (pos < m_end && (std::isdigit(*pos) || *pos == 'e' || *pos == 'E')) {
                    return false;
                }
                // Otherwise, we're an integer
                else {
                    return true;
                }
            }
            // If instead there is an exponent, we must be a floater
            else if (*pos == 'e' || *pos == 'E') {
                return false;
            }
            // Otherwise, that's the end of the number, and we're an integer
            else {
                return true;
            }
        }

        void m_ingestNumber(State & state) {
            // Determine if integer or floater
            if (m_isInteger()) {
                // Determine if signed
                if (*m_pos == '-') {
                    m_ingestInteger<true>(state);
                }
                else {
                    m_ingestInteger<false>(state);
                }
            }
            else {
                m_ingestFloater(state);
            }
        }

        template <bool isSigned>
        void m_ingestInteger(State & state) {
            std::conditional_t<isSigned, int64_t, uint64_t> val;
            std::from_chars_result res(std::from_chars(m_pos, m_end, val));

            // There was an issue parsing
            if (res.ec != std::errc()) {
                // If too large, parse as a floater instead
                if (res.ec == std::errc::result_out_of_range) {
                    m_ingestFloater(state);
                    return;
                }
                // Some other issue
                else {
                    throw DecodeError("Invalid integer", m_pos - m_start);
                }
            }

            m_pos = res.ptr;

            // Skip trailing decimal zeroes
            if (m_pos < m_end && *m_pos == '.') {
                ++m_pos;

                // Check for dangling decimal point
                if (m_pos >= m_end || *m_pos != '0') {
                    throw DecodeError("Dangling decimal point", m_pos - m_start);
                }
                ++m_pos;

                while (m_pos < m_end && *m_pos == '0') {
                    ++m_pos;
                }
            }

            // If unsigned and the most significant bit is not set, we default to reporting it as signed
            if constexpr (!isSigned) {
                if (!(val & 0x8000000000000000u)) {
                    m_composer.val(int64_t(val), state);
                    return;
                }
            }

            m_composer.val(val, state);
        }

        void m_ingestFloater(State & state) {
            double val;
            std::from_chars_result res(std::from_chars(m_pos, m_end, val));

            // There was an issue parsing, or a trailing decimal point
            if (res.ec != std::errc() || res.ptr[-1] == '.') {
                throw DecodeError("Invalid floater", m_pos - m_start);
            }

            m_pos = res.ptr;
            m_composer.val(val, state);
        }

    };

}

namespace qc::json {

    inline DecodeError::DecodeError(const string & msg, size_t position) :
        std::runtime_error(msg),
        position(position)
    {}

    template <typename Composer, typename State>
    inline void decode(string_view json, Composer & composer, State initialState) {
        return detail::Decoder<Composer, State>(json, composer)(initialState);
    }

}
