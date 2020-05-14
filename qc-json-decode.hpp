#pragma once

//
// QC Json 1.2.2
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

        DecodeError(const string & msg, size_t position) noexcept;

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

namespace qc::json {

    // This functionality is wrapped in a class as a convenient way to keep track of state
    template <typename Composer, typename State>
    class _Decoder {

        public:

        _Decoder(string_view str, Composer & composer) :
            _start(str.data()),
            _end(_start + str.length()),
            _pos(_start),
            _composer(composer),
            _stringBuffer()
        {}

        void operator()(State & initialState) {
            _skipWhitespace();
            _ingestValue(initialState);
            _skipWhitespace();

            if (_pos != _end) {
                throw DecodeError("Extraneous content", _pos - _start);
            }
        }

        private:

        const char * const _start;
        const char * const _end;
        const char * _pos;
        size_t _line;
        size_t _column;
        Composer & _composer;
        string _stringBuffer;

        void _skipWhitespace() {
            while (_pos < _end && std::isspace(*_pos)) ++_pos;
        }

        bool _tryConsumeChar(char c) {
            if (_pos < _end && *_pos == c) {
                ++_pos;
                return true;
            }
            else {
                return false;
            }
        }

        void _consumeChar(char c) {
            if (!_tryConsumeChar(c)) {
                throw DecodeError(("Expected `"s + c + "`"s).c_str(), _pos - _start);
            }
        }

        bool _tryConsumeChars(string_view str) {
            if (size_t(_end - _pos) >= str.length()) {
                for (size_t i(0); i < str.length(); ++i) {
                    if (_pos[i] != str[i]) {
                        return false;
                    }
                }
                _pos += str.length();
                return true;
            }
            else {
                return false;
            }
        }

        void _ingestValue(State & state) {
            if (_pos >= _end) {
                throw DecodeError("Expected value", _pos - _start);
            }

            switch (*_pos) {
                case '{': {
                    _ingestObject(state);
                    break;
                }
                case '[': {
                    _ingestArray(state);
                    break;
                }
                case '"': {
                    _ingestString(state);
                    break;
                }
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                    _ingestNumber(state);
                    break;
                }
                case '-': {
                    if (_end - _pos > 1 && std::isdigit(_pos[1])) {
                        _ingestNumber(state);
                        break;
                    }
                    else if (_tryConsumeChars("-inf"sv)) {
                        _composer.val(-std::numeric_limits<double>::infinity(), state);
                        break;
                    }
                    // Intentional fallthrough
                }
                default:
                    if (_tryConsumeChars("true"sv)) {
                        _composer.val(true, state);
                    }
                    else if (_tryConsumeChars("false"sv)) {
                        _composer.val(false, state);
                    }
                    else if (_tryConsumeChars("null"sv)) {
                        _composer.val(nullptr, state);
                    }
                    else if (_tryConsumeChars("inf"sv)) {
                        _composer.val(std::numeric_limits<double>::infinity(), state);
                    }
                    else if (_tryConsumeChars("nan"sv)) {
                        _composer.val(std::numeric_limits<double>::quiet_NaN(), state);
                    }
                    else {
                        throw DecodeError("Unknown value", _pos - _start);
                    }
            }
        }

        void _ingestObject(State & outerState) {
            State innerState(_composer.object(outerState));

            ++_pos; // We already know we have `{`
            _skipWhitespace();

            if (!_tryConsumeChar('}')) {
                while (true) {
                    if (*_pos != '"') {
                        throw DecodeError("Expected key", _pos - _start);
                    }
                    string_view key(_consumeString());
                    if (key.empty()) {
                        throw DecodeError("Key is empty", _pos - _start);
                    }
                    _composer.key(string(key), innerState);
                    _skipWhitespace();

                    _consumeChar(':');
                    _skipWhitespace();

                    _ingestValue(innerState);
                    _skipWhitespace();

                    if (_tryConsumeChar('}')) {
                        break;
                    }
                    else {
                        _consumeChar(',');
                        _skipWhitespace();
                    }
                }
            }

            _composer.end(std::move(innerState), outerState);
        }

        void _ingestArray(State & outerState) {
            State innerState(_composer.array(outerState));

            ++_pos; // We already know we have `[`
            _skipWhitespace();

            if (!_tryConsumeChar(']')) {
                while (true) {
                    _ingestValue(innerState);
                    _skipWhitespace();

                    if (_tryConsumeChar(']')) {
                        break;
                    }
                    else {
                        _consumeChar(',');
                        _skipWhitespace();
                    }
                }
            }

            _composer.end(std::move(innerState), outerState);
        }

        void _ingestString(State & state) {
            _composer.val(_consumeString(), state);
        }

