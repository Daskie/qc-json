#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>

namespace qjson {

    struct json_exception : public std::runtime_error{
        json_exception(const char * msg) : std::runtime_error(msg) {}
    };

    template <typename T> struct Encoder {
        using typename T::qjson_encoder_has_no_specialization_for_this_type; // Intentional compile error for unspecialized types
    };

    template <typename T> struct HexEncoder {
        using typename T::qjson_encoder_has_no_specialization_for_this_type; // Intentional compile error for unspecialized types
    };

    class Writer {

        public:

        Writer(bool compact = false, int indentSize = 4);
        Writer(const Writer & other) = delete;
        Writer(Writer && other);

        Writer & operator=(const Writer & other) = delete;
        Writer & operator=(Writer && other) = delete;

        void startObject(std::string_view key, bool compact);
        void startObject(bool compact);

        void startArray(std::string_view key, bool compact);
        void startArray(bool compact);

        template <typename T, typename E = Encoder<std::decay_t<T>>> void put(std::string_view key, const T & val);
        template <typename T, typename E = Encoder<std::decay_t<T>>> void put(const T & val);
        
        template <typename T, typename E = HexEncoder<std::decay_t<T>>> void putHex(std::string_view key, const T & val) { put<T, E>(key, val); }
        template <typename T, typename E = HexEncoder<std::decay_t<T>>> void putHex(const T & val) { put<T, E>(val); }

        void endObject();

        void endArray();

        std::string finish();

        private:

        void m_start(bool compact, char bracket);

        void m_end();

        void m_putPrefix();

        void m_putIndentation();

        void m_putKey(std::string_view key);

        void m_checkKey(std::string_view key) const;
        void m_checkKey() const;

        struct State { bool array, compact, content; };

        std::ostringstream m_ss;
        std::vector<State> m_state;
        int m_indentation;
        std::string_view m_indent;

    };

    // IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////

    using namespace std::string_view_literals;

    namespace detail {
        
        static constexpr char k_hexChars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    }

    using namespace detail;

    template <>
    struct Encoder<std::string_view> {
        std::ostream & operator()(std::ostream & os, std::string_view val) const {
            os << '"';

            for (char c : val) {
                if (std::isprint(c)) {
                    if (c == '"' || c == '\\') {
                        os << '\\';
                    }
                    os << c;
                }
                else {
                    switch (c) {
                        case '\b': os << '\\' << 'b'; break;
                        case '\f': os << '\\' << 'f'; break;
                        case '\n': os << '\\' << 'n'; break;
                        case '\r': os << '\\' << 'r'; break;
                        case '\t': os << '\\' << 't'; break;
                        default:
                            if (unsigned char(c) < 128) {
                                os << '\\' << 'u' << '0' << '0' << k_hexChars[(c >> 4) & 0xF] << k_hexChars[c & 0xF];
                            }
                            else {
                                throw json_exception("Non-ASCII unicode is unsupported");
                            }
                    }
                }
            }

            return os << '"';
        }
    };
    template <> struct Encoder<std::string> { std::ostream & operator()(std::ostream & os, const std::string & val) const { return Encoder<std::string_view>()(os, val); } };
    template <> struct Encoder<const char *> { std::ostream & operator()(std::ostream & os, const char * val) const { return Encoder<std::string_view>()(os, val); } };
    template <> struct Encoder<char *> { std::ostream & operator()(std::ostream & os, const char * val) const { return Encoder<std::string_view>()(os, val); } };
    template <> struct Encoder<char> { std::ostream & operator()(std::ostream & os, char val) const { return Encoder<std::string_view>()(os, std::string_view(&val, 1)); } };

    template <>
    struct Encoder<uint64_t> {
        std::ostream & operator()(std::ostream & os, uint64_t val) const {
            char buffer[20];
            int digits(0);

            do {
                uint64_t quotent(val / 10), remainder(val % 10); // Compiler should optimize this as one operation
                buffer[digits] = char('0' + remainder);
                ++digits;
                val = quotent;
            } while (val);

            while (digits) {
                os << buffer[--digits];
            }
            
            return os;
        }
    };
    template <> struct Encoder<uint32_t> { std::ostream & operator()(std::ostream & os, uint32_t val) const { return Encoder<uint64_t>()(os, val); } };
    template <> struct Encoder<uint16_t> { std::ostream & operator()(std::ostream & os, uint16_t val) const { return Encoder<uint64_t>()(os, val); } };
    template <> struct Encoder< uint8_t> { std::ostream & operator()(std::ostream & os,  uint8_t val) const { return Encoder<uint64_t>()(os, val); } };

