#include "CppUnitTest.h"

#include <functional>
#include <variant>
#include <deque>

#include "QJsonDecode.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

class TestDecoder {

    public:

    struct Object {};
    struct Array {};
    struct End {};
    struct Key { string_view k; };
    struct String { string_view v; };
    struct Integer { int64_t v; };
    struct Hex { uint64_t v; };
    struct Floater { double v; };
    struct Boolean { bool v; };
    struct Null {};

    friend bool operator==(const Object &, const Object &) { return true; }
    friend bool operator==(const Array &, const Array &) { return true; }
    friend bool operator==(const End &, const End &) { return true; }
    friend bool operator==(const Key & a, const Key & b) { return a.k == b.k; }
    friend bool operator==(const String & a, const String & b) { return a.v == b.v; }
    friend bool operator==(const Integer & a, const Integer & b) { return a.v == b.v; }
    friend bool operator==(const Hex & a, const Hex & b) { return a.v == b.v; }
    friend bool operator==(const Floater & a, const Floater & b) { return a.v == b.v; }
    friend bool operator==(const Boolean & a, const Boolean & b) { return a.v == b.v; }
    friend bool operator==(const Null &, const Null &) { return true; }

    using Element = std::variant<Object, Array, End, Key, String, Integer, Hex, Floater, Boolean, Null>;

    nullptr_t object(nullptr_t) { assertNextIs(Object{}); return nullptr; }
    nullptr_t array(nullptr_t) { assertNextIs(Array{}); return nullptr; }
    void end(nullptr_t, nullptr_t) { assertNextIs(End{}); }
    void key(string && k, nullptr_t) { assertNextIs(Key{k}); }
    void val(string_view v, nullptr_t) { assertNextIs(String{v}); }
    void val(int64_t v, nullptr_t) { assertNextIs(Integer{v}); }
    void val(uint64_t v, nullptr_t) { assertNextIs(Hex{v}); }
    void val(double v, nullptr_t) { assertNextIs(Floater{v}); }
    void val(bool v, nullptr_t) { assertNextIs(Boolean{v}); }
    void val(nullptr_t, nullptr_t) { assertNextIs(Null{}); }

    TestDecoder & expectObject() { m_sequence.emplace_back(Object{}); return *this; }
    TestDecoder & expectArray() { m_sequence.emplace_back(Array{}); return *this; }
    TestDecoder & expectEnd() { m_sequence.emplace_back(End{}); return *this; }
    TestDecoder & expectKey(string_view k) { m_sequence.emplace_back(Key{k}); return *this; }
    TestDecoder & expectString(string_view v) { m_sequence.emplace_back(String{v}); return *this; }
    TestDecoder & expectInteger(int64_t v) { m_sequence.emplace_back(Integer{v}); return *this; }
    TestDecoder & expectHex(uint64_t v) { m_sequence.emplace_back(Hex{v}); return *this; }
    TestDecoder & expectFloater(double v) { m_sequence.emplace_back(Floater{v}); return *this; }
    TestDecoder & expectBoolean(bool v) { m_sequence.emplace_back(Boolean{v}); return *this; }
    TestDecoder & expectNull() { m_sequence.emplace_back(Null{}); return *this; }

    bool isDone() const { return m_sequence.empty(); }

    private:

    std::deque<Element> m_sequence;

    void assertNextIs(const Element & e) {
        Assert::AreEqual(m_sequence.front(), e);
        m_sequence.pop_front();
    }

};

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
    //template <> static std::wstring ToString<uint16_t>(const uint16_t & v) { return std::to_wstring(v); }
    template <> static std::wstring ToString<TestDecoder::Element>(const TestDecoder::Element & v) {
        if (std::holds_alternative<TestDecoder::Object>(v)) return L"Object"s;
        else if (std::holds_alternative<TestDecoder::Array>(v)) return L"Array"s;
        else if (std::holds_alternative<TestDecoder::End>(v)) return L"End"s;
        else if (std::holds_alternative<TestDecoder::Key>(v)) return L"Key `"s + std::wstring(std::get<TestDecoder::Key>(v).k.cbegin(), std::get<TestDecoder::Key>(v).k.cend()) + L"`"s;
        else if (std::holds_alternative<TestDecoder::String>(v)) return L"String `"s + std::wstring(std::get<TestDecoder::String>(v).v.cbegin(), std::get<TestDecoder::String>(v).v.cend()) + L"`"s;
        else if (std::holds_alternative<TestDecoder::Integer>(v)) return L"Integer `"s + std::to_wstring(std::get<TestDecoder::Integer>(v).v) + L"`"s;
        else if (std::holds_alternative<TestDecoder::Hex>(v)) return L"Hex `"s + std::to_wstring(std::get<TestDecoder::Hex>(v).v) + L"`"s;
        else if (std::holds_alternative<TestDecoder::Floater>(v)) return L"Floater `"s + std::to_wstring(std::get<TestDecoder::Floater>(v).v) + L"`"s;
        else if (std::holds_alternative<TestDecoder::Boolean>(v)) return L"Boolean `"s + (std::get<TestDecoder::Boolean>(v).v ? L"true"s : L"false"s) + L"`"s;
        else if (std::holds_alternative<TestDecoder::Null>(v)) return L"Null"s;
    }
}}}

