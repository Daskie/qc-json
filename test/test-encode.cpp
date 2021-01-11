#include <gtest/gtest.h>

#include <qc-json/qc-json-encode.hpp>

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

// TODO: Remove once GCC gets this sorted
#ifdef _MSC_VER
static constexpr bool charconvSupported{true};
#else
static constexpr bool charconvSupported{false};
#endif

struct CustomVal { int x, y; };

template <>
struct qc_json_encode<CustomVal> {
    void operator()(qc_json::Encoder & encoder, const CustomVal & v) const {
        encoder.array(true).val(v.x).val(v.y).end();
    }
};

TEST(encode, object) {
    { // Empty non-compact
        qc_json::Encoder encoder;
        encoder.object(false).end();
        EXPECT_EQ(R"({})"s, encoder.finish());
    }
    { // Empty compact
        qc_json::Encoder encoder;
        encoder.object(true).end();
        EXPECT_EQ(R"({})"s, encoder.finish());
    }
    { // Non-compact
        qc_json::Encoder encoder;
        encoder.object(false).key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
        EXPECT_EQ(R"({
    "k1": "abc",
    "k2": 123,
    "k3": true
})"s, encoder.finish());
    }
    { // Compact
        qc_json::Encoder encoder;
        encoder.object(true).key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
        EXPECT_EQ(R"({ "k1": "abc", "k2": 123, "k3": true })"s, encoder.finish());
    }
    { // String view key
        qc_json::Encoder encoder;
        encoder.object(true).key("k"sv).val("v").end();
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // String key
        qc_json::Encoder encoder;
        encoder.object(true).key("k"s).val("v").end();
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Const C string key
        qc_json::Encoder encoder;
        encoder.object(true).key("k").val("v").end();
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Mutable C string key
        qc_json::Encoder encoder;
        encoder.object(true).key(const_cast<char *>("k")).val("v").end();
        EXPECT_EQ(R"({ "k": "v" })"s, encoder.finish());
    }
    { // Empty key
        qc_json::Encoder encoder;
        encoder.object();
        EXPECT_THROW(encoder.key(""), qc_json::EncodeError);
    }
    { // No key
        qc_json::Encoder encoder;
        encoder.object();
        EXPECT_THROW(encoder.val(123), qc_json::EncodeError);
    }
    { // Multiple keys
        qc_json::Encoder encoder;
        encoder.object().key("k1");
        EXPECT_THROW(encoder.key("k2"), qc_json::EncodeError);
    }
    { // Dangling key
        qc_json::Encoder encoder;
        encoder.object().key("k1");
        EXPECT_THROW(encoder.end(), qc_json::EncodeError);
    }
}

TEST(encode, array) {
    { // Empty non-compact
        qc_json::Encoder encoder;
        encoder.array(false).end();
        EXPECT_EQ(R"([])"s, encoder.finish());
    }
    { // Empty compact
        qc_json::Encoder encoder;
        encoder.array(true).end();
        EXPECT_EQ(R"([])"s, encoder.finish());
    }
    { // Compact
        qc_json::Encoder encoder;
        encoder.array(true).val("abc").val(123).val(true).end();
        EXPECT_EQ(R"([ "abc", 123, true ])"s, encoder.finish());
    }
    { // Non-compact
        qc_json::Encoder encoder;
        encoder.array(false).val("abc").val(123).val(true).end();
        EXPECT_EQ(R"([
    "abc",
    123,
    true
])"s, encoder.finish());
    }
    { // Compact
        qc_json::Encoder encoder;
        encoder.array(true).val("abc").val(123).val(true).end();
        EXPECT_EQ(R"([ "abc", 123, true ])"s, encoder.finish());
    }
    { // Key
        qc_json::Encoder encoder;
        encoder.array();
        EXPECT_THROW(encoder.key("k"), qc_json::EncodeError);
    }
}

