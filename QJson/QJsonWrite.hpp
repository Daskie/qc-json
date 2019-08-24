#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//
// Basic, lightweight JSON encoder.
//
// Example:
//      qjson::Writer writer();
//      writer.key("Name").val("Roslin");
//      writer.key("Favorite Books").array();
//      writer.val("Dark Day");
//      ...
//      writer.end();
//      std::string jsonString(writer.finish());
//------------------------------------------------------------------------------

#include <string>
#include <vector>
#include <sstream>

#ifndef QJSON_COMMON
#define QJSON_COMMON
namespace qjson {

    // If anything goes wrong, this exception will be thrown
    struct JsonError : public std::exception {
        JsonError() = default;
        JsonError(const char * msg) : std::exception(msg) {}
        virtual ~JsonError() = default;
    };

}
#endif

// Specialize `QJsonEncoder` to enable Writer::val for custom types.
// Must have a function call operator with signature `void operator()(qjson::Writer &, const T &)`
// Example specialization:
//
//      template <> struct QJsonEncoder<MyType> {
//          void operator()(qjson::Writer & writer, const MyType & v) {
//              writer.val(v.x).val(v.y);
//          }
//      };
//
template <typename T> struct QJsonEncoder;

namespace qjson {

    // This will be thrown if anything goes wrong during the encoding process
    struct JsonWriteError : public JsonError {
        JsonWriteError(const char * msg) : JsonError(msg) {}
    };

    class Writer {

        public:

        Writer(bool compact = false, int indentSize = 4);
        Writer(const Writer & other) = delete;
        Writer(Writer && other);

        Writer & operator=(const Writer & other) = delete;
        Writer & operator=(Writer && other) = delete;

        Writer & object(bool compact = false);

        Writer & array(bool compact = false);

        Writer & key(std::string_view k);

        Writer & val(std::string_view v);
        Writer & val(const std::string & v);
        Writer & val(const char * v);
        Writer & val(char v);
        Writer & val(int64_t v);
        Writer & val(int32_t v);
        Writer & val(int16_t v);
        Writer & val(int8_t v);
        Writer & val(uint64_t v);
        Writer & val(uint32_t v);
        Writer & val(uint16_t v);
        Writer & val(uint8_t v);
        Writer & val(double v);
        Writer & val(float v);
        Writer & val(bool v);
        Writer & val(nullptr_t);
        template <typename T> Writer & val(const T & v);

        Writer & end();

        std::string finish();

        private:

        struct State { bool array, compact, content; };

        std::ostringstream m_ss;
        std::vector<State> m_state;
        int m_indentation;
        std::string_view m_indent;
        bool m_isKey;

        void m_start(bool compact, char bracket);

        void m_end();

        void m_putPrefix();

        void m_putIndentation();

        void m_checkKey() const;

