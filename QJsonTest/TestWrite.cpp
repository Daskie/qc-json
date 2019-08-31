#include "CppUnitTest.h"

#include "QJsonWrite.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::Writer;
using qjson::JsonWriteError;

struct CustomVal { int x, y; };

void qjson_encode(qjson::Writer & writer, const CustomVal & v) {
    writer.array(true).val(v.x).val(v.y).end();
}

TEST_CLASS(Write) {

    public:
	    
    TEST_METHOD(Empty) {
        Assert::AreEqual("{}"s, Writer(false).finish());
        Assert::AreEqual("{}"s, Writer(true).finish());
    }

    TEST_METHOD(Key) {
        { // String view
            Writer writer(true);
            writer.key("k"sv).val("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // String
            Writer writer(true);
            writer.key("k"s).val("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // Const C string
            Writer writer(true);
            const char str[]{"k"};
            writer.key(str).val("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
        { // Mutable C string
            Writer writer(true);
            char str[]{"k"};
            writer.key(str).val("v");
            Assert::AreEqual(R"({ "k": "v" })"s, writer.finish());
        }
    }

    TEST_METHOD(ValString) {
        { // Empty
            Writer writer(true);
            writer.key("v").val("");
            Assert::AreEqual(R"({ "v": "" })"s, writer.finish());
        }
        { // String view
            Writer writer(true);
            writer.key("v").val("hello"sv);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // String
            Writer writer(true);
            writer.key("v").val("hello"s);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Const C string
            Writer writer(true);
            const char str[]{"hello"};
            writer.key("v").val(str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Mutable C string
            Writer writer(true);
            char str[]{"hello"};
            writer.key("v").val(str);
            Assert::AreEqual(R"({ "v": "hello" })"s, writer.finish());
        }
        { // Printable characters
            Writer writer(true);
            writer.key("v").val(R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)");
            Assert::AreEqual(R"({ "v": " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"s, writer.finish());
        }
        { // Escape characters
            Writer writer(true);
            writer.key("v").val("\b\f\n\r\t");
            Assert::AreEqual(R"({ "v": "\b\f\n\r\t" })"s, writer.finish());
        }
        { // Unicode
            Writer writer(true);
            writer.key("v").val("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
            Assert::AreEqual(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"s, writer.finish());
        }
        { // Non-ASCII unicode
            Writer writer(true);
            writer.key("v");
            char str[]{char(128)};
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(std::string_view(str, sizeof(str))); });
        }
        { // Single char
            Writer writer(true);
            writer.key("v").val('a');
            Assert::AreEqual(R"({ "v": "a" })"s, writer.finish());
        }
    }

    TEST_METHOD(ValInt) {
        { // Zero
            Writer writer(true);
            writer.key("v").val(int64_t(0));
            Assert::AreEqual(R"({ "v": 0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.key("v").val(9223372036854775807ll);
            Assert::AreEqual(R"({ "v": 9223372036854775807 })"s, writer.finish());
        }
        { // Min
            Writer writer(true);
            writer.key("v").val(-9223372036854775808ll);
            Assert::AreEqual(R"({ "v": -9223372036854775808 })"s, writer.finish());
        }
        { // Fewer digits
            Writer writer(true);
            writer.key("v").val(123);
            Assert::AreEqual(R"({ "v": 123 })"s, writer.finish());
        }
    }

    TEST_METHOD(ValUInt) {
        { // Zero
            Writer writer(true);
            writer.key("v").val(0u);
            Assert::AreEqual(R"({ "v": 0x0 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            writer.key("v").val(0xFFFFFFFFFFFFFFFFull);
            Assert::AreEqual(R"({ "v": 0xFFFFFFFFFFFFFFFF })"s, writer.finish());
        }
        { // Something else
            Writer writer(true);
            writer.key("v").val(0x00123ABCu);
            Assert::AreEqual(R"({ "v": 0x123ABC })"s, writer.finish());
        }
    }

    TEST_METHOD(ValDouble) {
        { // Zero
            Writer writer(true);
            writer.key("v").val(0.0);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            uint64_t val(0b0111111111101111111111111111111111111111111111111111111111111111ull);
            writer.key("v").val(reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": 1.7976931348623157e+308 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            uint64_t val(0b1000000000010000000000000000000000000000000000000000000000000000ull);
            writer.key("v").val(reinterpret_cast<double &>(val));
            Assert::AreEqual(R"({ "v": -2.2250738585072014e-308 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            writer.key("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(std::numeric_limits<double>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            writer.key("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(std::numeric_limits<double>::quiet_NaN()); });
        }
    }

    TEST_METHOD(ValFloat) {
        { // Zero
            Writer writer(true);
            writer.key("v").val(0.0f);
            Assert::AreEqual(R"({ "v": 0.00000000000000000 })"s, writer.finish());
        }
        { // Max
            Writer writer(true);
            uint32_t val(0b01111111011111111111111111111111);
            writer.key("v").val(reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": 3.4028234663852886e+38 })"s, writer.finish());
        }
        { // Negative min
            Writer writer(true);
            uint32_t val(0b10000000100000000000000000000000);
            writer.key("v").val(reinterpret_cast<float &>(val));
            Assert::AreEqual(R"({ "v": -1.1754943508222875e-38 })"s, writer.finish());
        }
        { // Infinity
            Writer writer(true);
            writer.key("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(std::numeric_limits<float>::infinity()); });
        }
        { // NaN
            Writer writer(true);
            writer.key("v");
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(std::numeric_limits<float>::quiet_NaN()); });
        }
    }

    TEST_METHOD(ValBool) {
        { // True
            Writer writer(true);
            writer.key("v").val(true);
            Assert::AreEqual(R"({ "v": true })"s, writer.finish());
        }
        { // False
            Writer writer(true);
            writer.key("v").val(false);
            Assert::AreEqual(R"({ "v": false })"s, writer.finish());
        }
    }

    TEST_METHOD(ValNull) {
        Writer writer(true);
        writer.key("v").val(nullptr);
        Assert::AreEqual(R"({ "v": null })"s, writer.finish());
    }

    TEST_METHOD(ValCustom) {
        Writer writer(true);
        writer.key("v").val(CustomVal{1, 2});
        Assert::AreEqual(R"({ "v": [ 1, 2 ] })"s, writer.finish());
    }

    TEST_METHOD(Object) {
        { // Empty
            Writer writer(true);
            writer.key("obj").object().end();
            Assert::AreEqual(R"({ "obj": {} })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.key("obj").object(true).key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
            Assert::AreEqual(
R"({
    "obj": { "k1": "abc", "k2": 123, "k3": true }
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.key("obj").object().key("k1").val("abc").key("k2").val(123).key("k3").val(true).end();
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
            writer.key("arr").array().end();
            Assert::AreEqual(R"({ "arr": [] })"s, writer.finish());
        }
        { // Compact
            Writer writer(false);
            writer.key("arr").array(true).val("abc").val(123).val(true).end();
            Assert::AreEqual(
R"({
    "arr": [ "abc", 123, true ]
})"s,
                writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.key("arr").array().val("abc").val(123).val(true).end();
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
            writer.key("o").object();
            writer.key("oo").object();
            writer.end();
            writer.key("oa").array();
            writer.end();
            writer.end();
            writer.key("a").array();
            writer.object(); // ao
            writer.end();
            writer.array(); // aa
            writer.end();
            writer.end();
            Assert::AreEqual(R"({ "o": { "oo": {}, "oa": [] }, "a": [ {}, [] ] })"s, writer.finish());
        }
        { // Not compact
            Writer writer(false);
            writer.key("o").object();
            writer.key("oo").object();
            writer.end();
            writer.key("oa").array();
            writer.end();
            writer.end();
            writer.key("a").array();
            writer.object(); // ao
            writer.end();
            writer.array(); // aa
            writer.end();
            writer.end();
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
        writer.key("a").array().object().key("a").array();
        Assert::AreEqual(R"({ "a": [ { "a": [] } ] })"s, writer.finish());
    }

    TEST_METHOD(FinishReset) {
        Writer writer(true);
        writer.key("val").val(123);
        Assert::AreEqual(R"({ "val": 123 })"s, writer.finish());
        writer.key("lav").val(321);
        Assert::AreEqual(R"({ "lav": 321 })"s, writer.finish());
    }

    TEST_METHOD(IndentSize) {
        { // 0 spaces
            Writer writer(false, 0);
            writer.key("a").array().val(123);
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
            writer.key("a").array().val(123);
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
            writer.key("a").array().val(123);
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
        { // Ending at root
            Writer writer(true);
            Assert::ExpectException<JsonWriteError>([&]() { writer.end(); });
        }
        { // Puting without key in object
            Writer writer(true);
            writer.key("o").object();
            Assert::ExpectException<JsonWriteError>([&]() { writer.val(123); });
        }
        { // Puting with key in array
            Writer writer(true);
            writer.key("a").array();
            Assert::ExpectException<JsonWriteError>([&]() { writer.key("k"); });
        }
        { // Empty key
            Writer writer(true);
            Assert::ExpectException<JsonWriteError>([&]() { writer.key(""); });
        }
        { // Putting key twice
            Writer writer(true);
            writer.key("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.key("k"); });
        }
        { // Ending object without value
            Writer writer(true);
            writer.key("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.end(); });
        }
        { // Finishing without value
            Writer writer(true);
            writer.key("k");
            Assert::ExpectException<JsonWriteError>([&]() { writer.finish(); });
        }
    }

};
