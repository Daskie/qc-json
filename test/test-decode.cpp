#include <cmath>

#include <deque>
#include <format>
#include <variant>

#include <gtest/gtest.h>

#include <qc-json-decode.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::decode;
using qc::json::DecodeError;
using qc::json::Density;
using qc::json::Container;

static qc::json::DummyComposer dummyComposer{};

class ExpectantComposer
{
    public: //------------------------------------------------------------------

    struct Object {};
    struct Array {};
    struct End { Density d; };
    struct Key { std::string_view k; };
    struct String { std::string_view v; };
    struct SignedInteger { int64_t v; };
    struct UnsignedInteger { uint64_t v; };
    struct Floater { double v; };
    struct Boolean { bool v; };
    struct Null {};
    struct Comment { std::string_view str; };

    friend bool operator==(const Object &, const Object &) { return true; }
    friend bool operator==(const Array &, const Array &) { return true; }
    friend bool operator==(const End & a, const End & b) { return a.d == b.d || a.d == Density::unspecified || b.d == Density::unspecified; }
    friend bool operator==(const Key & a, const Key & b) { return a.k == b.k; }
    friend bool operator==(const String & a, const String & b) { return a.v == b.v; }
    friend bool operator==(const SignedInteger & a, const SignedInteger & b) { return a.v == b.v; }
    friend bool operator==(const UnsignedInteger & a, const UnsignedInteger & b) { return a.v == b.v; }
    friend bool operator==(const Floater & a, const Floater & b) { return a.v == b.v || (std::isnan(a.v) && std::isnan(b.v)); }
    friend bool operator==(const Boolean & a, const Boolean & b) { return a.v == b.v; }
    friend bool operator==(const Null &, const Null &) { return true; }
    friend bool operator==(const Comment & a, const Comment & b) { return a.str == b.str; }

    using Element = std::variant<Object, Array, End, Key, String, SignedInteger, UnsignedInteger, Floater, Boolean, Null, Comment>;

    std::nullptr_t object(std::nullptr_t) { assertNextIs(Object{}); return nullptr; }
    std::nullptr_t array(std::nullptr_t) { assertNextIs(Array{}); return nullptr; }
    void end(Density d, std::nullptr_t, std::nullptr_t) { assertNextIs(End{d}); }
    void key(std::string_view k, std::nullptr_t) { assertNextIs(Key{k}); }
    void val(std::string_view v, std::nullptr_t) { assertNextIs(String{v}); }
    void val(int64_t v, std::nullptr_t) { assertNextIs(SignedInteger{v}); }
    void val(uint64_t v, std::nullptr_t) { assertNextIs(UnsignedInteger{v}); }
    void val(double v, std::nullptr_t) { assertNextIs(Floater{v}); }
    void val(bool v, std::nullptr_t) { assertNextIs(Boolean{v}); }
    void val(std::nullptr_t, std::nullptr_t) { assertNextIs(Null{}); }
    void comment(std::string_view str, std::nullptr_t) { assertNextIs(Comment{str}); }

    ExpectantComposer & expectObject() { m_sequence.emplace_back(Object{}); return *this; }
    ExpectantComposer & expectArray() { m_sequence.emplace_back(Array{}); return *this; }
    ExpectantComposer & expectEnd(Density d = Density::unspecified) { m_sequence.emplace_back(End{d}); return *this; }
    ExpectantComposer & expectKey(std::string_view k) { m_sequence.emplace_back(Key{k}); return *this; }
    ExpectantComposer & expectString(std::string_view v) { m_sequence.emplace_back(String{v}); return *this; }
    ExpectantComposer & expectSignedInteger(int64_t v) { m_sequence.emplace_back(SignedInteger{v}); return *this; }
    ExpectantComposer & expectUnsignedInteger(uint64_t v) { m_sequence.emplace_back(UnsignedInteger{v}); return *this; }
    ExpectantComposer & expectFloater(double v) { m_sequence.emplace_back(Floater{v}); return *this; }
    ExpectantComposer & expectBoolean(bool v) { m_sequence.emplace_back(Boolean{v}); return *this; }
    ExpectantComposer & expectNull() { m_sequence.emplace_back(Null{}); return *this; }
    ExpectantComposer & expectComment(std::string_view str) { m_sequence.emplace_back(Comment{str}); return *this; }

    bool isDone() const { return m_sequence.empty(); }

    private: //-----------------------------------------------------------------

    std::deque<Element> m_sequence;

    void assertNextIs(const Element & e) {
        EXPECT_FALSE(m_sequence.empty());
        EXPECT_EQ(m_sequence.front(), e);
        m_sequence.pop_front();
    }
};

