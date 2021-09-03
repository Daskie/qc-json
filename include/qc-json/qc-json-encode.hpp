#pragma once

///
/// QC JSON 1.4.7
///
/// Austin Quick : 2019 - 2021
///
/// https://github.com/Daskie/qc-json
///
/// Encodes data into a JSON string
///
/// See the GitHub link above for more info and examples
///

#include <cctype>
#include <cstddef>

#include <algorithm>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
        explicit Error(const string & msg = {}) noexcept :
            std::runtime_error{msg}
        {}
    };
}

#endif // QC_JSON_COMMON

namespace qc::json
{
    ///
    /// This will be thrown if anything goes wrong during the encoding process
    ///
    struct EncodeError : Error
    {
        explicit EncodeError(const string & msg) noexcept;
    };

    ///
    /// Namespace provided to allow the user to `using namespace qc::json::tokens` to avoid the verbosity of fully
    ///   qualifying the tokens namespace
    ///
    inline namespace tokens
    {

        ///
        /// Pass with an object or array to specify its density
        ///
        /// TODO: Change this to enum class and add a `using enum Density` after once intellisense supports it. This is
        ///   to prevent `Density` from being implicitly cast to an integer, which can lead to issues
        ///
        enum Density
        {
            unspecified, /// Use that of the root or parent element
            multiline,   /// Elements are put on new lines
            uniline,     /// Elements are put on one line separated by spaces
            compact      /// No whitespace is used whatsoever
        };

        ///
        /// Stream this `object` variable to start a new object. Optionally specify a density
        ///
        /// This weird struct/operator()/variable setup allows for both ` << object ` and ` << object(density) `
        ///
        constexpr struct ObjectToken { Density density{unspecified}; constexpr ObjectToken operator()(const Density density_) const noexcept { return ObjectToken{density_}; } } object{};

        ///
        /// Stream this `array` variable to start a new array. Optionally specify a density
        ///
        /// This weird struct/operator()/variable setup allows for both ` << array ` and ` << array(density) `
        ///
        constexpr struct ArrayToken { Density density{unspecified}; constexpr ArrayToken operator()(const Density density_) const noexcept { return ArrayToken{density_}; } } array{};

        ///
        /// Stream this to end the current object or array
        ///
        constexpr struct EndToken {} end;

        ///
        /// Stream ` << binary(val) `, ` << octal(val) `, or ` << hex(val) ` to encode an unsigned integer in that base
        ///
        struct BinaryToken { uint64_t val{}; };
        struct  OctalToken { uint64_t val{}; };
        struct    HexToken { uint64_t val{}; };
        constexpr struct { constexpr BinaryToken operator()(const uint64_t v) const noexcept { return BinaryToken{v}; } } binary;
        constexpr struct { constexpr  OctalToken operator()(const uint64_t v) const noexcept { return  OctalToken{v}; } }  octal;
        constexpr struct { constexpr    HexToken operator()(const uint64_t v) const noexcept { return    HexToken{v}; } }    hex;
    }

    ///
    /// Instantiate this class to do the encoding
    ///
    class Encoder
    {
        public: //--------------------------------------------------------------

        ///
        /// Construct a new `Encoder`
        ///
        /// @param density the starting density for the JSON
        /// @param singleQuotes whether to use `'` instead of `"` for strings
        /// @param identifiers whether to encode all eligible keys as identifiers instead of strings
        ///
        Encoder(Density density = unspecified, bool singleQuotes = false, bool identifiers = false);

        Encoder(const Encoder & other) = delete;
        Encoder(Encoder && other) noexcept;

        Encoder & operator=(const Encoder & other) = delete;
        Encoder & operator=(Encoder && other) noexcept;

        ~Encoder() noexcept = default;

        ///
        /// Start a new object
        ///
        /// @return this
        ///
        Encoder & operator<<(ObjectToken v);

        ///
        /// Start a new array
        ///
        /// @return this
        ///
        Encoder & operator<<(ArrayToken v);

        ///
        /// End the current object or array
        ///
        /// @return this
        ///
        Encoder & operator<<(EndToken);

