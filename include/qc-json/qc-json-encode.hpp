#pragma once

///
/// QC Json 1.3.4
/// Austin Quick
/// 2019 - 2021
/// https://github.com/Daskie/qc-json
///
/// Encodes data into a JSON string.
///
/// See the GitHub link above for more info and examples.
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
    ///
    /// Common exception type used for all `qc::json` exceptions.
    ///
    struct Error : std::runtime_error
    {
        explicit Error(const std::string & msg = {}) noexcept :
            std::runtime_error{msg}
        {}
    };
}

#endif // QC_JSON_COMMON

namespace qc::json
{
    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    namespace config
    {
        constexpr bool defaultCompact{false};
    }

    ///
    /// This will be thrown if anything goes wrong during the encoding process.
    ///
    struct EncodeError : Error
    {
        explicit EncodeError(const string & msg) noexcept;
    };

    ///
    /// Instantiate this class to do the encoding.
    ///
    class Encoder
    {
        public:

        Encoder() = default;

        Encoder(const Encoder & other) = delete;
        Encoder(Encoder && other) noexcept;

        Encoder & operator=(const Encoder & other) = delete;
        Encoder & operator=(Encoder && other) = delete;

        ~Encoder() = default;

        Encoder & object(bool compact = config::defaultCompact);

        Encoder & array(bool compact = config::defaultCompact);

        Encoder & key(string_view k);

        Encoder & val(string_view v);
        Encoder & val(const string & v);
        Encoder & val(const char * v);
        Encoder & val(char * v);
        Encoder & val(char v);
        Encoder & val(int64_t v);
        Encoder & val(int32_t v);
        Encoder & val(int16_t v);
        Encoder & val(int8_t v);
        Encoder & val(uint64_t v);
        Encoder & val(uint32_t v);
        Encoder & val(uint16_t v);
        Encoder & val(uint8_t v);
        Encoder & val(double v);
        Encoder & val(float v);
        Encoder & val(bool v);
        Encoder & val(std::nullptr_t);
        template <typename T> Encoder & val(const T & v);

        Encoder & end();

        string finish();

        private:

        struct _State { bool array, compact, content; };

        std::ostringstream _oss;
        std::vector<_State> _state;
        int _indentation{0};
        bool _isKey{false};
        bool _isComplete{false};

        template <typename T> void _val(T v);

        void _prefix();

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
/// Specialize `qc_json_encode` to enable encoding of custom types
/// Example:
///      template <>
///      struct qc_json_encode<std::pair<int, int>>
///      {
///          void operator()(qc::json::Encoder & encoder, const std::pair<int, int> & v)
///          {
///              encoder.array(true).val(v.first).val(v.second).end();
///          }
///      };
///
template <typename T> struct qc_json_encode;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json
{
    inline EncodeError::EncodeError(const string & msg) noexcept :
        Error{msg}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _oss{std::move(other._oss)},
        _state{std::move(other._state)},
        _indentation{std::exchange(other._indentation, 0)},
        _isKey{std::exchange(other._isKey, false)},
        _isComplete{std::exchange(other._isComplete, false)}
    {}

    inline Encoder & Encoder::object(bool compact)
    {
        _checkPre();
        _prefix();
        _oss << '{';
        if (!_state.empty()) {
            _state.back().content = true;
            _isKey = false;
            compact = compact || _state.back().compact;
        }
        _state.push_back(_State{false, compact, false});

        return *this;
    }

    inline Encoder & Encoder::array(bool compact)
    {
        _checkPre();
        _prefix();
        _oss << '[';
        if (!_state.empty()) {
            _state.back().content = true;
            _isKey = false;
            compact = compact || _state.back().compact;
        }
        _state.push_back(_State{true, compact, false});

        return *this;
    }

    inline Encoder & Encoder::key(const string_view key)
    {
        if (_isKey) {
            throw EncodeError{"A key has already been given"s};
        }
        if (_state.empty() || _state.back().array) {
            throw EncodeError{"A key may only be givin within an object"s};
        }
        if (key.empty()) {
            throw EncodeError{"Key must not be empty"s};
        }

        _prefix();
        _encode(key);
        _oss << ": "sv;
        _isKey = true;

        return *this;
    }

    inline Encoder & Encoder::val(const string_view v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::val(const string & v)
    {
        return val(string_view(v));
    }

    inline Encoder & Encoder::val(const char * const v)
    {
        return val(string_view(v));
    }

    inline Encoder & Encoder::val(char * const v)
    {
        return val(string_view(v));
    }

    inline Encoder & Encoder::val(const char v)
    {
        return val(string_view(&v, 1u));
    }

    inline Encoder & Encoder::val(const int64_t v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::val(const int32_t v)
    {
        return val(int64_t(v));
    }

    inline Encoder & Encoder::val(const int16_t v)
    {
        return val(int64_t(v));
    }

    inline Encoder & Encoder::val(const int8_t v)
    {
        return val(int64_t(v));
    }

    inline Encoder & Encoder::val(const uint64_t v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::val(const uint32_t v)
    {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(const uint16_t v)
    {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(const uint8_t v)
    {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(const double v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::val(const float v)
    {
        return val(double(v));
    }

    inline Encoder & Encoder::val(const bool v)
    {
        _val(v);
        return *this;
    }

    inline Encoder & Encoder::val(const std::nullptr_t)
    {
        _val(nullptr);
        return *this;
    }

    template <typename T>
    inline Encoder & Encoder::val(const T & v)
    {
        qc_json_encode<T>()(*this, v);
        return *this;
    }

    inline Encoder & Encoder::end()
    {
        if (_state.empty()) {
            throw EncodeError{"No object or array to end"s};
        }
        if (_isKey) {
            throw EncodeError{"Cannot end object with a dangling key"s};
        }

        const _State & state{_state.back()};
        if (state.content) {
            if (state.compact) {
                _oss << ' ';
            }
            else {
                _oss << '\n';
                --_indentation;
                _indent();
            }
        }
        _oss << (state.array ? ']' : '}');
        _state.pop_back();

        if (_state.empty()) {
            _isComplete = true;
        }

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
            _isKey = false;
        }
    }

    inline void Encoder::_prefix()
    {
        if (!_isKey && !_state.empty()) {
            const _State & state{_state.back()};
            if (state.content) {
                _oss << ',';
            }
            if (state.compact) {
                _oss << ' ';
            }
            else {
                _oss << '\n';
                _indentation += !state.content;
                _indent();
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
#ifndef __GNUC__ // TODO: Update once GCC supports `std::to_chars`
        char buffer[32];
        const std::to_chars_result res{std::to_chars(buffer, buffer + sizeof(buffer), v)};
        _oss << string_view{buffer, size_t(res.ptr - buffer)};
#else
#pragma message("`std::charconv` not supported by compiler - floating point serialization quality may suffer")
        char buffer[32];
        const int len{sprintf(buffer, "%g", v)};
        _oss << string_view{buffer, size_t(len)};
#endif
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
