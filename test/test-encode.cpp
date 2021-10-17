#include <format>

#include <gtest/gtest.h>

#include <qc-json/qc-json-encode.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::Encoder;
using qc::json::EncodeError;
using namespace qc::json::tokens;
using qc::json::Density;

struct CustomVal { int x, y; };

Encoder & operator<<(Encoder & encoder, const CustomVal & v)
{
    return encoder << array(Density::uniline) << v.x << v.y << end;
}

TEST(encode, object)
{
    { // Empty
        Encoder encoder{};
        encoder << object(Density::multiline) << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
        encoder << object(Density::uniline) << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
        encoder << object(Density::nospace) << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
    }
    { // Non-empty
        Encoder encoder{};
        encoder << object(Density::multiline) << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({
    "k1": "abc",
    "k2": 123,
    "k3": true
})"s, encoder.finish());
        encoder << object(Density::uniline) << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({ "k1": "abc", "k2": 123, "k3": true })"s, encoder.finish());
        encoder << object(Density::nospace) << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({"k1":"abc","k2":123,"k3":true})"s, encoder.finish());
    }
    { // String view key
        Encoder encoder{Density::uniline};
        encoder << object << "k"sv << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // String key
        Encoder encoder{Density::uniline};
        encoder << object << "k"s << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Const C string key
        Encoder encoder{Density::uniline};
        encoder << object << "k" << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Mutable C string key
        Encoder encoder{Density::uniline};
        encoder << object << const_cast<char *>("k") << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Character key
        Encoder encoder{Density::uniline};
        encoder << object << 'k' << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Empty key
        Encoder encoder{Density::uniline};
        encoder << object << "" << "" << end;
        EXPECT_EQ(R"({ "": "" })"s, encoder.finish());
    }
    { // Integer key
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder << 123, EncodeError);
    }
    { // Floater key
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder << 123.4, EncodeError);
    }
    { // Boolean key
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder << true, EncodeError);
    }
    { // Null key
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder << nullptr , EncodeError);
    }
    { // Dangling key
        Encoder encoder{};
        encoder << object << "k1";
        EXPECT_THROW(encoder << end, EncodeError);
    }
}

TEST(encode, array)
{
    { // Empty
        Encoder encoder{};
        encoder << array(Density::multiline) << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
        encoder << array(Density::uniline) << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
        encoder << array(Density::nospace) << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
    }
    { // Non-empty
        Encoder encoder{};
        encoder << array(Density::multiline) << "abc" << 123 << true << end;
        EXPECT_EQ(R"([
    "abc",
    123,
    true
])"s, encoder.finish());
        encoder << array(Density::uniline) << "abc" << 123 << true << end;
        EXPECT_EQ(R"([ "abc", 123, true ])"s, encoder.finish());
        encoder << array(Density::nospace) << "abc" << 123 << true << end;
        EXPECT_EQ(R"(["abc",123,true])"s, encoder.finish());
    }
}