        ///
        /// Set the numeric base of the next number to be encoded. If this is anything other than decimal, the number
        ///   will be represented in raw, unsigned, two's-compliment form. Negative numbers are encoded as if they were
        /// positive. Floating point numbers are unaffected
        ///
        /// This flag is defaulted back to decimal after ANY value is streamed
        ///
        /// @param base the base for the next number
        /// @return this
        ///
        Encoder & operator<<(BinaryToken v);
        Encoder & operator<<(OctalToken v);
        Encoder & operator<<(HexToken v);

        ///
        /// Prevent the easy mistake of streaming the density directly
        //
        /// TODO: Remove once intellisense supports `using enum`, see above
        ///
        void operator<<(Density) = delete;

        ///
        /// Encode a value into the JSON
        ///
        /// @param v the value to encode
        /// @return this
        ///
        Encoder & operator<<(string_view v);
        Encoder & operator<<(const string & v);
        Encoder & operator<<(const char * v);
        Encoder & operator<<(char * v);
        Encoder & operator<<(char v);
        Encoder & operator<<(int64_t v);
        Encoder & operator<<(int32_t v);
        Encoder & operator<<(int16_t v);
        Encoder & operator<<(int8_t v);
        Encoder & operator<<(uint64_t v);
        Encoder & operator<<(uint32_t v);
        Encoder & operator<<(uint16_t v);
        Encoder & operator<<(uint8_t v);
        Encoder & operator<<(double v);
        Encoder & operator<<(float v);
        Encoder & operator<<(bool v);
        Encoder & operator<<(std::nullptr_t);

        ///
        /// Collapses the internal string stream into the encoded JSON string. This function resets the internal state
        ///   of the encoder to a "clean slate" such that it can be safely reused
        ///
        /// @return the encoded JSON string
        ///
        string finish();

        private: //-------------------------------------------------------------

        struct _State
        {
            bool array;
            bool content;
            Density density;
        };

        Density _baseDensity{unspecified};
        char _quote{'"'};
        bool _identifiers{false};
        std::ostringstream _oss{};
        std::vector<_State> _state{};
        int _indentation{0};
        bool _isKey{false};
        bool _isComplete{false};
        char _buffer[68]{};

        template <typename T> void _val(T v);

        void _key(string_view key);

        template <bool unchecked = false> void _prefix();

        void _indent();

        void _checkPre() const;

        void _encode(string_view val);
        void _encode(int64_t val);
        void _encode(uint64_t val);
        void _encode(BinaryToken v);
        void _encode(OctalToken v);
        void _encode(HexToken v);
        void _encode(double val);
        void _encode(bool val);
        void _encode(std::nullptr_t);
    };
}

