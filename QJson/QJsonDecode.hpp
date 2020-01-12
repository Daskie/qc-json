#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//------------------------------------------------------------------------------
// Basic, lightweight JSON decoder.
//
// Parses a json string and sends its json constituents to the Decoder provided
// in SAX form.
//
// Basic usage:
//
//      try {
//          qjson::decode(myJsonString, myDecoder, myState);
//      }
//      catch (const qjson::DecoderError & e) {
//          std::cerr << "Error decoding json" << std::endl;
//          std::cerr << "What: " << e.what() << std::endl;
//          std::cerr << "Where: " << e.position << std::endl;
//      }
//
// A decoder must provide a set of methods that will be called from the `decode`
// function. An example prototype class for a decoder is provided below:
//
#if 0

class MyDecoder {

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

    //
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

#include <charconv>
#include <string>


namespace qjson {

#ifndef QJSON_COMMON
#define QJSON_COMMON

    struct Error : public std::exception {
        Error() = default;
        Error(const char * msg) : std::exception(msg) {}
        ~Error() override = default;
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
            string m_stringBuffer;

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

            bool m_tryConsumeChar(char c) {
                if (m_isMore() && *m_pos == c) {
                    ++m_pos;
                    return true;
                }
                else {
                    return false;
                }
            }

            void m_consumeChar(char c) {
                if (!m_tryConsumeChar(c)) {
                    throw DecodeError(("Expected `"s + c + "`"s).c_str(), m_position());
                }
            }

            bool m_tryConsumeChars(string_view str) {
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

                if (!(
                    m_tryIngestObject(state) ||
                    m_tryIngestArray(state) ||
                    m_tryIngestString(state) ||
                    m_tryIngestHex(state) ||
                    m_tryIngestIntegerOrFloater(state) ||
                    m_tryIngestBool(state) ||
                    m_tryIngestNull(state) ||
                    m_tryIngestSpecialFloater(state)
                )) {
                    throw DecodeError("Unknown value", m_position());
                }
            }

            bool m_tryIngestObject(State & outerState) {
                if (!m_tryConsumeChar('{')) {
                    return false;
                }

                State innerState(m_decoder.object(outerState));

                m_skipWhitespace();
                if (m_tryConsumeChar('}')) {
                    m_decoder.end(std::move(innerState), outerState);
                    return true;
                }

                while (true) {
                    auto [wasStr, key](m_tryConsumeString());
                    if (!wasStr) {
                        throw DecodeError("Expected key string", m_position());
                    }
                    if (key.empty()) {
                        throw DecodeError("Key is empty", m_position());
                    }
                    m_decoder.key(string(key), innerState);
                    m_skipWhitespace();
                    m_consumeChar(':');
                    m_skipWhitespace();
                    m_ingestValue(innerState);
                    m_skipWhitespace();
                    if (m_tryConsumeChar('}')) {
                        break;
                    }
                    m_consumeChar(',');
                    m_skipWhitespace();
                }

                m_decoder.end(std::move(innerState), outerState);
                return true;
            }

            bool m_tryIngestArray(State & outerState) {
                if (!m_tryConsumeChar('[')) {
                    return false;
                }

                State innerState(m_decoder.array(outerState));

                m_skipWhitespace();
                if (m_tryConsumeChar(']')) {
                    m_decoder.end(std::move(innerState), outerState);
                    return true;
                }

                while (true) {
                    m_ingestValue(innerState);
                    m_skipWhitespace();
                    if (m_tryConsumeChar(']')) {
                        break;
                    }
                    m_consumeChar(',');
                    m_skipWhitespace();
                }

                m_decoder.end(std::move(innerState), outerState);
                return true;
            }

            bool m_tryIngestString(State & state) {
                if (auto [wasStr, str](m_tryConsumeString()); wasStr) {
                    m_decoder.val(str, state);
                    return true;
                }
                else {
                    return false;
                }
            }