TEST(encode, string)
{
    { // Empty
        Encoder encoder{};
        encoder << "";
        EXPECT_EQ(R"("")"s, encoder.finish());
    }
    { // String view
        Encoder encoder{};
        encoder << "hello"sv;
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // String
        Encoder encoder{};
        encoder << "hello"s;
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Const C string
        Encoder encoder{};
        encoder << "hello";
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Mutable C string
        Encoder encoder{};
        encoder << const_cast<char *>("hello");
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Printable characters
        Encoder encoder{};
        const std::string actual{R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"s};
        const std::string expected{R"(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"s};
        encoder << actual;
        EXPECT_EQ(expected, encoder.finish());
    }
    { // Escape characters
        Encoder encoder{};
        encoder << "\0\b\t\n\v\f\r"sv;
        EXPECT_EQ(R"("\0\b\t\n\v\f\r")"s, encoder.finish());
    }
    { // `\x` code point
        std::string decodeStr(154, '\0');
        std::string expectedStr(1 + 154 * 4 + 1, '\0');
        expectedStr.front() = '"';
        expectedStr.back() = '"';
        int i{0};
        for (int cp{1}; cp < 8; ++cp, ++i) {
            decodeStr[i] = char(cp);
            std::format_to_n(&expectedStr[1 + 4 * i], 6, "\\x{:02X}"sv, cp);
        }
        for (int cp{14}; cp < 32; ++cp, ++i) {
            decodeStr[i] = char(cp);
            std::format_to_n(&expectedStr[1 + 4 * i], 6, "\\x{:02X}"sv, cp);
        }
        for (int cp{127}; cp < 256; ++cp, ++i) {
            decodeStr[i] = char(cp);
            std::format_to_n(&expectedStr[1 + 4 * i], 6, "\\x{:02X}"sv, cp);
        }
        Encoder encoder{};
        encoder << decodeStr;
        EXPECT_EQ(expectedStr, encoder.finish());
    }
    { // Single char
        Encoder encoder{};
        encoder << 'a';
        EXPECT_EQ(R"("a")"s, encoder.finish());
    }
    { // Double quotes
        Encoder encoder{Density::uniline, false};
        std::string expected{};
        encoder << R"(s"t'r)";
        expected = R"("s\"t'r")"s;
        EXPECT_EQ(expected, encoder.finish());
        encoder << object << R"(""")" << R"(''')" << end;
        expected = R"({ "\"\"\"": "'''" })"s;
        EXPECT_EQ(expected, encoder.finish());
        encoder << object << R"(''')" << R"(""")" << end;
        expected = R"({ "'''": "\"\"\"" })"s;
        EXPECT_EQ(expected, encoder.finish());
    }
    { // Single quotes
        Encoder encoder{Density::uniline, true};
        std::string expected{};
        encoder << R"(s"t'r)";
        expected = R"('s"t\'r')"s;
        EXPECT_EQ(expected, encoder.finish());
        encoder << object << R"(""")" << R"(''')" << end;
        expected = R"({ '"""': '\'\'\'' })"s;
        EXPECT_EQ(expected, encoder.finish());
        encoder << object << R"(''')" << R"(""")" << end;
        expected = R"({ '\'\'\'': '"""' })"s;
        EXPECT_EQ(expected, encoder.finish());
    }
}

TEST(encode, signedInteger)
{
    { // Zero
        Encoder encoder{};
        encoder << int64_t(0);
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << 123;
        EXPECT_EQ(R"(123)"s, encoder.finish());
    }
    { // Max 64
        Encoder encoder{};
        encoder << std::numeric_limits<int64_t>::max();
        EXPECT_EQ(R"(9223372036854775807)"s, encoder.finish());
    }
    { // Min 64
        Encoder encoder{};
        encoder << std::numeric_limits<int64_t>::min();
        EXPECT_EQ(R"(-9223372036854775808)"s, encoder.finish());
    }
    { // Max 32
        Encoder encoder{};
        encoder << std::numeric_limits<int32_t>::max();
        EXPECT_EQ(R"(2147483647)"s, encoder.finish());
    }
    { // Min 32
        Encoder encoder{};
        encoder << std::numeric_limits<int32_t>::min();
        EXPECT_EQ(R"(-2147483648)"s, encoder.finish());
    }
    { // Max 16
        Encoder encoder{};
        encoder << std::numeric_limits<int16_t>::max();
        EXPECT_EQ(R"(32767)"s, encoder.finish());
    }
    { // Min 16
        Encoder encoder{};
        encoder << std::numeric_limits<int16_t>::min();
        EXPECT_EQ(R"(-32768)"s, encoder.finish());
    }
    { // Max 8
        Encoder encoder{};
        encoder << std::numeric_limits<int8_t>::max();
        EXPECT_EQ(R"(127)"s, encoder.finish());
    }
    { // Min 8
        Encoder encoder{};
        encoder << std::numeric_limits<int8_t>::min();
        EXPECT_EQ(R"(-128)"s, encoder.finish());
    }
}

