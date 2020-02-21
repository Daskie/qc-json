#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019 - February 2020
//------------------------------------------------------------------------------
// Basic, lightweight JSON decoder.
//
// Decodes a json string and sends its json constituents to the Composer
// provided in a SAX fashion.
//
// Basic usage:
//
//      try {
//          qjson::decode(myJsonString, myComposer, myState);
//      }
//      catch (const qjson::DecoderError & e) {
//          std::cerr << "Error decoding json" << std::endl;
//          std::cerr << "What: " << e.what() << std::endl;
//          std::cerr << "Where: " << e.position << std::endl;
//      }
//
// A composer class must provide a set of methods that will be called from the
// `decode` function. An example prototype class is provided below:
//
#if 0

class MyComposer {

    struct MyState {
        // Some useful data
    };

    // Called when a json object is encountered.
    //
    // `outerState` is the state at the level containing the object
    // returns the new state to use for within the object
    //
    MyState object(MyState & outerState);

    // Called when an array is encountered.
    //
    // `outerState` is the state at the level containing the array
    // returns the new state to use for within the array
    //
    MyState array(MyState & outerState);

    // Called at then end of an object or array.
    //
    // `innerState` is the state that existed within the object or array
    // `outerState` is the state at the level containing the object or array
    //
    void end(MyState && innerState, MyState & outerState);

    // Called when an object key is parsed.
    //
    // `key` is the key string
    // `state` is the state of the current object
    //
    void key(std::string && key, MyState & state);

    // Called when a string is parsed.
    //
    // `val` is the string
    // `state` is the state of the current object or array
    //
    void val(string_view val, MyState & state);

    // Called when an integer is parsed.
    //
    // `val` is the integer
    // `state` is the state of the current object or array
    //
    void val(int64_t val, MyState & state);

    // Called when a hex value is parsed.
    //
    // `val` is the hex value
    // `state` is the state of the current object or array
    //
    void val(uint64_t val, MyState & state);

    // Called when a floating point number is parsed.
    //
    // `val` is the floating point number
    // `state` is the state of the current object or array
    //
    void val(double val, MyState & state);

    // Called when a boolean is parsed.
    //
    // `val` is the boolean
    // `state` is the state of the current object or array
    //
    void val(bool val, MyState & state);

    // Called when a `null` is parsed.
    //
    // `state` is the state of the current object or array
    //
    void val(nullptr_t, MyState & state);

};

#endif
//
// The state object is not strictly necessary, but will be useful, if not
// essential, for most functional decoders. But if you really don't need it,
// just pass nullptr.
//------------------------------------------------------------------------------

#include <cctype>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace qjson {

#ifndef QJSON_COMMON
#define QJSON_COMMON

    struct Error : public std::runtime_error {

        Error() : std::runtime_error(nullptr) {}
        Error(const std::string & msg) : std::runtime_error(msg) {}

        ~Error() override = default;

    };

#endif

    // This will be thrown if anything goes wrong during the decoding process.
    // `position` is the index into the string where the error occured.
    struct DecodeError : public Error {

        size_t position;

        DecodeError(const std::string & msg, size_t position) : Error(msg), position(position) {}

    };

    template <typename Composer, typename State> void decode(std::string_view json, Composer & decoder, State initialState);

}

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    namespace detail {

        // A helper class to keep track of state
        template <typename Composer, typename State>
        class DecodeHelper {

          public:

            DecodeHelper(string_view str, Composer & decoder) :
                m_start(str.data()),
                m_end(m_start + str.length()),
                m_pos(m_start),
                m_composer(decoder),
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
                    case '0': {
                        if (m_end - m_pos > 1 && m_pos[1] == 'x') {
                            m_ingestHex(state);
                        }
                        else {
                            m_ingestDecimal(state);
                        }
                        break;
                    }
                    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                        m_ingestDecimal(state);
                        break;
                    }
                    case '-': {
                        if (m_end - m_pos > 1 && std::isdigit(m_pos[1])) {
                            m_ingestDecimal(state);
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

                if (val & 0xFFFFFF80) {
                    throw DecodeError("Non-ASCII unicode is unsupported", m_pos - m_start);
                }

                m_pos += 4;

                return val;
            }

            void m_ingestHex(State & state) {
                m_pos += 2; // We already know we have `0x`

                uint64_t val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val, 16));

                if (res.ec != std::errc()) {
                    throw DecodeError("Invalid hex", m_pos - m_start);
                }

                m_pos = res.ptr;
                m_composer.val(val, state);
            }

            void m_ingestDecimal(State & state) {
                // Determine if integral or floating point
                const char * pos(m_pos + 1);
                while (pos < m_end && std::isdigit(*pos)) ++pos;
                if (pos == m_end || !(*pos == '.' || *pos == 'e' || *pos == 'E')) {
                    m_ingestInteger(state);
                }
                else {
                    m_ingestFloater(state);
                }
            }

            void m_ingestInteger(State & state) {
                int64_t val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val));

                if (res.ec != std::errc()) {
                    throw DecodeError("Invalid integer", m_pos - m_start);
                }

                m_pos = res.ptr;
                m_composer.val(val, state);
            }

            void m_ingestFloater(State & state) {
                double val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val));

                if (res.ec != std::errc() || res.ptr[-1] == '.') {
                    throw DecodeError("Invalid floater", m_pos - m_start);
                }

                m_pos = res.ptr;
                m_composer.val(val, state);
            }

        };

    }

    template <typename Composer, typename State>
    inline void decode(string_view json, Composer & decoder, State initialState) {
        return detail::DecodeHelper<Composer, State>(json, decoder)(initialState);
    }

}
