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
/// This standalone header provides a SAX interface for encoding JSON5
///
/// See the README for more info and examples!
///

#include <cctype>
#include <cstddef>

#include <charconv>
#include <stdexcept>
#include <string>
#include <type_traits>
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
    /// This will be thrown if anything goes wrong during the encoding process
    ///
    struct EncodeError : Error
    {
        explicit EncodeError(const string_view msg) noexcept;
    };

    // This weird struct/operator()/variable setup allows for both ` << object ` and ` << object(density) `
    struct _ObjectToken { Density density{Density::unspecified}; constexpr _ObjectToken operator()(Density density_) const noexcept { return _ObjectToken{density_}; } };

    // This weird struct/operator()/variable setup allows for both ` << array ` and ` << array(density) `
    struct _ArrayToken { Density density{Density::unspecified}; constexpr _ArrayToken operator()(Density density_) const noexcept { return _ArrayToken{density_}; } };

    struct _EndToken {};

    struct _BinaryToken { uint64_t val{}; };
    struct _OctalToken { uint64_t val{}; };
    struct _HexToken { uint64_t val{}; };

    struct _CommentToken { string_view comment{}; };

    ///
    /// Namespace provided to allow the user to `using namespace qc::json::tokens` to avoid the verbosity of fully
    /// qualifying the tokens namespace
    ///
    inline namespace tokens
    {
        ///
        /// Stream this `object` variable to start a new object. Optionally specify a density
        ///
        constexpr _ObjectToken object{};

        ///
        /// Stream this `array` variable to start a new array. Optionally specify a density
        ///
        constexpr _ArrayToken array{};

        ///
        /// Stream this to end the current object or array
        ///
        constexpr _EndToken end{};

        ///
        /// Stream ` << binary(val) `, ` << octal(val) `, or ` << hex(val) ` to encode an unsigned integer in that base
        ///
        constexpr struct { constexpr _BinaryToken operator()(uint64_t v) const noexcept { return _BinaryToken{v}; } } binary{};
        constexpr struct { constexpr  _OctalToken operator()(uint64_t v) const noexcept { return  _OctalToken{v}; } }  octal{};
        constexpr struct { constexpr    _HexToken operator()(uint64_t v) const noexcept { return    _HexToken{v}; } }    hex{};

        ///
        /// Stream ` << comment(str) ` to encode a comment
        ///
        constexpr struct { constexpr _CommentToken operator()(string_view str) const noexcept { return _CommentToken{str}; } } comment{};
    }

    ///
    /// Instantiate this class to do the encoding
    ///
    class Encoder
    {
        public: //--------------------------------------------------------------

        ///
        /// Construct a new `Encoder` with the given options
        ///
        /// @param density the starting density for the JSON
        /// @param indentSpaces the number of spaces to insert per level of indentation
        /// @param singleQuotes whether to use `'` instead of `"` for strings
        /// @param identifiers whether to encode all eligible keys as identifiers instead of strings
        ///
        Encoder(Density density = Density::unspecified, size_t indentSpaces = 4u, bool singleQuotes = false, bool identifiers = false);

        Encoder(const Encoder &) = delete;

        ///
        /// Move constructor
        ///
        /// @param other is left in a valid but unspecified state
        /// @return this
        ///
        Encoder(Encoder && other) noexcept;

        Encoder & operator=(const Encoder &) = delete;

        ///
        /// Move assignment operator
        ///
        /// @param other is left in a valid but unspecified state
        /// @return this
        ///
        Encoder & operator=(Encoder && other) noexcept;

        ~Encoder() noexcept = default;

        ///
        /// Start a new object
        ///
        /// @return this
        ///
        Encoder & operator<<(_ObjectToken v);

        ///
        /// Start a new array
        ///
        /// @return this
        ///
        Encoder & operator<<(_ArrayToken v);

        ///
        /// End the current object or array
        ///
        /// @return this
        ///
        Encoder & operator<<(_EndToken);

        ///
        /// Set the numeric base of the next number to be encoded. If this is anything other than decimal, the number
        /// will be represented in raw, unsigned, two's-compliment form. Negative numbers are encoded as if they were
        /// positive. Floating point numbers are unaffected
        ///
        /// This flag is defaulted back to decimal after ANY value is streamed
        ///
        /// @param base the base for the next number
        /// @return this
        ///
        Encoder & operator<<(_BinaryToken v);
        Encoder & operator<<(_OctalToken v);
        Encoder & operator<<(_HexToken v);

        ///
        /// Insert a comment. Comments always logically precede a value. Comments will be in line form (`// ...`) in
        /// multiline contexts, block form (`/* ... */`) in uniline contexts, and nospace block form (`/*...*/`) in
        /// nospace contexts
        ///
        /// @param v the comment
        /// @return this
        ///
        Encoder & operator<<(_CommentToken v);

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
        /// of the encoder to a "clean slate" such that it can be safely reused
        ///
        /// @return the encoded JSON string
        ///
        string finish();

        ///
        /// @return the current container
        ///
        Container container() const noexcept;

        ///
        /// @return the current density
        ///
        Density density() const noexcept;

        private: //-------------------------------------------------------------

        enum class _Element { none, key, val, start, comment };

        // Using deltas allows us to start with an empty scope vector without needing a bunch of special root-case logic
        struct _ScopeDelta
        {
            int8_t containerDelta;
            int8_t densityDelta;
        };

        Density _baseDensity;
        size_t _indentSpaces;
        char _quote;
        bool _useIdentifiers;

        std::string _str{};
        std::vector<_ScopeDelta> _scopeDeltas{};
        Container _container{Container::none};
        Density _density{_baseDensity};
        size_t _indentation{0u};
        _Element _prevElement{_Element::none};
        bool _isContent{false};
        bool _isKey{false};

        void _start(Container container, Density density);

        template <typename T> void _val(T v);

        void _key(string_view key);

        void _prefix();

        void _indent();

        void _putSpace();

        void _encode(string_view val);
        void _encode(int64_t val);
        void _encode(uint64_t val);
        void _encode(_BinaryToken v);
        void _encode(_OctalToken v);
        void _encode(_HexToken v);
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

    inline Encoder::Encoder(const Density density, const size_t indentSpaces, bool singleQuotes, bool preferIdentifiers) :
        _baseDensity{density},
        _indentSpaces{indentSpaces},
        _quote{singleQuotes ? '\'' : '"'},
        _useIdentifiers{preferIdentifiers}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _baseDensity{other._baseDensity},
        _indentSpaces{other._indentSpaces},
        _quote{other._quote},
        _useIdentifiers{other._useIdentifiers},
        _str{std::move(other._str)},
        _scopeDeltas{std::move(other._scopeDeltas)},
        _container{std::exchange(other._container, Container::none)},
        _density{std::exchange(other._density, other._baseDensity)},
        _indentation{std::exchange(other._indentation, 0u)},
        _prevElement{std::exchange(other._prevElement, _Element::none)},
        _isContent{std::exchange(other._isContent, false)},
        _isKey{std::exchange(other._isKey, false)}
    {}

    inline Encoder & Encoder::operator=(Encoder && other) noexcept
    {
        _baseDensity = other._baseDensity;
        _indentSpaces = other._indentSpaces;
        _quote = other._quote;
        _useIdentifiers = other._useIdentifiers;
        _str = std::move(other._str);
        _scopeDeltas = std::move(other._scopeDeltas);
        _container = std::exchange(other._container, Container::none);
        _density = std::exchange(other._density, other._baseDensity);
        _indentation = std::exchange(other._indentation, 0u);
        _prevElement = std::exchange(other._prevElement, _Element::none);
        _isContent = std::exchange(other._isContent, false);
        _isKey = std::exchange(other._isKey, false);

        return *this;
    }

    inline Encoder & Encoder::operator<<(const _ObjectToken v)
    {
        _start(Container::object, v.density);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const _ArrayToken v)
    {
        _start(Container::array, v.density);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const _EndToken)
    {
        if (_container == Container::none)
        {
            throw EncodeError{"No object or array to end"sv};
        }
        if (_isKey)
        {
            throw EncodeError{"Cannot end object with a dangling key"sv};
        }

        _indentation -= _indentSpaces;
        if (_prevElement == _Element::val || _prevElement == _Element::comment)
        {
            _putSpace();
        }
        _str += (_container == Container::object ? '}' : ']');
        _container = Container(int8_t(_container) - _scopeDeltas.back().containerDelta);
        _density = Density(int8_t(_density) - _scopeDeltas.back().densityDelta);
        _scopeDeltas.pop_back();
        _prevElement = _Element::val;
        _isContent = true;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const _BinaryToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const _OctalToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const _HexToken v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::operator<<(const _CommentToken v)
    {
        Density commentDensity{_density};

        // Comment between key and value must be dense
        if (_isKey && commentDensity <= Density::multiline)
        {
            commentDensity = Density::uniline;
        }

        size_t lineLength{v.comment.size()};

        // Check for invalid characters and determine first line length
        for (size_t i{0u}; i < v.comment.size(); ++i)
        {
            const char c{v.comment[i]};
            if (!std::isprint(uchar(c)))
            {
                if (c == '\n')
                {
                    if (commentDensity <= Density::multiline)
                    {
                        lineLength = i;
                        break;
                    }
                }
                else
                {
                    throw EncodeError{("Comment has invalid character `\\x"s += std::to_string(int(uchar(c)))) += '`'};
                }
            }
        }

        _prefix();

        _prevElement = _Element::comment;

        // Line comment
        if (commentDensity <= Density::multiline)
        {
            _str += "// "sv;
            _str += v.comment.substr(0u, lineLength);

            // Simply recurse to handle the remaining lines
            if (lineLength < v.comment.size())
            {
                operator<<(_CommentToken{v.comment.substr(lineLength + 1u)});
            }
        }
        // Block comment
        else
        {
            // Ensure block comment does not contain `*/`
            if (v.comment.find("*/"sv) != string_view::npos)
            {
                throw EncodeError{"Block comment must not contain `*/`"sv};
            }

            if (commentDensity == Density::uniline)
            {
                _str += "/* "sv;
                _str += v.comment;
                _str += " */"sv;
            }
            else
            {
                _str += "/*"sv;
                _str += v.comment;
                _str += "*/"sv;
            }
        }

        return *this;
    }

    inline Encoder & Encoder::operator<<(const string_view v)
    {
        if (_container == Container::object && !_isKey)
        {
            _key(v);
        }
        else
        {
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
        if (_container != Container::none || !_isContent)
        {
            throw EncodeError{"Cannot finish, JSON is not yet complete"sv};
        }

        const string str{std::move(_str)};

        // Reset state
        _str.clear();
        _prevElement = _Element::none;
        _isContent = false;

        return str;
    }

    inline Container Encoder::container() const noexcept
    {
        return _container;
    }

    inline Density Encoder::density() const noexcept
    {
        return _density;
    }

    inline void Encoder::_start(const Container container, const Density density)
    {
        if (_container == Container::none && _isContent)
        {
            throw EncodeError{"Cannot add to complete JSON"sv};
        }
        if (_container == Container::object && !_isKey)
        {
            throw EncodeError{"Cannot add to object without first providing a key"sv};
        }

        _prefix();
        _str += container == Container::object ? '{' : '[';

        const int8_t containerDelta{int8_t(int8_t(container) - int8_t(_container))};
        const Density newDensity{density > _density ? density : _density};
        const int8_t densityDelta{int8_t(int8_t(newDensity) - int8_t(_density))};
        _scopeDeltas.push_back(_ScopeDelta{containerDelta, densityDelta});
        _container = container;
        _density = newDensity;
        _indentation += _indentSpaces;
        _prevElement = _Element::start;
        _isKey = false;
    }

    template <typename T>
    inline void Encoder::_val(const T v)
    {
        if (_container == Container::none && _isContent)
        {
            throw EncodeError{"Cannot add to complete JSON"sv};
        }
        if (_container == Container::object && !_isKey)
        {
            throw EncodeError{"Cannot add to object without first providing a key"sv};
        }

        _prefix();
        _encode(v);

        _prevElement = _Element::val;
        _isContent = true;
        _isKey = false;
    }

    inline void Encoder::_key(const string_view key)
    {
        bool identifier{false};
        if (_useIdentifiers)
        {
            if (key.empty())
            {
                throw EncodeError{"Identifier must not be empty"sv};
            }

            // Ensure the key has only alphanumeric and underscore characters
            identifier = true;
            for (const char c: key)
            {
                if (!std::isalnum(uchar(c)) && c != '_')
                {
                    identifier = false;
                    break;
                }
            }
        }

        _prefix();
        if (identifier)
        {
            _str += key;
        }
        else
        {
            _encode(key);
        }
        _str += ':';

        _prevElement = _Element::key;
        _isKey = true;
    }

    inline void Encoder::_prefix()
    {
        if (_isKey)
        {
            if (_density < Density::nospace)
            {
                _str += ' ';
            }
        }
        else
        {
            switch (_prevElement)
            {
                case _Element::none: break;
                case _Element::key: break;
                case _Element::val: _str += ','; [[fallthrough]];
                case _Element::start: [[fallthrough]];
                case _Element::comment: _putSpace(); break;
            }
        }
    }

    inline void Encoder::_indent()
    {
        _str.append(_indentation, ' ');
    }

    inline void Encoder::_putSpace()
    {
        switch (_density)
        {
            case Density::unspecified: [[fallthrough]];
            case Density::multiline:
                _str += '\n';
                _indent();
                break;
            case Density::uniline:
                _str += ' ';
                break;
            case Density::nospace:
                break;
        }
    }

    inline void Encoder::_encode(const string_view v)
    {
        static constexpr char hexChars[16u]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

        _str += _quote;

        for (const char c : v)
        {
            if (std::isprint(uchar(c)))
            {
                if (c == _quote || c == '\\') _str += '\\';
                _str += c;
            }
            else
            {
                switch (c)
                {
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
        char buffer[24u];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const uint64_t v)
    {
        char buffer[24u];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const _BinaryToken v)
    {
        char buffer[66u];
        buffer[0] = '0';
        buffer[1] = 'b';
        const std::to_chars_result res{std::to_chars(buffer + 2, buffer + sizeof(buffer), v.val, 2)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const _OctalToken v)
    {
        char buffer[26u];
        buffer[0] = '0';
        buffer[1] = 'o';
        const std::to_chars_result res{std::to_chars(buffer + 2, buffer + sizeof(buffer), v.val, 8)};
        _str.append(buffer, size_t(res.ptr - buffer));
    }

    inline void Encoder::_encode(const _HexToken v)
    {
        // We're hand rolling this because `std::to_chars` doesn't support uppercase hex
        static constexpr char hexTable[16u]{
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', 'A', 'B',
            'C', 'D', 'E', 'F'};

        uint64_t val{v.val};
        char buffer[18u];
        size_t bufferI{sizeof(buffer)};

        if (val)
        {
            do
            {
                buffer[--bufferI] = hexTable[val & 0xFu];
                val >>= 4;
            } while (val);
        }
        else
        {
            buffer[--bufferI] = '0';
        }

        buffer[--bufferI] = 'x';
        buffer[--bufferI] = '0';

        _str.append(buffer + bufferI, sizeof(buffer) - bufferI);
    }

    inline void Encoder::_encode(const double v)
    {
        char buffer[24u];
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