TEST(encode, unsignedInteger)
{
    { // Zero
        Encoder encoder{};
        encoder << 0u;
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << 123u;
        EXPECT_EQ(R"(123)"s, encoder.finish());
    }
    { // Max 64
        Encoder encoder{};
        encoder << std::numeric_limits<uint64_t>::max();
        EXPECT_EQ(R"(18446744073709551615)"s, encoder.finish());
    }
    { // Max 32
        Encoder encoder{};
        encoder << std::numeric_limits<uint32_t>::max();
        EXPECT_EQ(R"(4294967295)"s, encoder.finish());
    }
    { // Max 16
        Encoder encoder{};
        encoder << std::numeric_limits<uint16_t>::max();
        EXPECT_EQ(R"(65535)"s, encoder.finish());
    }
    { // Max 8
        Encoder encoder{};
        encoder << std::numeric_limits<uint8_t>::max();
        EXPECT_EQ(R"(255)"s, encoder.finish());
    }
}

TEST(encode, hex)
{
    { // Zero
        Encoder encoder{};
        encoder << hex(0u);
        EXPECT_EQ(R"(0x0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << hex(26u);
        EXPECT_EQ(R"(0x1A)"s, encoder.finish());
    }
    { // Max unsigned
        Encoder encoder{};
        encoder << hex(std::numeric_limits<uint64_t>::max());
        EXPECT_EQ(R"(0xFFFFFFFFFFFFFFFF)"s, encoder.finish());
    }
    { // Min signed
        Encoder encoder{};
        encoder << hex(std::numeric_limits<int64_t>::min());
        EXPECT_EQ(R"(0x8000000000000000)"s, encoder.finish());
    }
    { // -1
        Encoder encoder{};
        #pragma warning(suppress: 4245)
        encoder << hex(-1);
        EXPECT_EQ(R"(0xFFFFFFFFFFFFFFFF)"s, encoder.finish());
    }
}

TEST(encode, octal)
{
    { // Zero
        Encoder encoder{};
        encoder << octal(0u);
        EXPECT_EQ(R"(0o0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << octal(10u);
        EXPECT_EQ(R"(0o12)"s, encoder.finish());
    }
    { // Max unsigned
        Encoder encoder{};
        encoder << octal(std::numeric_limits<uint64_t>::max());
        EXPECT_EQ(R"(0o1777777777777777777777)"s, encoder.finish());
    }
    { // Min signed
        Encoder encoder{};
        encoder << octal(std::numeric_limits<int64_t>::min());
        EXPECT_EQ(R"(0o1000000000000000000000)"s, encoder.finish());
    }
    { // -1
        Encoder encoder{};
        #pragma warning(suppress: 4245)
        encoder << octal(-1);
        EXPECT_EQ(R"(0o1777777777777777777777)"s, encoder.finish());
    }
}

TEST(encode, binary)
{
    { // Zero
        Encoder encoder{};
        encoder << binary(0u);
        EXPECT_EQ(R"(0b0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << binary(5u);
        EXPECT_EQ(R"(0b101)"s, encoder.finish());
    }
    { // Max unsigned
        Encoder encoder{};
        encoder << binary(std::numeric_limits<uint64_t>::max());
        EXPECT_EQ(R"(0b1111111111111111111111111111111111111111111111111111111111111111)"s, encoder.finish());
    }
    { // Min signed
        Encoder encoder{};
        encoder << binary(std::numeric_limits<int64_t>::min());
        EXPECT_EQ(R"(0b1000000000000000000000000000000000000000000000000000000000000000)"s, encoder.finish());
    }
    { // -1
        Encoder encoder{};
        #pragma warning(suppress: 4245)
        encoder << binary(-1);
        EXPECT_EQ(R"(0b1111111111111111111111111111111111111111111111111111111111111111)"s, encoder.finish());
    }
}

TEST(encode, floater)
{
    { // Zero
        Encoder encoder{};
        encoder << 0.0;
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        Encoder encoder{};
        encoder << 123.45;
        EXPECT_EQ(R"(123.45)"s, encoder.finish());
    }
    { // Max integer 64
        Encoder encoder{};
        uint64_t val{0b0'10000110011'1111111111111111111111111111111111111111111111111111u};
        encoder << reinterpret_cast<const double &>(val);
        EXPECT_EQ(R"(9007199254740991)"s, encoder.finish());
    }
    { // Max integer 32
        Encoder encoder{};
        uint32_t val{0b0'10010110'11111111111111111111111u};
        encoder << reinterpret_cast<const float &>(val);
        EXPECT_EQ(R"(16777215)"s, encoder.finish());
    }
    { // Max 64
        Encoder encoder{};
        uint64_t val{0b0'11111111110'1111111111111111111111111111111111111111111111111111u};
        encoder << reinterpret_cast<const double &>(val);
        EXPECT_EQ(R"(1.7976931348623157e+308)"s, encoder.finish());
    }
    { // Max 32
        Encoder encoder{};
        uint32_t val{0b0'11111110'11111111111111111111111u};
        encoder << reinterpret_cast<const float &>(val);
        EXPECT_EQ(R"(3.4028234663852886e+38)"s, encoder.finish());
    }
    { // Min normal 64
        Encoder encoder{};
        uint64_t val{0b0'00000000001'0000000000000000000000000000000000000000000000000000u};
        encoder << reinterpret_cast<const double &>(val);
        EXPECT_EQ(R"(2.2250738585072014e-308)"s, encoder.finish());
    }
    { // Min normal 32
        Encoder encoder{};
        uint32_t val{0b0'00000001'00000000000000000000000u};
        encoder << reinterpret_cast<const float &>(val);
        EXPECT_EQ(R"(1.1754943508222875e-38)"s, encoder.finish());
    }
    { // Min subnormal 64
        Encoder encoder{};
        uint64_t val{0b0'00000000000'0000000000000000000000000000000000000000000000000001u};
        encoder << reinterpret_cast<const double &>(val);
        EXPECT_EQ(R"(5e-324)"s, encoder.finish());
    }
    { // Min subnormal 32
        Encoder encoder{};
        uint64_t val{0b0'00000000'00000000000000000000001u};
        encoder << reinterpret_cast<const float &>(val);
        EXPECT_EQ(R"(1.401298464324817e-45)"s, encoder.finish());
    }
    { // Positive infinity
        Encoder encoder{};
        encoder << std::numeric_limits<double>::infinity();
        EXPECT_EQ(R"(inf)"s, encoder.finish());
    }
    { // Negative infinity
        Encoder encoder{};
        encoder << -std::numeric_limits<double>::infinity();
        EXPECT_EQ(R"(-inf)"s, encoder.finish());
    }
    { // NaN
        Encoder encoder{};
        encoder << std::numeric_limits<double>::quiet_NaN();
        EXPECT_EQ(R"(nan)"s, encoder.finish());
    }
}

TEST(encode, boolean)
{
    { // True
        Encoder encoder{};
        encoder << true;
        EXPECT_EQ(R"(true)"s, encoder.finish());
    }
    { // False
        Encoder encoder{};
        encoder << false;
        EXPECT_EQ(R"(false)"s, encoder.finish());
    }
}

TEST(encode, null)
{
    Encoder encoder{};
    encoder << nullptr;
    EXPECT_EQ(R"(null)"s, encoder.finish());
}

TEST(encode, custom)
{
    Encoder encoder{};
    encoder << CustomVal{1, 2};
    EXPECT_EQ(R"([ 1, 2 ])"s, encoder.finish());
}

TEST(encode, finish)
{
    { // Encoder left in clean state after finish
        Encoder encoder{Density::uniline};
        encoder << object << "val" << 123 << end;
        EXPECT_EQ(R"({ "val": 123 })"s, encoder.finish());
        encoder << array << 321 << end;
        EXPECT_EQ(R"([ 321 ])"s, encoder.finish());
    }
    { // Finishing at root
        Encoder encoder{};
        EXPECT_THROW(encoder.finish(), EncodeError);
    }
    { // Finishing in object
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder.finish(), EncodeError);
    }
    { // Finishing in array
        Encoder encoder{};
        encoder << array;
        EXPECT_THROW(encoder.finish(), EncodeError);
    }
}

TEST(encode, density)
{
    { // Top level multiline
        Encoder encoder{Density::multiline};
        encoder << object << "k1" << array << "v1" << "v2" << end << "k2" << "v3" << end;
        EXPECT_EQ(R"({
    "k1": [
        "v1",
        "v2"
    ],
    "k2": "v3"
})"s, encoder.finish());
    }
    { // Top level uniline
        Encoder encoder{Density::uniline};
        encoder << object << "k1" << array << "v1" << "v2" << end << "k2" << "v3" << end;
        EXPECT_EQ(R"({ "k1": [ "v1", "v2" ], "k2": "v3" })"s, encoder.finish());
    }
    { // Top level nospace
        Encoder encoder{Density::nospace};
        encoder << object << "k1" << array << "v1" << "v2" << end << "k2" << "v3" << end;
        EXPECT_EQ(R"({"k1":["v1","v2"],"k2":"v3"})"s, encoder.finish());
    }
    { // Inner density
        Encoder encoder{};
        encoder << object;
            encoder << "k1" << array(Density::uniline) << "v1" << array(Density::nospace) << "v2" << "v3" << end << end;
            encoder << "k2" << object(Density::uniline) << "k3" << "v4" << "k4" << object(Density::nospace) << "k5" << "v5" << "k6" << "v6" << end << end;
        encoder << end;
        EXPECT_EQ(R"({
    "k1": [ "v1", ["v2","v3"] ],
    "k2": { "k3": "v4", "k4": {"k5":"v5","k6":"v6"} }
})"s, encoder.finish());
    }
    { // Density priority
        Encoder encoder{};
        encoder << object(Density::uniline) << "k" << array(Density::multiline) << "v" << end << end;
        EXPECT_EQ(R"({ "k": [ "v" ] })"s, encoder.finish());
        encoder << array(Density::uniline) << object(Density::multiline) << "k" << "v" << end << end;
        EXPECT_EQ(R"([ { "k": "v" } ])"s, encoder.finish());
        encoder << object(Density::nospace) << "k" << array(Density::uniline) << "v" << end << end;
        EXPECT_EQ(R"({"k":["v"]})"s, encoder.finish());
        encoder << array(Density::nospace) << object(Density::uniline) << "k" << "v" << end << end;
        EXPECT_EQ(R"([{"k":"v"}])"s, encoder.finish());
    }
}