///
/// Specialize `qc::json::Encoder & operator<<(qc::json::Encoder &, const Custom &)` to enable encoding for `Custom` type
///
/// Example:
///     qc::json::Encoder & operator<<(qc::json::Encoder & encoder, const std::pair<int, int> & v)
///     {
///         return encoder << array << v.first << v.second << end;
///     }
///

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json
{
    inline EncodeError::EncodeError(const string & msg) noexcept :
        Error{msg}
    {}

    inline Encoder::Encoder(const Density density, bool singleQuotes, bool preferIdentifiers) :
        _baseDensity{density},
        _quote{singleQuotes ? '\'' : '"'},
        _identifiers{preferIdentifiers}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _baseDensity{std::exchange(other._baseDensity, unspecified)},
        _quote{std::exchange(other._quote, '"')},
        _identifiers{std::exchange(other._identifiers, false)},
        _oss{std::move(other._oss)},
        _state{std::move(other._state)},
        _indentation{std::exchange(other._indentation, 0)},
        _isKey{std::exchange(other._isKey, false)},
        _isComplete{std::exchange(other._isComplete, false)}
    {}

    inline Encoder & Encoder::operator=(Encoder && other) noexcept
    {
        _baseDensity = std::exchange(other._baseDensity, unspecified);
        _quote = std::exchange(other._quote, '"');
        _identifiers = std::exchange(other._identifiers, false);
        _oss = std::move(other._oss);
        _state = std::move(other._state);
        _indentation = std::exchange(other._indentation, 0);
        _isKey = std::exchange(other._isKey, false);
        _isComplete = std::exchange(other._isComplete, false);

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ObjectToken v)
    {
        _checkPre();
        _prefix();
        _oss << '{';

        Density density{std::max(_baseDensity, v.density)};
        if (!_state.empty()) {
            _State & parentState{_state.back()};
            parentState.content = true;
            if (parentState.density > density) density = parentState.density;
        }
        _state.push_back(_State{false, false, density});
        _isKey = false;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ArrayToken v)
    {
        _checkPre();
        _prefix();
        _oss << '[';

        Density density{std::max(_baseDensity, v.density)};
        if (!_state.empty()) {
            _State & parentState{_state.back()};
            parentState.content = true;
            if (parentState.density > density) density = parentState.density;
        }
        _state.push_back(_State{true, false, density});
        _isKey = false;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const EndToken)
    {
        if (_state.empty()) {
            throw EncodeError{"No object or array to end"s};
        }
        if (_isKey) {
            throw EncodeError{"Cannot end object with a dangling key"s};
        }

        const _State & state{_state.back()};
        if (state.content) {
            switch (state.density) {
                case unspecified: [[fallthrough]];
                case multiline:
                    _oss << '\n';
                    --_indentation;
                    _indent();
                    break;
                case uniline:
                    _oss << ' ';
                    break;
                case compact:
                    break;
            }
        }
        _oss << (state.array ? ']' : '}');
        _state.pop_back();

        if (_state.empty()) {
            _isComplete = true;
        }

        return *this;
    }

    inline Encoder & Encoder::operator<<(const BinaryToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const OctalToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const HexToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const string_view v)
    {
        if (!_state.empty() && !_state.back().array && !_isKey) {
            _key(v);
        }
        else {
            _val(v);
        }

        return *this;
    }

    inline Encoder & Encoder::operator<<(const string & v)
    {
        return operator<<(string_view(v));
    }

    inline Encoder & Encoder::operator<<(const char * const v)
    {
        return operator<<(string_view(v));
    }

    inline Encoder & Encoder::operator<<(char * const v)
    {
        return operator<<(string_view(v));
    }

    inline Encoder & Encoder::operator<<(const char v)
    {
        return operator<<(string_view(&v, 1u));
    }

    inline Encoder & Encoder::operator<<(const int64_t v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const int32_t v)
    {
        return operator<<(int64_t(v));
    }

    inline Encoder & Encoder::operator<<(const int16_t v)
    {
        return operator<<(int64_t(v));
    }

    inline Encoder & Encoder::operator<<(const int8_t v)
    {
        return operator<<(int64_t(v));
    }

    inline Encoder & Encoder::operator<<(const uint64_t v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const uint32_t v)
    {
        return operator<<(uint64_t(v));
    }

    inline Encoder & Encoder::operator<<(const uint16_t v)
    {
        return operator<<(uint64_t(v));
    }

    inline Encoder & Encoder::operator<<(const uint8_t v)
    {
        return operator<<(uint64_t(v));
    }

    inline Encoder & Encoder::operator<<(const double v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const float v)
    {
        return operator<<(double(v));
    }

    inline Encoder & Encoder::operator<<(const bool v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const std::nullptr_t)
    {
        _val(nullptr);
        return *this;
    }

    inline string Encoder::finish()
    {
        if (!_isComplete) {
            throw EncodeError{"Cannot finish, JSON is not yet complete"s};
        }

        const string str{_oss.str()};

        // Reset state
        _oss.str(""s);
        _oss.clear();
        _isComplete = false;

        return str;
    }

    template <typename T>
    inline void Encoder::_val(const T v)
    {
        _checkPre();
        _prefix();
        _encode(v);

        if (_state.empty()) {
            _isComplete = true;
        }
        else {
            _state.back().content = true;
        }
        _isKey = false;
    }

    inline void Encoder::_key(const string_view key)
    {
        if (key.empty()) {
            throw EncodeError{"Key must not be empty"s};
        }

        bool identifier{false};
        if (_identifiers) {
            // Ensure the key has only alphanumeric and underscore characters
            if (std::find_if(key.cbegin(), key.cend(), [](const char c) { return !std::isalnum(uchar(c)) && c != '_'; }) == key.cend()) {
                identifier = true;
            }
        }

        _prefix<true>();

        if (identifier) {
            _oss << key;
        }
        else {
            _encode(key);
        }

        _oss << ':';
        if (_state.back().density < compact) {
            _oss << ' ';
        }

        _isKey = true;
    }

    template <bool unchecked>
    inline void Encoder::_prefix()
    {
        #pragma warning(suppress: 4127) // Condition intentionally constant when `unchecked` is true
        if (unchecked || !_isKey && !_state.empty()) {
            const _State & state{_state.back()};
            if (state.content) {
                _oss << ',';
            }
            switch (state.density) {
                case unspecified: [[fallthrough]];
                case multiline:
                    _oss << '\n';
                    _indentation += !state.content;
                    _indent();
                    break;
                case uniline:
                    _oss << ' ';
                    break;
                case compact:
                    break;
            }
        }
    }

    inline void Encoder::_indent()
    {
        for (int i{0}; i < _indentation; ++i) {
            _oss << "    "sv;
        }
    }

    inline void Encoder::_checkPre() const
    {
        if (_isComplete) {
            throw EncodeError{"Cannot add value to complete JSON"s};
        }
        if (!_isKey && !(_state.empty() || _state.back().array)) {
            throw EncodeError{"Cannot add value to object without first providing a key"s};
        }
    }

    inline void Encoder::_encode(const string_view v)
    {
        static constexpr char hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

        _oss << _quote;

        for (const char c : v) {
            if (std::isprint(uchar(c))) {
                if (c == _quote || c == '\\') _oss << '\\';
                _oss << c;
            }
            else {
                switch (c) {
                    case '\0': _oss << R"(\0)"; break;
                    case '\b': _oss << R"(\b)"; break;
                    case '\t': _oss << R"(\t)"; break;
                    case '\n': _oss << R"(\n)"; break;
                    case '\v': _oss << R"(\v)"; break;
                    case '\f': _oss << R"(\f)"; break;
                    case '\r': _oss << R"(\r)"; break;
                    default:
                        _oss << "\\x"sv << hexChars[(uchar(c) >> 4) & 0xF] << hexChars[uchar(c) & 0xF];
                }
            }
        }

        _oss << _quote;
    }

    inline void Encoder::_encode(const int64_t v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v)};
        _oss << string_view{_buffer, size_t(res.ptr - _buffer)};
    }

    inline void Encoder::_encode(const uint64_t v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v)};
        _oss << string_view{_buffer, size_t(res.ptr - _buffer)};
    }

    inline void Encoder::_encode(const BinaryToken v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v.val, 2)};
        _oss << "0b"sv << string_view{_buffer, size_t(res.ptr - _buffer)};
    }

    inline void Encoder::_encode(const OctalToken v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v.val, 8)};
        _oss << "0o"sv << string_view{_buffer, size_t(res.ptr - _buffer)};
    }

    inline void Encoder::_encode(const HexToken v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v.val, 16)};
        const size_t length{size_t(res.ptr - _buffer)};

        // Manually convert to uppercase hex because apparently `std::to_chars` doesn't have an option for that
        for (size_t i{0u}; i < length; ++i) {
            if (_buffer[i] >= 'a') {
                _buffer[i] -= ('a' - 'A');
            }
        }

        _oss << "0x"sv << string_view{_buffer, length};
    }

    inline void Encoder::_encode(const double v)
    {
        const std::to_chars_result res{std::to_chars(_buffer, _buffer + sizeof(_buffer), v)};
        _oss << string_view{_buffer, size_t(res.ptr - _buffer)};
    }

    inline void Encoder::_encode(const bool v)
    {
        _oss << (v ? "true"sv : "false"sv);
    }

    inline void Encoder::_encode(std::nullptr_t)
    {
        _oss << "null"sv;
    }
}