TEST_CLASS(Decode) {

    public:

    TEST_METHOD(Object) {
        { // Empty
            TestDecoder decoder;
            decoder.expectObject().expectEnd();
            qjson::decode(R"({})"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Single key
            TestDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectEnd();
            qjson::decode(R"({ "a": null })"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Multiple keys
            TestDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectKey("c"sv).expectNull().expectEnd();
            qjson::decode(R"({ "a": null, "b": null, "c": null })"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Array) {
        { // Empty
            TestDecoder decoder;
            decoder.expectArray().expectEnd();
            qjson::decode(R"([])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Single element
            TestDecoder decoder;
            decoder.expectArray().expectNull().expectEnd();
            qjson::decode(R"([ null ])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Multiple elements
            TestDecoder decoder;
            decoder.expectArray().expectNull().expectNull().expectNull().expectEnd();
            qjson::decode(R"([ null, null, null ])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(String) {
        { // Empty string
            TestDecoder decoder;
            decoder.expectString(""sv);
            qjson::decode(R"("")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // All printable
            TestDecoder decoder;
            decoder.expectString(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv);
            qjson::decode(R"(" !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Escape characters
            TestDecoder decoder;
            decoder.expectString("\b\f\n\r\t"sv);
            qjson::decode(R"("\b\f\n\r\t")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Unicode
            TestDecoder decoder;
            decoder.expectString("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
            qjson::decode(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Non-ascii unicode
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"(\u0080)", TestDecoder(), nullptr); });
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            TestDecoder decoder;
            decoder.expectInteger(0);
            qjson::decode(R"(0)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Normal
            TestDecoder decoder;
            decoder.expectInteger(123);
            qjson::decode(R"(123)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Min
            TestDecoder decoder;
            decoder.expectInteger(std::numeric_limits<int64_t>::min());
            qjson::decode(R"(-9223372036854775808)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Max
            TestDecoder decoder;
            decoder.expectInteger(std::numeric_limits<int64_t>::max());
            qjson::decode(R"(9223372036854775807)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Overflow
            TestDecoder decoder;
            decoder.expectInteger(-1);
            qjson::decode(R"(18446744073709551615)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Hex) {
        { // Zero
            TestDecoder decoder;
            decoder.expectHex(0x0u);
            qjson::decode(R"(0x0)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Uppercase
            TestDecoder decoder;
            decoder.expectHex(0x0123456789ABCDEFull);
            qjson::decode(R"(0x0123456789ABCDEF)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Lowercase
            TestDecoder decoder;
            decoder.expectHex(0x0123456789ABCDEFull);
            qjson::decode(R"(0x0123456789abcdef)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Max
            TestDecoder decoder;
            decoder.expectHex(0xFFFFFFFFFFFFFFFFull);
            qjson::decode(R"(0xFFFFFFFFFFFFFFFF)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Float) {
        { // Zero
            TestDecoder decoder;
            decoder.expectFloater(0.0);
            qjson::decode(R"(0.0)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Whole
            TestDecoder decoder;
            decoder.expectFloater(123.0);
            qjson::decode(R"(123.0)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Fractional
            TestDecoder decoder;
            decoder.expectFloater(123.456);
            qjson::decode(R"(123.456)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Exponent
            TestDecoder decoder;
            decoder.expectFloater(123.456e17);
            qjson::decode(R"(123.456e17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Positive exponent
            TestDecoder decoder;
            decoder.expectFloater(123.456e17);
            qjson::decode(R"(123.456e+17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Negative
            TestDecoder decoder;
            decoder.expectFloater(123.456e-17);
            qjson::decode(R"(-123.456e-17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Uppercase exponent without fraction
            TestDecoder decoder;
            decoder.expectFloater(123.0e34);
            qjson::decode(R"(123E34)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Bool) {
        { // True
            TestDecoder decoder;
            decoder.expectBoolean(true);
            qjson::decode(R"(true)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // False
            TestDecoder decoder;
            decoder.expectBoolean(false);
            qjson::decode(R"(false)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Null) {
        TestDecoder decoder;
        decoder.expectNull();
        qjson::decode(R"(null)"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(General) {
        TestDecoder decoder;
        decoder.expectObject();
            decoder.expectKey("Name"sv).expectString("Salt's Crust"sv);
            decoder.expectKey("Founded"sv).expectInteger(1964);
            decoder.expectKey("Employees"sv).expectArray();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Ol' Joe Fisher"sv);
                    decoder.expectKey("Title"sv).expectString("Fisherman"sv);
                    decoder.expectKey("Age"sv).expectInteger(69);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Mark Rower"sv);
                    decoder.expectKey("Title"sv).expectString("Cook"sv);
                    decoder.expectKey("Age"sv).expectInteger(41);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Phineas"sv);
                    decoder.expectKey("Title"sv).expectString("Server Boy"sv);
                    decoder.expectKey("Age"sv).expectInteger(19);
                decoder.expectEnd();
            decoder.expectEnd();
            decoder.expectKey("Dishes"sv).expectArray();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Basket o' Barnacles"sv);
                    decoder.expectKey("Price"sv).expectFloater(5.45);
                    decoder.expectKey("Ingredients"sv).expectArray().expectString("Salt"sv).expectString("Barnacles"sv).expectEnd();
                    decoder.expectKey("Gluten-Free"sv).expectBoolean(false);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Two Tuna"sv);
                    decoder.expectKey("Price"sv).expectFloater(14.99);
                    decoder.expectKey("Ingredients"sv).expectArray().expectString("Tuna"sv).expectEnd();
                    decoder.expectKey("Gluten-Free"sv).expectBoolean(true);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("18 Leg Bouquet"sv);
                    decoder.expectKey("Price"sv).expectFloater(18.00);
                    decoder.expectKey("Ingredients"sv).expectArray().expectString("Salt"sv).expectString("Octopus"sv).expectString("Crab"sv).expectEnd();
                    decoder.expectKey("Gluten-Free"sv).expectBoolean(false);
                decoder.expectEnd();
            decoder.expectEnd();
        decoder.expectEnd();
        qjson::decode(
R"({
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
            "Gluten-Free": false
        },
        {
            "Name": "Two Tuna",
            "Price": 14.99,
            "Ingredients": [ "Tuna" ],
            "Gluten-Free": true
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": 18.00,
            "Ingredients": [ "Salt", "Octopus", "Crab" ],
            "Gluten-Free": false
        }
    ]
})"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(NoWhitespace) {
        TestDecoder decoder;
        decoder.expectObject().expectKey("a"sv).expectArray().expectString("abc"sv).expectInteger(-123).expectFloater(-123.456e-78).expectBoolean(true).expectNull().expectEnd().expectEnd();
        qjson::decode(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(ExtraneousWhitespace) {
        TestDecoder decoder;
        decoder.expectObject().expectEnd();
        qjson::decode(" \t\n\r\v{} \t\n\r\v"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(Exceptions) {
        { // Content outside root object
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({}, "hi")", TestDecoder(), nullptr); });
        }
        { // Key not within quotes
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ a: 0 })", TestDecoder(), nullptr); });
        }
        { // No colon after key
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a" 0 })", TestDecoder(), nullptr); });
        }
        { // No comma between object elements
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0 "b": 1 })", TestDecoder(), nullptr); });
        }
        { // Comma after last object element
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0, })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0, "b": 1, })", TestDecoder(), nullptr); });
        }
        { // No comma between array elements
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": [ 0 1 ] })", TestDecoder(), nullptr); });
        }
        { // Comma after last array element
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": [ 0, ] })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": [ 0, 1, ] })", TestDecoder(), nullptr); });
        }
        { // Missing value
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": })", TestDecoder(), nullptr); });
        }
        { // Unknown value
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": v })", TestDecoder(), nullptr); });
        }
        { // Missing escape sequence
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\" })", TestDecoder(), nullptr); });
        }
        { // Unknown escape sequence
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\v" })", TestDecoder(), nullptr); });
        }
        { // Missing unicode
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\u" })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\u0" })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\u00" })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": "\u000" })", TestDecoder(), nullptr); });
        }
        { // Unknown string content
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode("{ \"a\": \"\n\" }", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode("{ \"a\": \"\t\" }", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode("{ \"a\": \"\0\" }", TestDecoder(), nullptr); });
        }
        { // Missing hex
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0x })", TestDecoder(), nullptr); });
        }
        { // Invalid hex digit
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0xG })", TestDecoder(), nullptr); });
        }
        { // Hex too long
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0x00000000000000000 })", TestDecoder(), nullptr); });
        }
        { // Plus sign
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": +0 })", TestDecoder(), nullptr); });
        }
        { // Missing number after minus
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": - })", TestDecoder(), nullptr); });
        }
        { // Invalid digit
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": -A })", TestDecoder(), nullptr); });
        }
        { // Integer value too large
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 18446744073709551616 })", TestDecoder(), nullptr); });
        }
        { // Too many integer digits
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 000000000000000000000 })", TestDecoder(), nullptr); });
        }
        { // Missing fractional component
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0. })", TestDecoder(), nullptr); });
        }
        { // Fractional component too large
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0.18446744073709551616 })", TestDecoder(), nullptr); });
        }
        { // Missing exponent
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0e })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0e+ })", TestDecoder(), nullptr); });
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0e- })", TestDecoder(), nullptr); });
        }
        { // Exponent too large
            Assert::ExpectException<qjson::DecodeError>([]() { qjson::decode(R"({ "a": 0e1001 })", TestDecoder(), nullptr); });
        }
    }

};
