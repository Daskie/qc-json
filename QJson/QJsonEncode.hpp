#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//
// Basic, lightweight JSON encoder.
//
// Example:
//      qjson::Encoder encoder();
//      encoder.key("Name").val("Roslin");
//      encoder.key("Favorite Books").array();
//      encoder.val("Dark Day");
//      ...
//      encoder.end();
//      std::string jsonString(encoder.finish());
//------------------------------------------------------------------------------

#include <string>
#include <vector>
#include <sstream>

namespace qjson {

#ifndef QJSON_COMMON
#define QJSON_COMMON

    // If anything goes wrong, this exception will be thrown
    struct Error : public std::exception {
        Error() = default;
        Error(const char * msg) : std::exception(msg) {}
        virtual ~Error() = default;
    };

#endif

    using std::string;
    using std::string_view;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    constexpr bool k_defaultCompact{false};
    constexpr int k_defaultIndentSize{4};

    // This will be thrown if anything goes wrong during the encoding process
    struct EncodeError : public Error {
        EncodeError(const char * msg) : Error(msg) {}
    };

    class Encoder {

        public:

        Encoder(bool compact = false, int indentSize = k_defaultIndentSize);
        Encoder(const Encoder & other) = delete;
        Encoder(Encoder && other);

        Encoder & operator=(const Encoder & other) = delete;
        Encoder & operator=(Encoder && other) = delete;

        Encoder & object(bool compact = false);

        Encoder & array(bool compact = false);

        Encoder & key(string_view k);

        Encoder & val(string_view v);
        Encoder & val(const string & v);
        Encoder & val(const char * v);
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
        Encoder & val(nullptr_t);
        template <typename T> Encoder & val(const T & v);

        Encoder & end();

        string finish();

        private:

        struct State { bool array, compact, content; };

        std::ostringstream m_ss;
        std::vector<State> m_state;
        int m_indentation;
        string_view m_indent;
        bool m_isKey;

        void m_start(bool compact, char bracket);

        void m_end();

        void m_putPrefix();

        void m_putIndentation();

        void m_checkKey() const;

        void m_encode(string_view val);
        void m_encode(int64_t val);
        void m_encode(uint64_t val);
        void m_encode(double val);
        void m_encode(bool val);
        void m_encode(nullptr_t);

    };

}

