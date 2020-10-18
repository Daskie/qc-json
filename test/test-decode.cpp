#include <cmath>

#include <variant>
#include <deque>

#include <gtest/gtest.h>

#include <qc-json/qc-json-decode.hpp>

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

class DummyComposer {

  public:

    std::nullptr_t object(std::nullptr_t) { return nullptr; }
    std::nullptr_t array(std::nullptr_t) { return nullptr; }
    void end(std::nullptr_t, std::nullptr_t) {}
    void key(string && k, std::nullptr_t) {}
    void val(string_view v, std::nullptr_t) {}
    void val(int64_t v, std::nullptr_t) {}
    void val(uint64_t v, std::nullptr_t) {}
    void val(double v, std::nullptr_t) {}
    void val(bool v, std::nullptr_t) {}
    void val(std::nullptr_t, std::nullptr_t) {}

} dummyComposer;

class ExpectantComposer {

  public:

    struct Object {};
    struct Array {};
    struct End {};
    struct Key { string_view k; };
    struct String { string_view v; };
    struct SignedInteger { int64_t v; };
    struct UnsignedInteger { uint64_t v; };
    struct Floater { double v; };
    struct Boolean { bool v; };
    struct Null {};

    friend bool operator==(const Object &, const Object &) { return true; }
    friend bool operator==(const Array &, const Array &) { return true; }
    friend bool operator==(const End &, const End &) { return true; }
    friend bool operator==(const Key & a, const Key & b) { return a.k == b.k; }
    friend bool operator==(const String & a, const String & b) { return a.v == b.v; }
    friend bool operator==(const SignedInteger & a, const SignedInteger & b) { return a.v == b.v; }
    friend bool operator==(const UnsignedInteger & a, const UnsignedInteger & b) { return a.v == b.v; }
    friend bool operator==(const Floater & a, const Floater & b) { return a.v == b.v || (std::isnan(a.v) && std::isnan(b.v)); }
    friend bool operator==(const Boolean & a, const Boolean & b) { return a.v == b.v; }
    friend bool operator==(const Null &, const Null &) { return true; }

    using Element = std::variant<Object, Array, End, Key, String, SignedInteger, UnsignedInteger, Floater, Boolean, Null>;

    std::nullptr_t object(std::nullptr_t) { assertNextIs(Object{}); return nullptr; }
    std::nullptr_t array(std::nullptr_t) { assertNextIs(Array{}); return nullptr; }
    void end(std::nullptr_t, std::nullptr_t) { assertNextIs(End{}); }
    void key(string && k, std::nullptr_t) { assertNextIs(Key{k}); }
    void val(string_view v, std::nullptr_t) { assertNextIs(String{v}); }
    void val(int64_t v, std::nullptr_t) { assertNextIs(SignedInteger{v}); }
    void val(uint64_t v, std::nullptr_t) { assertNextIs(UnsignedInteger{v}); }
    void val(double v, std::nullptr_t) { assertNextIs(Floater{v}); }
    void val(bool v, std::nullptr_t) { assertNextIs(Boolean{v}); }
    void val(std::nullptr_t, std::nullptr_t) { assertNextIs(Null{}); }

    ExpectantComposer & expectObject() { m_sequence.emplace_back(Object{}); return *this; }
    ExpectantComposer & expectArray() { m_sequence.emplace_back(Array{}); return *this; }
    ExpectantComposer & expectEnd() { m_sequence.emplace_back(End{}); return *this; }
    ExpectantComposer & expectKey(string_view k) { m_sequence.emplace_back(Key{k}); return *this; }
    ExpectantComposer & expectString(string_view v) { m_sequence.emplace_back(String{v}); return *this; }
    ExpectantComposer & expectSignedInteger(int64_t v) { m_sequence.emplace_back(SignedInteger{v}); return *this; }
    ExpectantComposer & expectUnsignedInteger(uint64_t v) { m_sequence.emplace_back(UnsignedInteger{v}); return *this; }
    ExpectantComposer & expectFloater(double v) { m_sequence.emplace_back(Floater{v}); return *this; }
    ExpectantComposer & expectBoolean(bool v) { m_sequence.emplace_back(Boolean{v}); return *this; }
    ExpectantComposer & expectNull() { m_sequence.emplace_back(Null{}); return *this; }

