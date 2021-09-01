#pragma once

///
/// QC JSON 1.4.0
/// Austin Quick
/// 2019 - 2021
/// https://github.com/Daskie/qc-json
///
/// Encodes data into a JSON string
///
/// See the GitHub link above for more info and examples
///

#include <cctype>
#include <cstddef>

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
    /// qualifying the tokens' namespace
    ///
    inline namespace tokens
    {
        ///
        /// Stream this to start a new object
        ///
        enum ObjectToken { object };

        ///
        /// Stream this to start a new array
        ///
        enum ArrayToken { array };

        ///
        /// Stream this to end the current object or array
        ///
        enum EndToken { end };

        ///
        /// Stream this to specify the density of the current object or array
        ///
        enum Density
        {
            multiline, /// Elements are put on new lines
            uniline,   /// Elements are put on one line separated by spaces
            compact    /// No whitespace is used whatsoever
        };
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
        ///
        Encoder(Density density = multiline);

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
        Encoder & operator<<(ObjectToken);

        ///
        /// Start a new array
        ///
        /// @return this
        ///
        Encoder & operator<<(ArrayToken);

        ///
        /// End the current object or array
        ///
        /// @return this
        ///
        Encoder & operator<<(EndToken);

        ///
        /// Set the density of the current object or array. Must be passed before any keys or elements
        ///
        /// Density can only go up, therefore specifying a lower density than is currently present will have no effect
        ///
        /// @param density the density to set the current object or array to
        /// @return this
        ///
        Encoder & operator<<(Density density);

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

        private: //-------------------------------------------------------------

        struct _State
        {
            bool array;
            bool content;
            Density density;
        };

        std::ostringstream _oss{};
        std::vector<_State> _state{};
        Density _baseDensity{multiline};
        int _indentation{0};
        bool _isKey{false};
        bool _isComplete{false};

        template <typename T> void _val(T v);

        void _key(string_view key);

        template <bool unchecked = false> void _prefix();

        void _indent();

        void _checkPre() const;

        void _encode(string_view val);
        void _encode(int64_t val);
        void _encode(uint64_t val);
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

    inline Encoder::Encoder(const Density density) :
        _baseDensity{density}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _oss{std::move(other._oss)},
        _state{std::move(other._state)},
        _baseDensity{std::exchange(other._baseDensity, multiline)},
        _indentation{std::exchange(other._indentation, 0)},
        _isKey{std::exchange(other._isKey, false)},
        _isComplete{std::exchange(other._isComplete, false)}
    {}

    inline Encoder & Encoder::operator=(Encoder && other) noexcept
    {
        _oss = std::move(other._oss);
        _state = std::move(other._state);
        _baseDensity = std::exchange(other._baseDensity, multiline);
        _indentation = std::exchange(other._indentation, 0);
        _isKey = std::exchange(other._isKey, false);
        _isComplete = std::exchange(other._isComplete, false);

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ObjectToken)
    {
        _checkPre();
        _prefix();
        _oss << '{';

        Density density{_baseDensity};
        if (!_state.empty()) {
            _state.back().content = true;
            density = _state.back().density;
        }
        _state.push_back(_State{false, false, density});
        _isKey = false;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ArrayToken)
    {
        _checkPre();
        _prefix();
        _oss << '[';

        Density density{_baseDensity};
        if (!_state.empty()) {
            _state.back().content = true;
            density = _state.back().density;
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

    inline Encoder & Encoder::operator<<(const Density density)
    {
        if (_state.empty()) {
            throw EncodeError{"Density must be given within an object or array"};
        }

        _State & state{_state.back()};

        if (_isKey || state.content) {
            throw EncodeError{"Density must be given at the start of the object or array"};
        }

        if (density > state.density) {
            state.density = density;
        }

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

        _prefix<true>();
        _encode(key);
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

        _oss << '"';

        for (const unsigned char c : v) {
            if (std::isprint(c)) {
                if (c == '"' || c == '\\') _oss << '\\';
                _oss << c;
            }
            else {
                switch (c) {
                    case '\b': _oss << R"(\b)"; break;
                    case '\f': _oss << R"(\f)"; break;
                    case '\n': _oss << R"(\n)"; break;
                    case '\r': _oss << R"(\r)"; break;
                    case '\t': _oss << R"(\t)"; break;
                    default:
                        if (c < 128) {
                            _oss << R"(\u00)" << hexChars[(c >> 4) & 0xF] << hexChars[c & 0xF];
                        }
                        else {
                            throw EncodeError{"Non-ASCII unicode is not supported"s};
                        }
                }
            }
        }

        _oss << '"';
    }

    inline void Encoder::_encode(const int64_t v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _oss << string_view{buffer, size_t(res.ptr - buffer)};
    }

    inline void Encoder::_encode(const uint64_t v)
    {
        char buffer[24];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _oss << string_view{buffer, size_t(res.ptr - buffer)};
    }

    inline void Encoder::_encode(const double v)
    {
        char buffer[32];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _oss << string_view{buffer, size_t(res.ptr - buffer)};
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