TEST(encode, string) {
    { // Empty
        qc_json::Encoder encoder;
        encoder.val("");
        EXPECT_EQ(R"("")"s, encoder.finish());
    }
    { // String view
        qc_json::Encoder encoder;
        encoder.val("hello"sv);
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // String
        qc_json::Encoder encoder;
        encoder.val("hello"s);
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Const C string
        qc_json::Encoder encoder;
        encoder.val("hello");
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Mutable C string
        qc_json::Encoder encoder;
        encoder.val(const_cast<char *>("hello"));
        EXPECT_EQ(R"("hello")"s, encoder.finish());
    }
    { // Printable characters
        qc_json::Encoder encoder;
        const std::string actual{R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"s};
        const std::string expected{R"(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"s};
        encoder.val(actual);
        EXPECT_EQ(expected, encoder.finish());
    }
    { // Escape characters
        qc_json::Encoder encoder;
        encoder.val("\b\f\n\r\t");
        EXPECT_EQ(R"("\b\f\n\r\t")"s, encoder.finish());
    }
    { // Unicode
        qc_json::Encoder encoder;
        encoder.val("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
        EXPECT_EQ(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"s, encoder.finish());
    }
    { // Single char
        qc_json::Encoder encoder;
        encoder.val('a');
        EXPECT_EQ(R"("a")"s, encoder.finish());
    }
    { // Non-ASCII unicode
        for (char c(-128); c < 0; ++c) {
            qc_json::Encoder encoder;
            EXPECT_THROW(encoder.val(c), qc_json::EncodeError);
        }
    }
}

TEST(encode, signedInteger) {
    { // Zero
        qc_json::Encoder encoder;
        encoder.val(int64_t(0));
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        qc_json::Encoder encoder;
        encoder.val(123);
        EXPECT_EQ(R"(123)"s, encoder.finish());
    }
    { // Max 64
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int64_t>::max());
        EXPECT_EQ(R"(9223372036854775807)"s, encoder.finish());
    }
    { // Min 64
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int64_t>::min());
        EXPECT_EQ(R"(-9223372036854775808)"s, encoder.finish());
    }
    { // Max 32
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int32_t>::max());
        EXPECT_EQ(R"(2147483647)"s, encoder.finish());
    }
    { // Min 32
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int32_t>::min());
        EXPECT_EQ(R"(-2147483648)"s, encoder.finish());
    }
    { // Max 16
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int16_t>::max());
        EXPECT_EQ(R"(32767)"s, encoder.finish());
    }
    { // Min 16
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int16_t>::min());
        EXPECT_EQ(R"(-32768)"s, encoder.finish());
    }
    { // Max 8
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int8_t>::max());
        EXPECT_EQ(R"(127)"s, encoder.finish());
    }
    { // Min 8
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<int8_t>::min());
        EXPECT_EQ(R"(-128)"s, encoder.finish());
    }
}

TEST(encode, unsignedInteger) {
    { // Zero
        qc_json::Encoder encoder;
        encoder.val(0u);
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        qc_json::Encoder encoder;
        encoder.val(123u);
        EXPECT_EQ(R"(123)"s, encoder.finish());
    }
    { // Max 64
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<uint64_t>::max());
        EXPECT_EQ(R"(18446744073709551615)"s, encoder.finish());
    }
    { // Max 32
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<uint32_t>::max());
        EXPECT_EQ(R"(4294967295)"s, encoder.finish());
    }
    { // Max 16
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<uint16_t>::max());
        EXPECT_EQ(R"(65535)"s, encoder.finish());
    }
    { // Max 8
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<uint8_t>::max());
        EXPECT_EQ(R"(255)"s, encoder.finish());
    }
}

TEST(encode, floater) {
    { // Zero
        qc_json::Encoder encoder;
        encoder.val(0.0);
        EXPECT_EQ(R"(0)"s, encoder.finish());
    }
    { // Typical
        qc_json::Encoder encoder;
        encoder.val(123.45);
        EXPECT_EQ(R"(123.45)"s, encoder.finish());
    }
    { // Max integer 64
        qc_json::Encoder encoder;
        uint64_t val{0b0'10000110011'1111111111111111111111111111111111111111111111111111u};
        encoder.val(reinterpret_cast<const double &>(val));
        EXPECT_EQ(charconvSupported ? R"(9007199254740991)"s : R"(9.0072e+15)"s, encoder.finish());
    }
    { // Max integer 32
        qc_json::Encoder encoder;
        uint32_t val{0b0'10010110'11111111111111111111111u};
        encoder.val(reinterpret_cast<const float &>(val));
        EXPECT_EQ(charconvSupported ? R"(16777215)"s : R"(1.67772e+07)", encoder.finish());
    }
    { // Max 64
        qc_json::Encoder encoder;
        uint64_t val{0b0'11111111110'1111111111111111111111111111111111111111111111111111u};
        encoder.val(reinterpret_cast<const double &>(val));
        EXPECT_EQ(charconvSupported ? R"(1.7976931348623157e+308)"s : R"(1.79769e+308)", encoder.finish());
    }
    { // Max 32
        qc_json::Encoder encoder;
        uint32_t val{0b0'11111110'11111111111111111111111u};
        encoder.val(reinterpret_cast<const float &>(val));
        EXPECT_EQ(charconvSupported ? R"(3.4028234663852886e+38)"s : R"(3.40282e+38)", encoder.finish());
    }
    { // Min normal 64
        qc_json::Encoder encoder;
        uint64_t val{0b0'00000000001'0000000000000000000000000000000000000000000000000000u};
        encoder.val(reinterpret_cast<const double &>(val));
        EXPECT_EQ(charconvSupported ? R"(2.2250738585072014e-308)"s : R"(2.22507e-308)", encoder.finish());
    }
    { // Min normal 32
        qc_json::Encoder encoder;
        uint32_t val{0b0'00000001'00000000000000000000000u};
        encoder.val(reinterpret_cast<const float &>(val));
        EXPECT_EQ(charconvSupported ? R"(1.1754943508222875e-38)"s : R"(1.17549e-38)", encoder.finish());
    }
    { // Min subnormal 64
        qc_json::Encoder encoder;
        uint64_t val{0b0'00000000000'0000000000000000000000000000000000000000000000000001u};
        encoder.val(reinterpret_cast<const double &>(val));
        EXPECT_EQ(charconvSupported ? R"(5e-324)"s : R"(4.94066e-324)", encoder.finish());
    }
    { // Min subnormal 32
        qc_json::Encoder encoder;
        uint64_t val{0b0'00000000'00000000000000000000001u};
        encoder.val(reinterpret_cast<const float &>(val));
        EXPECT_EQ(charconvSupported ? R"(1.401298464324817e-45)"s : R"(1.4013e-45)", encoder.finish());
    }
    { // Positive infinity
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<double>::infinity());
        EXPECT_EQ(R"(inf)"s, encoder.finish());
    }
    { // Negative infinity
        qc_json::Encoder encoder;
        encoder.val(-std::numeric_limits<double>::infinity());
        EXPECT_EQ(R"(-inf)"s, encoder.finish());
    }
    { // NaN
        qc_json::Encoder encoder;
        encoder.val(std::numeric_limits<double>::quiet_NaN());
        EXPECT_EQ(R"(nan)"s, encoder.finish());
    }
}