TEST(encode, identifiers) {
    { // Valid identifiers
        Encoder encoder{Density::uniline, false, true};
        encoder << object << "a" << "v" << end;
        EXPECT_EQ(R"({ a: "v" })"s, encoder.finish());
        encoder << object << "A" << "v" << end;
        EXPECT_EQ(R"({ A: "v" })"s, encoder.finish());
        encoder << object << "0" << "v" << end;
        EXPECT_EQ(R"({ 0: "v" })"s, encoder.finish());
        encoder << object << "_" << "v" << end;
        EXPECT_EQ(R"({ _: "v" })"s, encoder.finish());
        encoder << object << "_0a" << "v" << end;
        EXPECT_EQ(R"({ _0a: "v" })"s, encoder.finish());
        encoder << object << "_0a" << "v" << end;
        EXPECT_EQ(R"({ _0a: "v" })"s, encoder.finish());
    }
    { // Invalid identifiers
        Encoder encoder{Density::uniline, false, true};
        encoder << object << "w o a" << "v" << end;
        EXPECT_EQ(R"({ "w o a": "v" })"s, encoder.finish());
    }
    { // Preference off
        Encoder encoder{Density::uniline};
        encoder << object << "k" << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Empty key
        Encoder encoder{Density::uniline, false, true};
        encoder << object;
        EXPECT_THROW(encoder << "", EncodeError);
    }
}