    bool isDone() const { return m_sequence.empty(); }

  private:

    std::deque<Element> m_sequence;

    void assertNextIs(const Element & e) {
        EXPECT_EQ(m_sequence.front(), e);
        m_sequence.pop_front();
    }

};

//namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
//    //template <> static std::wstring ToString<uint16_t>(const uint16_t & v) { return std::to_wstring(v); }
//    template <> static std::wstring ToString<ExpectantComposer::Element>(const ExpectantComposer::Element & v) {
//        if (std::holds_alternative<ExpectantComposer::Object>(v)) return L"Object"s;
//        else if (std::holds_alternative<ExpectantComposer::Array>(v)) return L"Array"s;
//        else if (std::holds_alternative<ExpectantComposer::End>(v)) return L"End"s;
//        else if (std::holds_alternative<ExpectantComposer::Key>(v)) return L"Key `"s + std::wstring(std::get<ExpectantComposer::Key>(v).k.cbegin(), std::get<ExpectantComposer::Key>(v).k.cend()) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::String>(v)) return L"String `"s + std::wstring(std::get<ExpectantComposer::String>(v).v.cbegin(), std::get<ExpectantComposer::String>(v).v.cend()) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::SignedInteger>(v)) return L"Signed Integer `"s + std::to_wstring(std::get<ExpectantComposer::SignedInteger>(v).v) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::UnsignedInteger>(v)) return L"Unsigned Integer `"s + std::to_wstring(std::get<ExpectantComposer::UnsignedInteger>(v).v) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::Floater>(v)) return L"Floater `"s + std::to_wstring(std::get<ExpectantComposer::Floater>(v).v) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::Boolean>(v)) return L"Boolean `"s + (std::get<ExpectantComposer::Boolean>(v).v ? L"true"s : L"false"s) + L"`"s;
//        else if (std::holds_alternative<ExpectantComposer::Null>(v)) return L"Null"s;
//        else return L"Unknown Element";
//    }
//}}}