TEST(encode, boolean) {
    { // True
        qc_json::Encoder encoder;
        encoder.val(true);
        EXPECT_EQ(R"(true)"s, encoder.finish());
    }
    { // False
        qc_json::Encoder encoder;
        encoder.val(false);
        EXPECT_EQ(R"(false)"s, encoder.finish());
    }
}

TEST(encode, null) {
    qc_json::Encoder encoder;
    encoder.val(nullptr);
    EXPECT_EQ(R"(null)"s, encoder.finish());
}

TEST(encode, custom) {
    qc_json::Encoder encoder;
    encoder.val(CustomVal{1, 2});
    EXPECT_EQ(R"([ 1, 2 ])"s, encoder.finish());
}

TEST(encode, finish) {
    { // Encoder left in clean state after finish
        qc_json::Encoder encoder;
        encoder.object(true).key("val").val(123).end();
        EXPECT_EQ(R"({ "val": 123 })"s, encoder.finish());
        encoder.array(true).val(321).end();
        EXPECT_EQ(R"([ 321 ])"s, encoder.finish());
    }
    { // Finishing at root
        qc_json::Encoder encoder;
        EXPECT_THROW(encoder.finish(), qc_json::EncodeError);
    }
    { // Finishing in object
        qc_json::Encoder encoder;
        encoder.object();
        EXPECT_THROW(encoder.finish(), qc_json::EncodeError);
    }
    { // Finishing in array
        qc_json::Encoder encoder;
        encoder.array();
        EXPECT_THROW(encoder.finish(), qc_json::EncodeError);
    }
}

TEST(encode, general) {
    qc_json::Encoder encoder;
    encoder.object();
        encoder.key("Name"sv).val("Salt's Crust"sv);
        encoder.key("Founded"sv).val(1964);
        encoder.key("Employees"sv).array();
            encoder.object(true).key("Name"sv).val("Ol' Joe Fisher"sv).key("Title"sv).val("Fisherman"sv).key("Age"sv).val(69).end();
            encoder.object(true).key("Name"sv).val("Mark Rower"sv).key("Title"sv).val("Cook"sv).key("Age"sv).val(41).end();
            encoder.object(true).key("Name"sv).val("Phineas"sv).key("Title"sv).val("Server Boy"sv).key("Age"sv).val(19).end();
        encoder.end();
        encoder.key("Dishes"sv).array();
            encoder.object();
                encoder.key("Name"sv).val("Basket o' Barnacles"sv);
                encoder.key("Price"sv).val(5.45);
                encoder.key("Ingredients"sv).array(true).val("Salt"sv).val("Barnacles"sv).end();
                encoder.key("Gluten Free"sv).val(false);
            encoder.end();
            encoder.object();
                encoder.key("Name"sv).val("Two Tuna"sv);
                encoder.key("Price"sv).val(14.99);
                encoder.key("Ingredients"sv).array(true).val("Tuna"sv).end();
                encoder.key("Gluten Free"sv).val(true);
            encoder.end();
            encoder.object();
                encoder.key("Name"sv).val("18 Leg Bouquet"sv);
                encoder.key("Price"sv).val(18.18);
                encoder.key("Ingredients"sv).array(true).val("Salt"sv).val("Octopus"sv).val("Crab"sv).end();
                encoder.key("Gluten Free"sv).val(false);
            encoder.end();
        encoder.end();
        encoder.key("Profit Margin"sv).val(nullptr);
    encoder.end();

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

TEST(encode, misc) {
    { // Extraneous content
        qc_json::Encoder encoder;
        encoder.val("a");
        EXPECT_THROW(encoder.val("b"), qc_json::EncodeError);
    }
    { // Compactness priority
        qc_json::Encoder encoder;
        encoder.object(true).key("k").array(false).val("v").end().end();
        EXPECT_EQ(R"({ "k": [ "v" ] })"s, encoder.finish());
        encoder.array(true).object(false).key("k").val("v").end().end();
        EXPECT_EQ(R"([ { "k": "v" } ])"s, encoder.finish());
    }
}
