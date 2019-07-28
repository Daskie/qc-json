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
//      writer.put("Name", "Roslin");
//      writer.startArray("Favorite Books");
//      writer.put("Dark Day");
//      ...
//      writer.endArray();
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

        void startObject(std::string_view key, bool compact = false);
        void startObject(const char * key, bool compact = false) { startObject(std::string_view(key), compact); }
        void startObject(char * key, bool compact = false) { startObject(std::string_view(key), compact); }
        void startObject(bool compact = false);

        void startArray(std::string_view key, bool compact = false);
        void startArray(const char * key, bool compact = false) { startArray(std::string_view(key), compact); }
        void startArray(char * key, bool compact = false) { startArray(std::string_view(key), compact); }
        void startArray(bool compact = false);

        void put(std::string_view key, std::string_view val);
        void put(std::string_view val);
        void put(std::string_view key, const char * val);
        void put(const char * val);
        void put(std::string_view key, char val);
        void put(char val);
        void put(std::string_view key, int64_t val);
        void put(int64_t val);
        void put(std::string_view key, int32_t val);
        void put(int32_t val);
        void put(std::string_view key, int16_t val);
        void put(int16_t val);
        void put(std::string_view key, int8_t val);
        void put(int8_t val);
        void put(std::string_view key, uint64_t val);
        void put(uint64_t val);
        void put(std::string_view key, uint32_t val);
        void put(uint32_t val);
        void put(std::string_view key, uint16_t val);
        void put(uint16_t val);
        void put(std::string_view key, uint8_t val);
        void put(uint8_t val);
        void put(std::string_view key, double val);
        void put(double val);
        void put(std::string_view key, bool val);
        void put(bool val);
        void put(std::string_view key, nullptr_t);
        void put(nullptr_t);

        void endObject();

        void endArray();

        std::string finish();

      private:

        struct State { bool array, compact, content; };

        std::ostringstream m_ss;
        std::vector<State> m_state;
        int m_indentation;
        std::string_view m_indent;

        void m_start(bool compact, char bracket);

        void m_end();

        void m_putPrefix();

        void m_putIndentation();

        void m_putKey(std::string_view key);

        void m_checkKey(std::string_view key) const;
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
        m_indentation(0),
        m_indent("        ", indentSize)
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

    inline void Writer::startObject(std::string_view key, bool compact) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_start(compact, '{');
    }

    inline void Writer::startObject(bool compact) {
        m_checkKey();
        m_putPrefix();
        m_start(compact, '{');
    }

    inline void Writer::startArray(std::string_view key, bool compact) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_start(compact, '[');
    }

    inline void Writer::startArray(bool compact) {
        m_checkKey();
        m_putPrefix();
        m_start(compact, '[');
    }

    inline void Writer::put(std::string_view key, std::string_view val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view val) {
        m_checkKey();
        m_putPrefix();
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view key, const char * val) {
        put(key, std::string_view(val));
    }

    inline void Writer::put(const char * val) {
        put(std::string_view(val));
    }

    inline void Writer::put(std::string_view key, char val) {
        put(key, std::string_view(&val, 1));
    }

    inline void Writer::put(char val) {
        put(std::string_view(&val, 1));
    }

    inline void Writer::put(std::string_view key, int64_t val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(int64_t val) {
        m_checkKey();
        m_putPrefix();
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view key, int32_t val) {
        put(key, int64_t(val));
    }

    inline void Writer::put(int32_t val) {
        put(int64_t(val));
    }

    inline void Writer::put(std::string_view key, int16_t val) {
        put(key, int64_t(val));
    }

    inline void Writer::put(int16_t val) {
        put(int64_t(val));
    }

    inline void Writer::put(std::string_view key, int8_t val) {
        put(key, int64_t(val));
    }

    inline void Writer::put(int8_t val) {
        put(uint64_t(val));
    }

    inline void Writer::put(std::string_view key, uint64_t val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(uint64_t val) {
        m_checkKey();
        m_putPrefix();
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view key, uint32_t val) {
        put(key, uint64_t(val));
    }

    inline void Writer::put(uint32_t val) {
        put(uint64_t(val));
    }

    inline void Writer::put(std::string_view key, uint16_t val) {
        put(key, uint64_t(val));
    }

    inline void Writer::put(uint16_t val) {
        put(uint64_t(val));
    }

    inline void Writer::put(std::string_view key, uint8_t val) {
        put(key, uint64_t(val));
    }

    inline void Writer::put(uint8_t val) {
        put(uint64_t(val));
    }

    inline void Writer::put(std::string_view key, double val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(double val) {
        m_checkKey();
        m_putPrefix();
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view key, bool val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(bool val) {
        m_checkKey();
        m_putPrefix();
        m_encode(val);
        m_state.back().content = true;
    }

    inline void Writer::put(std::string_view key, nullptr_t) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        m_encode(nullptr);
        m_state.back().content = true;
    }

    inline void Writer::put(nullptr_t) {
        m_checkKey();
        m_putPrefix();
        m_encode(nullptr);
        m_state.back().content = true;
    }

    inline void Writer::endObject() {
        if (m_state.size() <= 1 || m_state.back().array) {
            throw JsonWriteError("No object to end");
        }

        m_end();
    }

    inline void Writer::endArray() {
        if (m_state.size() <= 1 || !m_state.back().array) {
            throw JsonWriteError("No array to end");
        }

        m_end();
    }

    inline std::string Writer::finish() {
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

    inline void Writer::m_putKey(std::string_view key) {
        m_encode(key);
        m_ss << ": "sv;
    }

    inline void Writer::m_checkKey(std::string_view key) const {
        if (m_state.back().array) {
            throw JsonWriteError("Array elements must not have keys");
        }
        if (key.empty()) {
            throw JsonWriteError("Key must not be empty");
        }
    }

    inline void Writer::m_checkKey() const {
        if (!m_state.back().array) {
            throw JsonWriteError("Object elements must have keys");
        }
    }

    inline void Writer::m_encode(std::string_view val) {
        m_ss << '"';

        for (char c : val) {
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

    inline void Writer::m_encode(int64_t val) {
        if (val < 0) {
            m_ss << '-';
            val = -val;
        }

        uint64_t uval(val); // Necessary for the case of UINT_MIN

        char buffer[20];
        char * end(buffer + 20);
        char * pos(end);

        do {
            uint64_t quotent(uval / 10), remainder(uval % 10); // Compiler should optimize this as one operation
            *--pos = char('0' + remainder);
            uval = quotent;
        } while (uval);

        m_ss << std::string_view(pos, end - pos);
    }

    inline void Writer::m_encode(uint64_t val) {
        m_ss << "0x"sv;

        char buffer[16];
        char * end(buffer + 16);
        char * pos(end);

        do {
            *--pos = k_hexChars[val & 0xF];
            val >>= 4;
        } while (val);

        m_ss << std::string_view(pos, end - pos);
    }

    inline void Writer::m_encode(double val) {
        if (std::isinf(val)) {
            throw JsonWriteError("The JSON standard does not support infinity");
        }
        if (std::isnan(val)) {
            throw JsonWriteError("The JSON standard does not support NaN");
        }

        char buffer[32];
        int len(std::snprintf(buffer, sizeof(buffer), "%#.17g", val));
        if (len < 0 || len >= sizeof(buffer)) {
            throw JsonWriteError("Error encoding float");
        }

        m_ss << std::string_view(buffer, len);
    }

    inline void Writer::m_encode(bool val) {
        m_ss << (val ? "true"sv : "false"sv);
    }

    inline void Writer::m_encode(nullptr_t) {
        m_ss << "null"sv;
    }

}