            std::pair<bool, string_view> m_tryConsumeString() {
                if (!m_tryConsumeChar('"')) {
                    return {};
                }

                m_stringBuffer.clear();

                while (true) {
                    if (!m_isMore()) {
                        throw DecodeError("Expected end quote", m_position());
                    }

                    char c(*m_pos);
                    if (c == '"') {
                        ++m_pos;
                        return {true, m_stringBuffer};
                    }
                    else if (c == '\\') {
                        ++m_pos;
                        m_stringBuffer.push_back(m_ingestEscaped());
                    }
                    else if (std::isprint(c)) {
                        m_stringBuffer.push_back(c);
                        ++m_pos;
                    }
                    else {
                        throw DecodeError("Unknown string content", m_position());
                    }
                }
            }

            char m_ingestEscaped() {
                if (!m_isMore()) {
                    throw DecodeError("Expected escape sequence", m_position());
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
                    case  'u': return m_ingestUnicode();
                    default:
                        throw DecodeError("Unknown escape sequence", m_position() - 1);
                }
            }

            char m_ingestUnicode() {
                if (m_remaining() < 4) {
                    throw DecodeError("Expected four digits of unicode", m_position());
                }

                uint32_t val;
                std::from_chars_result res(std::from_chars(m_pos, m_pos + 4, val, 16));
                if (res.ec != std::errc()) {
                    throw DecodeError("Invalid unicode", m_position());
                }

                if (val & 0xFFFFFF80) {
                    throw DecodeError("Non-ASCII unicode is unsupported", m_position());
                }

                m_pos += 4;

                return val;
            }

            bool m_tryIngestHex(State & state) {
                if (!m_tryConsumeChars("0x"sv)) {
                    return false;
                }

                uint64_t val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val, 16));

                if (res.ec != std::errc()) {
                    throw DecodeError("Invalid hex", m_position());
                }

                m_pos = res.ptr;
                m_decoder.val(val, state);
                return true;
            }

            bool m_tryIngestIntegerOrFloater(State & state) {
                if (!(std::isdigit(*m_pos) || (*m_pos == '-' && m_remaining() >= 2 && std::isdigit(m_pos[1])))) {
                    return false;
                }

                // Determine if integral or floating point
                const char * pos(m_pos + 1);
                while (pos < m_end && std::isdigit(*pos)) ++pos;
                if (pos == m_end || !(*pos == '.' || *pos == 'e' || *pos == 'E')) {
                    m_ingestInteger(state);
                }
                else {
                    m_ingestFloater(state);
                }
                return true;
            }

            void m_ingestInteger(State & state) {
                int64_t val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val));

                if (res.ec != std::errc()) {
                    throw DecodeError("Invalid integer", m_position());
                }

                m_pos = res.ptr;
                m_decoder.val(val, state);
            }

            void m_ingestFloater(State & state) {
                double val;
                std::from_chars_result res(std::from_chars(m_pos, m_end, val));

                if (res.ec != std::errc() || res.ptr[-1] == '.') {
                    throw DecodeError("Invalid floater", m_position());
                }

                m_pos = res.ptr;
                m_decoder.val(val, state);
            }

            bool m_tryIngestBool(State & state) {
                if (m_tryConsumeChars("true"sv)) {
                    m_decoder.val(true, state);
                    return true;
                }
                else if (m_tryConsumeChars("false"sv)) {
                    m_decoder.val(false, state);
                    return true;
                }
                else {
                    return false;
                }
            }

            bool m_tryIngestNull(State & state) {
                if (m_tryConsumeChars("null"sv)) {
                    m_decoder.val(nullptr, state);
                    return true;
                }
                else {
                    return false;
                }
            }

            bool m_tryIngestSpecialFloater(State & state) {
                if (m_tryConsumeChars("inf"sv)) {
                    m_decoder.val(std::numeric_limits<double>::infinity(), state);
                    return true;
                }
                else if (m_tryConsumeChars("-inf"sv)) {
                    m_decoder.val(-std::numeric_limits<double>::infinity(), state);
                    return true;
                }
                else if (m_tryConsumeChars("nan"sv)) {
                    m_decoder.val(std::numeric_limits<double>::quiet_NaN(), state);
                    return true;
                }
                else {
                    return false;
                }
            }

        };

    }

    template <typename Decoder, typename State>
    inline void decode(string_view json, Decoder & decoder, State initialState) {
        return detail::DecodeHelper<Decoder, State>(json, decoder)(initialState);
    }

}
