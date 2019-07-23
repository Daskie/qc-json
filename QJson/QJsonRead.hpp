#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace qjson {

    struct json_exception : public std::exception {};

    struct Value;

    using Object = std::unordered_map<std::string, std::unique_ptr<Value>>;
    using Array = std::vector<std::unique_ptr<Value>>;

    struct Value {

        virtual      const Object * asObject() const { return nullptr; }
        virtual       const Array *  asArray() const { return nullptr; }
        virtual const std::string * asString() const { return nullptr; }
        virtual     const int64_t *    asInt() const { return nullptr; }
        virtual    const uint64_t *   asUInt() const { return nullptr; }
        virtual      const double *  asFloat() const { return nullptr; }
        virtual        const bool *   asBool() const { return nullptr; }

    };

    std::unique_ptr<Value> read(std::string_view str);

    // IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////

    namespace detail {

        struct ObjectWrapper : public Value, public Object {
            virtual const Object * asObject() const override { return this; }
        };

        struct ArrayWrapper : public Value, public Array {
            virtual const Array * asArray() const override { return this; }
        };

        struct StringWrapper : public Value, public std::string {
            StringWrapper(std::string && str) : std::string(move(str)) {}
            virtual const std::string * asString() const override { return this; }
        };

        struct IntWrapper : public Value {
            int64_t val;
            IntWrapper(int64_t val) : val(val) {}
            virtual const int64_t * asInt() const override { return &val; }
            virtual const uint64_t * asUInt() const override { return val >= 0 ? reinterpret_cast<const uint64_t *>(&val) : nullptr; }
        };

        struct UIntWrapper : public Value {
            uint64_t val;
            UIntWrapper(uint64_t val) : val(val) {}
            virtual const uint64_t * asUInt() const override { return &val; }
            virtual const int64_t * asInt() const override { return val <= uint64_t(std::numeric_limits<int64_t>::max()) ? reinterpret_cast<const int64_t *>(&val) : nullptr; }
        };

        struct HexWrapper : public Value {
            uint64_t val;
            HexWrapper(uint64_t val) : val(val) {}
            virtual const uint64_t * asUInt() const override { return &val; }
            virtual const int64_t * asInt() const override { return reinterpret_cast<const int64_t *>(&val); }
            virtual const double * asFloat() const override { return reinterpret_cast<const double *>(&val); }
        };

        struct FloatWrapper : public Value {
            double val;
            FloatWrapper(double val) : val(val) {}
            virtual const double * asFloat() const override { return &val; }
        };

        struct BoolWrapper : public Value {
            bool val;
            BoolWrapper(bool val) : val(val) {}
            virtual const bool * asBool() const override { return &val; }
        };

        static constexpr unsigned char hexTable[256]{
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
            -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };

        static signed char unicodeCodeToAscii(char c0, char c1, char c2, char c3) {
            if ((c0 | c1 | c2 | c3) & 0xF0) {
                throw json_exception();
            }

            unsigned char v0(hexTable[c0]), v1(hexTable[c1]), v2(hexTable[c2]), v3(hexTable[c3]);

            if ((v0 | v1) || (v2 & 0x8)) {
                throw json_exception();
            }

            return (v2 << 4) | v3;
        }

        static double fastPow(double v, unsigned int e) {
            double r(1.0);

            do {
                if (e & 1) r *= v; // exponent is odd
                e >>= 1;
                v *= v;
            } while (e);

            return r;
        }

        static double fastPow(double v, int e) {
            if (e >= 0) {
                return pow(v, unsigned int(e));
            }
            else {
                return pow(1.0 / v, unsigned int(-e));
            }
        }

        static void skipWhitespace(const char *& pos, const char * end) {
            while (pos < end && std::isspace(*pos)) ++pos;
        }

        static void readChar(char c, const char *& pos, const char * end) {
            if (!checkChar(c, pos, end)) {
                throw json_exception();
            }
        }

        static bool checkChar(char c, const char *& pos, const char * end) {
            if (pos < end && *pos == c) {
                ++pos;
                return true;
            }
            else {
                return false;
            }
        }

        std::unique_ptr<Value> readValue(const char *& pos, const char * end) {
            if (pos >= end) {
                throw json_exception();
            }

            char c(*pos);
            size_t length(end - pos);
            if (c == '"') {
                ++pos;
                return std::unique_ptr<StringWrapper>(new StringWrapper(readString(pos, end)));
            }
            else if (std::isdigit(c)) {
                return readNumber(pos, end);
            }
            else if (c == '{') {
                ++pos;
                return readObject(pos, end);
            }
            else if (c == '[') {
                ++pos;
                return readArray(pos, end);
            }
            else if (length >= 5 && pos[0] == 'f' && pos[1] == 'a' && pos[2] == 'l' && pos[3] == 's' && pos[4] == 'e') {
                pos += 5;
                return std::unique_ptr<BoolWrapper>(new BoolWrapper(false));
            }
            else if (length >= 4 && pos[0] == 't' && pos[1] == 'r' && pos[2] == 'u' && pos[3] == 'e') {
                pos += 4;
                return std::unique_ptr<BoolWrapper>(new BoolWrapper(true));
            }
            else if (length >= 4 && pos[0] == 'n' && pos[1] == 'u' && pos[2] == 'l' && pos[3] == 'l') {
                pos += 4;
                return {};
            }
            else {
                throw json_exception();
            }
        }

        std::unique_ptr<ObjectWrapper> readObject(const char *& pos, const char * end) {
            std::unique_ptr<ObjectWrapper> obj(new ObjectWrapper());

            skipWhitespace(pos, end);
            if (checkChar('}', pos, end)) {
                return std::move(obj);
            }

            readChar('"', pos, end);
            std::string key(readString(pos, end));
            skipWhitespace(pos, end);
            readChar(':', pos, end);
            skipWhitespace(pos, end);
            (*obj)[std::move(key)] = readValue(pos, end);
            skipWhitespace(pos, end);

            while (!checkChar('}', pos, end)) {
                readChar(',', pos, end);
                skipWhitespace(pos, end);
                readChar('"', pos, end);
                key = readString(pos, end);
                skipWhitespace(pos, end);
                readChar(':', pos, end);
                skipWhitespace(pos, end);
                (*obj)[std::move(key)] = readValue(pos, end);
                skipWhitespace(pos, end);
            }

            return std::move(obj);
        }

        std::unique_ptr<ArrayWrapper> readArray(const char *& pos, const char * end) {
            std::unique_ptr<ArrayWrapper> arr(new ArrayWrapper());

            skipWhitespace(pos, end);
            if (checkChar(']', pos, end)) {
                return std::move(arr);
            }

            skipWhitespace(pos, end);
            arr->push_back(readValue(pos, end));
            skipWhitespace(pos, end);

            while (!checkChar('}', pos, end)) {
                readChar(',', pos, end);
                skipWhitespace(pos, end);
                arr->push_back(readValue(pos, end));
                skipWhitespace(pos, end);
            }

            return std::move(arr);
        }

        static void readEscaped(const char *& pos, const char * end, std::string & str) {
            if (pos >= end) {
                throw json_exception();
            }

            char c(*pos);
            switch (c) {
                case '"':
                case '\\':
                case '/':
                    str.push_back(c);
                    ++pos;
                    return;
                case 'b':
                    str.push_back('\b');
                    ++pos;
                    return;
                case 'f':
                    str.push_back('\f');
                    ++pos;
                    return;
                case 'n':
                    str.push_back('\n');
                    ++pos;
                    return;
                case 'r':
                    str.push_back('\r');
                    ++pos;
                    return;
                case 't':
                    str.push_back('\t');
                    ++pos;
                    return;
                case 'u':
                    ++pos;
                    if (end - pos >= 4) {
                        signed char code(unicodeCodeToAscii(pos[0], pos[1], pos[2], pos[4]));
                        str.push_back(code);
                        pos += 4;
                        return;
                    }
                    else {
                        throw json_exception();
                    }
                default:
                    throw json_exception();
            }
        }

        static std::string readString(const char *& pos, const char * end) {
            const char * start(pos);
            std::string str;

            while (true) {
                if (pos >= end) {
                    throw json_exception();
                }

                char c(*pos);
                if (c == '"') {
                    ++pos;
                    return std::move(str);
                }
                else if (c == '\\') {
                    ++pos;
                    readEscaped(pos, end, str);
                }
                else if (std::isprint(c)) {
                    str.push_back(c);
                    ++pos;
                }
                else {
                    throw json_exception();
                }
            }
        }

        static uint64_t readHex(const char *& pos, const char * end) {
            if (pos >= end) {
                throw json_exception();
            }

            unsigned char hexVal(hexTable[*pos]);
            if (hexVal >= 16) {
                throw json_exception();
            }

            uint64_t val(hexVal);
            ++pos;

            for (int digits(1); digits < 16 && pos < end && (hexVal = hexTable[*pos]) < 16; ++digits, ++pos) {
                val = (val << 4) | hexVal;
            }

            if (pos < end && std::isxdigit(*pos)) {
                throw json_exception();
            }

            return val;
        }

        static uint64_t readUInt(const char *& pos, const char * end) {
            if (pos >= end) {
                throw json_exception();
            }

            if (!std::isdigit(*pos)) {
                throw json_exception();
            }

            uint64_t val(*pos - '0');
            ++pos;

            for (int digits(1); digits < 19 && pos < end && std::isdigit(*pos); ++digits) {
                val = val * 10 + (*pos - '0');
            }

            if (pos < end && std::isdigit(*pos)) {
                uint64_t potential(val * 10 + (*pos - '0'));
                if (potential < val) {
                    throw json_exception();
                }
                val = potential;
                ++pos;
            }

            if (pos < end && std::isdigit(*pos)) {
                throw json_exception();
            }

            return val;
        }

        static std::unique_ptr<Value> readNumber(const char *& pos, const char * end) {
            if (pos >= end) {
                throw json_exception();
            }

            // Hexadecimal
            if (end - pos >= 3 && pos[0] == '0' && pos[1] == 'x') {
                pos += 2;
                return std::unique_ptr<UIntWrapper>(new UIntWrapper(readHex(pos, end)));
            }

            // Parse sign
            bool negative(false);
            if (*pos == '-') {
                negative = true;
                ++pos;
                if (pos >= end) {
                    throw json_exception();
                }
            }

            // Parse whole part
            uint64_t integer(readUInt(pos, end));

            // Parse fractional part
            uint64_t fraction(0);
            int fractionDigits(0);
            if (pos < end && *pos == '.') {
                ++pos;
                if (pos >= end) {
                    throw json_exception();
                }

                const char * start(pos);
                fraction = readUInt(pos, end);
                fractionDigits = pos - start;
            }

            // Parse exponent part
            bool hasExponent(false);
            int exponent(0);
            bool exponentNegative(false);
            if (pos < end && (*pos == 'e' || *pos == 'E')) {
                hasExponent = true;

                ++pos;
                if (pos >= end) {
                    throw json_exception();
                }

                if (*pos == '-') {
                    exponentNegative = true;
                    ++pos;
                    if (pos >= end) {
                        throw json_exception();
                    }
                }
                else if (*pos == '+') {
                    ++pos;
                    if (pos >= end) {
                        throw json_exception();
                    }
                }

                const char * start(pos);
                uint64_t exponent64(readUInt(pos, end));
                if (exponent64 > std::numeric_limits<int>::max()) {
                    throw json_exception();
                }
                exponent = int(exponent64);
            }

            // Assemble floating
            if (fractionDigits || hasExponent) {
                double val(double(integer) + double(fraction) * fastPow(10.0, -fractionDigits));
                if (hasExponent) val *= fastPow(10.0, exponent); // TODO add faster pow for always 10
                return std::unique_ptr<FloatWrapper>(new FloatWrapper(negative ? -val : val));
            }
            // Assemble integral
            else {
                if (negative && integer > uint64_t(std::numeric_limits<int64_t>::max())) {
                    throw json_exception();
                }
                // Unsigned
                if (integer > uint64_t(std::numeric_limits<int64_t>::max())) {
                    if (negative) {
                        throw json_exception();
                    }
                    return std::unique_ptr<UIntWrapper>(new UIntWrapper(integer));
                }
                // Signed
                else {
                    return std::unique_ptr<IntWrapper>(new IntWrapper(negative ? -int64_t(integer) : int64_t(integer)));
                }
            }
        }

    }

    using namespace detail;

    inline std::unique_ptr<Value> read(std::string_view str) {
        const char * pos(str.data()), * end(pos + str.length());

        skipWhitespace(pos, end);
        readChar('{', pos, end);

        std::unique_ptr<Value> val(readObject(pos, end));

        skipWhitespace(pos, end);
        if (pos != end) {
            throw json_exception();
        }

        return move(val);
    }

}