    template <>
    struct Encoder<int64_t> {
        std::ostream & operator()(std::ostream & os, int64_t val) const {
            if (val < 0) {
                os << '-';
                val = -val;
            }

            return Encoder<uint64_t>()(os, uint64_t(val));
        }
    };
    template <> struct Encoder<int32_t> { std::ostream & operator()(std::ostream & os, int32_t val) const { return Encoder<int64_t>()(os, val); } };
    template <> struct Encoder<int16_t> { std::ostream & operator()(std::ostream & os, int16_t val) const { return Encoder<int64_t>()(os, val); } };
    template <> struct Encoder< int8_t> { std::ostream & operator()(std::ostream & os,  int8_t val) const { return Encoder<int64_t>()(os, val); } };

    template <>
    struct Encoder<double> {
        std::ostream & operator()(std::ostream & os, double val) const {
            if (std::isinf(val)) {
                throw json_exception("The JSON standard does not support infinity");
            }
            if (std::isnan(val)) {
                throw json_exception("The JSON standard does not support NaN");
            }

            char buffer[32];
            int len(std::snprintf(buffer, sizeof(buffer), "%#.17g", val));
            if (len < 0 || len >= sizeof(buffer)) {
                throw json_exception("Error encoding float");
            }

            return os << std::string_view(buffer, len);
        }
    };
    template <> struct Encoder<float> { std::ostream & operator()(std::ostream & os, float val) const { return Encoder<double>()(os, val); } };

    template <>
    struct Encoder<bool> {
        std::ostream & operator()(std::ostream & os, bool val) const {
            return os << (val ? "true"sv : "false"sv);
        }
    };

    template <>
    struct Encoder<nullptr_t> {
        std::ostream & operator()(std::ostream & os, nullptr_t) const {
            return os << "null"sv;
        }
    };
    
    template <>
    struct HexEncoder<uint64_t> {
        template <typename T = uint64_t, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        std::ostream & operator()(std::ostream & os, T val) const {
            os << "0x"sv;

            char buffer[sizeof(T) * 2];

            for (int i(sizeof(T) * 2 - 1); i >= 0; --i) {
                buffer[i] = k_hexChars[val & 0xF];
                val >>= 4;
            };

            return os << std::string_view(buffer, sizeof(buffer));
        }
    };
    template <> struct HexEncoder<uint32_t> { std::ostream & operator()(std::ostream & os, uint32_t val) const { return HexEncoder<uint64_t>()(os, val); } };
    template <> struct HexEncoder<uint16_t> { std::ostream & operator()(std::ostream & os, uint16_t val) const { return HexEncoder<uint64_t>()(os, val); } };
    template <> struct HexEncoder< uint8_t> { std::ostream & operator()(std::ostream & os,  uint8_t val) const { return HexEncoder<uint64_t>()(os, val); } };

    inline Writer::Writer(bool compact, int indentSize) :
        m_ss(),
        m_state(),
        m_indentation(0),
        m_indent("        ", indentSize)
    {
        if (indentSize < 0 || indentSize > 8) {
            throw json_exception("Invalid indent size - must be in range [0, 8]");
        }

        m_ss << std::setprecision(15);
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

    template <typename T, typename E>
    inline void Writer::put(std::string_view key, const T & val) {
        m_checkKey(key);
        m_putPrefix();
        m_putKey(key);
        E()(m_ss, val);
        m_state.back().content = true;
    }

    template <typename T, typename E>
    inline void Writer::put(const T & val) {
        m_checkKey();
        m_putPrefix();
        E()(m_ss, val);
        m_state.back().content = true;
    }

    inline void Writer::endObject() {
        if (m_state.empty() || m_state.back().array) {
            throw json_exception("No object to end");
        }

        m_end();
    }

    inline void Writer::endArray() {
        if (m_state.empty() || !m_state.back().array) {
            throw json_exception("No array to end");
        }

        m_end();
    }

    inline std::string Writer::finish() {
        while (!m_state.empty()) {
            m_end();
        }
        return m_ss.str();
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
        Encoder<std::string_view>()(m_ss, key) << ": "sv;
    }

    inline void Writer::m_checkKey(std::string_view key) const {
        if (m_state.back().array) {
            throw json_exception("Array elements must not have keys");
        }
        if (key.empty()) {
            throw json_exception("Key must not be empty");
        }
    }

    inline void Writer::m_checkKey() const {
        if (!m_state.back().array) {
            throw json_exception("Object elements must have keys");
        }
    }

}