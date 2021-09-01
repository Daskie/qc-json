#include <gtest/gtest.h>

#include <qc-json/qc-json-encode.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::Encoder;
using qc::json::EncodeError;
using namespace qc::json::tokens;

struct CustomVal { int x, y; };

Encoder & operator<<(Encoder & encoder, const CustomVal & v)
{
    return encoder << array << uniline << v.x << v.y << end;
}

TEST(encode, object) {
    { // Empty
        Encoder encoder{};
        encoder << object << multiline << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
        encoder << object << uniline << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
        encoder << object << compact << end;
        EXPECT_EQ(R"({})"s, encoder.finish());
    }
    { // Non-empty
        Encoder encoder{};
        encoder << object << multiline << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({
    "k1": "abc",
    "k2": 123,
    "k3": true
})"s, encoder.finish());
        encoder << object << uniline << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({ "k1": "abc", "k2": 123, "k3": true })"s, encoder.finish());
        encoder << object << compact << "k1" << "abc" << "k2" << 123 << "k3" << true << end;
        EXPECT_EQ(R"({"k1":"abc","k2":123,"k3":true})"s, encoder.finish());
    }
    { // String view key
        Encoder encoder{uniline};
        encoder << object << "k"sv << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // String key
        Encoder encoder{uniline};
        encoder << object << "k"s << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Const C string key
        Encoder encoder{uniline};
        encoder << object << "k" << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Mutable C string key
        Encoder encoder{uniline};
        encoder << object << const_cast<char *>("k") << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Character key
        Encoder encoder{uniline};
        encoder << object << 'k' << "v" << end;
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Empty key
        Encoder encoder{};
        encoder << object;
        EXPECT_THROW(encoder << "", EncodeError);
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

TEST(encode, array) {
    { // Empty
        Encoder encoder{};
        encoder << array << multiline << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
        encoder << array << uniline << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
        encoder << array << compact << end;
        EXPECT_EQ(R"([])"s, encoder.finish());
    }
    { // Non-empty
        Encoder encoder{};
        encoder << array << multiline << "abc" << 123 << true << end;
        EXPECT_EQ(R"([
    "abc",
    123,
    true
])"s, encoder.finish());
        encoder << array << uniline << "abc" << 123 << true << end;
        EXPECT_EQ(R"([ "abc", 123, true ])"s, encoder.finish());
        encoder << array << compact << "abc" << 123 << true << end;
        EXPECT_EQ(R"(["abc",123,true])"s, encoder.finish());
    }
}

TEST(encode, string) {
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
        encoder << "\b\f\n\r\t";
        EXPECT_EQ(R"("\b\f\n\r\t")"s, encoder.finish());
    }
    { // Unicode
        Encoder encoder{};
        encoder << "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv;
        EXPECT_EQ(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"s, encoder.finish());
    }
    { // Single char
        Encoder encoder{};
        encoder << 'a';
        EXPECT_EQ(R"("a")"s, encoder.finish());
    }
    { // Non-ASCII unicode
        for (char c(-128); c < 0; ++c) {
            Encoder encoder{};
            EXPECT_THROW(encoder << c, EncodeError);
        }
    }
}

TEST(encode, signedInteger) {
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

TEST(encode, unsignedInteger) {
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

TEST(encode, floater) {
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

TEST(encode, boolean) {
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

TEST(encode, null) {
    Encoder encoder{};
    encoder << nullptr;
    EXPECT_EQ(R"(null)"s, encoder.finish());
}

TEST(encode, custom) {
    Encoder encoder{};
    encoder << CustomVal{1, 2};
    EXPECT_EQ(R"([ 1, 2 ])"s, encoder.finish());
}

TEST(encode, finish) {
    { // Encoder left in clean state after finish
        Encoder encoder{uniline};
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

TEST(encode, density) {
    { // Top level multiline
        Encoder encoder{multiline};
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
        Encoder encoder{uniline};
        encoder << object << "k1" << array << "v1" << "v2" << end << "k2" << "v3" << end;
        EXPECT_EQ(R"({ "k1": [ "v1", "v2" ], "k2": "v3" })"s, encoder.finish());
    }
    { // Top level compact
        Encoder encoder{compact};
        encoder << object << "k1" << array << "v1" << "v2" << end << "k2" << "v3" << end;
        EXPECT_EQ(R"({"k1":["v1","v2"],"k2":"v3"})"s, encoder.finish());
    }
    { // Inner density
        Encoder encoder{};
        encoder << object;
            encoder << "k1" << array << uniline << "v1" << array << compact << "v2" << "v3" << end << end;
            encoder << "k2" << object << uniline << "k3" << "v4" << "k4" << object << compact << "k5" << "v5" << "k6" << "v6" << end << end;
        encoder << end;
        EXPECT_EQ(R"({
    "k1": [ "v1", ["v2","v3"] ],
    "k2": { "k3": "v4", "k4": {"k5":"v5","k6":"v6"} }
})"s, encoder.finish());
    }
    { // Density priority
        Encoder encoder{};
        encoder << object << uniline << "k" << array << multiline << "v" << end << end;
        EXPECT_EQ(R"({ "k": [ "v" ] })"s, encoder.finish());
        encoder << array << uniline << object << multiline << "k" << "v" << end << end;
        EXPECT_EQ(R"([ { "k": "v" } ])"s, encoder.finish());
        encoder << object << compact << "k" << array << uniline << "v" << end << end;
        EXPECT_EQ(R"({"k":["v"]})"s, encoder.finish());
        encoder << array << compact << object << uniline << "k" << "v" << end << end;
        EXPECT_EQ(R"([{"k":"v"}])"s, encoder.finish());
    }
    { // Missplaced density
        Encoder encoder{};
        EXPECT_THROW(encoder << multiline, EncodeError);
        encoder << object << "k";
        EXPECT_THROW(encoder << uniline, EncodeError);
        encoder << "v";
        EXPECT_THROW(encoder << compact, EncodeError);
        encoder << "arr" << array << 123;
        EXPECT_THROW(encoder << multiline, EncodeError);
        encoder << end << end;
        EXPECT_THROW(encoder << uniline, EncodeError);
    }
}

TEST(encode, misc) {
    { // Extraneous content
        Encoder encoder{};
        encoder << "a";
        EXPECT_THROW(encoder << "b", EncodeError);
    }
}

TEST(encode, general) {
    Encoder encoder{};
    encoder << object;
        encoder << "Name"sv << "Salt's Crust"sv;
        encoder << "Founded"sv << 1964;
        encoder << "Employees"sv << array;
            encoder << object << uniline << "Name"sv << "Ol' Joe Fisher"sv << "Title"sv << "Fisherman"sv << "Age"sv << 69 << end;
            encoder << object << uniline << "Name"sv << "Mark Rower"sv << "Title"sv << "Cook"sv << "Age"sv << 41 << end;
            encoder << object << uniline << "Name"sv << "Phineas"sv << "Title"sv << "Server Boy"sv << "Age"sv << 19 << end;
        encoder << end;
        encoder << "Dishes"sv << array;
            encoder << object;
                encoder << "Name"sv << "Basket o' Barnacles"sv;
                encoder << "Price"sv << 5.45;
                encoder << "Ingredients"sv << array << uniline << "Salt"sv << "Barnacles"sv << end;
                encoder << "Gluten Free"sv << false;
            encoder << end;
            encoder << object;
                encoder << "Name"sv << "Two Tuna"sv;
                encoder << "Price"sv << 14.99;
                encoder << "Ingredients"sv << array << uniline << "Tuna"sv << end;
                encoder << "Gluten Free"sv << true;
            encoder << end;
            encoder << object;
                encoder << "Name"sv << "18 Leg Bouquet"sv;
                encoder << "Price"sv << 18.18;
                encoder << "Ingredients"sv << array << uniline << "Salt"sv << "Octopus"sv << "Crab"sv << end;
                encoder << "Gluten Free"sv << false;
            encoder << end;
        encoder << end;
        encoder << "Profit Margin"sv << nullptr;
    encoder << end;

    EXPECT_EQ(R"({
    "Name": "Salt's Crust",
    "Founded": 1964,
    "Employees": [
        { "Name": "Ol' Joe Fisher", "Title": "Fisherman", "Age": 69 },
        { "Name": "Mark Rower", "Title": "Cook", "Age": 41 },
        { "Name": "Phineas", "Title": "Server Boy", "Age": 19 }
    ],
    "Dishes": [
        {
            "Name": "Basket o' Barnacles",
            "Price": 5.45,
            "Ingredients": [ "Salt", "Barnacles" ],
            "Gluten Free": false
        },
        {
            "Name": "Two Tuna",
            "Price": 14.99,
            "Ingredients": [ "Tuna" ],
            "Gluten Free": true
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": 18.18,
            "Ingredients": [ "Salt", "Octopus", "Crab" ],
            "Gluten Free": false
        }
    ],
    "Profit Margin": null
})"s, encoder.finish());
}
