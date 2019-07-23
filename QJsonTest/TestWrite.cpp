#include "CppUnitTest.h"

#include "QJsonWrite.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::Writer;
using qjson::json_exception;

TEST_CLASS(TestWrite) {

    public:
	    
    TEST_METHOD(TestEmpty) {
        Assert::AreEqual("{}"s, Writer(false).finish());
        Assert::AreEqual("{}"s, Writer(true).finish());
    }

    TEST_METHOD(TestPutStringView) {
        Writer writer(true);
        writer.put("v", "hello"sv);
        Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
    }

    TEST_METHOD(TestPutString) {
        Writer writer(true);
        writer.put("v", "hello"s);
        Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
    }

    TEST_METHOD(TestPutCString) {
        { // const
            Writer writer(true);
            const char str[]{"hello"};
            writer.put("v", str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // non-const
            Writer writer(true);
            char str[]{"hello"};
            writer.put("v", str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutChar) {
        Writer writer(true);
        writer.put("v", 'h');
        Assert::AreEqual(R"({ "v": "h" })"s, writer.finish());
    }

    TEST_METHOD(TestPutStringEmpty) {
        Writer writer(true);
        writer.put("v", "");
        Assert::AreEqual(R"({ "v": "" })"s, writer.finish());
    }

    TEST_METHOD(TestPutStringAllPrintable) {
        Writer writer(true);
        writer.put("v", R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)");
        Assert::AreEqual(R"({ "v": " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"s, writer.finish());
    }

    TEST_METHOD(TestPutStringAsciiUnicode) {
        Writer writer(true);
        writer.put("v", "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
        Assert::AreEqual(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"s, writer.finish());
    }

    TEST_METHOD(TestPutStringNonAsciiUnicode) {
        Writer writer(true);
        char str[]{char(128)};
        Assert::ExpectException<json_exception>([&]() { writer.put("v", std::string_view(str, sizeof(str))); });
    }

    TEST_METHOD(TestPutInt64) {
        { // Zero
            Writer writer(true);
            writer.put("v", int64_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<int64_t>::max());
            Assert::AreEqual(R"({ "v": 9223372036854775807 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.put("v", std::numeric_limits<int64_t>::min());
            Assert::AreEqual(R"({ "v": -9223372036854775808 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutInt32) {
        { // Zero
            Writer writer(true);
            writer.put("v", int32_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<int32_t>::max());
            Assert::AreEqual(R"({ "v": 2147483647 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.put("v", std::numeric_limits<int32_t>::min());
            Assert::AreEqual(R"({ "v": -2147483648 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutInt16) {
        { // Zero
            Writer writer(true);
            writer.put("v", int16_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<int16_t>::max());
            Assert::AreEqual(R"({ "v": 32767 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.put("v", std::numeric_limits<int16_t>::min());
            Assert::AreEqual(R"({ "v": -32768 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutInt8) {
        { // Zero
            Writer writer(true);
            writer.put("v", int8_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<int8_t>::max());
            Assert::AreEqual(R"({ "v": 127 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.put("v", std::numeric_limits<int8_t>::min());
            Assert::AreEqual(R"({ "v": -128 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutUInt64) {
        { // Zero
            Writer writer(true);
            writer.put("v", uint64_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<uint64_t>::max());
            Assert::AreEqual(R"({ "v": 18446744073709551615 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutUInt32) {
        { // Zero
            Writer writer(true);
            writer.put("v", uint32_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<uint32_t>::max());
            Assert::AreEqual(R"({ "v": 4294967295 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutUInt16) {
        { // Zero
            Writer writer(true);
            writer.put("v", uint16_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<uint16_t>::max());
            Assert::AreEqual(R"({ "v": 65535 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutUInt8) {
        { // Zero
            Writer writer(true);
            writer.put("v", uint8_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.put("v", std::numeric_limits<uint8_t>::max());
            Assert::AreEqual(R"({ "v": 255 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutDouble) {
        { // Zero
            Writer writer(true);
            writer.put("v", 0.0);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            uint64_t val(0b0111111111101111111111111111111111111111111111111111111111111111ull);
            writer.put("v", reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": 1.7976931348623157e+308 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            uint64_t val(0b1000000000010000000000000000000000000000000000000000000000000000ull);
            writer.put("v", reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": -2.2250738585072014e-308 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.put("v", std::numeric_limits<double>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.put("v", std::numeric_limits<double>::quiet_NaN()); });
        }
    }

    TEST_METHOD(TestPutFloat) {
        { // Zero
            Writer writer(true);
            writer.put("v", 0.0f);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            uint32_t val(0b01111111011111111111111111111111);
            writer.put("v", reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": 3.4028234663852886e+38 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            uint32_t val(0b10000000100000000000000000000000);
            writer.put("v", reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": -1.1754943508222875e-38 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.put("v", std::numeric_limits<float>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.put("v", std::numeric_limits<float>::quiet_NaN()); });
        }
    }

    TEST_METHOD(TestPutBool) {
        { // True
            Writer writer(true);
            writer.put("v", true);
            Assert::AreEqual(R"({ "v": true })"s, writer.finish());
        }
        { // False
            Writer writer(true);
            writer.put("v", false);
            Assert::AreEqual(R"({ "v": false })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutNull) {
        Writer writer(true);
        writer.put("v", nullptr);
        Assert::AreEqual(R"({ "v": null })"s, writer.finish());
    }

    TEST_METHOD(TestPutHexUInt64) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", uint64_t(0));
            Assert::AreEqual(R"({ "v": 0x0000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<uint64_t>::max());
            Assert::AreEqual(R"({ "v": 0xFFFFFFFFFFFFFFFF })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexUInt32) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", uint32_t(0));
            Assert::AreEqual(R"({ "v": 0x00000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<uint32_t>::max());
            Assert::AreEqual(R"({ "v": 0xFFFFFFFF })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexUInt16) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", uint16_t(0));
            Assert::AreEqual(R"({ "v": 0x0000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<uint16_t>::max());
            Assert::AreEqual(R"({ "v": 0xFFFF })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexUInt8) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", uint8_t(0));
            Assert::AreEqual(R"({ "v": 0x00 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<uint8_t>::max());
            Assert::AreEqual(R"({ "v": 0xFF })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexInt64) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", int64_t(0));
            Assert::AreEqual(R"({ "v": 0x0000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int64_t>::max());
            Assert::AreEqual(R"({ "v": 0x7FFFFFFFFFFFFFFF })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int64_t>::min());
            Assert::AreEqual(R"({ "v": 0x8000000000000000 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexInt32) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", int32_t(0));
            Assert::AreEqual(R"({ "v": 0x00000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int32_t>::max());
            Assert::AreEqual(R"({ "v": 0x7FFFFFFF })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int32_t>::min());
            Assert::AreEqual(R"({ "v": 0x80000000 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexInt16) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", int16_t(0));
            Assert::AreEqual(R"({ "v": 0x0000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int16_t>::max());
            Assert::AreEqual(R"({ "v": 0x7FFF })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int16_t>::min());
            Assert::AreEqual(R"({ "v": 0x8000 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexInt8) {
        { // Zero
            Writer writer(true);
            writer.putHex("v", int8_t(0));
            Assert::AreEqual(R"({ "v": 0x00 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int8_t>::max());
            Assert::AreEqual(R"({ "v": 0x7F })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.putHex("v", std::numeric_limits<int8_t>::min());
            Assert::AreEqual(R"({ "v": 0x80 })"s, writer.finish());
        }
    }

    TEST_METHOD(TestPutHexChar) {
        Writer writer(true);
        writer.putHex("v", 'A');
        Assert::AreEqual(R"({ "v": 0x41 })"s, writer.finish());
    }

    TEST_METHOD(TestPutHexDouble) {
        uint64_t val(0x1234567812345678ull);
        Writer writer(true);
        writer.putHex("v", reinterpret_cast<const double &>(val));
        Assert::AreEqual(R"({ "v": 0x1234567812345678 })"s, writer.finish());
    }

    TEST_METHOD(TestPutHexFloat) {
        uint64_t val(0x12345678ull);
        Writer writer(true);
        writer.putHex("v", reinterpret_cast<const float &>(val));
        Assert::AreEqual(R"({ "v": 0x12345678 })"s, writer.finish());
    }

    TEST_METHOD(TestObject) {
        { // Empty
            Writer writer(true);
            writer.startObject("obj");
            writer.endObject();
            Assert::AreEqual(R"({ "obj": {} })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.startObject("obj", true);
            writer.put("k1", "abc");
            writer.put("k2", 123);
            writer.put("k3", true);
            writer.endObject();
            Assert::AreEqual(
R"({
    "obj": { "k1": "abc", "k2": 123, "k3": true }
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.startObject("obj");
            writer.put("k1", "abc");
            writer.put("k2", 123);
            writer.put("k3", true);
            writer.endObject();
            Assert::AreEqual(
R"({
    "obj": {
        "k1": "abc",
        "k2": 123,
        "k3": true
    }
})"s,
            writer.finish());
        }
    }

    TEST_METHOD(TestArray) {
        { // Empty
            Writer writer(true);
            writer.startArray("arr");
            writer.endArray();
            Assert::AreEqual(R"({ "arr": [] })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.startArray("arr", true);
            writer.put("abc");
            writer.put(123);
            writer.put(true);
            writer.endArray();
            Assert::AreEqual(
R"({
    "arr": [ "abc", 123, true ]
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.startArray("arr");
            writer.put("abc");
            writer.put(123);
            writer.put(true);
            writer.endArray();
            Assert::AreEqual(
R"({
    "arr": [
        "abc",
        123,
        true
    ]
})"s,
                writer.finish());
        }
    }

    TEST_METHOD(TestNested) {
        { // Compact
            Writer writer(true);
            writer.startObject("o");
            writer.startObject("oo");
            writer.endObject();
            writer.startArray("oa");
            writer.endArray();
            writer.endObject();
            writer.startArray("a");
            writer.startObject(); // ao
            writer.endObject();
            writer.startArray(); // aa
            writer.endArray();
            writer.endArray();
            Assert::AreEqual(R"({ "o": { "oo": {}, "oa": [] }, "a": [ {}, [] ] })"s, writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.startObject("o");
            writer.startObject("oo");
            writer.endObject();
            writer.startArray("oa");
            writer.endArray();
            writer.endObject();
            writer.startArray("a");
            writer.startObject(); // ao
            writer.endObject();
            writer.startArray(); // aa
            writer.endArray();
            writer.endArray();
            Assert::AreEqual(
R"({
    "o": {
        "oo": {},
        "oa": []
    },
    "a": [
        {},
        []
    ]
})"s,
            writer.finish());
        }
    }

    TEST_METHOD(TestEarlyFinish) {
        Writer writer(true);
        writer.startArray("a");
        writer.startObject();
        writer.startArray("a");
        Assert::AreEqual(R"({ "a": [ { "a": [] } ] })"s, writer.finish());
    }

    TEST_METHOD(TestFinishReset) {
        Writer writer(true);
        writer.put("val", 123);
        Assert::AreEqual(R"({ "val": 123 })"s, writer.finish());
        writer.put("lav", 321);
        Assert::AreEqual(R"({ "lav": 321 })"s, writer.finish());
    }

    TEST_METHOD(TestIndentSize) {
        { // 0 spaces
            Writer writer(false, 0);
            writer.startArray("a");
            writer.put(123);
            Assert::AreEqual(
R"({
"a": [
123
]
})"s, 
            writer.finish());
        }
        { // 3 spaces
            Writer writer(false, 3);
            writer.startArray("a");
            writer.put(123);
            Assert::AreEqual(
R"({
   "a": [
      123
   ]
})"s,
                writer.finish());
        }
        { // 8 spaces
            Writer writer(false, 8);
            writer.startArray("a");
            writer.put(123);
            Assert::AreEqual(
R"({
        "a": [
                123
        ]
})"s,
                writer.finish());
        }
    }

    TEST_METHOD(TestExceptionsIndentSize) {
        { // Indent size too small
            Assert::ExpectException<json_exception>([]() { Writer(true, -1); });
        }
        { // Indent size too large
            Assert::ExpectException<json_exception>([]() { Writer(true, 9); });
        }
    }

    TEST_METHOD(TestExceptionsContainerMismatch) {
        { // Ending array in object
            Writer writer(true);
            writer.startObject("o");
            Assert::ExpectException<json_exception>([&]() { writer.endArray(); });
        }
        { // Ending object in array
            Writer writer(true);
            writer.startArray("a");
            Assert::ExpectException<json_exception>([&]() { writer.endObject(); });
        }
        { // Ending at root
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.endObject(); });
        }
    }

    TEST_METHOD(TestExceptionsInvalidKeying) {
        { // Puting without key in object
            Writer writer(true);
            writer.startObject("o");
            Assert::ExpectException<json_exception>([&]() { writer.put(123); });
        }
        { // Puting with key in array
            Writer writer(true);
            writer.startArray("a");
            Assert::ExpectException<json_exception>([&]() { writer.put("k", 123); });
        }
        { // Empty key
            Writer writer(true);
            Assert::ExpectException<json_exception>([&]() { writer.put("", 123); });
        }
    }

};
