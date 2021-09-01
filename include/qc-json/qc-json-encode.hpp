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

    ///
    /// This will be thrown if anything goes wrong during the encoding process.
    ///
    struct EncodeError : Error
    {
        explicit EncodeError(const string & msg) noexcept;
    };

    ///
    /// Namespace provided to allow the user to `using namespace qc::json::tokens` to avoid the verbosity of fully
    /// qualifying the tokens' namespace.
    ///
    inline namespace tokens
    {
        constexpr enum class ObjectToken {} object{};

        constexpr enum class ArrayToken {} array{};

        constexpr enum class EndToken {} end{};

        constexpr enum class CompactToken {} compact{};
    }

    ///
    /// Instantiate this class to do the encoding.
    ///
    class Encoder
    {
        public:

        Encoder() = default;
        Encoder(CompactToken);

        Encoder(const Encoder & other) = delete;
        Encoder(Encoder && other) noexcept;

        Encoder & operator=(const Encoder & other) = delete;
        Encoder & operator=(Encoder && other) = delete;

        ~Encoder() = default;

        Encoder & operator<<(ObjectToken);
        Encoder & operator<<(ArrayToken);
        Encoder & operator<<(EndToken);
        Encoder & operator<<(CompactToken);
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

        string finish();

        private:

        struct _State { bool array, compact, content; };

        std::ostringstream _oss{};
        std::vector<_State> _state{};
        int _indentation{0};
        bool _compact{false};
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
///         return encoder.array(true).val(v.first).val(v.second).end();
///     }
///

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json
{
    inline EncodeError::EncodeError(const string & msg) noexcept :
        Error{msg}
    {}

    inline Encoder::Encoder(const CompactToken) :
        _compact{true}
    {}

    inline Encoder::Encoder(Encoder && other) noexcept :
        _oss{std::move(other._oss)},
        _state{std::move(other._state)},
        _indentation{std::exchange(other._indentation, 0)},
        _compact{std::exchange(other._compact, false)},
        _isKey{std::exchange(other._isKey, false)},
        _isComplete{std::exchange(other._isComplete, false)}
    {}

    inline Encoder & Encoder::operator<<(const ObjectToken)
    {
        _checkPre();
        _prefix();
        _oss << '{';

        bool compact_{_compact};
        if (!_state.empty()) {
            _state.back().content = true;
            compact_ = _state.back().compact;
        }
        _state.push_back(_State{false, compact_, false});
        _isKey = false;

        return *this;
    }

    inline Encoder & Encoder::operator<<(const ArrayToken)
    {
        _checkPre();
        _prefix();
        _oss << '[';

        bool compact_{_compact};
        if (!_state.empty()) {
            _state.back().content = true;
            compact_ = _state.back().compact;
        }
        _state.push_back(_State{true, compact_, false});
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

    inline Encoder & Encoder::operator<<(const CompactToken)
    {
        if (_state.empty()) {
            throw EncodeError{"Compact token must be given within an object or array"};
        }

        _State & state{_state.back()};

        if (_isKey || state.content) {
            throw EncodeError{"Compact token must be given at the start of the object or array"};
        }

        state.compact = true;

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
        _oss << ": "sv;
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