        void m_encode(std::string_view val);
        void m_encode(int64_t val);
        void m_encode(uint64_t val);
        void m_encode(double val);
        void m_encode(bool val);
        void m_encode(nullptr_t);

    };

}

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    using namespace std::string_view_literals;

    namespace detail {

        static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    }

    using namespace detail;

    inline Writer::Writer(bool compact, int indentSize) :
        m_ss(),
        m_state(),
        m_indentation(),
        m_indent("        ", indentSize),
        m_isKey()
    {
        if (indentSize < 0 || indentSize > 8) {
            throw JsonWriteError("Invalid indent size - must be in range [0, 8]");
        }

        m_ss << '{';
        m_state.push_back(State{false, compact, false});
    }

    inline Writer::Writer(Writer && other) :
        m_ss(std::move(other.m_ss)),
        m_state(std::move(other.m_state)),
        m_indentation(other.m_indentation),
        m_indent(other.m_indent)
    {
        other.m_indentation = 0;
        other.m_indent = {};
    }

    inline Writer & Writer::object(bool compact) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_start(compact, '{');
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::array(bool compact) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_start(compact, '[');
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::key(std::string_view key) {
        if (m_isKey) {
            throw JsonWriteError("Expected value to follow key");
        }
        if (m_state.back().array) {
            throw JsonWriteError("Array elements must not have keys");
        }
        if (key.empty()) {
            throw JsonWriteError("Key must not be empty");
        }

        m_putPrefix();
        m_encode(key);
        m_ss << ": "sv;
        m_isKey = true;

        return *this;
    }

    inline Writer & Writer::val(std::string_view v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::val(const std::string & v) {
        return val(std::string_view(v));
    }

    inline Writer & Writer::val(const char * v) {
        return val(std::string_view(v));
    }

    inline Writer & Writer::val(char v) {
        return val(std::string_view(&v, 1));
    }

    inline Writer & Writer::val(int64_t v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::val(int32_t v) {
        return val(int64_t(v));
    }

    inline Writer & Writer::val(int16_t v) {
        return val(int64_t(v));
    }

    inline Writer & Writer::val(int8_t v) {
        return val(uint64_t(v));
    }

    inline Writer & Writer::val(uint64_t v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::val(uint32_t v) {
        return val(uint64_t(v));
    }

    inline Writer & Writer::val(uint16_t v) {
        return val(uint64_t(v));
    }

    inline Writer & Writer::val(uint8_t v) {
        return val(uint64_t(v));
    }

    inline Writer & Writer::val(double v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::val(float v) {
        return val(double(v));
    }

    inline Writer & Writer::val(bool v) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(v);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    inline Writer & Writer::val(nullptr_t) {
        m_checkKey();
        if (!m_isKey) m_putPrefix();
        m_encode(nullptr);
        m_state.back().content = true;
        m_isKey = false;

        return *this;
    }

    template <typename T>
    inline Writer & Writer::val(const T & v) {
        QJsonEncoder<T>()(*this, v);
        return *this;
    }

    inline Writer & Writer::end() {
        if (m_state.size() <= 1) {
            throw JsonWriteError("No object or array to end");
        }
        if (m_isKey) {
            throw JsonWriteError("Expected value to follow key");
        }

        m_end();

        return *this;
    }

    inline std::string Writer::finish() {
        if (m_isKey) {
            throw JsonWriteError("Expected value");
        }

        bool compact(m_state.front().compact);

        while (!m_state.empty()) {
            m_end();
        }
        std::string str(m_ss.str());

        // Reset state
        m_ss.str("");
        m_ss.clear();
        m_ss << '{';
        m_state.push_back(State{false, compact, false});

        return std::move(str);
    }

    inline void Writer::m_start(bool compact, char bracket) {
        m_ss << bracket;
        m_state.back().content = true;
        m_state.push_back(State{bracket == '[', m_state.back().compact || compact, false});
    }

    inline void Writer::m_end() {
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

    inline void Writer::m_putPrefix() {
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

    inline void Writer::m_putIndentation() {
        for (int i(0); i < m_indentation; ++i) {
            m_ss << m_indent;
        }
    }

    inline void Writer::m_checkKey() const {
        if (!m_isKey && !m_state.back().array) {
            throw JsonWriteError("Object elements must have keys");
        }
    }

    inline void Writer::m_encode(std::string_view v) {
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
                            throw JsonWriteError("Non-ASCII unicode is unsupported");
                        }
                }
            }
        }

        m_ss << '"';
    }

    inline void Writer::m_encode(int64_t v) {
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

        m_ss << std::string_view(pos, end - pos);
    }

    inline void Writer::m_encode(uint64_t v) {
        m_ss << "0x"sv;

        char buffer[16];
        char * end(buffer + 16);
        char * pos(end);

        do {
            *--pos = k_hexChars[v & 0xF];
            v >>= 4;
        } while (v);

        m_ss << std::string_view(pos, end - pos);
    }

    inline void Writer::m_encode(double v) {
        if (std::isinf(v)) {
            throw JsonWriteError("The JSON standard does not support infinity");
        }
        if (std::isnan(v)) {
            throw JsonWriteError("The JSON standard does not support NaN");
        }

        char buffer[32];
        int len(std::snprintf(buffer, sizeof(buffer), "%#.17g", v));
        if (len < 0 || len >= sizeof(buffer)) {
            throw JsonWriteError("Error encoding float");
        }

        m_ss << std::string_view(buffer, len);
    }

    inline void Writer::m_encode(bool v) {
        m_ss << (v ? "true"sv : "false"sv);
    }

    inline void Writer::m_encode(nullptr_t) {
        m_ss << "null"sv;
    }

}
