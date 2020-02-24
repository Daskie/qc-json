#include "CppUnitTest.h"

#include "QJsonEncode.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

struct CustomVal { int x, y; };

void qjson_encode(qjson::Encoder & encoder, const CustomVal & v) {
    encoder.array(true).val(v.x).val(v.y).end();
}

TEST_CLASS(Encode) {

  public:

    TEST_METHOD(Object) {
        { // Empty non-compact
            qjson::Encoder encoder;
            encoder.object(false).end();
            Assert::AreEqual(R"({})"s, encoder.finish());
        }
        { // Empty compact
            qjson::Encoder encoder;
            encoder.object(true).end();
            Assert::AreEqual(R"({})"s, encoder.finish());
        }
        { // Non-compact
            qjson::Encoder encoder;
            encoder.object(false).key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
            Assert::AreEqual(R"({
    "k1": "abc",
    "k2": 123,
    "k3": true
})"s, encoder.finish());
        }
        { // Compact
            qjson::Encoder encoder;
            encoder.object(true).key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
            Assert::AreEqual(R"({ "k1": "abc", "k2": 123, "k3": true })"s, encoder.finish());
        }
        { // String view key
            qjson::Encoder encoder;
            encoder.object(true).key("k"sv).val("v").end();
            Assert::AreEqual(R"({ "k": "v" })"s, encoder.finish());
        }
        { // String key
            qjson::Encoder encoder;
            encoder.object(true).key("k"s).val("v").end();
            Assert::AreEqual(R"({ "k": "v" })"s, encoder.finish());
        }
        { // Const C string key
            qjson::Encoder encoder;
            encoder.object(true).key("k").val("v").end();
            Assert::AreEqual(R"({ "k": "v" })"s, encoder.finish());
        }
        { // Mutable C string key
            qjson::Encoder encoder;
            encoder.object(true).key(const_cast<char *>("k")).val("v").end();
            Assert::AreEqual(R"({ "k": "v" })"s, encoder.finish());
        }
        { // Empty key
            qjson::Encoder encoder;
            encoder.object();
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.key(""); });
        }
        { // No key
            qjson::Encoder encoder;
            encoder.object();
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.val(123); });
        }
        { // Multiple keys
            qjson::Encoder encoder;
            encoder.object().key("k1");
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.key("k2"); });
        }
        { // Dangling key
            qjson::Encoder encoder;
            encoder.object().key("k1");
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.end(); });
        }
    }

    TEST_METHOD(Array) {
        { // Empty non-compact
            qjson::Encoder encoder;
            encoder.array(false).end();
            Assert::AreEqual(R"([])"s, encoder.finish());
        }
        { // Empty compact
            qjson::Encoder encoder;
            encoder.array(true).end();
            Assert::AreEqual(R"([])"s, encoder.finish());
        }
        { // Compact
            qjson::Encoder encoder;
            encoder.array(true).val("abc").val(123).val(true).end();
            Assert::AreEqual(R"([ "abc", 123, true ])"s, encoder.finish());
        }
        { // Non-compact
            qjson::Encoder encoder;
            encoder.array(false).val("abc").val(123).val(true).end();
            Assert::AreEqual(R"([
    "abc",
    123,
    true
])"s, encoder.finish());
        }
        { // Compact
            qjson::Encoder encoder;
            encoder.array(true).val("abc").val(123).val(true).end();
            Assert::AreEqual(R"([ "abc", 123, true ])"s, encoder.finish());
        }
        { // Key
            qjson::Encoder encoder;
            encoder.array();
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.key("k"); });
        }
    }

    TEST_METHOD(String) {
        { // Empty
            qjson::Encoder encoder;
            encoder.val("");
            Assert::AreEqual(R"("")"s, encoder.finish());
        }
        { // String view
            qjson::Encoder encoder;
            encoder.val("hello"sv);
            Assert::AreEqual(R"("hello")"s, encoder.finish());
        }
        { // String
            qjson::Encoder encoder;
            encoder.val("hello"s);
            Assert::AreEqual(R"("hello")"s, encoder.finish());
        }
        { // Const C string
            qjson::Encoder encoder;
            encoder.val("hello");
            Assert::AreEqual(R"("hello")"s, encoder.finish());
        }
        { // Mutable C string
            qjson::Encoder encoder;
            encoder.val(const_cast<char *>("hello"));
            Assert::AreEqual(R"("hello")"s, encoder.finish());
        }
        { // Printable characters
            qjson::Encoder encoder;
            encoder.val(R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)");
            Assert::AreEqual(R"(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"s, encoder.finish());
        }
        { // Escape characters
            qjson::Encoder encoder;
            encoder.val("\b\f\n\r\t");
            Assert::AreEqual(R"("\b\f\n\r\t")"s, encoder.finish());
        }
        { // Unicode
            qjson::Encoder encoder;
            encoder.val("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
            Assert::AreEqual(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"s, encoder.finish());
        }
        { // Single char
            qjson::Encoder encoder;
            encoder.val('a');
            Assert::AreEqual(R"("a")"s, encoder.finish());
        }
        { // Non-ASCII unicode
            for (char c(-128); c < 0; ++c) {
                qjson::Encoder encoder;
                Assert::ExpectException<qjson::EncodeError>([&]() { encoder.val(c); });
            }
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            qjson::Encoder encoder;
            encoder.val(int64_t(0));
            Assert::AreEqual(R"(0)"s, encoder.finish());
        }
        { // Typical
            qjson::Encoder encoder;
            encoder.val(123);
            Assert::AreEqual(R"(123)"s, encoder.finish());
        }
        { // Max 64
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int64_t>::max());
            Assert::AreEqual(R"(9223372036854775807)"s, encoder.finish());
        }
        { // Min 64
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int64_t>::min());
            Assert::AreEqual(R"(-9223372036854775808)"s, encoder.finish());
        }
        { // Max 32
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int32_t>::max());
            Assert::AreEqual(R"(2147483647)"s, encoder.finish());
        }
        { // Min 32
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int32_t>::min());
            Assert::AreEqual(R"(-2147483648)"s, encoder.finish());
        }
        { // Max 16
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int16_t>::max());
            Assert::AreEqual(R"(32767)"s, encoder.finish());
        }
        { // Min 16
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int16_t>::min());
            Assert::AreEqual(R"(-32768)"s, encoder.finish());
        }
        { // Max 8
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int8_t>::max());
            Assert::AreEqual(R"(127)"s, encoder.finish());
        }
        { // Min 8
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<int8_t>::min());
            Assert::AreEqual(R"(-128)"s, encoder.finish());
        }
    }

    TEST_METHOD(Hex) {
        { // Zero
            qjson::Encoder encoder;
            encoder.val(0u);
            Assert::AreEqual(R"(0x0)"s, encoder.finish());
        }
        { // Typical
            qjson::Encoder encoder;
            encoder.val(0x123ABCu);
            Assert::AreEqual(R"(0x123ABC)"s, encoder.finish());
        }
        { // Max 64
            qjson::Encoder encoder;
            encoder.val(uint64_t(0xFFFFFFFFFFFFFFFFu));
            Assert::AreEqual(R"(0xFFFFFFFFFFFFFFFF)"s, encoder.finish());
        }
        { // Max 32
            qjson::Encoder encoder;
            encoder.val(uint32_t(0xFFFFFFFFu));
            Assert::AreEqual(R"(0xFFFFFFFF)"s, encoder.finish());
        }
        { // Max 16
            qjson::Encoder encoder;
            encoder.val(uint16_t(0xFFFFu));
            Assert::AreEqual(R"(0xFFFF)"s, encoder.finish());
        }
        { // Max 8
            qjson::Encoder encoder;
            encoder.val(uint8_t(0xFFu));
            Assert::AreEqual(R"(0xFF)"s, encoder.finish());
        }
    }

    TEST_METHOD(Floater) {
        { // Zero
            qjson::Encoder encoder;
            encoder.val(0.0);
            Assert::AreEqual(R"(0.0)"s, encoder.finish());
        }
        { // Typical
            qjson::Encoder encoder;
            encoder.val(123.45);
            Assert::AreEqual(R"(123.45)"s, encoder.finish());
        }
        { // Max integer 64
            qjson::Encoder encoder;
            uint64_t val(0b0'10000110011'1111111111111111111111111111111111111111111111111111u);
            encoder.val(reinterpret_cast<const double &>(val));
            Assert::AreEqual(R"(9007199254740991.0)"s, encoder.finish());
        }
        { // Max integer 32
            qjson::Encoder encoder;
            uint32_t val(0b0'10010110'11111111111111111111111u);
            encoder.val(reinterpret_cast<const float &>(val));
            Assert::AreEqual(R"(16777215.0)"s, encoder.finish());
        }
        { // Max 64
            qjson::Encoder encoder;
            uint64_t val(0b0'11111111110'1111111111111111111111111111111111111111111111111111u);
            encoder.val(reinterpret_cast<const double &>(val));
            Assert::AreEqual(R"(1.7976931348623157e+308)"s, encoder.finish());
        }
        { // Max 32
            qjson::Encoder encoder;
            uint32_t val(0b0'11111110'11111111111111111111111u);
            encoder.val(reinterpret_cast<const float &>(val));
            Assert::AreEqual(R"(3.4028234663852886e+38)"s, encoder.finish());
        }
        { // Min normal 64
            qjson::Encoder encoder;
            uint64_t val(0b0'00000000001'0000000000000000000000000000000000000000000000000000u);
            encoder.val(reinterpret_cast<const double &>(val));
            Assert::AreEqual(R"(2.2250738585072014e-308)"s, encoder.finish());
        }
        { // Min normal 32
            qjson::Encoder encoder;
            uint32_t val(0b0'00000001'00000000000000000000000u);
            encoder.val(reinterpret_cast<const float &>(val));
            Assert::AreEqual(R"(1.1754943508222875e-38)"s, encoder.finish());
        }
        { // Min subnormal 64
            qjson::Encoder encoder;
            uint64_t val(0b0'00000000000'0000000000000000000000000000000000000000000000000001u);
            encoder.val(reinterpret_cast<const double &>(val));
            Assert::AreEqual(R"(5e-324)"s, encoder.finish());
        }
        { // Min subnormal 32
            qjson::Encoder encoder;
            uint64_t val(0b0'00000000'00000000000000000000001u);
            encoder.val(reinterpret_cast<const float &>(val));
            Assert::AreEqual(R"(1.401298464324817e-45)"s, encoder.finish());
        }
        { // Positive infinity
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<double>::infinity());
            Assert::AreEqual(R"(inf)"s, encoder.finish());
        }
        { // Negative infinity
            qjson::Encoder encoder;
            encoder.val(-std::numeric_limits<double>::infinity());
            Assert::AreEqual(R"(-inf)"s, encoder.finish());
        }
        { // NaN
            qjson::Encoder encoder;
            encoder.val(std::numeric_limits<double>::quiet_NaN());
            Assert::AreEqual(R"(nan)"s, encoder.finish());
        }
    }

    TEST_METHOD(Boolean) {
        { // True
            qjson::Encoder encoder;
            encoder.val(true);
            Assert::AreEqual(R"(true)"s, encoder.finish());
        }
        { // False
            qjson::Encoder encoder;
            encoder.val(false);
            Assert::AreEqual(R"(false)"s, encoder.finish());
        }
    }

    TEST_METHOD(Null) {
        qjson::Encoder encoder;
        encoder.val(nullptr);
        Assert::AreEqual(R"(null)"s, encoder.finish());
    }

    TEST_METHOD(Custom) {
        qjson::Encoder encoder;
        encoder.val(CustomVal{1, 2});
        Assert::AreEqual(R"([ 1, 2 ])"s, encoder.finish());
    }

    TEST_METHOD(Finish) {
        { // Encoder left in clean state after finish
            qjson::Encoder encoder;
            encoder.object(true).key("val").val(123).end();
            Assert::AreEqual(R"({ "val": 123 })"s, encoder.finish());
            encoder.array(true).val(321).end();
            Assert::AreEqual(R"([ 321 ])"s, encoder.finish());
        }
        { // Finishing at root
            qjson::Encoder encoder;
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.finish(); });
        }
        { // Finishing in object
            qjson::Encoder encoder;
            encoder.object();
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.finish(); });
        }
        { // Finishing in array
            qjson::Encoder encoder;
            encoder.array();
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.finish(); });
        }
    }

    TEST_METHOD(General) {
        qjson::Encoder encoder;
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
                    encoder.key("Code"sv).val(0x8080u);
                encoder.end();
                encoder.object();
                    encoder.key("Name"sv).val("Two Tuna"sv);
                    encoder.key("Price"sv).val(14.99);
                    encoder.key("Ingredients"sv).array(true).val("Tuna"sv).end();
                    encoder.key("Gluten Free"sv).val(true);
                    encoder.key("Code"sv).val(0xA034u);
                encoder.end();
                encoder.object();
                    encoder.key("Name"sv).val("18 Leg Bouquet"sv);
                    encoder.key("Price"sv).val(18.00);
                    encoder.key("Ingredients"sv).array(true).val("Salt"sv).val("Octopus"sv).val("Crab"sv).end();
                    encoder.key("Gluten Free"sv).val(false);
                    encoder.key("Code"sv).val(0x17E4u);
                encoder.end();
            encoder.end();
            encoder.key("Profit Margin"sv).val(nullptr);
        encoder.end();

        Assert::AreEqual(R"({
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
            "Gluten Free": false,
            "Code": 0x8080
        },
        {
            "Name": "Two Tuna",
            "Price": 14.99,
            "Ingredients": [ "Tuna" ],
            "Gluten Free": true,
            "Code": 0xA034
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": 18.0,
            "Ingredients": [ "Salt", "Octopus", "Crab" ],
            "Gluten Free": false,
            "Code": 0x17E4
        }
    ],
    "Profit Margin": null
})"s, encoder.finish());
    }

    TEST_METHOD(Misc) {
        { // Extraneous content
            qjson::Encoder encoder;
            encoder.val("a");
            Assert::ExpectException<qjson::EncodeError>([&]() { encoder.val("b"); });
        }
        { // Compactness priority
            qjson::Encoder encoder;
            encoder.object(true).key("k").array(false).val("v").end().end();
            Assert::AreEqual(R"({ "k": [ "v" ] })"s, encoder.finish());
            encoder.array(true).object(false).key("k").val("v").end().end();
            Assert::AreEqual(R"([ { "k": "v" } ])"s, encoder.finish());
        }
    }

};
