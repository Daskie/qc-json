#include "CppUnitTest.h"

#include <variant>
#include <deque>

#include "qc-json-decode.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

class DummyDecoder {

  public:

    nullptr_t object(nullptr_t) { return nullptr; }
    nullptr_t array(nullptr_t) { return nullptr; }
    void end(nullptr_t, nullptr_t) {}
    void key(string && k, nullptr_t) {}
    void val(string_view v, nullptr_t) {}
    void val(int64_t v, nullptr_t) {}
    void val(double v, nullptr_t) {}
    void val(bool v, nullptr_t) {}
    void val(nullptr_t, nullptr_t) {}

};

class ExpectantDecoder {

  public:

    struct Object {};
    struct Array {};
    struct End {};
    struct Key { string_view k; };
    struct String { string_view v; };
    struct Integer { int64_t v; };
    struct Floater { double v; };
    struct Boolean { bool v; };
    struct Null {};

    friend bool operator==(const Object &, const Object &) { return true; }
    friend bool operator==(const Array &, const Array &) { return true; }
    friend bool operator==(const End &, const End &) { return true; }
    friend bool operator==(const Key & a, const Key & b) { return a.k == b.k; }
    friend bool operator==(const String & a, const String & b) { return a.v == b.v; }
    friend bool operator==(const Integer & a, const Integer & b) { return a.v == b.v; }
    friend bool operator==(const Floater & a, const Floater & b) { return a.v == b.v || (std::isnan(a.v) && std::isnan(b.v)); }
    friend bool operator==(const Boolean & a, const Boolean & b) { return a.v == b.v; }
    friend bool operator==(const Null &, const Null &) { return true; }

    using Element = std::variant<Object, Array, End, Key, String, Integer, Floater, Boolean, Null>;

    nullptr_t object(nullptr_t) { assertNextIs(Object{}); return nullptr; }
    nullptr_t array(nullptr_t) { assertNextIs(Array{}); return nullptr; }
    void end(nullptr_t, nullptr_t) { assertNextIs(End{}); }
    void key(string && k, nullptr_t) { assertNextIs(Key{k}); }
    void val(string_view v, nullptr_t) { assertNextIs(String{v}); }
    void val(int64_t v, nullptr_t) { assertNextIs(Integer{v}); }
    void val(double v, nullptr_t) { assertNextIs(Floater{v}); }
    void val(bool v, nullptr_t) { assertNextIs(Boolean{v}); }
    void val(nullptr_t, nullptr_t) { assertNextIs(Null{}); }

    ExpectantDecoder & expectObject() { m_sequence.emplace_back(Object{}); return *this; }
    ExpectantDecoder & expectArray() { m_sequence.emplace_back(Array{}); return *this; }
    ExpectantDecoder & expectEnd() { m_sequence.emplace_back(End{}); return *this; }
    ExpectantDecoder & expectKey(string_view k) { m_sequence.emplace_back(Key{k}); return *this; }
    ExpectantDecoder & expectString(string_view v) { m_sequence.emplace_back(String{v}); return *this; }
    ExpectantDecoder & expectInteger(int64_t v) { m_sequence.emplace_back(Integer{v}); return *this; }
    ExpectantDecoder & expectFloater(double v) { m_sequence.emplace_back(Floater{v}); return *this; }
    ExpectantDecoder & expectBoolean(bool v) { m_sequence.emplace_back(Boolean{v}); return *this; }
    ExpectantDecoder & expectNull() { m_sequence.emplace_back(Null{}); return *this; }

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
    template <> static std::wstring ToString<ExpectantDecoder::Element>(const ExpectantDecoder::Element & v) {
        if (std::holds_alternative<ExpectantDecoder::Object>(v)) return L"Object"s;
        else if (std::holds_alternative<ExpectantDecoder::Array>(v)) return L"Array"s;
        else if (std::holds_alternative<ExpectantDecoder::End>(v)) return L"End"s;
        else if (std::holds_alternative<ExpectantDecoder::Key>(v)) return L"Key `"s + std::wstring(std::get<ExpectantDecoder::Key>(v).k.cbegin(), std::get<ExpectantDecoder::Key>(v).k.cend()) + L"`"s;
        else if (std::holds_alternative<ExpectantDecoder::String>(v)) return L"String `"s + std::wstring(std::get<ExpectantDecoder::String>(v).v.cbegin(), std::get<ExpectantDecoder::String>(v).v.cend()) + L"`"s;
        else if (std::holds_alternative<ExpectantDecoder::Integer>(v)) return L"Integer `"s + std::to_wstring(std::get<ExpectantDecoder::Integer>(v).v) + L"`"s;
        else if (std::holds_alternative<ExpectantDecoder::Floater>(v)) return L"Floater `"s + std::to_wstring(std::get<ExpectantDecoder::Floater>(v).v) + L"`"s;
        else if (std::holds_alternative<ExpectantDecoder::Boolean>(v)) return L"Boolean `"s + (std::get<ExpectantDecoder::Boolean>(v).v ? L"true"s : L"false"s) + L"`"s;
        else if (std::holds_alternative<ExpectantDecoder::Null>(v)) return L"Null"s;
        else return L"Unknown Element";
    }
}}}