        string_view _consumeString() {
            _stringBuffer.clear();

            ++_pos; // We already know we have `"`

            while (true) {
                if (_pos >= _end) {
                    throw DecodeError("Expected end quote", _pos - _start);
                }

                char c(*_pos);
                if (c == '"') {
                    ++_pos;
                    return _stringBuffer;
                }
                else if (c == '\\') {
                    ++_pos;
                    _stringBuffer.push_back(_consumeEscaped());
                }
                else if (std::isprint(c)) {
                    _stringBuffer.push_back(c);
                    ++_pos;
                }
                else {
                    throw DecodeError("Unknown string content", _pos - _start);
                }
            }
        }

        char _consumeEscaped() {
            if (_pos >= _end) {
                throw DecodeError("Expected escape sequence", _pos - _start);
            }

            switch (*_pos++) {
                case  '"': return  '"';
                case '\\': return '\\';
                case  '/': return  '/';
                case  'b': return '\b';
                case  'f': return '\f';
                case  'n': return '\n';
                case  'r': return '\r';
                case  't': return '\t';
                case  'u': return _consumeUnicode();
                default:
                    throw DecodeError("Unknown escape sequence", _pos - _start - 1);
            }
        }

        char _consumeUnicode() {
            if (_end - _pos < 4) {
                throw DecodeError("Expected four digits of unicode", _pos - _start);
            }

            uint32_t val;
            std::from_chars_result res(std::from_chars(_pos, _pos + 4, val, 16));
            if (res.ec != std::errc()) {
                throw DecodeError("Invalid unicode", _pos - _start);
            }

            // Is the high bit set?
            if (val & ~0x7Fu) {
                throw DecodeError("Non-ASCII unicode is unsupported", _pos - _start);
            }

            _pos += 4;

            return val;
        }

        bool _isInteger() const {
            // A precondition is that we know that the first character is either a digit or `-`
            const char * pos(_pos + 1);
            // Skip all remaining leading digits
            while (pos < _end && std::isdigit(*pos)) ++pos;
            // If that's it, we're an integer
            if (pos >= _end) {
                return true;
            }
            // If instead there is a decimal point...
            else if (*pos == '.') {
                ++pos;
                // Skip all zeroes
                while (pos < _end && *pos == '0') ++pos;
                // If there's a digit or an exponent, we must be a floater
                if (pos < _end && (std::isdigit(*pos) || *pos == 'e' || *pos == 'E')) {
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

        void _ingestNumber(State & state) {
            // Determine if integer or floater
            if (_isInteger()) {
                // Determine if signed
                if (*_pos == '-') {
                    _ingestInteger<true>(state);
                }
                else {
                    _ingestInteger<false>(state);
                }
            }
            else {
                _ingestFloater(state);
            }
        }

        template <bool isSigned>
        void _ingestInteger(State & state) {
            std::conditional_t<isSigned, int64_t, uint64_t> val;
            std::from_chars_result res(std::from_chars(_pos, _end, val));

            // There was an issue parsing
            if (res.ec != std::errc()) {
                // If too large, parse as a floater instead
                if (res.ec == std::errc::result_out_of_range) {
                    _ingestFloater(state);
                    return;
                }
                // Some other issue
                else {
                    throw DecodeError("Invalid integer", _pos - _start);
                }
            }

            _pos = res.ptr;

            // Skip trailing decimal zeroes
            if (_pos < _end && *_pos == '.') {
                ++_pos;

                // Check for dangling decimal point
                if (_pos >= _end || *_pos != '0') {
                    throw DecodeError("Dangling decimal point", _pos - _start);
                }
                ++_pos;

                while (_pos < _end && *_pos == '0') {
                    ++_pos;
                }
            }

            // If unsigned and the most significant bit is not set, we default to reporting it as signed
            if constexpr (!isSigned) {
                if (!(val & 0x8000000000000000u)) {
                    _composer.val(int64_t(val), state);
                    return;
                }
            }

            _composer.val(val, state);
        }

        void _ingestFloater(State & state) {
            double val;
            std::from_chars_result res(std::from_chars(_pos, _end, val));

            // There was an issue parsing, or a trailing decimal point
            if (res.ec != std::errc() || res.ptr[-1] == '.') {
                throw DecodeError("Invalid floater", _pos - _start);
            }

            _pos = res.ptr;
            _composer.val(val, state);
        }

    };

    inline DecodeError::DecodeError(const string & msg, size_t position) noexcept :
        std::runtime_error(msg),
        position(position)
    {}

    template <typename Composer, typename State>
    inline void decode(string_view json, Composer & composer, State initialState) {
        return _Decoder<Composer, State>(json, composer)(initialState);
    }

}
