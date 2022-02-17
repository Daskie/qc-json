#pragma once

///
/// QC JSON 2.0.2
///
/// Quick and clean JSON5 header library for C++20
///
/// Austin Quick : 2019 - 2022
///
/// https://github.com/daskie/qc-json
///
/// This standalone header provides a SAX interface for decoding JSON5
///
/// See the README for more info and examples!
///

#include <cctype>

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#ifndef QC_JSON_COMMON
#define QC_JSON_COMMON

namespace qc::json
{
    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    using uchar = unsigned char;

    ///
    /// Common exception type used for all `qc::json` exceptions
    ///
    struct Error : std::runtime_error
    {
        explicit Error(const string_view msg = {}) noexcept :
            std::runtime_error(msg.data())
        {}
    };

    ///
    /// Simple enum representing a json container type
    ///
    enum class Container : int8_t
    {
        none,
        object,
        array
    };

    ///
    /// Pass with an object or array to specify its density
    ///
    enum class Density : int8_t
    {
        unspecified = 0b000, /// Use that of the root or parent element
        multiline   = 0b001, /// Elements are put on new lines
        uniline     = 0b011, /// Elements are put on one line separated by spaces
        nospace     = 0b111  /// No whitespace is used whatsoever
    };
}

#endif // QC_JSON_COMMON

namespace qc::json
{
    ///
    /// This will be thrown if anything goes wrong during the decoding process
    ///
    struct DecodeError : Error
    {
        size_t position; /// The index into the string where the error occurred

        DecodeError(const string_view msg, size_t position) noexcept;
    };

    ///
    /// Decodes the JSON string
    ///
    /// A note on numbers:
    ///
    /// A number will be parsed and sent to the composer as either a `int64_t`, a `uint64_t`, or a `double`
    /// - `int64_t` if the number is an integer (a fractional component of zero is okay) and can fit in a `int64_t`
    /// - `uint64_t` if the number is a positive integer, can fit in a `uint64_t`, but cannot fit in a `int64_t`
    /// - `double` if the number has a non-zero fractional component, has an exponent, or is an integer that is too large to fit in a `int64_t` or `uint64_t`
    ///
    /// @param json the string to decode
    /// @param composer the contents of the JSON are decoded in order and passed to this to do something with
    /// @param initialState the initial state object to be passed to the composer
    ///
    template <typename Composer, typename State> void decode(string_view json, Composer & composer, State & initialState);
    template <typename Composer, typename State> void decode(string_view json, Composer & composer, State && initialState);

    ///
    /// An example composer whose operations are all no-ops
    ///
    /// Any custom composer must provide matching methods. This class may be extended for baseline no-ops
    ///
    template <typename State = nullptr_t>
    class DummyComposer
    {
        public: //--------------------------------------------------------------