TEST_CLASS(Decode) {

  public:

    TEST_METHOD(Object) {
        { // Empty
            ExpectantDecoder decoder;
            decoder.expectObject().expectEnd();
            qc::json::decode(R"({})"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Single key
            ExpectantDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectEnd();
            qc::json::decode(R"({ "a": null })"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Multiple keys
            ExpectantDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectKey("c"sv).expectNull().expectEnd();
            qc::json::decode(R"({ "a": null, "b": null, "c": null })"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // No space
            ExpectantDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
            qc::json::decode(R"({"a":null,"b":null})"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Weird spacing
            ExpectantDecoder decoder;
            decoder.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
            qc::json::decode(R"({"a" :null ,"b" :null})"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Key not within quotes
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ a: 0 })", DummyDecoder(), nullptr); });
        }
        { // No colon after key
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a" 0 })", DummyDecoder(), nullptr); });
        }
        { // Empty key
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "": 0 })", DummyDecoder(), nullptr); });
        }
        { // Missing value
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a": })", DummyDecoder(), nullptr); });
        }
        { // No comma between elements
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a": 0 "b": 1 })", DummyDecoder(), nullptr); });
        }
        { // Comma after last element
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a": 0, })", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a": 0, "b": 1, })", DummyDecoder(), nullptr); });
        }
        { // Empty entry
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"({ "a": 0, , "b": 1 })", DummyDecoder(), nullptr); });
        }
    }

    TEST_METHOD(Array) {
        { // Empty
            ExpectantDecoder decoder;
            decoder.expectArray().expectEnd();
            qc::json::decode(R"([])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Single element
            ExpectantDecoder decoder;
            decoder.expectArray().expectNull().expectEnd();
            qc::json::decode(R"([ null ])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Multiple elements
            ExpectantDecoder decoder;
            decoder.expectArray().expectNull().expectNull().expectNull().expectEnd();
            qc::json::decode(R"([ null, null, null ])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // No space
            ExpectantDecoder decoder;
            decoder.expectArray().expectNull().expectNull().expectEnd();
            qc::json::decode(R"([null,null])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Weird spacing
            ExpectantDecoder decoder;
            decoder.expectArray().expectNull().expectNull().expectEnd();
            qc::json::decode(R"([null ,null])"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // No comma between elements
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"([ 0 1 ])", DummyDecoder(), nullptr); });
        }
        { // Comma after last element
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"([ 0, ])", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"([ 0, 1, ])", DummyDecoder(), nullptr); });
        }
        { // Empty entry
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"([ 0, , 1 ])", DummyDecoder(), nullptr); });
        }
    }

    TEST_METHOD(String) {
        { // Empty string
            ExpectantDecoder decoder;
            decoder.expectString(""sv);
            qc::json::decode(R"("")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // All printable
            ExpectantDecoder decoder;
            decoder.expectString(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv);
            qc::json::decode(R"(" !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Escape characters
            ExpectantDecoder decoder;
            decoder.expectString("\b\f\n\r\t"sv);
            qc::json::decode(R"("\b\f\n\r\t")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Missing escape sequence
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\")", DummyDecoder(), nullptr); });
        }
        { // Unknown escape sequence
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\v")", DummyDecoder(), nullptr); });
        }
        { // Unicode
            ExpectantDecoder decoder;
            decoder.expectString("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
            qc::json::decode(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Non-ascii unicode
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u0080")", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u0F00")", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\uF000")", DummyDecoder(), nullptr); });
        }
        { // Missing all unicode digits
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u")", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u0")", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u00")", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("\u000")", DummyDecoder(), nullptr); });
        }
        { // Missing end quote
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"("abc)", DummyDecoder(), nullptr); });
        }
        { // Unknown content
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode("\"\n\"", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode("\"\t\"", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode("\"\0\"", DummyDecoder(), nullptr); });
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            ExpectantDecoder decoder;
            decoder.expectInteger(0);
            qc::json::decode(R"(0)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Normal
            ExpectantDecoder decoder;
            decoder.expectInteger(123);
            qc::json::decode(R"(123)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Min
            ExpectantDecoder decoder;
            decoder.expectInteger(std::numeric_limits<int64_t>::min());
            qc::json::decode(R"(-9223372036854775808)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Max
            ExpectantDecoder decoder;
            decoder.expectInteger(std::numeric_limits<int64_t>::max());
            qc::json::decode(R"(9223372036854775807)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Max unsigned
            ExpectantDecoder decoder;
            decoder.expectInteger(0xFFFFFFFFFFFFFFFF);
            qc::json::decode(R"(-1)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Number too large
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(9223372036854775808)"sv, DummyDecoder(), nullptr); });
        }
        { // Invalid minus sign
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(-)", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(-A)", DummyDecoder(), nullptr); });
        }
        { // Plus sign
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(+)", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(+123)", DummyDecoder(), nullptr); });
        }
    }

    TEST_METHOD(Floater) {
        { // Zero
            ExpectantDecoder decoder;
            decoder.expectFloater(0.0);
            qc::json::decode(R"(0.0)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Whole
            ExpectantDecoder decoder;
            decoder.expectFloater(123.0);
            qc::json::decode(R"(123.0)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Fractional
            ExpectantDecoder decoder;
            decoder.expectFloater(123.456);
            qc::json::decode(R"(123.456)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Exponent lowercase
            ExpectantDecoder decoder;
            decoder.expectFloater(123.456e17);
            qc::json::decode(R"(123.456e17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Exponent uppercase
            ExpectantDecoder decoder;
            decoder.expectFloater(123.456e17);
            qc::json::decode(R"(123.456E17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Positive exponent
            ExpectantDecoder decoder;
            decoder.expectFloater(123.456e17);
            qc::json::decode(R"(123.456e+17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Negative exponent
            ExpectantDecoder decoder;
            decoder.expectFloater(-123.456e-17);
            qc::json::decode(R"(-123.456e-17)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Exponent without fraction
            ExpectantDecoder decoder;
            decoder.expectFloater(123.0e34);
            qc::json::decode(R"(123e34)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Max integer
            ExpectantDecoder decoder;
            decoder.expectFloater(9007199254740991.0);
            qc::json::decode(R"(9007199254740991.0)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // infinity
            ExpectantDecoder decoder;
            decoder.expectFloater(std::numeric_limits<double>::infinity());
            qc::json::decode(R"(inf)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // -infinity
            ExpectantDecoder decoder;
            decoder.expectFloater(-std::numeric_limits<double>::infinity());
            qc::json::decode(R"(-inf)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // NaN
            ExpectantDecoder decoder;
            decoder.expectFloater(std::numeric_limits<double>::quiet_NaN());
            qc::json::decode(R"(nan)", decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // Missing fractional component
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(0.)", DummyDecoder(), nullptr); });
        }
        { // Missing exponent
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(0e)", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(0e+)", DummyDecoder(), nullptr); });
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(0e-)", DummyDecoder(), nullptr); });
        }
        { // Magnitude too large
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(1e1000)", DummyDecoder(), nullptr); });
        }
        { // Magnitude too small
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(1e-1000)", DummyDecoder(), nullptr); });
        }
    }

    TEST_METHOD(Boolean) {
        { // True
            ExpectantDecoder decoder;
            decoder.expectBoolean(true);
            qc::json::decode(R"(true)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
        { // False
            ExpectantDecoder decoder;
            decoder.expectBoolean(false);
            qc::json::decode(R"(false)"sv, decoder, nullptr);
            Assert::IsTrue(decoder.isDone());
        }
    }

    TEST_METHOD(Null) {
        ExpectantDecoder decoder;
        decoder.expectNull();
        qc::json::decode(R"(null)"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(General) {
        ExpectantDecoder decoder;
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
                    decoder.expectKey("Gluten Free"sv).expectBoolean(false);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("Two Tuna"sv);
                    decoder.expectKey("Price"sv).expectFloater(14.99);
                    decoder.expectKey("Ingredients"sv).expectArray().expectString("Tuna"sv).expectEnd();
                    decoder.expectKey("Gluten Free"sv).expectBoolean(true);
                decoder.expectEnd();
                decoder.expectObject();
                    decoder.expectKey("Name"sv).expectString("18 Leg Bouquet"sv);
                    decoder.expectKey("Price"sv).expectFloater(18.00);
                    decoder.expectKey("Ingredients"sv).expectArray().expectString("Salt"sv).expectString("Octopus"sv).expectString("Crab"sv).expectEnd();
                    decoder.expectKey("Gluten Free"sv).expectBoolean(false);
                decoder.expectEnd();
            decoder.expectEnd();
            decoder.expectKey("Profit Margin").expectNull();
        decoder.expectEnd();
        qc::json::decode(
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
            "Price": 18.00,
            "Ingredients": [ "Salt", "Octopus", "Crab" ],
            "Gluten Free": false
        }
    ],
    "Profit Margin": null
})"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(NoWhitespace) {
        ExpectantDecoder decoder;
        decoder.expectObject().expectKey("a"sv).expectArray().expectString("abc"sv).expectInteger(-123).expectFloater(-123.456e-78).expectBoolean(true).expectNull().expectEnd().expectEnd();
        qc::json::decode(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(ExtraneousWhitespace) {
        ExpectantDecoder decoder;
        decoder.expectObject().expectEnd();
        qc::json::decode(" \t\n\r\v{} \t\n\r\v"sv, decoder, nullptr);
        Assert::IsTrue(decoder.isDone());
    }

    TEST_METHOD(Misc) {
        { // Empty
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"()", DummyDecoder(), nullptr); });
        }
        { // Only whitespace
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(   )", DummyDecoder(), nullptr); });
        }
        { // Unknown value
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(v)", DummyDecoder(), nullptr); });
        }
        { // Multiple root values
            Assert::ExpectException<qc::json::DecodeError>([]() { qc::json::decode(R"(1 2)", DummyDecoder(), nullptr); });
        }
    }

};