std::ostream & operator<<(std::ostream & os, const ExpectantComposer::Element & v)
{
    if (std::holds_alternative<ExpectantComposer::Object>(v)) return os << "Object";
    if (std::holds_alternative<ExpectantComposer::Array>(v)) return os << "Array";
    if (std::holds_alternative<ExpectantComposer::End>(v)) return os << "End `" << int(std::get<ExpectantComposer::End>(v).d) << "`";
    if (std::holds_alternative<ExpectantComposer::Key>(v)) return os << "Key `" << std::get<ExpectantComposer::Key>(v).k << "`";
    if (std::holds_alternative<ExpectantComposer::String>(v)) return os << "String `" << std::get<ExpectantComposer::String>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::SignedInteger>(v)) return os << "Signed Integer `" << std::get<ExpectantComposer::SignedInteger>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::UnsignedInteger>(v)) return os << "Unsigned Integer `" << std::get<ExpectantComposer::UnsignedInteger>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::Floater>(v)) return os << "Floater `" << std::get<ExpectantComposer::Floater>(v).v << "`";
    if (std::holds_alternative<ExpectantComposer::Boolean>(v)) return os << "Boolean `" << (std::get<ExpectantComposer::Boolean>(v).v ? "true" : "false") << "`";
    if (std::holds_alternative<ExpectantComposer::Null>(v)) return os << "Null";
    if (std::holds_alternative<ExpectantComposer::Comment>(v)) return os << "Comment `" << std::get<ExpectantComposer::Comment>(v).str << "`";
    return os << "Unknown Element";
}

