#include <cmath>

#include <deque>
#include <format>
#include <variant>

#include <gtest/gtest.h>

#include <qc-json/qc-json-decode.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::decode;
using qc::json::DecodeError;

class DummyComposer {

  public: //--------------------------------------------------------------------

    std::nullptr_t object(std::nullptr_t) { return nullptr; }
    std::nullptr_t array(std::nullptr_t) { return nullptr; }
    void end(std::nullptr_t, std::nullptr_t) {}
    void key(std::string &&, std::nullptr_t) {}
    void val(std::string_view, std::nullptr_t) {}
    void val(int64_t, std::nullptr_t) {}
    void val(uint64_t, std::nullptr_t) {}
    void val(double, std::nullptr_t) {}
    void val(bool, std::nullptr_t) {}
    void val(std::nullptr_t, std::nullptr_t) {}

} dummyComposer;

class ExpectantComposer {

  public: //--------------------------------------------------------------------

    struct Object {};
    struct Array {};
    struct End {};
    struct Key { std::string_view k; };
    struct String { std::string_view v; };
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
    void key(std::string && k, std::nullptr_t) { assertNextIs(Key{k}); }
    void val(std::string_view v, std::nullptr_t) { assertNextIs(String{v}); }
    void val(int64_t v, std::nullptr_t) { assertNextIs(SignedInteger{v}); }
    void val(uint64_t v, std::nullptr_t) { assertNextIs(UnsignedInteger{v}); }
    void val(double v, std::nullptr_t) { assertNextIs(Floater{v}); }
    void val(bool v, std::nullptr_t) { assertNextIs(Boolean{v}); }
    void val(std::nullptr_t, std::nullptr_t) { assertNextIs(Null{}); }

    ExpectantComposer & expectObject() { m_sequence.emplace_back(Object{}); return *this; }
    ExpectantComposer & expectArray() { m_sequence.emplace_back(Array{}); return *this; }
    ExpectantComposer & expectEnd() { m_sequence.emplace_back(End{}); return *this; }
    ExpectantComposer & expectKey(std::string_view k) { m_sequence.emplace_back(Key{k}); return *this; }
    ExpectantComposer & expectString(std::string_view v) { m_sequence.emplace_back(String{v}); return *this; }
    ExpectantComposer & expectSignedInteger(int64_t v) { m_sequence.emplace_back(SignedInteger{v}); return *this; }
    ExpectantComposer & expectUnsignedInteger(uint64_t v) { m_sequence.emplace_back(UnsignedInteger{v}); return *this; }
    ExpectantComposer & expectFloater(double v) { m_sequence.emplace_back(Floater{v}); return *this; }
    ExpectantComposer & expectBoolean(bool v) { m_sequence.emplace_back(Boolean{v}); return *this; }
    ExpectantComposer & expectNull() { m_sequence.emplace_back(Null{}); return *this; }

    bool isDone() const { return m_sequence.empty(); }

  private: //-------------------------------------------------------------------

    std::deque<Element> m_sequence;

    void assertNextIs(const Element & e) {
        EXPECT_EQ(m_sequence.front(), e);
        m_sequence.pop_front();
    }

};

std::ostream & operator<<(std::ostream & os, const ExpectantComposer::Element & v)
{
    if (std::holds_alternative<ExpectantComposer::Object>(v)) return os << "Object";
    if (std::holds_alternative<ExpectantComposer::Array>(v)) return os << "Array";
    if (std::holds_alternative<ExpectantComposer::End>(v)) return os << "End";
    if (std::holds_alternative<ExpectantComposer::Key>(v)) return os << "Key `" << std::get<ExpectantComposer::Key>(v).k << "`";
    if (std::holds_alternative<ExpectantComposer::String>(v)) return os << "String `" << std::get<ExpectantComposer::String>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::SignedInteger>(v)) return os << "Signed Integer `" << std::get<ExpectantComposer::SignedInteger>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::UnsignedInteger>(v)) return os << "Unsigned Integer `" << std::get<ExpectantComposer::UnsignedInteger>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::Floater>(v)) return os << "Floater `" << std::get<ExpectantComposer::Floater>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::Boolean>(v)) return os << "Boolean `" << (std::get<ExpectantComposer::Boolean>(v).v ? "true" : "false") << "`";
    if (std::holds_alternative<ExpectantComposer::Null>(v)) return os << "Null";
    return os << "Unknown Element";
}