        State object(State & /*outerState*/) { return State{}; }
        State array(State & /*outerState*/) { return State{}; }
        void end(const Density /*density*/, State && /*innerState*/, State & /*outerState*/) {}
        void key(const std::string_view /*key*/, State & /*state*/) {}
        void val(const std::string_view /*val*/, State & /*state*/) {}
        void val(const int64_t /*val*/, State & /*state*/) {}
        void val(const uint64_t /*val*/, State & /*state*/) {}
        void val(const double /*val*/, State & /*state*/) {}
        void val(const bool /*val*/, State & /*state*/) {}
        void val(const std::nullptr_t, State & /*state*/) {}
        void comment(const std::string_view /*comment*/, State & /*state*/) {}
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json
{
    inline Density & operator&=(Density & d1, const Density d2) noexcept
    {
        reinterpret_cast<uint8_t &>(d1) &= uint8_t(int8_t(d2));
        return d1;
    }

    inline Density & operator|=(Density & d1, const Density d2) noexcept
    {
        reinterpret_cast<uint8_t &>(d1) |= uint8_t(int8_t(d2));
        return d1;
    }

    // This functionality is wrapped in a class purely as a convenient way to keep track of state
    template <typename Composer, typename State>
    class _Decoder
    {
        public: //--------------------------------------------------------------

        _Decoder(const string_view str, Composer & composer) :
            _start{str.data()},
            _end{_start + str.length()},
            _pos{_start},
            _composer{composer}
        {}

        void operator()(State & initialState)
        {
            _skipSpaceAndIngestComments(initialState);
            _ingestValue(initialState);
            _skipSpaceAndIngestComments(initialState);

            // Allow trailing comma
            if (_tryConsumeChar(','))
            {
                _skipSpaceAndIngestComments(initialState);
            }

            if (_pos != _end)
            {
                throw DecodeError{"Extraneous content"sv, size_t(_pos - _start)};
            }
        }

        private: //-------------------------------------------------------------

        const char * const _start{nullptr};
        const char * const _end{nullptr};
        const char * _pos{nullptr};
        size_t _line{0u};
        size_t _column{0u};
        Composer & _composer;
        string _stringBuffer{};

        Density _skipWhitespace()
        {
            Density density{Density::nospace};

            while (_pos < _end)
            {
                if (std::isspace(uchar(*_pos)))
                {
                    if (*_pos == '\n') density &= Density::multiline;
                    else density &= Density::uniline;
                    ++_pos;
                }
                else
                {
                    break;
                }
            }

            return density;
        }

        Density _ingestLineComment(bool concat, State & state)
        {
            // We already know we have `//`
            _pos += 2;
            const char * commentStart{_pos};

            // Seek to end of line
            while (_pos < _end && *_pos != '\n')
            {
                ++_pos;
            }
            const char * commentEnd{_pos};

            // Trim space after `//`
            if (commentStart < commentEnd && *commentStart == ' ')
            {
                ++commentStart;
            }

            // Trim `\r` from end
            if (commentEnd[-1] == '\r')
            {
                --commentEnd;
            }

            // If this is a continuation, add it to the buffer
            if (concat)
            {
                _stringBuffer.push_back('\n');
                _stringBuffer.append(commentStart, commentEnd);
            }

            // Check for continuation on next line
            bool isContinuation{false};
            Density density{Density::nospace};

            // This comment ended with a newline (as opposed to the end of the json)
            if (_pos < _end)
            {
                // Skip newline
                ++_pos;
                density = Density::multiline;

                // There are no additional newlines
                if (_skipWhitespace() > Density::multiline)
                {
                    isContinuation = _pos + 2 < _end && _pos[0] == '/' && _pos[1] == '/';
                }
            }

            // There is more comment to come
            if (isContinuation)
            {
                if (!concat)
                {
                    _stringBuffer.assign(commentStart, commentEnd);
                }

                _ingestLineComment(true, state);

                if (!concat)
                {
                    _composer.comment(string_view{_stringBuffer}, state);
                }
            }
            // This is the end of the comment
            else
            {
                if (!concat)
                {
                    _composer.comment(string_view{commentStart, size_t(commentEnd - commentStart)}, state);
                }
            }

            return density;
        }

        void _ingestBlockComment(State & state)
        {
            // We already know we have `/*`
            _pos += 2;
            const char * commentStart{_pos};

            // Seek to `*/`
            while (_pos + 1 < _end && !(_pos[0] == '*' && _pos[1] == '/'))
            {
                ++_pos;
            }

            // If `*/` found
            if (_pos + 1 < _end)
            {
                const char * commentEnd{_pos};
                _pos += 2;

                // Trim space after `/*`
                if (*commentStart == ' ')
                {
                    ++commentStart;
                }

                // Trim space before `*/`
                if (commentEnd > commentStart && commentEnd[-1] == ' ')
                {
                    --commentEnd;
                }

                _composer.comment(string_view{commentStart, size_t(commentEnd - commentStart)}, state);
            }
            else
            {
                throw DecodeError{"Block comment is unterminated"sv, size_t(commentStart - 2 - _start)};
            }
        }

        Density _skipSpaceAndIngestComments(State & state)
        {
            // Skip whitespace
            Density density{_skipWhitespace()};

            while (true)
            {
                // Check for comment
                if (_pos + 1 < _end && _pos[0] == '/')
                {
                    // Ingest line comment
                    if (_pos[1] == '/')
                    {
                        density &= _ingestLineComment(false, state);
                        // `_ingesetLineComment` skips trailing whitespace already
                        continue;
                    }
                    // Ingest block comment
                    else if (_pos[1] == '*')
                    {
                        _ingestBlockComment(state);
                        // Skip whitespace
                        density &= _skipWhitespace();
                        continue;
                    }
                }

                break;
            }

            return density;
        }

        bool _tryConsumeChar(const char c)
        {
            if (_pos < _end && *_pos == c)
            {
                ++_pos;
                return true;
            }
            else
            {
                return false;
            }
        }

        void _consumeChar(const char c)
        {
            if (!_tryConsumeChar(c))
            {
                throw DecodeError{("Expected `"s += c) += '`', size_t(_pos - _start)};
            }
        }

        bool _tryConsumeChars(const string_view str)
        {
            if (size_t(_end - _pos) >= str.length())
            {
                for (size_t i{0u}; i < str.length(); ++i)
                {
                    if (_pos[i] != str[i])
                    {
                        return false;
                    }
                }
                _pos += str.length();
                return true;
            }
            else
            {
                return false;
            }
        }

        void _consumeChars(const string_view str)
        {
            if (!_tryConsumeChars(str))
            {
                throw DecodeError{("Expected `"s += str) += '`', size_t(_pos - _start)};
            }
        }

        void _ingestValue(State & state)
        {
            if (_pos >= _end)
            {
                throw DecodeError{"Expected value"sv, size_t(_pos - _start)};
            }

            char c{*_pos};

            // First check for typical easy values

            switch (c)
            {
                case '{':
                {
                    _ingestObject(state);
                    return;
                }
                case '[':
                {
                    _ingestArray(state);
                    return;
                }
                case '"':
                {
                    _ingestString('"', state);
                    return;
                }
                case '\'':
                {
                    _ingestString('\'', state);
                    return;
                }
            }

            // Now determine whether there is a +/- sign to narrow it down to numbers or not

            const int sign{(c == '+') - (c == '-')};
            if (sign)
            {
                // There was a sign, so we'll keep track of that and increment our position
                ++_pos;
                if (_pos >= _end)
                {
                    throw DecodeError{"Expected number"sv, size_t(_pos - _start)};
                }
                c = *_pos;
            }
            else
            {
                // There was no sign, so we can check the non-number keywords
                if (_tryConsumeChars("true"sv))
                {
                    _composer.val(true, state);
                    return;
                }
                else if (_tryConsumeChars("false"sv))
                {
                    _composer.val(false, state);
                    return;
                }
                else if (_tryConsumeChars("null"sv))
                {
                    _composer.val(nullptr, state);
                    return;
                }
            }

            // At this point, we know it is a number (or invalid)

            if (std::isdigit(uchar(c)) || (c == '.' && _pos + 1 < _end && std::isdigit(_pos[1])))
            {
                _ingestNumber(sign, state);
                return;
            }
            else if (_tryConsumeChars("nan"sv) || _tryConsumeChars("NaN"sv))
            {
                _composer.val(std::numeric_limits<double>::quiet_NaN(), state);
                return;
            }
            else if (_tryConsumeChars("inf"sv) || _tryConsumeChars("Infinity"sv))
            {
                _composer.val(sign < 0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity(), state);
                return;
            }

            // Nothing matched, throw an error
            throw DecodeError{"Unknown value"sv, size_t(_pos - _start)};
        }

        void _ingestObject(State & outerState)
        {
            State innerState{_composer.object(outerState)};

            ++_pos; // We already know we have `{`
            Density density{_skipSpaceAndIngestComments(innerState)};

            if (!_tryConsumeChar('}'))
            {
                while (true)
                {
                    // Parse key
                    if (_pos >= _end)
                    {
                        throw DecodeError{"Expected key"sv, size_t(_pos - _start)};
                    }
                    const char c{*_pos};
                    const string_view key{(c == '"' || c == '\'') ? _consumeString(c) : _consumeIdentifier()};
                    _composer.key(key, innerState);
                    density &= _skipSpaceAndIngestComments(innerState);

                    _consumeChar(':');
                    density &= _skipSpaceAndIngestComments(innerState);

                    _ingestValue(innerState);
                    density &= _skipSpaceAndIngestComments(innerState);

                    if (_tryConsumeChar('}'))
                    {
                        break;
                    }
                    else
                    {
                        _consumeChar(',');
                        density &= _skipSpaceAndIngestComments(innerState);

                        // Allow trailing comma
                        if (_tryConsumeChar('}'))
                        {
                            break;
                        }
                    }
                }
            }

            _composer.end(density, std::move(innerState), outerState);
        }

        void _ingestArray(State & outerState)
        {
            State innerState{_composer.array(outerState)};

            ++_pos; // We already know we have `[`
            Density density{_skipSpaceAndIngestComments(innerState)};

            if (!_tryConsumeChar(']'))
            {
                while (true)
                {
                    _ingestValue(innerState);
                    density &= _skipSpaceAndIngestComments(innerState);

                    if (_tryConsumeChar(']'))
                    {
                        break;
                    }
                    else
                    {
                        _consumeChar(',');
                        density &= _skipSpaceAndIngestComments(innerState);

                        // Allow trailing comma
                        if (_tryConsumeChar(']'))
                        {
                            break;
                        }
                    }
                }
            }

            _composer.end(density, std::move(innerState), outerState);
        }

        void _ingestString(const char quote, State & state)
        {
            _composer.val(_consumeString(quote), state);
        }

        string_view _consumeString(const char quote)
        {
            _stringBuffer.clear();

            ++_pos; // We already know we have `"` or `'`

            while (true)
            {
                if (_pos >= _end)
                {
                    throw DecodeError{"Expected end quote"sv, size_t(_pos - _start)};
                }

                const char c{*_pos};
                if (c == quote)
                {
                    ++_pos;
                    return _stringBuffer;
                }
                else if (c == '\\')
                {
                    ++_pos;

                    // Check for escaped newline
                    if (*_pos == '\n')
                    {
                        ++_pos;
                    }
                    else if (*_pos == '\r' && _pos + 1 < _end && _pos[1] == '\n')
                    {
                        _pos += 2;
                    }
                    else
                    {
                        _stringBuffer.push_back(_consumeEscaped());
                    }
                }
                else if (std::isprint(uchar(c)))
                {
                    _stringBuffer.push_back(c);
                    ++_pos;
                }
                else
                {
                    throw DecodeError{"Invalid string content"sv, size_t(_pos - _start)};
                }
            }
        }

        char _consumeEscaped()
        {
            if (_pos >= _end)
            {
                throw DecodeError{"Expected escape sequence"sv, size_t(_pos - _start)};
            }

            const char c{*_pos};
            ++_pos;

            switch (c)
            {
                case '0': return '\0';
                case 'b': return '\b';
                case 't': return '\t';
                case 'n': return '\n';
                case 'v': return '\v';
                case 'f': return '\f';
                case 'r': return '\r';
                case 'x': return _consumeCodePoint(2);
                case 'u': return _consumeCodePoint(4);
                case 'U': return _consumeCodePoint(8);
                default:
                    if (std::isprint(uchar(c)))
                    {
                        return c;
                    }
                    else
                    {
                        throw DecodeError{"Invalid escape sequence"sv, size_t(_pos - _start - 1)};
                    }
            }
        }

        char _consumeCodePoint(const int digits)
        {
            if (_end - _pos < digits)
            {
                throw DecodeError{("Expected "s += std::to_string(digits)) += " code point digits"sv, size_t(_pos - _start)};
            }

            uint32_t val;
            const std::from_chars_result res{std::from_chars(_pos, _pos + digits, val, 16)};
            if (res.ec != std::errc{})
            {
                throw DecodeError{"Invalid code point"sv, size_t(_pos - _start)};
            }

            _pos += digits;

            return char(val);
        }

        string_view _consumeIdentifier()
        {
            _stringBuffer.clear();

            // Ensure identifier is at least one character long
            char c{*_pos};
            if (std::isalnum(uchar(c)) || c == '_')
            {
                _stringBuffer.push_back(c);
                ++_pos;
            }
            else
            {
                throw DecodeError{"Expected identifier"sv, size_t(_pos - _start)};
            }

            while (true)
            {
                if (_pos >= _end)
                {
                    return _stringBuffer;
                }

                c = *_pos;
                if (std::isalnum(uchar(c)) || c == '_')
                {
                    _stringBuffer.push_back(c);
                    ++_pos;
                }
                else
                {
                    return _stringBuffer;
                }
            }
        }

        // Returns the string length of the number, including trailing decimal point & zeroes, or `0` if it's not an integer
        size_t _isInteger() const
        {
            const char * pos{_pos};
            // Skip all leading digits
            while (pos < _end && std::isdigit(uchar(*pos))) ++pos;
            // If that's it, we're an integer
            if (pos >= _end)
            {
                return pos - _pos;
            }
            // If instead there is a decimal point...
            else if (*pos == '.')
            {
                ++pos;
                // Skip all zeroes
                while (pos < _end && *pos == '0') ++pos;
                // If there's a digit or an exponent, we must be a floater
                if (pos < _end && (std::isdigit(uchar(*pos)) || *pos == 'e' || *pos == 'E'))
                {
                    return 0;
                }
                // Otherwise, we're an integer
                else
                {
                    return pos - _pos;
                }
            }
            // If instead there is an exponent, we must be a floater
            else if (*pos == 'e' || *pos == 'E')
            {
                return 0;
            }
            // Otherwise, that's the end of the number, and we're an integer
            else
            {
                return pos - _pos;
            }
        }

        void _ingestNumber(const int sign, State & state)
        {
            // Check if hex/octal/binary
            if (*_pos == '0' && _pos + 1 < _end)
            {
                int base{0};
                switch (_pos[1])
                {
                    case 'x': case 'X': base = 16; break;
                    case 'o': case 'O': base =  8; break;
                    case 'b': case 'B': base =  2; break;
                }

                if (base)
                {
                    if (sign)
                    {
                        throw DecodeError{"Hex, octal, and binary numbers must not be signed"sv, size_t(_pos - _start)};
                    }
                    _pos += 2;
                    _ingestHexOctalBinary(base, state);
                    return;
                }
            }

            // Determine if integer or floater
            if (size_t length{_isInteger()}; length)
            {
                if (sign < 0)
                {
                    _ingestInteger<true>(length, state);
                }
                else
                {
                    _ingestInteger<false>(length, state);
                }
            }
            else
            {
                _ingestFloater(sign < 0, state);
            }
        }

        void _ingestHexOctalBinary(const int base, State & state)
        {
            uint64_t val;
            const std::from_chars_result res{std::from_chars(_pos, _end, val, base)};

            // There was an issue parsing
            if (res.ec != std::errc{})
            {
                throw DecodeError{base == 2 ? "Invalid binary"sv : base == 8 ? "Invalid octal"sv : "Invalid hex"sv, size_t(_pos - _start)};
            }

            _pos = res.ptr;

            _composer.val(val, state);
        }

        template <bool negative>
        void _ingestInteger(const size_t length, State & state)
        {
            std::conditional_t<negative, int64_t, uint64_t> val;

            // Edge case that `.0` should evaluate to the integer `0`
            if (*_pos == '.')
            {
                val = 0;
            }
            else
            {
                const std::from_chars_result res{std::from_chars(_pos - negative, _end, val)};

                // There was an issue parsing
                if (res.ec != std::errc{})
                {
                    // If too large, parse as a floater instead
                    if (res.ec == std::errc::result_out_of_range)
                    {
                        _ingestFloater(negative, state);
                        return;
                    }
                    // Some other issue
                    else
                    {
                        throw DecodeError{"Invalid integer"sv, size_t(_pos - _start)};
                    }
                }
            }

            _pos += length;

            // If unsigned and the most significant bit is not set, we default to reporting it as signed
            if constexpr (!negative)
            {
                if (!(val & 0x8000000000000000u))
                {
                    _composer.val(int64_t(val), state);
                    return;
                }
            }

            _composer.val(val, state);
        }

        void _ingestFloater(const bool negative, State & state)
        {
            double val;
            const std::from_chars_result res{std::from_chars(_pos - negative, _end, val)};

            // There was an issue parsing
            if (res.ec != std::errc{})
            {
                throw DecodeError{"Invalid floater"sv, size_t(_pos - _start)};
            }

            _pos = res.ptr;
            _composer.val(val, state);
        }
    };

    inline DecodeError::DecodeError(const string_view msg, const size_t position) noexcept :
        Error{msg},
        position{position}
    {}

    template <typename Composer, typename State> concept _ComposerHasObjectMethod = requires (Composer composer, State state) { State{composer.object(state)}; };
    template <typename Composer, typename State> concept _ComposerHasArrayMethod = requires (Composer composer, State state) { State{composer.array(state)}; };
    template <typename Composer, typename State> concept _ComposerHasEndMethod = requires (Composer composer, const Density density, State innerState, State outerState) { composer.end(density, std::move(innerState), outerState); };
    template <typename Composer, typename State> concept _ComposerHasKeyMethod = requires (Composer composer, const std::string_view key, State state) { composer.key(key, state); };
    template <typename Composer, typename State> concept _ComposerHasStringValMethod = requires (Composer composer, const std::string_view val, State state) { composer.val(val, state); };
    template <typename Composer, typename State> concept _ComposerHasSignedIntegerValMethod = requires (Composer composer, const int64_t val, State state) { composer.val(val, state); };
    template <typename Composer, typename State> concept _ComposerHasUnsignedIntegerValMethod = requires (Composer composer, const uint64_t val, State state) { composer.val(val, state); };
    template <typename Composer, typename State> concept _ComposerHasFloaterValMethod = requires (Composer composer, const double val, State state) { composer.val(val, state); };
    template <typename Composer, typename State> concept _ComposerHasBooleanValMethod = requires (Composer composer, const bool val, State state) { composer.val(val, state); };
    template <typename Composer, typename State> concept _ComposerHasNullValMethod = requires (Composer composer, State state) { composer.val(nullptr, state); };
    template <typename Composer, typename State> concept _ComposerHasCommentMethod = requires (Composer composer, const string_view comment, State state) { composer.comment(comment, state); };

    template <typename Composer, typename State>
    inline void decode(string_view json, Composer & composer, State & initialState)
    {
        // Much more understandable compile errors than just letting the template code fly
        static_assert(_ComposerHasObjectMethod<Composer, State>);
        static_assert(_ComposerHasArrayMethod<Composer, State>);
        static_assert(_ComposerHasEndMethod<Composer, State>);
        static_assert(_ComposerHasKeyMethod<Composer, State>);
        static_assert(_ComposerHasStringValMethod<Composer, State>);
        static_assert(_ComposerHasSignedIntegerValMethod<Composer, State>);
        static_assert(_ComposerHasUnsignedIntegerValMethod<Composer, State>);
        static_assert(_ComposerHasFloaterValMethod<Composer, State>);
        static_assert(_ComposerHasBooleanValMethod<Composer, State>);
        static_assert(_ComposerHasNullValMethod<Composer, State>);
        static_assert(_ComposerHasCommentMethod<Composer, State>);

        return _Decoder<Composer, State>{json, composer}(initialState);
    }

    template <typename Composer, typename State>
    inline void decode(string_view json, Composer & composer, State && initialState)
    {
        return decode(json, composer, initialState);
    }
}