// Implement `qjson_encode` to enable Encoder::val for custom types
// Example:
//      void qjson_encode(qjson::Encoder & encoder, const std::pair<int, int> & v) {
//          encoder.val(v.first).val(v.second);
//      }
//

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    namespace detail {

        static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    }

    using namespace detail;

    inline Encoder::Encoder(bool compact, int indentSize) :
        m_ss(),
        m_state(),
        m_indentation(),
        m_indent("        ", indentSize),
        m_isKey()
    {
        if (indentSize < 0 || indentSize > 8) {
            throw EncodeError("Invalid indent size - must be in range [0, 8]");
        }

        m_ss << '{';
        m_state.push_back(State{false, compact, false});
    }

    inline Encoder::Encoder(Encoder && other) :
        m_ss(std::move(other.m_ss)),
        m_state(std::move(other.m_state)),
        m_indentation(other.m_indentation),
        m_indent(other.m_indent)
    {
        other.m_indentation = 0;
        other.m_indent = {};
    }

    inline Encoder & Encoder::object(bool compact) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_start(compact, '{');
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::array(bool compact) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_start(compact, '[');
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::key(string_view key) {
        if (m_isKey) {
            throw EncodeError("Expected value to follow key");
        }
        if (m_state.back().array) {
            throw EncodeError("Array elements must not have keys");
        }
        if (key.empty()) {
            throw EncodeError("Key must not be empty");
        }

        m_putPrefix();
        m_encode(key);
        m_ss << ": "sv;
        m_isKey = true;

        return *this;
    }

    inline Encoder & Encoder::val(string_view v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::val(const string & v) {
        return val(string_view(v));
    }

    inline Encoder & Encoder::val(const char * v) {
        return val(string_view(v));
    }

    inline Encoder & Encoder::val(int64_t v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::val(int32_t v) {
        return val(int64_t(v));
    }

    inline Encoder & Encoder::val(int16_t v) {
        return val(int64_t(v));
    }

    inline Encoder & Encoder::val(int8_t v) {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(uint64_t v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::val(uint32_t v) {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(uint16_t v) {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(uint8_t v) {
        return val(uint64_t(v));
    }

    inline Encoder & Encoder::val(double v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::val(float v) {
        return val(double(v));
    }

    inline Encoder & Encoder::val(bool v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Encoder & Encoder::val(nullptr_t) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(nullptr);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    template <typename T>
    inline Encoder & Encoder::val(const T & v) {
        ::qjson_encode(*this, v);
        return *this;
    }

    inline Encoder & Encoder::end() {
        if (m_state.size() <= 1) {
            throw EncodeError("No object or array to end");
        }
        if (m_isKey) {
            throw EncodeError("Expected value to follow key");
        }

        m_end();

        return *this;
    }

    inline string Encoder::finish() {
        if (!m_state.empty()) {
            throw EncodeError("Premature finish");
        }

        bool compact(m_state.front().compact);

        string str(m_ss.str());

        // Reset state
        m_ss.str("");
        m_ss.clear();
        m_ss << '{';
        m_state.push_back(State{false, compact, false});

        return str;
    }

    inline void Encoder::m_start(bool compact, char bracket) {
        m_ss << bracket;
        m_state.back().content = true;
        m_state.push_back(State{bracket == '[', m_state.back().compact || compact, false});
    }

    inline void Encoder::m_end() {
        const State & state(m_state.back());
        if (state.content) {
            if (state.compact) {
                m_ss << ' ';
            }
            else {
                m_ss << '\n';
                --m_indentation;
                m_putIndentation();
            }
        }
        m_ss << (state.array ? ']' : '}');
        m_state.pop_back();
    }

    inline void Encoder::m_putPrefix() {
        const State & state(m_state.back());
        if (state.content) {
            m_ss << ',';
        }
        if (state.compact) {
            m_ss << ' ';
        }
        else {
            m_ss << '\n';
            m_indentation += !state.content;
            m_putIndentation();
        }
    }

    inline void Encoder::m_putIndentation() {
        for (int i(0); i < m_indentation; ++i) {
            m_ss << m_indent;
        }
    }

    inline void Encoder::m_checkKey() const {
        if (!m_isKey && !m_state.back().array) {
            throw EncodeError("Object elements must have keys");
        }
    }

    inline void Encoder::m_encode(string_view v) {
        m_ss << '"';

        for (char c : v) {
            if (std::isprint(c)) {
                if (c == '"' || c == '\\') {
                    m_ss << '\\';
                }
                m_ss << c;
            }
            else {
                switch (c) {
                    case '\b': m_ss << '\\' << 'b'; break;
                    case '\f': m_ss << '\\' << 'f'; break;
                    case '\n': m_ss << '\\' << 'n'; break;
                    case '\r': m_ss << '\\' << 'r'; break;
                    case '\t': m_ss << '\\' << 't'; break;
                    default:
                        if (unsigned char(c) < 128) {
                            m_ss << '\\' << 'u' << '0' << '0' << k_hexChars[(c >> 4) & 0xF] << k_hexChars[c & 0xF];
                        }
                        else {
                            throw EncodeError("Non-ASCII unicode is unsupported");
                        }
                }
            }
        }

        m_ss << '"';
    }

    inline void Encoder::m_encode(int64_t v) {
        if (v < 0) {
            m_ss << '-';
            v = -v;
        }

        uint64_t uv(v); // Necessary for the case of UINT_MIN

        char buffer[20];
        char * end(buffer + 20);
        char * pos(end);

        do {
            uint64_t quotent(uv / 10), remainder(uv % 10); // Compiler should optimize this as one operation
            *--pos = char('0' + remainder);
            uv = quotent;
        } while (uv);

        m_ss << string_view(pos, end - pos);
    }

    inline void Encoder::m_encode(uint64_t v) {
        m_ss << "0x"sv;

        char buffer[16];
        char * end(buffer + 16);
        char * pos(end);

        do {
            *--pos = k_hexChars[v & 0xF];
            v >>= 4;
        } while (v);

        m_ss << string_view(pos, end - pos);
    }

    inline void Encoder::m_encode(double v) {
        if (std::isinf(v)) {
            throw EncodeError("The JSON standard does not support infinity");
        }
        if (std::isnan(v)) {
            throw EncodeError("The JSON standard does not support NaN");
        }

        char buffer[32];
        int len(std::snprintf(buffer, sizeof(buffer), "%#.17g", v));
        if (len < 0 || len >= sizeof(buffer)) {
            throw EncodeError("Error encoding float");
        }

        m_ss << string_view(buffer, len);
    }

    inline void Encoder::m_encode(bool v) {
        m_ss << (v ? "true"sv : "false"sv);
    }

    inline void Encoder::m_encode(nullptr_t) {
        m_ss << "null"sv;
    }

}