TEST(decode, object) {
    { // Empty
        ExpectantComposer composer;
        composer.expectObject().expectEnd();
        decode(R"({})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single key
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectEnd();
        decode(R"({ "a": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple keys
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectKey("c"sv).expectNull().expectEnd();
        decode(R"({ "a": null, "b": null, "c": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        decode(R"({"a":null,"b":null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer;
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        decode(R"({"a" :null ,"b" :null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Key not within quotes
        EXPECT_THROW(decode(R"({ a: 0 })", dummyComposer, nullptr), DecodeError);
    }
    { // No colon after key
        EXPECT_THROW(decode(R"({ "a" 0 })", dummyComposer, nullptr), DecodeError);
    }
    { // Empty key
        EXPECT_THROW(decode(R"({ "": 0 })", dummyComposer, nullptr), DecodeError);
    }
    { // Missing value
        EXPECT_THROW(decode(R"({ "a": })", dummyComposer, nullptr), DecodeError);
    }
    { // No comma between elements
        EXPECT_THROW(decode(R"({ "a": 0 "b": 1 })", dummyComposer, nullptr), DecodeError);
    }
    { // Comma after last element
        EXPECT_THROW(decode(R"({ "a": 0, })", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({ "a": 0, "b": 1, })", dummyComposer, nullptr), DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(decode(R"({ "a": 0, , "b": 1 })", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, array) {
    { // Empty
        ExpectantComposer composer;
        composer.expectArray().expectEnd();
        decode(R"([])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single element
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectEnd();
        decode(R"([ null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple elements
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectNull().expectEnd();
        decode(R"([ null, null, null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectEnd();
        decode(R"([null,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer;
        composer.expectArray().expectNull().expectNull().expectEnd();
        decode(R"([null ,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No comma between elements
        EXPECT_THROW(decode(R"([ 0 1 ])", dummyComposer, nullptr), DecodeError);
    }
    { // Comma after last element
        EXPECT_THROW(decode(R"([ 0, ])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 0, 1, ])", dummyComposer, nullptr), DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(decode(R"([ 0, , 1 ])", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, string) {
    { // Empty string
        ExpectantComposer composer;
        composer.expectString(""sv);
        decode(R"("")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All printable
        ExpectantComposer composer;
        composer.expectString(R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv);
        decode(R"(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All non-printable
        std::string decodeStr{R"(" ")"};
        for (int i{0}; i < 256; ++i) {
            if (!std::isprint(i)) {
                decodeStr[1] = char(i);
                EXPECT_THROW(decode(decodeStr, dummyComposer, nullptr), DecodeError);
            }
        }
    }
    { // Escape characters
        ExpectantComposer composer;
        composer.expectString("\0\b\t\n\v\f\r"sv);
        decode(R"("\0\b\t\n\v\f\r")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Missing escape sequence
        const std::string_view brokenSeq{R"("\\\")"};
        EXPECT_THROW(decode(brokenSeq, dummyComposer, nullptr), DecodeError);
        const std::string_view brokenSeqInArray{R"([ "\\\" ])"};
        EXPECT_THROW(decode(brokenSeqInArray, dummyComposer, nullptr), DecodeError);
    }
    { // Unknown escape sequence
        EXPECT_THROW(decode("\"\\\0\"", dummyComposer, nullptr), DecodeError);
    }
    { // 'x' code point
        ExpectantComposer composer;
        std::string expectedStr(256, '\0');
        std::string decodeStr(1 + 256 * 4 + 1, '\0');
        decodeStr.front() = '"';
        decodeStr.back() = '"';
        for (int i{0}; i < 256; ++i) {
            expectedStr[i] = char(i);
            std::format_to_n(&decodeStr[1 + 4 * i], 4, "\\x{:02X}"sv, i);
        }
        composer.expectString(expectedStr);
        decode(decodeStr, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // 'u' code point
        ExpectantComposer composer;
        std::string expectedStr(256, '\0');
        std::string decodeStr(1 + 256 * 6 + 1, '\0');
        decodeStr.front() = '"';
        decodeStr.back() = '"';
        for (int i{0}; i < 256; ++i) {
            expectedStr[i] = char(i);
            std::format_to_n(&decodeStr[1 + 6 * i], 6, "\\u{:04X}"sv, i);
        }
        composer.expectString(expectedStr);
        decode(decodeStr, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Uppercase and lowercase code point hex digits
        ExpectantComposer composer{};
        composer.expectString("\u00AA\u00BB\u00CC\u00DD");
        decode(R"("\u00aa\u00BB\u00cC\u00Dd")", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Incorrect number of code point digits
        // Raw strings, `\x`/`\u`, and macros don't play nice together
        EXPECT_THROW(decode("\"\\x\"", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode("\"\\x1\"", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode("\"\\u\"", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode("\"\\u1\"", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode("\"\\u11\"", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode("\"\\u111\"", dummyComposer, nullptr), DecodeError);
    }
    { // Missing end quote
        EXPECT_THROW(decode(R"("abc)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ "abc ])", dummyComposer, nullptr), DecodeError);
    }
    { // Excaped newlines
        ExpectantComposer composer{};
        composer.expectString("abc");
        decode("\"a\\\nb\\\nc\"", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectString("abc");
        decode("\"a\\\r\nb\\\r\nc\"", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectString("");
        decode("\"\\\n\\\n\\\n\"", composer, nullptr);
        composer.expectString("");
        decode("\"\\\r\n\\\n\\\r\n\"", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, signedInteger) {
    { // Zero
        ExpectantComposer composer;
        composer.expectSignedInteger(0);
        decode(R"(0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Normal
        ExpectantComposer composer;
        composer.expectSignedInteger(123);
        decode(R"(123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Min
        ExpectantComposer composer;
        composer.expectSignedInteger(std::numeric_limits<int64_t>::min());
        decode(R"(-9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer;
        composer.expectSignedInteger(std::numeric_limits<int64_t>::max());
        decode(R"(9223372036854775807)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer;
        composer.expectSignedInteger(123);
        decode(R"(123.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid minus sign
        EXPECT_THROW(decode(R"(-)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ - ])", dummyComposer, nullptr), DecodeError);
    }
    { // Plus sign
        EXPECT_THROW(decode(R"(+)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(+123)", dummyComposer, nullptr), DecodeError);
    }
    { // Dangling decimal point
        EXPECT_THROW(decode(R"(123.)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 123. ])", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, unsignedInteger) {
    { // Min unsigned
        ExpectantComposer composer;
        composer.expectUnsignedInteger(uint64_t(std::numeric_limits<int64_t>::max()) + 1u);
        decode(R"(9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max unsigned
        ExpectantComposer composer;
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(18446744073709551615)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer;
        composer.expectUnsignedInteger(10000000000000000000u);
        decode(R"(10000000000000000000.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid minus sign
        EXPECT_THROW(decode(R"(-)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ - ])", dummyComposer, nullptr), DecodeError);
    }
    { // Plus sign
        EXPECT_THROW(decode(R"(+)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(+123)", dummyComposer, nullptr), DecodeError);
    }
    { // Dangling decimal point
        EXPECT_THROW(decode(R"(10000000000000000000.)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 10000000000000000000. ])", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, floater) {
    { // Fractional
        ExpectantComposer composer;
        composer.expectFloater(123.456);
        decode(R"(123.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent lowercase
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        decode(R"(123.456e17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent uppercase
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        decode(R"(123.456E17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Positive exponent
        ExpectantComposer composer;
        composer.expectFloater(123.456e17);
        decode(R"(123.456e+17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Negative exponent
        ExpectantComposer composer;
        composer.expectFloater(-123.456e-17);
        decode(R"(-123.456e-17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent without fraction
        ExpectantComposer composer;
        composer.expectFloater(123.0e34);
        decode(R"(123e34)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max integer
        ExpectantComposer composer;
        composer.expectFloater(9007199254740991.0);
        decode(R"(9007199254740991.0e0)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized signed integer
        ExpectantComposer composer;
        composer.expectFloater(-9223372036854775809.0);
        decode(R"(-9223372036854775809)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized unsigned integer
        ExpectantComposer composer;
        composer.expectFloater(18446744073709551616.0);
        decode(R"(18446744073709551616)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // infinity
        ExpectantComposer composer;
        composer.expectFloater(std::numeric_limits<double>::infinity());
        decode(R"(inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // -infinity
        ExpectantComposer composer;
        composer.expectFloater(-std::numeric_limits<double>::infinity());
        decode(R"(-inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // NaN
        ExpectantComposer composer;
        composer.expectFloater(std::numeric_limits<double>::quiet_NaN());
        decode(R"(nan)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Dangling decimal point
        EXPECT_THROW(decode(R"(0.)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 0. ])", dummyComposer, nullptr), DecodeError);
    }
    { // Dangling exponent
        EXPECT_THROW(decode(R"(0e)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 0e ])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0e+)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 0e+ ])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0e-)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ 0e- ])", dummyComposer, nullptr), DecodeError);
    }
    { // Magnitude too large
        EXPECT_THROW(decode(R"(1e1000)", dummyComposer, nullptr), DecodeError);
    }
    { // Magnitude too small
        EXPECT_THROW(decode(R"(1e-1000)", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, boolean) {
    { // True
        ExpectantComposer composer;
        composer.expectBoolean(true);
        decode(R"(true)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // False
        ExpectantComposer composer;
        composer.expectBoolean(false);
        decode(R"(false)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, null) {
    ExpectantComposer composer;
    composer.expectNull();
    decode(R"(null)"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, noWhitespace) {
    ExpectantComposer composer;
    composer.expectObject().expectKey("a"sv).expectArray().expectString("abc"sv).expectSignedInteger(-123).expectFloater(-123.456e-78).expectBoolean(true).expectNull().expectEnd().expectEnd();
    decode(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, extraneousWhitespace) {
    ExpectantComposer composer;
    composer.expectObject().expectEnd();
    decode(" \t\n\r\v{} \t\n\r\v"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, misc) {
    { // Empty
        EXPECT_THROW(decode(R"()", dummyComposer, nullptr), DecodeError);
    }
    { // Only whitespace
        EXPECT_THROW(decode(R"(   )", dummyComposer, nullptr), DecodeError);
    }
    { // Unknown value
        EXPECT_THROW(decode(R"(v)", dummyComposer, nullptr), DecodeError);
    }
    { // Multiple root values
        EXPECT_THROW(decode(R"(1 2)", dummyComposer, nullptr), DecodeError);
    }
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
        composer.expectKey("Profit Margin"sv).expectNull();
        composer.expectKey("Ha\x03r Name"sv).expectString("M\0\0n"sv);
        composer.expectKey("Green Eggs and Ham"sv).expectString(
R"(I do not like them in a box
I do not like them with a fox
I do not like them in a house
I do not like them with a mouse
I do not like them here or there
I do not like them anywhere
I do not like green eggs and ham
I do not like them Sam I am
)");
    composer.expectEnd();
    decode(
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
    "Profit Margin": null,
    "Ha\x03r Name": "M\u0000\0n",
    "Green Eggs and Ham":
"\
I do not like them in a box\n\
I do not like them with a fox\n\
I do not like them in a house\n\
I do not like them with a mouse\n\
I do not like them here or there\n\
I do not like them anywhere\n\
I do not like green eggs and ham\n\
I do not like them Sam I am\n\
"
})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}