TEST(decode, object)
{
    { // Empty
        ExpectantComposer composer{};
        composer.expectObject().expectEnd();
        decode(R"({})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single key
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectNull().expectEnd();
        decode(R"({ "a": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple keys
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectKey("c"sv).expectNull().expectEnd();
        decode(R"({ "a": null, "b": null, "c": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        decode(R"({"a":null,"b":null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectNull().expectKey("b"sv).expectNull().expectEnd();
        decode(R"({"a" :null ,"b" :null})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Empty key
        ExpectantComposer composer{};
        composer.expectObject().expectKey(""sv).expectNull().expectEnd();
        decode(R"({ "": null })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No colon after key
        EXPECT_THROW(decode(R"({ "a" 0 })", dummyComposer, nullptr), DecodeError);
    }
    { // Missing value
        EXPECT_THROW(decode(R"({ "a": })", dummyComposer, nullptr), DecodeError);
    }
    { // No comma between elements
        EXPECT_THROW(decode(R"({ "a": 0 "b": 1 })", dummyComposer, nullptr), DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(decode(R"({ "a": 0, , "b": 1 })", dummyComposer, nullptr), DecodeError);
    }
    { // Cut off
        EXPECT_THROW(decode(R"({)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({")", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a")", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,")", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,"b)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,"b")", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,"b":)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"a":0,"b":1)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"("a":0,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(a":0,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(":0,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(:0,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(,"b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"("b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(b":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(":1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(:1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1})", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(})", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, array)
{
    { // Empty
        ExpectantComposer composer{};
        composer.expectArray().expectEnd();
        decode(R"([])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Single element
        ExpectantComposer composer{};
        composer.expectArray().expectNull().expectEnd();
        decode(R"([ null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple elements
        ExpectantComposer composer{};
        composer.expectArray().expectNull().expectNull().expectNull().expectEnd();
        decode(R"([ null, null, null ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No space
        ExpectantComposer composer{};
        composer.expectArray().expectNull().expectNull().expectEnd();
        decode(R"([null,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weird spacing
        ExpectantComposer composer{};
        composer.expectArray().expectNull().expectNull().expectEnd();
        decode(R"([null ,null])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // No comma between elements
        EXPECT_THROW(decode(R"([ 0 1 ])", dummyComposer, nullptr), DecodeError);
    }
    { // Empty entry
        EXPECT_THROW(decode(R"([ 0, , 1 ])", dummyComposer, nullptr), DecodeError);
    }
    { // Cut off
        EXPECT_THROW(decode(R"([)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0,)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0,1)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0,1])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(,1])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1])", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(])", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, string)
{
    { // Empty string
        ExpectantComposer composer{};
        composer.expectString(""sv);
        decode(R"("")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All printable
        ExpectantComposer composer{};
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
        ExpectantComposer composer{};
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
        ExpectantComposer composer{};
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
        ExpectantComposer composer{};
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
    { // Escaped newlines
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
    { // Single/double quotes
        ExpectantComposer composer{};
        composer.expectString(R"(a'b"c)");
        decode(R"('a\'b"c')", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectString(R"(a'b"c)");
        decode(R"("a'b\"c")", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey(R"(""")").expectString(R"(''')").expectEnd();
        decode(R"({ '"""': "'''" })", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey(R"(''')").expectString(R"(""")").expectEnd();
        decode(R"({ '\'\'\'': "\"\"\"" })", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, signedInteger)
{
    { // Zero
        ExpectantComposer composer{};
        composer.expectSignedInteger(0);
        decode(R"(0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Normal
        ExpectantComposer composer{};
        composer.expectSignedInteger(123);
        decode(R"(123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Positive
        ExpectantComposer composer{};
        composer.expectSignedInteger(123);
        decode(R"(+123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Negative
        ExpectantComposer composer{};
        composer.expectSignedInteger(-123);
        decode(R"(-123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Min
        ExpectantComposer composer{};
        composer.expectSignedInteger(std::numeric_limits<int64_t>::min());
        decode(R"(-9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer{};
        composer.expectSignedInteger(std::numeric_limits<int64_t>::max());
        decode(R"(9223372036854775807)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing decimal
        ExpectantComposer composer{};
        composer.expectSignedInteger(123);
        decode(R"(123.)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer{};
        composer.expectSignedInteger(123);
        decode(R"(123.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading zeroes
        ExpectantComposer composer{};
        composer.expectSignedInteger(123);
        decode(R"(0123)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(00)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(+00)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(-00)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Fractional zero
        ExpectantComposer composer{};
        composer.expectSignedInteger(0);
        decode(R"(.0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(+.0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(-.0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid minus sign
        EXPECT_THROW(decode(R"(-)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ - ])", dummyComposer, nullptr), DecodeError);
    }
    { // Invalid plus sign
        EXPECT_THROW(decode(R"(+)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([ + ])", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, unsignedInteger)
{
    { // Min unsigned
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(uint64_t(std::numeric_limits<int64_t>::max()) + 1u);
        decode(R"(9223372036854775808)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max unsigned
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(18446744073709551615)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing decimal
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10000000000000000000u);
        decode(R"(10000000000000000000.)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10000000000000000000u);
        decode(R"(10000000000000000000.000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Positive
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10000000000000000000u);
        decode(R"(+10000000000000000000)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, hex)
{
    { // Zero
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(0u);
        decode(R"(0x0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Lowercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(26u);
        decode(R"(0x1a)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Uppercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(26u);
        decode(R"(0X1A)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0xFFFFFFFFFFFFFFFF)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(26u);
        decode(R"(0x001A)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max with leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0x00000000FFFFFFFFFFFFFFFF)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid digit
        EXPECT_THROW(decode(R"(0x1G)", dummyComposer, nullptr), DecodeError);
    }
    { // Signed
        EXPECT_THROW(decode(R"(+0x1A)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(-0x1A)", dummyComposer, nullptr), DecodeError);
    }
    { // Prefix leading zero
        EXPECT_THROW(decode(R"(00x1A)", dummyComposer, nullptr), DecodeError);
    }
    { // Decimal
        EXPECT_THROW(decode(R"(0x1A.)", dummyComposer, nullptr), DecodeError);
    }
    { // Too big
        EXPECT_THROW(decode(R"(0x10000000000000000)", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, octal)
{
    { // Zero
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(0u);
        decode(R"(0o0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Lowercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10u);
        decode(R"(0o12)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Uppercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10u);
        decode(R"(0O12)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0o1777777777777777777777)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(10u);
        decode(R"(0o0012)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max with leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0o000000001777777777777777777777)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid digit
        EXPECT_THROW(decode(R"(0o18)", dummyComposer, nullptr), DecodeError);
    }
    { // Signed
        EXPECT_THROW(decode(R"(+0o12)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(-0o12)", dummyComposer, nullptr), DecodeError);
    }
    { // Prefix leading zero
        EXPECT_THROW(decode(R"(00o12)", dummyComposer, nullptr), DecodeError);
    }
    { // Decimal
        EXPECT_THROW(decode(R"(0o12.)", dummyComposer, nullptr), DecodeError);
    }
    { // Too big
        EXPECT_THROW(decode(R"(0o2000000000000000000000)", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, binary)
{
    { // Zero
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(0u);
        decode(R"(0b0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Lowercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(5u);
        decode(R"(0b101)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Uppercase
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(5u);
        decode(R"(0B101)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0b1111111111111111111111111111111111111111111111111111111111111111)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(5u);
        decode(R"(0b00101)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max with leading zeroes
        ExpectantComposer composer{};
        composer.expectUnsignedInteger(std::numeric_limits<uint64_t>::max());
        decode(R"(0b000000001111111111111111111111111111111111111111111111111111111111111111)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid digit
        EXPECT_THROW(decode(R"(0b121)", dummyComposer, nullptr), DecodeError);
    }
    { // Signed
        EXPECT_THROW(decode(R"(+0b101)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(-0b101)", dummyComposer, nullptr), DecodeError);
    }
    { // Prefix leading zero
        EXPECT_THROW(decode(R"(00b101)", dummyComposer, nullptr), DecodeError);
    }
    { // Decimal
        EXPECT_THROW(decode(R"(0b101.)", dummyComposer, nullptr), DecodeError);
    }
    { // Too big
        EXPECT_THROW(decode(R"(0b10000000000000000000000000000000000000000000000000000000000000000)", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, floater)
{
    { // Fractional
        ExpectantComposer composer{};
        composer.expectFloater(123.456);
        decode(R"(123.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent lowercase
        ExpectantComposer composer{};
        composer.expectFloater(123.456e17);
        decode(R"(123.456e17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent uppercase
        ExpectantComposer composer{};
        composer.expectFloater(123.456e17);
        decode(R"(123.456E17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Positive exponent
        ExpectantComposer composer{};
        composer.expectFloater(123.456e17);
        decode(R"(123.456e+17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Negative exponent
        ExpectantComposer composer{};
        composer.expectFloater(-123.456e-17);
        decode(R"(-123.456e-17)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Exponent without fraction
        ExpectantComposer composer{};
        composer.expectFloater(123.0e34);
        decode(R"(123e34)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Max integer
        ExpectantComposer composer{};
        composer.expectFloater(9007199254740991.0);
        decode(R"(9007199254740991.0e0)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized signed integer
        ExpectantComposer composer{};
        composer.expectFloater(-9223372036854775809.0);
        decode(R"(-9223372036854775809)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Oversized unsigned integer
        ExpectantComposer composer{};
        composer.expectFloater(18446744073709551616.0);
        decode(R"(18446744073709551616)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading decimal
        ExpectantComposer composer{};
        composer.expectFloater(0.456);
        decode(R"(.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(0.456);
        decode(R"(+.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(-0.456);
        decode(R"(-.456)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Trailing decimal
        ExpectantComposer composer{};
        composer.expectFloater(123);
        decode(R"(123.e0)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Leading zeroes
        ExpectantComposer composer{};
        composer.expectFloater(1.2);
        decode(R"(01.2)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(0.2);
        decode(R"(00.2)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(1e02);
        decode(R"(1e02)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(1e+02);
        decode(R"(1e+02)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(1e-02);
        decode(R"(1e-02)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(1e00);
        decode(R"(1e00)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Valid infinity
        ExpectantComposer composer{};
        composer.expectFloater(std::numeric_limits<double>::infinity());
        decode(R"(inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(-std::numeric_limits<double>::infinity());
        decode(R"(-inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(std::numeric_limits<double>::infinity());
        decode(R"(+inf)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(std::numeric_limits<double>::infinity());
        decode(R"(Infinity)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(-std::numeric_limits<double>::infinity());
        decode(R"(-Infinity)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(std::numeric_limits<double>::infinity());
        decode(R"(+Infinity)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid infinity
        EXPECT_THROW(decode(R"(Inf)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(iNf)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(inF)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(INF)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infi)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infin)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infini)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infinit)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infinity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(iNfinity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(inFinity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infInity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infiNity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infinIty)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infiniTy)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infinitY)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(Infi)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(Infin)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(Infini)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(Infinit)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(INfinity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InFinity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InfInity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InfiNity)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InfinIty)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InfiniTy)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(InfinitY)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(INFINITY)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(infstuff)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(Infinitystuff)", dummyComposer, nullptr), DecodeError);
    }
    { // Valid NaN
        ExpectantComposer composer{};
        composer.expectFloater(std::numeric_limits<double>::quiet_NaN());
        decode(R"(nan)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectFloater(std::numeric_limits<double>::quiet_NaN());
        decode(R"(NaN)", composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid NaN
        EXPECT_THROW(decode(R"(Nan)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(nAn)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(naN)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(NAN)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(nanstuff)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(NaNstuff)", dummyComposer, nullptr), DecodeError);
    }
    { // Exponent decimal point
        EXPECT_THROW(decode(R"(1.0e1.0)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1.0e1.)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1e1.0)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1e1.)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1e.1)", dummyComposer, nullptr), DecodeError);
    }
    { // Dangling exponent
        EXPECT_THROW(decode(R"(0e)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0e+)", dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0e-)", dummyComposer, nullptr), DecodeError);
    }
    { // Magnitude too large
        EXPECT_THROW(decode(R"(1e1000)", dummyComposer, nullptr), DecodeError);
    }
    { // Magnitude too small
        EXPECT_THROW(decode(R"(1e-1000)", dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, boolean)
{
    { // True
        ExpectantComposer composer{};
        composer.expectBoolean(true);
        decode(R"(true)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // False
        ExpectantComposer composer{};
        composer.expectBoolean(false);
        decode(R"(false)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, null)
{
    ExpectantComposer composer{};
    composer.expectNull();
    decode(R"(null)"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, noWhitespace)
{
    ExpectantComposer composer{};
    composer.expectObject().expectKey("a"sv).expectArray().expectString("abc"sv).expectSignedInteger(-123).expectFloater(-123.456e-78).expectBoolean(true).expectNull().expectEnd().expectEnd();
    decode(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, extraneousWhitespace)
{
    ExpectantComposer composer{};
    composer.expectObject().expectEnd();
    decode(" \t\n\r\v{} \t\n\r\v"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}

TEST(decode, trailingComma)
{
    { // Valid
        ExpectantComposer composer{};
        composer.expectSignedInteger(0);
        decode(R"(0,)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(0, )"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0);
        decode(R"(0 ,)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectEnd();
        decode(R"([0,])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectEnd();
        decode(R"([0, ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectEnd();
        decode(R"([0 ,])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("k"sv).expectSignedInteger(0).expectEnd();
        decode(R"({"k":0,})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("k"sv).expectSignedInteger(0).expectEnd();
        decode(R"({"k":0, })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("k"sv).expectSignedInteger(0).expectEnd();
        decode(R"({"k":0 ,})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Invalid
        EXPECT_THROW(decode(R"(,)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"( ,)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(, )"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0,,)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0 ,,)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0, ,)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(0,, )"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0,,])"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0 ,,])"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0, ,])"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"([0,, ])"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"k":0,,})"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"k":0 ,,})"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"k":0, ,})"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({"k":0,, })"sv, dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, identifiers)
{
    { // Valid
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectEnd();
        decode(R"({a:0})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("0"sv).expectSignedInteger(0).expectEnd();
        decode(R"({0:0})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("_"sv).expectSignedInteger(0).expectEnd();
        decode(R"({_:0})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("_"sv).expectSignedInteger(0).expectEnd();
        decode(R"({ _ : 0 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("___"sv).expectSignedInteger(0).expectEnd();
        decode(R"({ ___ : 0 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multiple elements
        ExpectantComposer composer{};
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd();
        decode(R"({ a: 0, b: 1 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd();
        decode(R"({ "a": 0, b: 1 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd();
        decode(R"({ a: 0, "b": 1 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All valid characters
        ExpectantComposer composer{};
        composer.expectObject().expectKey("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"sv).expectSignedInteger(0).expectEnd();
        decode(R"({_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:0})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // All invalid characters
        for (int i{0}; i < 256; ++i) {
            if (!std::isalnum(i) && i != '_') {
                EXPECT_THROW(decode("{"s + char(i) + ":0}"s, dummyComposer, nullptr), DecodeError);
            }
        }
    }
    { // Invalid patterns
        EXPECT_THROW(decode(R"({"a:0})"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({a":0})"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"({a b:0})"sv, dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, comments)
{
    { // Line comment
        ExpectantComposer composer{};
        composer.expectSignedInteger(0).expectComment("AAAAA"sv);
        decode(R"(0 // AAAAA)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectSignedInteger(0).expectComment("AAAAA // BBBBB 1"sv);
        decode(R"(0 // AAAAA // BBBBB 1)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Multi-line comment
        ExpectantComposer composer{};
        composer.expectComment("AAAAA\n BBBBB \nCCCCC"sv).expectComment("DD DD").expectSignedInteger(0);
        decode("// AAAAA\n//  BBBBB \n //CCCCC\n\n// DD DD\n0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Block comment
        ExpectantComposer composer{};
        composer.expectComment("AAAAA"sv).expectSignedInteger(0);
        decode(R"(/* AAAAA */ 0)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment("AAAAA \n BBBBB \r\n CCCCC"sv).expectSignedInteger(0);
        decode("/* AAAAA \n BBBBB \r\n CCCCC */ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Comment in string
        ExpectantComposer composer{};
        composer.expectString("// AAAAA"sv);
        decode(R"("// AAAAA")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectString("/* AAAAA */"sv);
        decode(R"("/* AAAAA */")"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Comments in array
        ExpectantComposer composer{};
        composer.expectComment("AAAAA"sv).expectArray();
            composer.expectComment("BBBBB"sv).expectSignedInteger(0).expectComment("CCCCC"sv).expectComment("DDDDD"sv).expectSignedInteger(1).expectComment("EEEEE"sv);
        composer.expectEnd().expectComment("FFFFF"sv).expectComment("GGGGG"sv);
        decode(R"(/* AAAAA */ [ /* BBBBB */ 0 /* CCCCC */ , /* DDDDD */ 1 /* EEEEE */ ] /* FFFFF */ // GGGGG)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment("AAAAA"sv).expectArray();
            composer.expectComment("BBBBB"sv).expectSignedInteger(0).expectComment("CCCCC"sv).expectComment("DDDDD"sv).expectSignedInteger(1).expectComment("EEEEE"sv);
        composer.expectEnd().expectComment("FFFFF"sv).expectComment("GGGGG"sv);
        decode(R"(/*AAAAA*/[/*BBBBB*/0/*CCCCC*/,/*DDDDD*/1/*EEEEE*/]/*FFFFF*///GGGGG)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectComment("AAAAA"sv);
            composer.expectSignedInteger(0).expectComment("BBBBB"sv);
            composer.expectSignedInteger(1).expectComment("CCCCC"sv);
        composer.expectEnd().expectComment("DDDDD"sv);
        decode(
R"([ // AAAAA
    0, // BBBBB
    1 // CCCCC
] // DDDDD)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Comments in object
        ExpectantComposer composer{};
        composer.expectComment("AAAAA"sv).expectObject();
            composer.expectComment("BBBBB"sv).expectKey("0"sv).expectComment("CCCCC"sv).expectComment("DDDDD"sv).expectSignedInteger(0).expectComment("EEEEE"sv);
            composer.expectComment("FFFFF"sv).expectKey("1"sv).expectComment("GGGGG"sv).expectComment("HHHHH"sv).expectSignedInteger(1).expectComment("IIIII"sv);
        composer.expectEnd().expectComment("JJJJJ"sv).expectComment("KKKKK"sv);
        decode(R"(/* AAAAA */ { /* BBBBB */ 0 /* CCCCC */ : /* DDDDD */ 0 /* EEEEE */ , /* FFFFF */ 1 /* GGGGG */ : /* HHHHH */ 1 /* IIIII */ } /* JJJJJ */ // KKKKK)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment("AAAAA"sv).expectObject();
            composer.expectComment("BBBBB"sv).expectKey("0"sv).expectComment("CCCCC"sv).expectComment("DDDDD"sv).expectSignedInteger(0).expectComment("EEEEE"sv);
            composer.expectComment("FFFFF"sv).expectKey("1"sv).expectComment("GGGGG"sv).expectComment("HHHHH"sv).expectSignedInteger(1).expectComment("IIIII"sv);
        composer.expectEnd().expectComment("JJJJJ"sv).expectComment("KKKKK"sv);
        decode(R"(/*AAAAA*/{/*BBBBB*/0/*CCCCC*/:/*DDDDD*/0/*EEEEE*/,/*FFFFF*/1/*GGGGG*/:/*HHHHH*/1/*IIIII*/}/*JJJJJ*///KKKKK)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectComment("AAAAA"sv);
            composer.expectKey("0"sv).expectSignedInteger(0).expectComment("BBBBB"sv);
            composer.expectKey("1"sv).expectSignedInteger(1).expectComment("CCCCC"sv);
        composer.expectEnd().expectComment("DDDDD"sv);
        decode(
R"({ // AAAAA
    0: 0, // BBBBB
    1: 1 // CCCCC
} // DDDDD)"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // CRLF
        ExpectantComposer composer{};
        composer.expectArray().expectComment("AAAAA"sv).expectSignedInteger(0).expectComment("BBBBB"sv).expectSignedInteger(1).expectComment("CCCCC"sv).expectEnd().expectComment("DDDDD "sv);
        decode("[ // AAAAA\r\n    0, // BBBBB\n    1 // CCCCC\r\n] // DDDDD \n"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectComment("AAAAA\r 0, // BBBBB"sv).expectSignedInteger(1).expectComment("CCCCC"sv).expectEnd().expectComment("DDDDD "sv);
        decode("[ // AAAAA\r 0, // BBBBB\n    1 // CCCCC\r\n] // DDDDD \n"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Weirdness
        ExpectantComposer composer{};
        EXPECT_THROW(decode(R"(/*/ 0)"sv, dummyComposer, nullptr), DecodeError);
        composer.expectComment(""sv).expectSignedInteger(0);
        decode("//\n0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(""sv).expectSignedInteger(0);
        decode("// \n0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(" "sv).expectSignedInteger(0);
        decode("//  \n0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(""sv).expectSignedInteger(0);
        decode("/**/ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(""sv).expectSignedInteger(0);
        decode("/* */ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(""sv).expectSignedInteger(0);
        decode("/*  */ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment(" "sv).expectSignedInteger(0);
        decode("/*   */ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment("/"sv).expectSignedInteger(0);
        decode("/*/*/ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectComment("///*** /* //** ** /"sv).expectSignedInteger(0);
        decode("/*///*** /* //** ** /*/ 0"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Nothing but comments
        EXPECT_THROW(decode("// AAAAA\n/* BBBBB */ // CCCCC\n"sv, dummyComposer, nullptr), DecodeError);
    }
    { // Block comment that doesn't terminate
        EXPECT_THROW(decode("{ /* AAAAA\nBBBBB    CCCCC\n"sv, dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, density)
{
    { // Multiline
        ExpectantComposer composer{};
        composer.expectArray();
            composer.expectSignedInteger(0);
            composer.expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"([
    0,
    1
])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray();
            composer.expectSignedInteger(0);
            composer.expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"([
0,1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray();
            composer.expectSignedInteger(0);
            composer.expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"([0
,1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray();
            composer.expectSignedInteger(0);
            composer.expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"([0,
1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray();
            composer.expectSignedInteger(0);
            composer.expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"([0,1
])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({
    "a": 0,
    "b": 1
})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({
"a":0,"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({"a"
:0,"b"
:1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({"a":
0,"b":
1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({"a":0
,"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({"a":0,
"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject();
            composer.expectKey("a"sv).expectSignedInteger(0);
            composer.expectKey("b"sv).expectSignedInteger(1);
        composer.expectEnd(Density::multiline);
        decode(
R"({"a":0,"b":1
})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Uniline
        ExpectantComposer composer{};
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"([ 0, 1 ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"([ 0,1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"([0 ,1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"([0, 1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"([0,1 ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({ "a": 0, "b": 1 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({ "a":0,"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({"a" :0,"b" :1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({"a": 0,"b": 1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({"a":0 ,"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({"a":0, "b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::uniline);
        decode(R"({"a":0,"b":1 })"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Nospace
        ExpectantComposer composer{};
        composer.expectArray().expectSignedInteger(0).expectSignedInteger(1).expectEnd(Density::nospace);
        decode(R"([0,1])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectKey("a"sv).expectSignedInteger(0).expectKey("b"sv).expectSignedInteger(1).expectEnd(Density::nospace);
        decode(R"({"a":0,"b":1})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectEnd(Density::nospace);
        decode(R"([])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectObject().expectEnd(Density::nospace);
        decode(R"({})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Denser within
        ExpectantComposer composer{};
        composer.expectArray();
            composer.expectArray().expectSignedInteger(0).expectEnd(Density::uniline);
        composer.expectEnd(Density::multiline);
        decode(
R"([
    [ 0 ]
])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectArray().expectSignedInteger(0).expectEnd(Density::nospace).expectEnd(Density::uniline);
        decode(R"([ [0] ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Denser without
        ExpectantComposer composer{};
        composer.expectArray().expectArray();
            composer.expectSignedInteger(0);
        composer.expectEnd(Density::multiline).expectEnd(Density::uniline);
        decode(
R"([ [
    0
] ])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
        composer.expectArray().expectArray().expectSignedInteger(0).expectEnd(Density::uniline).expectEnd(Density::nospace);
        decode(R"([[ 0 ]])"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
    { // Complex
        ExpectantComposer composer{};
        composer.expectObject();
            composer.expectKey("a"sv).expectArray().expectObject();
                composer.expectKey("b"sv).expectArray().expectSignedInteger(0).expectSignedInteger(1).expectSignedInteger(2).expectEnd(Density::uniline);
            composer.expectEnd(Density::multiline).expectSignedInteger(3).expectEnd(Density::nospace);
            composer.expectKey("c"sv).expectObject().expectKey("d"sv).expectSignedInteger(4).expectKey("e"sv).expectArray().expectEnd(Density::nospace).expectKey("f"sv).expectSignedInteger(5).expectEnd(Density::uniline);
            composer.expectKey("g"sv).expectObject().expectEnd(Density::nospace);
        composer.expectEnd(Density::multiline);
        decode(
R"({
    "a": [{
        "b": [ 0, 1, 2 ]
    },3],
    "c": { "d": 4, "e": [], "f": 5 },
    "g": {}
})"sv, composer, nullptr);
        EXPECT_TRUE(composer.isDone());
    }
}

TEST(decode, misc)
{
    { // Empty
        EXPECT_THROW(decode(R"()"sv, dummyComposer, nullptr), DecodeError);
    }
    { // Only whitespace
        EXPECT_THROW(decode(R"(   )"sv, dummyComposer, nullptr), DecodeError);
    }
    { // Unknown value
        EXPECT_THROW(decode(R"(v)"sv, dummyComposer, nullptr), DecodeError);
    }
    { // Multiple root values
        EXPECT_THROW(decode(R"(1 2)"sv, dummyComposer, nullptr), DecodeError);
        EXPECT_THROW(decode(R"(1, 2)"sv, dummyComposer, nullptr), DecodeError);
    }
    { // Lone decimal
        EXPECT_THROW(decode(R"(.)"sv, dummyComposer, nullptr), DecodeError);
    }
}

TEST(decode, general)
{
    ExpectantComposer composer{};
    composer.expectComment("*\n * Third quarter summary document\n * Protected information, do not propagate!\n"sv);
    composer.expectObject();
        composer.expectKey("Name"sv).expectString("Salt's Crust"sv);
        composer.expectKey("Founded"sv).expectSignedInteger(1964);
        composer.expectComment("Not necessarily up to date"sv);
        composer.expectKey("Employees"sv).expectArray();
            composer.expectObject().expectKey("Name"sv).expectString("Ol' Joe Fisher"sv).expectKey("Title"sv).expectString("Fisherman"sv).expectKey("Age"sv).expectSignedInteger(69).expectEnd(Density::uniline);
            composer.expectObject().expectKey("Name"sv).expectString("Mark Rower"sv).expectKey("Title"sv).expectString("Cook"sv).expectKey("Age"sv).expectSignedInteger(41).expectEnd(Density::uniline);
            composer.expectObject().expectKey("Name"sv).expectString("Phineas"sv).expectKey("Title"sv).expectString("Server Boy"sv).expectKey("Age"sv).expectSignedInteger(19).expectEnd(Density::uniline);
        composer.expectEnd(Density::multiline);
        composer.expectKey("Dishes"sv).expectArray();
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Basket o' Barnacles"sv);
                composer.expectKey("Price"sv).expectFloater(5.45);
                composer.expectKey("Ingredients"sv).expectArray().expectString("\"Salt\""sv).expectString("Barnacles"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(false);
            composer.expectEnd(Density::multiline);
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("Two Tuna"sv);
                composer.expectKey("Price"sv).expectFloater(-std::numeric_limits<double>::infinity());
                composer.expectKey("Ingredients"sv).expectArray().expectComment("It's actually cod lmao"sv).expectString("Tuna"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(true);
            composer.expectEnd(Density::multiline);
            composer.expectObject();
                composer.expectKey("Name"sv).expectString("18 Leg Bouquet"sv);
                composer.expectKey("Price"sv).expectFloater(std::numeric_limits<double>::quiet_NaN());
                composer.expectKey("Ingredients"sv).expectArray().expectString("\"Salt\""sv).expectString("Octopus"sv).expectString("Crab"sv).expectEnd();
                composer.expectKey("Gluten Free"sv).expectBoolean(false);
            composer.expectEnd(Density::multiline);
        composer.expectEnd(Density::multiline);
        composer.expectKey("Profit Margin"sv).expectComment("Pay no heed"sv).expectNull();
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
        composer.expectKey("Magic Numbers"sv).expectArray().expectUnsignedInteger(777).expectUnsignedInteger(777).expectUnsignedInteger(777).expectEnd(Density::nospace).expectComment("What could they mean?!"sv);
    composer.expectEnd(Density::multiline);
    decode(
R"(/**
 * Third quarter summary document
 * Protected information, do not propagate!
 */
{
    "Name": "Salt's Crust",
    "Founded": 1964,
    // Not necessarily up to date
    "Employees": [
        { Name: "Ol' Joe Fisher", Title: "Fisherman", Age: 69 },
        { Name: "Mark Rower", Title: "Cook", Age: 41. },
        { Name: "Phineas", Title: "Server Boy", Age: 19.0 },
    ],
    "Dishes": [
        {
            'Name': 'Basket o\' Barnacles',
            'Price': 5.45,
            'Ingredients': [ '"Salt"', 'Barnacles' ],
            'Gluten Free': false
        },
        {
            "Name": "Two Tuna",
            "Price": -inf,
            "Ingredients": [ /* It's actually cod lmao */ "Tuna" ],
            "Gluten Free": true
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": +nan,
            "Ingredients": [ "\"Salt\"", "Octopus", "Crab", ],
            "Gluten Free": false
        }
    ],
    "Profit Margin": /* Pay no heed */ null,
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
",
    "Magic Numbers": [0x309,0o1411,0b1100001001] // What could they mean?!
})"sv, composer, nullptr);
    EXPECT_TRUE(composer.isDone());
}