TEST(decode, object) {
    { // Empty
        ExpectantComposer composer;
        composer.expectObject().expectEnd();
        qc::json::decode(R"({})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single key
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectEnd();
        qc::json::decode(R"({ "a": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple keys
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectKey("c"sv).expectNull().expectEnd();
        qc::json::decode(R"({ "a": null, "b": null, "c": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        qc::json::decode(R"({"a":null,"b":null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        qc::json::decode(R"({"a" :null ,"b" :null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Key not within quotes
        EXPECT_THROW(qc::json::decode(R"({ a: 0 })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // No colon after key
        EXPECT_THROW(qc::json::decode(R"({ "a" 0 })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Empty key
        EXPECT_THROW(qc::json::decode(R"({ "": 0 })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Missing value
        EXPECT_THROW(qc::json::decode(R"({ "a": })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // No comma between elements
        EXPECT_THROW(qc::json::decode(R"({ "a": 0 "b": 1 })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Comma after last element
        EXPECT_THROW(qc::json::decode(R"({ "a": 0, })", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"({ "a": 0, "b": 1, })", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(qc::json::decode(R"({ "a": 0, , "b": 1 })", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, array) {
    { // Empty
        ExpectantComposer composer;
        composer.expectArray().expectEnd();
        qc::json::decode(R"([])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single element
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectEnd();
        qc::json::decode(R"([ null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple elements
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectNull().expectEnd();
        qc::json::decode(R"([ null, null, null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectEnd();
        qc::json::decode(R"([null,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectEnd();
        qc::json::decode(R"([null ,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No comma between elements
        EXPECT_THROW(qc::json::decode(R"([ 0 1 ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Comma after last element
        EXPECT_THROW(qc::json::decode(R"([ 0, ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 0, 1, ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(qc::json::decode(R"([ 0, , 1 ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, string) {
    { // Empty string
        ExpectantComposer composer;
        composer.expectString(""sv);
        qc::json::decode(R"("")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All printable
        ExpectantComposer composer;
        composer.expectString(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv);
        qc::json::decode(R"(" !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Escape characters
        ExpectantComposer composer;
        composer.expectString("\b\f\n\r\t"sv);
        qc::json::decode(R"("\b\f\n\r\t")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Missing escape sequence
        EXPECT_THROW(qc::json::decode(R"("\")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "\" ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Unknown escape sequence
        EXPECT_THROW(qc::json::decode(R"("\v")", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Unicode
        ExpectantComposer composer;
        composer.expectString("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
        qc::json::decode(R"("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Non-ascii unicode
        EXPECT_THROW(qc::json::decode(R"("\u0080")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"("\u0F00")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"("\uF000")", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Missing all unicode digits
        EXPECT_THROW(qc::json::decode(R"("\u")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "\u" ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"("\u0")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "\u0" ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"("\u00")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "\u00" ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"("\u000")", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "\u000" ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Missing end quote
        EXPECT_THROW(qc::json::decode(R"("abc)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ "abc ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Unknown content
        EXPECT_THROW(qc::json::decode("\"\n\"", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode("\"\t\"", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode("\"\0\"", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, signedInteger) {
    { // Zero
        ExpectantComposer composer;
        composer.expectSignedInteger(0);
        qc::json::decode(R"(0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Normal
        ExpectantComposer composer;
        composer.expectSignedInteger(123);
        qc::json::decode(R"(123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Min
        ExpectantComposer composer;
        composer.expectSignedInteger(std::numeric_limits<int64_t>::min());
        qc::json::decode(R"(-9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer;
        composer.expectSignedInteger(std::numeric_limits<int64_t>::max());
        qc::json::decode(R"(9223372036854775807)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer;
        composer.expectSignedInteger(123);
        qc::json::decode(R"(123.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid minus sign
        EXPECT_THROW(qc::json::decode(R"(-)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ - ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Plus sign
        EXPECT_THROW(qc::json::decode(R"(+)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"(+123)", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Dangling decimal point
        EXPECT_THROW(qc::json::decode(R"(123.)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 123. ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, unsignedInteger) {
    { // Min unsigned
        ExpectantComposer composer;
        composer.expectUnsignedInteger(uint64_t(std::numeric_limits<int64_t>::max()) + 1u);
        qc::json::decode(R"(9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max unsigned
        ExpectantComposer composer;
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        qc::json::decode(R"(18446744073709551615)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer;
        composer.expectUnsignedInteger(10000000000000000000u);
        qc::json::decode(R"(10000000000000000000.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid minus sign
        EXPECT_THROW(qc::json::decode(R"(-)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ - ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Plus sign
        EXPECT_THROW(qc::json::decode(R"(+)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"(+123)", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Dangling decimal point
        EXPECT_THROW(qc::json::decode(R"(10000000000000000000.)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 10000000000000000000. ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, floater) {
    { // Fractional
        ExpectantComposer composer;
        composer.expectFloater(123.456);
        qc::json::decode(R"(123.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent lowercase
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        qc::json::decode(R"(123.456e17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent uppercase
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        qc::json::decode(R"(123.456E17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Positive exponent
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        qc::json::decode(R"(123.456e+17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Negative exponent
        ExpectantComposer composer;
        composer.expectFloater(-123.456e-17);
        qc::json::decode(R"(-123.456e-17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent without fraction
        ExpectantComposer composer;
        composer.expectFloater(123.0e34);
        qc::json::decode(R"(123e34)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max integer
        ExpectantComposer composer;
        composer.expectFloater(9007199254740991.0);
        qc::json::decode(R"(9007199254740991.0e0)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized signed integer
        ExpectantComposer composer;
        composer.expectFloater(-9223372036854775809.0);
        qc::json::decode(R"(-9223372036854775809)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized unsigned integer
        ExpectantComposer composer;
        composer.expectFloater(18446744073709551616.0);
        qc::json::decode(R"(18446744073709551616)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // infinity
        ExpectantComposer composer;
        composer.expectFloater(std::numeric_limits<double>::infinity());
        qc::json::decode(R"(inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // -infinity
        ExpectantComposer composer;
        composer.expectFloater(-std::numeric_limits<double>::infinity());
        qc::json::decode(R"(-inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // NaN
        ExpectantComposer composer;
        composer.expectFloater(std::numeric_limits<double>::quiet_NaN());
        qc::json::decode(R"(nan)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Dangling decimal point
        EXPECT_THROW(qc::json::decode(R"(0.)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 0. ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Dangling exponent
        EXPECT_THROW(qc::json::decode(R"(0e)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 0e ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"(0e+)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 0e+ ])", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"(0e-)", dummyComposer, nullptr), qc::json::DecodeError);
        EXPECT_THROW(qc::json::decode(R"([ 0e- ])", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Magnitude too large
        EXPECT_THROW(qc::json::decode(R"(1e1000)", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Magnitude too small
        EXPECT_THROW(qc::json::decode(R"(1e-1000)", dummyComposer, nullptr), qc::json::DecodeError);
    }
}

TEST(decode, boolean) {
    { // True
        ExpectantComposer composer;
        composer.expectBoolean(true);
        qc::json::decode(R"(true)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // False
        ExpectantComposer composer;
        composer.expectBoolean(false);
        qc::json::decode(R"(false)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, null) {
    ExpectantComposer composer;
    composer.expectNull();
    qc::json::decode(R"(null)"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, general) {
    ExpectantComposer composer;
    composer.expectObject();
        composer.expectKey("Name"sv).expectString("Salt's Crust"sv);
        composer.expectKey("Founded"sv).expectSignedInteger(1964);
        composer.expectKey("Employees"sv).expectArray();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Ol' Joe Fisher"sv);
                composer.expectKey("Title"sv).expectString("Fisherman"sv);
                composer.expectKey("Age"sv).expectSignedInteger(69);
            composer.expectEnd();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Mark Rower"sv);
                composer.expectKey("Title"sv).expectString("Cook"sv);
                composer.expectKey("Age"sv).expectSignedInteger(41);
            composer.expectEnd();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Phineas"sv);
                composer.expectKey("Title"sv).expectString("Server Boy"sv);
                composer.expectKey("Age"sv).expectSignedInteger(19);
            composer.expectEnd();
        composer.expectEnd();
        composer.expectKey("Dishes"sv).expectArray();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Basket o' Barnacles"sv);
                composer.expectKey("Price"sv).expectFloater(5.45);
                composer.expectKey("Ingredients"sv).expectArray().expectString("Salt"sv).expectString("Barnacles"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(false);
            composer.expectEnd();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Two Tuna"sv);
                composer.expectKey("Price"sv).expectFloater(14.99);
                composer.expectKey("Ingredients"sv).expectArray().expectString("Tuna"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(true);
            composer.expectEnd();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("18 Leg Bouquet"sv);
                composer.expectKey("Price"sv).expectFloater(18.18);
                composer.expectKey("Ingredients"sv).expectArray().expectString("Salt"sv).expectString("Octopus"sv).expectString("Crab"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(false);
            composer.expectEnd();
        composer.expectEnd();
        composer.expectKey("Profit Margin").expectNull();
    composer.expectEnd();
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
            "Price": 18.18,
            "Ingredients": [ "Salt", "Octopus", "Crab" ],
            "Gluten Free": false
        }
    ],
    "Profit Margin": null
})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, noWhitespace) {
    ExpectantComposer composer;
    composer.expectObject().expectKey("a"sv).expectArray().expectString("abc"sv).expectSignedInteger(-123).expectFloater(-123.456e-78).expectBoolean(true).expectNull().expectEnd().expectEnd();
    qc::json::decode(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, extraneousWhitespace) {
    ExpectantComposer composer;
    composer.expectObject().expectEnd();
    qc::json::decode(" \t\n\r\v{} \t\n\r\v"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, misc) {
    { // Empty
        EXPECT_THROW(qc::json::decode(R"()", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Only whitespace
        EXPECT_THROW(qc::json::decode(R"(   )", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Unknown value
        EXPECT_THROW(qc::json::decode(R"(v)", dummyComposer, nullptr), qc::json::DecodeError);
    }
    { // Multiple root values
        EXPECT_THROW(qc::json::decode(R"(1 2)", dummyComposer, nullptr), qc::json::DecodeError);
    }
}
