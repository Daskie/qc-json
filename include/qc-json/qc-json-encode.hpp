#pragma once

///
/// QC JSON 1.4.8
///
/// Austin Quick : 2019 - 2021
///
/// https://github.com/Daskie/qc-json
///
/// Encodes data into a JSON string
///
/// See the README for more info and examples
///

#include <cctype>
#include <cstddef>

#include <charconv>
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
        explicit Error(const string_view msg = {}) noexcept :
            std::runtime_error{msg.data()}
        {}
    };

    ///
    /// Pass with an object or array to specify its density
    ///
    /// TODO: Change this to enum class and add a `using enum Density` after once intellisense supports it. This is
    ///   to prevent `Density` from being implicitly cast to an integer, which can lead to issues
    ///
    enum class Density : int
    {
        unspecified = 0b000, /// Use that of the root or parent element
        multiline   = 0b001, /// Elements are put on new lines
        uniline     = 0b011, /// Elements are put on one line separated by spaces
        compact     = 0b111  /// No whitespace is used whatsoever
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
        explicit EncodeError(const string_view msg) noexcept;
    };

    ///
    /// Namespace provided to allow the user to `using namespace qc::json::tokens` to avoid the verbosity of fully
    ///   qualifying the tokens namespace
    ///
    inline namespace tokens
    {
        ///
        /// Stream this `object` variable to start a new object. Optionally specify a density
        ///
        /// This weird struct/operator()/variable setup allows for both ` << object ` and ` << object(density) `
        ///
        constexpr struct ObjectToken { Density density{Density::unspecified}; constexpr ObjectToken operator()(Density density_) const noexcept { return ObjectToken{density_}; } } object{};

        ///
        /// Stream this `array` variable to start a new array. Optionally specify a density
        ///
        /// This weird struct/operator()/variable setup allows for both ` << array ` and ` << array(density) `
        ///
        constexpr struct ArrayToken { Density density{Density::unspecified}; constexpr ArrayToken operator()(Density density_) const noexcept { return ArrayToken{density_}; } } array{};

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
        constexpr struct { constexpr BinaryToken operator()(uint64_t v) const noexcept { return BinaryToken{v}; } } binary;
        constexpr struct { constexpr  OctalToken operator()(uint64_t v) const noexcept { return  OctalToken{v}; } }  octal;
        constexpr struct { constexpr    HexToken operator()(uint64_t v) const noexcept { return    HexToken{v}; } }    hex;

        ///
        /// Stream ` << comment(str) ` to encode a comment
        ///
        struct CommentToken { string_view comment{}; };
        constexpr struct { constexpr CommentToken operator()(string_view str) const noexcept { return CommentToken{str}; } } comment;
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
        Encoder(Density density = Density::unspecified, bool singleQuotes = false, bool identifiers = false);

        Encoder(const Encoder &) = delete;

        ///
        /// Move constructor
        ///
        /// @param other is not valid after moved
        /// @return this
        ///
        Encoder(Encoder && other) noexcept;

        Encoder & operator=(const Encoder &) = delete;

        ///
        /// Move assignment operator
        ///
        /// @param other is not valid after moved
        /// @return this
        ///
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
        ///   positive. Floating point numbers are unaffected
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
        /// Insert a comment. Comments always logically precede a value. Comments will be in line form (`// ...`) in
        ///   multiline contexts, block form (`/* ... */`) in uniline contexts, and compact block form (`/*...*/`) in
        ///   compact contexts
        ///
        /// @param v the comment
        /// @return this
        ///
        Encoder & operator<<(CommentToken v);

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

        enum class _Container : int { none, object, array };

        enum class _Element { none, key, val, start, comment };

        // Using deltas allows us to start with an empty scope vector without needing a bunch of special root-case logic
        struct _ScopeDelta
        {
            int containerDelta;
            int densityDelta;
        };

        char _quote{'"'};
        bool _identifiers{false};
        std::string _str{};
        std::vector<_ScopeDelta> _scopeDeltas{};
        _Container _container{_Container::none};
        Density _density{Density::unspecified};
        int _indentation{0};
        _Element _prevElement{_Element::none};
        bool _isContent{false};

        void _start(_Container container, Density density);

        template <typename T> void _val(T v);

        void _key(string_view key);

        void _prefix();

        void _indent();

        void _putSpace();

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
    inline EncodeError::EncodeError(const string_view msg) noexcept :
        Error{msg}
    {}

    inline Encoder::Encoder(const Density density, bool singleQuotes, bool preferIdentifiers) :
        _quote{singleQuotes ? '\'' : '"'},
        _identifiers{preferIdentifiers},
        _density{density}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _quote{other._quote},
        _identifiers{other._identifiers},
        _str{std::move(other._str)},
        _scopeDeltas{std::move(other._scopeDeltas)},
        _container{other._container},
        _density{other._density},
        _indentation{other._indentation},
        _prevElement{other._prevElement},
        _isContent{other._isContent}
    {}

    inline Encoder & Encoder::operator=(Encoder && other) noexcept
    {
        _quote = other._quote;
        _identifiers = other._identifiers;
        _str = std::move(other._str);
        _scopeDeltas = std::move(other._scopeDeltas);
        _container = other._container;
        _density = other._density;
        _indentation = other._indentation;
        _prevElement = other._prevElement;
        _isContent = other._isContent;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ObjectToken v)
    {
        _start(_Container::object, v.density);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const ArrayToken v)
    {
        _start(_Container::array, v.density);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const EndToken)
    {
        if (_container == _Container::none) {
            throw EncodeError{"No object or array to end"sv};
        }
        if (_prevElement == _Element::key) {
            throw EncodeError{"Cannot end object with a dangling key"sv};
        }

        --_indentation;
        if (_prevElement == _Element::val || _prevElement == _Element::comment) {
            _putSpace();
        }
        _str += (_container == _Container::object ? '}' : ']');
        _container = _Container(int(_container) - _scopeDeltas.back().containerDelta);
        _density = Density(int(_density) - _scopeDeltas.back().densityDelta);
        _scopeDeltas.pop_back();
        _prevElement = _Element::val;
        _isContent = true;

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

    inline Encoder & Encoder::operator<<(const CommentToken v)
    {
        // Ensure comment does not come after key
        if (_prevElement == _Element::key) {
            throw EncodeError{"Comment can not come between key and value"sv};
        }

        size_t lineLength{v.comment.size()};

        // Check for invalid characters and determine first line length
        for (size_t i{0u}; i < v.comment.size(); ++i) {
            const char c{v.comment[i]};
            if (!std::isprint(uchar(c))) {
                if (c == '\n' && _density <= Density::multiline) {
                    lineLength = i;
                    break;
                }
                else {
                    throw EncodeError{("Comment has invalid character `\\x"s += std::to_string(int(uchar(c)))) += '`'};
                }
            }
        }

        _prefix();

        // Line comment
        if (_density <= Density::multiline) {
            _str += "// "sv;
            _str += v.comment.substr(0u, lineLength);
        }
        // Block comment
        else {
            // Ensure block comment does not contain `*/`
            if (v.comment.find("*/"sv) != string_view::npos) {
                throw EncodeError{"Block comment must not contain `*/`"sv};
            }

            if (_density == Density::uniline) {
                _str += "/* "sv;
                _str += v.comment;
                _str += " */"sv;
            }
            else {
                _str += "/*"sv;
                _str += v.comment;
                _str += "*/"sv;
            }
        }

        _prevElement = _Element::comment;

        // Simply recurse to handle the remaining lines
        if (lineLength < v.comment.size()) {
            operator<<(CommentToken{v.comment.substr(lineLength + 1)});
        }

        return *this;
    }

    inline Encoder & Encoder::operator<<(const string_view v)
    {
        if (_container == _Container::object && _prevElement != _Element::key) {
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
        if (_container != _Container::none || !_isContent) {
            throw EncodeError{"Cannot finish, JSON is not yet complete"sv};
        }

        const string str{std::move(_str)};

        // Reset state
        _str.clear();
        _prevElement = _Element::none;
        _isContent = false;

        return str;
    }

    inline void Encoder::_start(const _Container container, const Density density)
    {
        if (_container == _Container::none && _isContent) {
            throw EncodeError{"Cannot add to complete JSON"sv};
        }
        if (_container == _Container::object && _prevElement != _Element::key) {
            throw EncodeError{"Cannot add to object without first providing a key"sv};
        }

        _prefix();
        _str += container == _Container::object ? '{' : '[';

        const int containerDelta{int(container) - int(_container)};
        const Density newDensity{density > _density ? density : _density};
        const int densityDelta{int(newDensity) - int(_density)};
        _scopeDeltas.push_back(_ScopeDelta{containerDelta, densityDelta});
        _container = container;
        _density = newDensity;
        ++_indentation;
        _prevElement = _Element::start;
    }

    template <typename T>
    inline void Encoder::_val(const T v)
    {
        if (_container == _Container::none && _isContent) {
            throw EncodeError{"Cannot add to complete JSON"sv};
        }
        if (_container == _Container::object && _prevElement != _Element::key) {
            throw EncodeError{"Cannot add to object without first providing a key"sv};
        }

        _prefix();
        _encode(v);

        _prevElement = _Element::val;
        _isContent = true;
    }

    inline void Encoder::_key(const string_view key)
    {
        bool identifier{false};
        if (_identifiers) {
            if (key.empty()) {
                throw EncodeError{"Identifier must not be empty"sv};
            }

            // Ensure the key has only alphanumeric and underscore characters
            identifier = true;
            for (const char c : key) {
                if (!std::isalnum(uchar(c)) && c != '_') {
                    identifier = false;
                    break;
                }
            }
        }

        _prefix();
        if (identifier) {
            _str += key;
        }
        else {
            _encode(key);
        }
        _str += ':';

        _prevElement = _Element::key;
    }

    inline void Encoder::_prefix()
    {
        switch (_prevElement) {
            case _Element::none: break;
            case _Element::key: if (_density < Density::compact) _str += ' '; break;
            case _Element::val: _str += ','; [[fallthrough]];
            case _Element::start: [[fallthrough]];
            case _Element::comment: _putSpace(); break;
        }
    }

    inline void Encoder::_indent()
    {
        for (int i{0}; i < _indentation; ++i) {
            _str += "    "sv;
        }
    }

    inline void Encoder::_putSpace()
    {
        switch (_density) {
            case Density::unspecified: [[fallthrough]];
            case Density::multiline:
                _str += '\n';
                _indent();
                break;
            case Density::uniline:
                _str += ' ';
                break;
            case Density::compact:
                break;
        }
    }

    inline void Encoder::_encode(const string_view v)
    {
        static constexpr char hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

        _str += _quote;

        for (const char c : v) {
            if (std::isprint(uchar(c))) {
                if (c == _quote || c == '\\') _str += '\\';
                _str += c;
            }
            else {
                switch (c) {
                    case '\0': _str += R"(\0)"; break;
                    case '\b': _str += R"(\b)"; break;
                    case '\t': _str += R"(\t)"; break;
                    case '\n': _str += R"(\n)"; break;
                    case '\v': _str += R"(\v)"; break;
                    case '\f': _str += R"(\f)"; break;
                    case '\r': _str += R"(\r)"; break;
                    default:
                        _str += "\\x"sv;
                        _str += hexChars[(uchar(c) >> 4) & 0xF];
                        _str += hexChars[uchar(c) & 0xF];
                }
            }
        }

        _str += _quote;
    }

    inline void Encoder::_encode(const int64_t v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const uint64_t v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const BinaryToken v)
    {
        char buffer[64];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v.val, 2)};
        _str += "0b"sv;
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const OctalToken v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v.val, 8)};
        _str += "0o"sv;
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const HexToken v)
    {
        char buffer[16];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v.val, 16)};
        const size_t length{size_t(res.ptr - buffer)};

        // Manually convert to uppercase hex because apparently `std::to_chars` doesn't have an option for that
        for (size_t i{0u}; i < length; ++i) {
            if (buffer[i] >= 'a') {
                buffer[i] -= ('a' - 'A');
            }
        }

        _str += "0x"sv;
        _str.append(buffer, length);
    }

    inline void Encoder::_encode(const double v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const bool v)
    {
        _str += v ? "true"sv : "false"sv;
    }

    inline void Encoder::_encode(std::nullptr_t)
    {
        _str += "null"sv;
    }
}
