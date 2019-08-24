#include "CppUnitTest.h"

#include "QJsonWrite.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::Writer;
using qjson::JsonWriteError;

TEST_CLASS(Write) {

    public:
	    
    TEST_METHOD(Empty) {
        Assert::AreEqual("{}"s, Writer(false).finish());
        Assert::AreEqual("{}"s, Writer(true).finish());
    }

    TEST_METHOD(PutKey) {
        { // String view
            Writer writer(true);
            writer.putKey("k"sv);
            writer.putVal("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // String
            Writer writer(true);
            writer.putKey("k"s);
            writer.putVal("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // Const C string
            Writer writer(true);
            const char str[]{"k"};
            writer.putKey(str);
            writer.putVal("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // Mutable C string
            Writer writer(true);
            char str[]{"k"};
            writer.putKey(str);
            writer.putVal("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
    }

    TEST_METHOD(PutValString) {
        { // Empty
            Writer writer(true);
            writer.putKey("v");
            writer.putVal("");
            Assert::AreEqual(R"({ "v": "" })"s, writer.finish());
        }
        { // String view
            Writer writer(true);
            writer.putKey("v");
            writer.putVal("hello"sv);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // String
            Writer writer(true);
            writer.putKey("v");
            writer.putVal("hello"s);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Const C string
            Writer writer(true);
            writer.putKey("v");
            const char str[]{"hello"};
            writer.putVal(str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Mutable C string
            Writer writer(true);
            writer.putKey("v");
            char str[]{"hello"};
            writer.putVal(str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Printable characters
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)");
            Assert::AreEqual(R"({ "v": " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"s, writer.finish());
        }
        { // Escape characters
            Writer writer(true);
            writer.putKey("v");
            writer.putVal("\b\f\n\r\t");
            Assert::AreEqual(R"({ "v": "\b\f\n\r\t" })"s, writer.finish());
        }
        { // Unicode
            Writer writer(true);
            writer.putKey("v");
            writer.putVal("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
            Assert::AreEqual(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"s, writer.finish());
        }
        { // Non-ASCII unicode
            Writer writer(true);
            writer.putKey("v");
            char str[]{char(128)};
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(std::string_view(str, sizeof(str))); });
        }
        { // Single char
            Writer writer(true);
            writer.putKey("v");
            writer.putVal('a');
            Assert::AreEqual(R"({ "v": "a" })"s, writer.finish());
        }
    }

    TEST_METHOD(PutValInt) {
        { // Zero
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(int64_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(9223372036854775807ll);
            Assert::AreEqual(R"({ "v": 9223372036854775807 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(-9223372036854775808ll);
            Assert::AreEqual(R"({ "v": -9223372036854775808 })"s, writer.finish());
        }
        { // Fewer digits
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(123);
            Assert::AreEqual(R"({ "v": 123 })"s, writer.finish());
        }
    }

    TEST_METHOD(PutValUInt) {
        { // Zero
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(0u);
            Assert::AreEqual(R"({ "v": 0x0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(0xFFFFFFFFFFFFFFFFull);
            Assert::AreEqual(R"({ "v": 0xFFFFFFFFFFFFFFFF })"s, writer.finish());
        }
        { // Something else
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(0x00123ABCu);
            Assert::AreEqual(R"({ "v": 0x123ABC })"s, writer.finish());
        }
    }

    TEST_METHOD(PutValDouble) {
        { // Zero
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(0.0);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putKey("v");
            uint64_t val(0b0111111111101111111111111111111111111111111111111111111111111111ull);
            writer.putVal(reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": 1.7976931348623157e+308 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            writer.putKey("v");
            uint64_t val(0b1000000000010000000000000000000000000000000000000000000000000000ull);
            writer.putVal(reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": -2.2250738585072014e-308 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            writer.putKey("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(std::numeric_limits<double>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            writer.putKey("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(std::numeric_limits<double>::quiet_NaN()); });
        }
    }

    TEST_METHOD(PutValFloat) {
        { // Zero
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(0.0f);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.putKey("v");
            uint32_t val(0b01111111011111111111111111111111);
            writer.putVal(reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": 3.4028234663852886e+38 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            writer.putKey("v");
            uint32_t val(0b10000000100000000000000000000000);
            writer.putVal(reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": -1.1754943508222875e-38 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            writer.putKey("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(std::numeric_limits<float>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            writer.putKey("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(std::numeric_limits<float>::quiet_NaN()); });
        }
    }

    TEST_METHOD(PutValBool) {
        { // True
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(true);
            Assert::AreEqual(R"({ "v": true })"s, writer.finish());
        }
        { // False
            Writer writer(true);
            writer.putKey("v");
            writer.putVal(false);
            Assert::AreEqual(R"({ "v": false })"s, writer.finish());
        }
    }

    TEST_METHOD(PutValNull) {
        Writer writer(true);
        writer.putKey("v");
        writer.putVal(nullptr);
        Assert::AreEqual(R"({ "v": null })"s, writer.finish());
    }

    TEST_METHOD(Object) {
        { // Empty
            Writer writer(true);
            writer.putKey("obj");
            writer.startObject();
            writer.endObject();
            Assert::AreEqual(R"({ "obj": {} })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.putKey("obj");
            writer.startObject(true);
            writer.putKey("k1");
            writer.putVal("abc");
            writer.putKey("k2");
            writer.putVal(123);
            writer.putKey("k3");
            writer.putVal(true);
            writer.endObject();
            Assert::AreEqual(
R"({
    "obj": { "k1": "abc", "k2": 123, "k3": true }
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.putKey("obj");
            writer.startObject();
            writer.putKey("k1");
            writer.putVal("abc");
            writer.putKey("k2");
            writer.putVal(123);
            writer.putKey("k3");
            writer.putVal(true);
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

    TEST_METHOD(Array) {
        { // Empty
            Writer writer(true);
            writer.putKey("arr");
            writer.startArray();
            writer.endArray();
            Assert::AreEqual(R"({ "arr": [] })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.putKey("arr");
            writer.startArray(true);
            writer.putVal("abc");
            writer.putVal(123);
            writer.putVal(true);
            writer.endArray();
            Assert::AreEqual(
R"({
    "arr": [ "abc", 123, true ]
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.putKey("arr");
            writer.startArray();
            writer.putVal("abc");
            writer.putVal(123);
            writer.putVal(true);
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

    TEST_METHOD(Nested) {
        { // Compact
            Writer writer(true);
            writer.putKey("o");
            writer.startObject();
            writer.putKey("oo");
            writer.startObject();
            writer.endObject();
            writer.putKey("oa");
            writer.startArray();
            writer.endArray();
            writer.endObject();
            writer.putKey("a");
            writer.startArray();
            writer.startObject(); // ao
            writer.endObject();
            writer.startArray(); // aa
            writer.endArray();
            writer.endArray();
            Assert::AreEqual(R"({ "o": { "oo": {}, "oa": [] }, "a": [ {}, [] ] })"s, writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.putKey("o");
            writer.startObject();
            writer.putKey("oo");
            writer.startObject();
            writer.endObject();
            writer.putKey("oa");
            writer.startArray();
            writer.endArray();
            writer.endObject();
            writer.putKey("a");
            writer.startArray();
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

    TEST_METHOD(EarlyFinish) {
        Writer writer(true);
        writer.putKey("a");
        writer.startArray();
        writer.startObject();
        writer.putKey("a");
        writer.startArray();
        Assert::AreEqual(R"({ "a": [ { "a": [] } ] })"s, writer.finish());
    }

    TEST_METHOD(FinishReset) {
        Writer writer(true);
        writer.putKey("val");
        writer.putVal(123);
        Assert::AreEqual(R"({ "val": 123 })"s, writer.finish());
        writer.putKey("lav");
        writer.putVal(321);
        Assert::AreEqual(R"({ "lav": 321 })"s, writer.finish());
    }

    TEST_METHOD(IndentSize) {
        { // 0 spaces
            Writer writer(false, 0);
            writer.putKey("a");
            writer.startArray();
            writer.putVal(123);
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
            writer.putKey("a");
            writer.startArray();
            writer.putVal(123);
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
            writer.putKey("a");
            writer.startArray();
            writer.putVal(123);
            Assert::AreEqual(
R"({
        "a": [
                123
        ]
})"s,
                writer.finish());
        }
    }

    TEST_METHOD(Exceptions) {
        { // Indent size too small
            Assert::ExpectException<JsonWriteError>([]() { Writer(true, -1); });
        }
        { // Indent size too large
            Assert::ExpectException<JsonWriteError>([]() { Writer(true, 9); });
        }
        { // Ending array in object
            Writer writer(true);
            writer.putKey("o");
            writer.startObject();
            Assert::ExpectException<JsonWriteError>([&]() { writer.endArray(); });
        }
        { // Ending object in array
            Writer writer(true);
            writer.putKey("a");
            writer.startArray();
            Assert::ExpectException<JsonWriteError>([&]() { writer.endObject(); });
        }
        { // Ending at root
            Writer writer(true);
            Assert::ExpectException<JsonWriteError>([&]() { writer.endObject(); });
        }
        { // Puting without key in object
            Writer writer(true);
            writer.putKey("o");
            writer.startObject();
            Assert::ExpectException<JsonWriteError>([&]() { writer.putVal(123); });
        }
        { // Puting with key in array
            Writer writer(true);
            writer.putKey("a");
            writer.startArray();
            Assert::ExpectException<JsonWriteError>([&]() { writer.putKey("k"); });
        }
        { // Empty key
            Writer writer(true);
            Assert::ExpectException<JsonWriteError>([&]() { writer.putKey(""); });
        }
        { // Putting key twice
            Writer writer(true);
            writer.putKey("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.putKey("k"); });
        }
        { // Ending object without value
            Writer writer(true);
            writer.putKey("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.endObject(); });
        }
        { // Finishing without value
            Writer writer(true);
            writer.putKey("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.finish(); });
        }
    }

};