TEST(encode, comments)
{
    { // Simple one-line comments
        Encoder encoder{Density::multiline};
        const std::string_view str{"A comment"};
        encoder << comment(str) << comment(str) << 0 << comment(str) << comment(str);
        EXPECT_EQ(
R"(// A comment
// A comment
0,
// A comment
// A comment)"s, encoder.finish());
    }
    { // Simple multi-line comments
        Encoder encoder{Density::multiline};
        const std::string_view str{"A comment\nand some more"};
        encoder << comment(str) << comment(str) << 0 << comment(str) << comment(str);
        EXPECT_EQ(
R"(// A comment
// and some more
// A comment
// and some more
0,
// A comment
// and some more
// A comment
// and some more)"s, encoder.finish());
    }
    { // Complex
        Encoder encoder{Density::multiline};
        const std::string_view single{"A comment"};
        const std::string_view multi{"A comment\nand some more"};
        encoder << comment(single);
        encoder << array;
            encoder << comment(multi);
            encoder << comment(single);
            encoder << 0;
            encoder << comment(multi);
            encoder << 1;
            encoder << object;
                encoder << comment(single);
                encoder << "a" << 0;
                encoder << comment(multi);
                encoder << "b" << 1;
                encoder << comment(multi);
                encoder << comment(multi);
                encoder << "c" << 2;
                encoder << "d" << array(Density::uniline) << comment(single) << 0 << comment(single) << comment(single) << array << comment(single) << end << comment(single) << end;
                encoder << "e" << array;
                    encoder << comment(single);
                encoder << end;
                encoder << "f" << array;
                    encoder << comment(multi);
                encoder << end;
                encoder << comment(single);
                encoder << comment(multi);
            encoder << end;
        encoder << comment(single);
        encoder << end;
    encoder << comment(multi);
        EXPECT_EQ(
R"(// A comment
[
    // A comment
    // and some more
    // A comment
    0,
    // A comment
    // and some more
    1,
    {
        // A comment
        "a": 0,
        // A comment
        // and some more
        "b": 1,
        // A comment
        // and some more
        // A comment
        // and some more
        "c": 2,
        "d": [ /* A comment */ 0, /* A comment */ /* A comment */ [ /* A comment */ ], /* A comment */ ],
        "e": [
            // A comment
        ],
        "f": [
            // A comment
            // and some more
        ],
        // A comment
        // A comment
        // and some more
    },
    // A comment
],
// A comment
// and some more)"s, encoder.finish());
    }
    { // Comment between key and value
        Encoder encoder{};
        encoder << object << "k";
        EXPECT_THROW(encoder << comment("nope"), EncodeError);
    }
    { // Escape sequence in block comment
        Encoder encoder{Density::uniline};
        encoder << comment("A comment /* and some more") << nullptr;
        EXPECT_EQ(R"(/* A comment /* and some more */ null)", encoder.finish());
        EXPECT_THROW(encoder << comment("A comment */ and some more"), EncodeError);
    }
    { // Escape sequence in line comment
        Encoder encoder{Density::multiline};
        encoder << comment("A comment /* and some more") << nullptr;
        EXPECT_EQ(
R"(// A comment /* and some more
null)", encoder.finish());
        encoder << comment("A comment */ and some more") << nullptr;
        EXPECT_EQ(
R"(// A comment */ and some more
null)", encoder.finish());
    }
    { // Invalid characters
        Encoder encoder{Density::uniline};
        EXPECT_THROW(encoder << comment("A\rcomment"sv), EncodeError);
        EXPECT_THROW(encoder << comment("A\tcomment"sv), EncodeError);
        EXPECT_THROW(encoder << comment("A\0comment"sv), EncodeError);
    }
    { // Nospace
        Encoder encoder{Density::nospace};
        encoder << comment("A comment") << array << comment("A comment") << comment("A comment") << 0 << comment("A comment") << end << comment("A comment");
        EXPECT_EQ("/*A comment*/[/*A comment*//*A comment*/0,/*A comment*/],/*A comment*/", encoder.finish());
    }
    { // Weird
        Encoder encoder{Density::multiline};
        encoder << comment("") << nullptr;
        EXPECT_EQ("// \nnull", encoder.finish());
        encoder << comment("\n") << nullptr;
        EXPECT_EQ("// \n// \nnull", encoder.finish());
        encoder << comment("a\nb\nc") << nullptr;
        EXPECT_EQ("// a\n// b\n// c\nnull", encoder.finish());
        encoder << comment("\n\n\n") << nullptr;
        EXPECT_EQ("// \n// \n// \n// \nnull", encoder.finish());
    }
}

TEST(encode, misc)
{
    { // Extraneous content
        Encoder encoder{};
        encoder << "a";
        EXPECT_THROW(encoder << "b", EncodeError);
    }
}

TEST(encode, general)
{
    Encoder encoder{};
    encoder << comment("Third quarter summary document\nProtected information, do not propagate!"sv);
    encoder << object;
        encoder << "Name"sv << "Salt's Crust"sv;
        encoder << "Founded"sv << 1964;
        encoder << comment("Not necessarily up to date"sv) << "Employees"sv << array;
            encoder << object(Density::uniline) << "Name"sv << "Ol' Joe Fisher"sv << "Title"sv << "Fisherman"sv << "Age"sv << 69 << end;
            encoder << object(Density::uniline) << "Name"sv << "Mark Rower"sv << "Title"sv << "Cook"sv << "Age"sv << 41 << end;
            encoder << object(Density::uniline) << "Name"sv << "Phineas"sv << "Title"sv << "Server Boy"sv << "Age"sv << 19 << end;
        encoder << end;
        encoder << "Dishes"sv << array;
            encoder << object;
                encoder << "Name"sv << "Basket o' Barnacles"sv;
                encoder << "Price"sv << 5.45;
                encoder << "Ingredients"sv << array(Density::uniline) << "\"Salt\""sv << "Barnacles"sv << end;
                encoder << "Gluten Free"sv << false;
            encoder << end;
            encoder << object;
                encoder << "Name"sv << "Two Tuna"sv;
                encoder << "Price"sv << -std::numeric_limits<double>::infinity();
                encoder << "Ingredients"sv << array(Density::uniline) << "Tuna"sv << comment("It's actually cod lmao") << end;
                encoder << "Gluten Free"sv << true;
            encoder << end;
            encoder << object;
                encoder << "Name"sv << "18 Leg Bouquet"sv;
                encoder << "Price"sv << std::numeric_limits<double>::quiet_NaN();
                encoder << "Ingredients"sv << array(Density::uniline) << "\"Salt\""sv << "Octopus"sv << "Crab"sv << end;
                encoder << "Gluten Free"sv << false;
            encoder << end;
        encoder << end;
        encoder << "Profit Margin"sv << nullptr;
        encoder << "Ha\x03r Name"sv << "M\0\0n"sv;
        encoder << "Green Eggs and Ham"sv <<
R"(I do not like them in a box
I do not like them with a fox
I do not like them in a house
I do not like them with a mouse
I do not like them here or there
I do not like them anywhere
I do not like green eggs and ham
I do not like them Sam I am
)";
        encoder << "Magic Numbers"sv << array(Density::nospace) << hex(777) << octal(777u) << binary(777) << end;
    encoder << end;

    EXPECT_EQ(R"(// Third quarter summary document
// Protected information, do not propagate!
{
    "Name": "Salt's Crust",
    "Founded": 1964,
    // Not necessarily up to date
    "Employees": [
        { "Name": "Ol' Joe Fisher", "Title": "Fisherman", "Age": 69 },
        { "Name": "Mark Rower", "Title": "Cook", "Age": 41 },
        { "Name": "Phineas", "Title": "Server Boy", "Age": 19 }
    ],
    "Dishes": [
        {
            "Name": "Basket o' Barnacles",
            "Price": 5.45,
            "Ingredients": [ "\"Salt\"", "Barnacles" ],
            "Gluten Free": false
        },
        {
            "Name": "Two Tuna",
            "Price": -inf,
            "Ingredients": [ "Tuna", /* It's actually cod lmao */ ],
            "Gluten Free": true
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": nan,
            "Ingredients": [ "\"Salt\"", "Octopus", "Crab" ],
            "Gluten Free": false
        }
    ],
    "Profit Margin": null,
    "Ha\x03r Name": "M\0\0n",
    "Green Eggs and Ham": "I do not like them in a box\nI do not like them with a fox\nI do not like them in a house\nI do not like them with a mouse\nI do not like them here or there\nI do not like them anywhere\nI do not like green eggs and ham\nI do not like them Sam I am\n",
    "Magic Numbers": [0x309,0o1411,0b1100001001]
})"s, encoder.finish());
}
