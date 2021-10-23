#include <cmath>

#include <gtest/gtest.h>

#include <qc-json.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::Value;
using qc::json::Object;
using qc::json::Array;
using qc::json::decode;
using qc::json::encode;
using qc::json::Type;
using qc::json::TypeError;
using namespace qc::json::tokens;
using qc::json::Density;
using qc::json::Safety::safe;
using qc::json::Safety::unsafe;

using qc::json::makeObject;
using qc::json::makeArray;

struct CustomVal { int x, y; };

bool operator==(const CustomVal & cv1, const CustomVal & cv2)
{
    return cv1.x == cv2.x && cv1.y == cv2.y;
}

template <qc::json::Safety isSafe>
struct qc::json::ValueTo<CustomVal, isSafe>
{
    CustomVal operator()(const Value & val) const
    {
        const Array & arr{val.asArray<isSafe>()};
        return {arr.at(0).get<int, isSafe>(), arr.at(1).get<int, isSafe>()};
    }
};

template <>
struct qc::json::ValueFrom<CustomVal>
{
    Value operator()(const CustomVal & v) const
    {
        return qc::json::makeArray(v.x, v.y);
    }
};

TEST(json, encodeDecodeString)
{
    { // Empty
        std::string_view val{""sv};
        EXPECT_EQ(val, decode(encode(val)).asString());
    }
    { // Typical
        std::string_view val{"abc"sv};
        EXPECT_EQ(val, decode(encode(val)).asString());
    }
    { // Printable characters
        std::string_view val{R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv};
        EXPECT_EQ(val, decode(encode(val)).asString());
    }
    { // Escape characters
        std::string_view val{"\b\f\n\r\t"sv};
        EXPECT_EQ(val, decode(encode(val)).asString());
    }
    { // Unicode
        std::string_view val{"\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv};
        EXPECT_EQ(val, decode(encode(val)).asString());
    }
}

TEST(json, encodeDecodeSignedInteger)
{
    { // Zero
        int val{0};
        EXPECT_EQ(val, decode(encode(val)).get<int>());
    }
    { // Typical
        int val{123};
        EXPECT_EQ(val, decode(encode(val)).get<int>());
    }
    { // Max 64
        int64_t val{std::numeric_limits<int64_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<int64_t>());
    }
    { // Min 64
        int64_t val{std::numeric_limits<int64_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).get<int64_t>());
    }
    { // Max 32
        int32_t val{std::numeric_limits<int32_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<int32_t>());
    }
    { // Min 32
        int32_t val{std::numeric_limits<int32_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).get<int32_t>());
    }
    { // Max 16
        int16_t val{std::numeric_limits<int16_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<int16_t>());
    }
    { // Min 16
        int16_t val{std::numeric_limits<int16_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).get<int16_t>());
    }
    { // Max 8
        int8_t val{std::numeric_limits<int8_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<int8_t>());
    }
    { // Min 8
        int8_t val{std::numeric_limits<int8_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).get<int8_t>());
    }
}

TEST(json, encodeDecodeUnsignedInteger)
{
    { // Zero
        unsigned int val{0u};
        EXPECT_EQ(val, decode(encode(val)).get<unsigned int>());
    }
    { // Typical
        unsigned int val{123u};
        EXPECT_EQ(val, decode(encode(val)).get<unsigned int>());
    }
    { // Max 64
        uint64_t val{std::numeric_limits<uint64_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<uint64_t>());
    }
    { // Max 32
        uint32_t val{std::numeric_limits<uint32_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<uint32_t>());
    }
    { // Max 16
        uint16_t val{std::numeric_limits<uint16_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<uint16_t>());
    }
    { // Max 8
        uint8_t val{std::numeric_limits<uint8_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).get<uint8_t>());
    }
}

TEST(json, encodeDecodeFloater)
{
    uint64_t val64;
    uint32_t val32;

    { // Zero
        double val{0.0};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Typical
        double val{123.45};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Max integer 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'10000110011'1111111111111111111111111111111111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Max integer 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'10010110'11111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Max 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'11111111110'1111111111111111111111111111111111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Max 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'11111110'11111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Min normal 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'00000000001'0000000000000000000000000000000000000000000000000000u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Min normal 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'00000001'00000000000000000000000u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Min subnormal 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'00000000000'0000000000000000000000000000000000000000000000000001u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Min subnormal 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'00000000'00000000000000000000001u)};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Positive infinity
        double val{std::numeric_limits<double>::infinity()};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // Negative infinity
        double val{-std::numeric_limits<double>::infinity()};
        EXPECT_EQ(val, decode(encode(val)).get<double>());
    }
    { // NaN
        EXPECT_TRUE(std::isnan(decode(encode(std::numeric_limits<double>::quiet_NaN())).get<double>()));
    }
}

TEST(json, encodeDecodeBoolean)
{
    // true
    EXPECT_EQ(true, decode(encode(true)).asBoolean());
    // false
    EXPECT_EQ(false, decode(encode(false)).asBoolean());
}

TEST(json, encodeDecodeNull)
{
    EXPECT_TRUE(decode(encode(nullptr)).isNull());
}

TEST(json, encodeDecodeCustom)
{
    CustomVal val{1, 2};
    EXPECT_EQ(val, decode(encode(val)).get<CustomVal>());
}

TEST(json, valueConstruction)
{
    // Default
    EXPECT_EQ(Type::null, Value().type());
    // Object
    EXPECT_EQ(Type::object, Value(Object()).type());
    // Array
    EXPECT_EQ(Type::array, Value(Array()).type());
    // String
    EXPECT_EQ(Type::string, Value("abc"sv).type());
    EXPECT_EQ(Type::string, Value("abc"s).type());
    EXPECT_EQ(Type::string, Value("abc").type());
    EXPECT_EQ(Type::string, Value(const_cast<char *>("abc")).type());
    EXPECT_EQ(Type::string, Value('a').type());
    // Number
    EXPECT_EQ(Type::integer, Value(int64_t(0)).type());
    EXPECT_EQ(Type::integer, Value(int32_t(0)).type());
    EXPECT_EQ(Type::integer, Value(int16_t(0)).type());
    EXPECT_EQ(Type::integer, Value(int8_t(0)).type());
    EXPECT_EQ(Type::unsigner, Value(uint64_t(0)).type());
    EXPECT_EQ(Type::unsigner, Value(uint32_t(0)).type());
    EXPECT_EQ(Type::unsigner, Value(uint16_t(0)).type());
    EXPECT_EQ(Type::unsigner, Value(uint8_t(0)).type());
    EXPECT_EQ(Type::floater, Value(0.0).type());
    EXPECT_EQ(Type::floater, Value(0.0f).type());
    // Boolean
    EXPECT_EQ(Type::boolean, Value(false).type());
    // Null
    EXPECT_EQ(Type::null, Value(nullptr).type());
}

TEST(json, valueMove)
{
    Value v1("abc"sv);
    EXPECT_EQ(Type::string, v1.type());
    EXPECT_EQ("abc"sv, v1.asString());

    Value v2(std::move(v1));
    EXPECT_EQ(Type::null, v1.type());
    EXPECT_EQ(Type::string, v2.type());
    EXPECT_EQ("abc"sv, v2.asString());

    v1 = std::move(v2);
    EXPECT_EQ(Type::string, v1.type());
    EXPECT_EQ("abc"sv, v1.asString());
    EXPECT_EQ(Type::null, v2.type());
}

TEST(json, valueAssignAndEquality)
{
    Value v{};

    const Object objRef{makeObject("a", 1, "b", "wow", "c", nullptr)};
    v = makeObject("a", 1, "b", "wow", "c", nullptr);
    EXPECT_EQ(Type::object, v.type());
    EXPECT_TRUE(v == objRef);
    EXPECT_FALSE(v != objRef);

    const Array arrRef{qc::json::makeArray(0, "a", true)};
    v = qc::json::makeArray(0, "a", true);
    EXPECT_EQ(Type::array, v.type());
    EXPECT_TRUE(v == arrRef);
    EXPECT_FALSE(v != arrRef);

    v = "hello"s;
    EXPECT_EQ(Type::string, v.type());
    EXPECT_TRUE(v == "hello"s);
    EXPECT_FALSE(v != "hello"s);

    v = "hellu"sv;
    EXPECT_EQ(Type::string, v.type());
    EXPECT_TRUE(v == "hellu"sv);
    EXPECT_FALSE(v != "hellu"sv);

    v = "helli";
    EXPECT_EQ(Type::string, v.type());
    EXPECT_TRUE(v == "helli");
    EXPECT_FALSE(v != "helli");

    v = const_cast<char *>("hella");
    EXPECT_EQ(Type::string, v.type());
    EXPECT_TRUE(v == const_cast<char *>("hella"));
    EXPECT_FALSE(v != const_cast<char *>("hella"));

    v = 'h';
    EXPECT_EQ(Type::string, v.type());
    EXPECT_TRUE(v == 'h');
    EXPECT_FALSE(v != 'h');

    v = int64_t(5);
    EXPECT_EQ(Type::integer, v.type());
    EXPECT_TRUE(v == int64_t(5));
    EXPECT_FALSE(v != int64_t(5));

    v = int32_t(6);
    EXPECT_EQ(Type::integer, v.type());
    EXPECT_TRUE(v == int32_t(6));
    EXPECT_FALSE(v != int32_t(6));

    v = int16_t(7);
    EXPECT_EQ(Type::integer, v.type());
    EXPECT_TRUE(v == int16_t(7));
    EXPECT_FALSE(v != int16_t(7));

    v = int8_t(8);
    EXPECT_EQ(Type::integer, v.type());
    EXPECT_TRUE(v == int8_t(8));
    EXPECT_FALSE(v != int8_t(8));

    v = uint64_t(10u);
    EXPECT_EQ(Type::unsigner, v.type());
    EXPECT_TRUE(v == uint64_t(10u));
    EXPECT_FALSE(v != uint64_t(10u));

    v = uint32_t(11u);
    EXPECT_EQ(Type::unsigner, v.type());
    EXPECT_TRUE(v == uint32_t(11u));
    EXPECT_FALSE(v != uint32_t(11u));

    v = uint16_t(12u);
    EXPECT_EQ(Type::unsigner, v.type());
    EXPECT_TRUE(v == uint16_t(12u));
    EXPECT_FALSE(v != uint16_t(12u));

    v = uint8_t(13u);
    EXPECT_EQ(Type::unsigner, v.type());
    EXPECT_TRUE(v == uint8_t(13u));
    EXPECT_FALSE(v != uint8_t(13u));

    v = 7.7;
    EXPECT_EQ(Type::floater, v.type());
    EXPECT_TRUE(v == 7.7);
    EXPECT_FALSE(v != 7.7);

    v = 7.7f;
    EXPECT_EQ(Type::floater, v.type());
    EXPECT_TRUE(v == 7.7f);
    EXPECT_FALSE(v != 7.7f);

    v = true;
    EXPECT_EQ(Type::boolean, v.type());
    EXPECT_TRUE(v == true);
    EXPECT_FALSE(v != true);

    v = nullptr;
    EXPECT_EQ(Type::null, v.type());
    EXPECT_TRUE(v == nullptr);
    EXPECT_FALSE(v != nullptr);
}

TEST(json, swap)
{
    const Array arrRef{qc::json::makeArray(0, "a", true)};
    Value v1{qc::json::makeArray(0, "a", true)};
    EXPECT_EQ(Type::array, v1.type());
    EXPECT_EQ(arrRef, v1);

    Value v2{"hello"s};
    EXPECT_EQ(Type::string, v2.type());
    EXPECT_EQ("hello"s, v2);

    std::swap(v1, v2);

    EXPECT_EQ(Type::string, v1.type());
    EXPECT_EQ("hello"s, v1);

    EXPECT_EQ(Type::array, v2.type());
    EXPECT_EQ(arrRef, v2);
}

TEST(json, valueTypes)
{
    { // Object
        Value v(Object{}, Density::uniline);
        EXPECT_EQ(Type::object, v.type());
        EXPECT_EQ(Density::uniline, v.density());
        EXPECT_TRUE(v.isObject());
        EXPECT_TRUE(v.is<Object>());
        v.asObject<safe>();
        v.asObject<unsafe>();
    }
    { // Array
        Value v(Array{}, Density::multiline);
        EXPECT_EQ(Type::array, v.type());
        EXPECT_EQ(Density::multiline, v.density());
        EXPECT_TRUE(v.isArray());
        EXPECT_TRUE(v.is<Array>());
        v.asArray<safe>();
        v.asArray<unsafe>();
    }
    { // String
        Value v("abc"sv);
        EXPECT_EQ(Type::string, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isString());
        EXPECT_TRUE(v.is<std::string>());
        EXPECT_TRUE(v.is<std::string_view>());
        EXPECT_TRUE(v.is<const char *>());
        EXPECT_TRUE(v.is<char *>());
        EXPECT_FALSE(v.is<char>());
        v.asString<safe>();
        v.asString<unsafe>();
        v.get<std::string_view, safe>();
        v.get<std::string_view, unsafe>();
        v.get<const char *, safe>();
        v.get<const char *, unsafe>();
    }
    { // Character
        Value v('a');
        EXPECT_EQ(Type::string, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isString());
        EXPECT_TRUE(v.is<std::string>());
        EXPECT_TRUE(v.is<std::string_view>());
        EXPECT_TRUE(v.is<const char *>());
        EXPECT_TRUE(v.is<char *>());
        EXPECT_TRUE(v.is<char>());
        v.asString<safe>();
        v.asString<unsafe>();
        v.get<std::string_view, safe>();
        v.get<std::string_view, unsafe>();
        v.get<const char *, safe>();
        v.get<const char *, unsafe>();
        v.get<char, safe>();
        v.get<char, unsafe>();
    }
    { // Signed integer
        Value v(123);
        EXPECT_EQ(Type::integer, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isNumber());
        EXPECT_TRUE(v.isInteger());
        EXPECT_TRUE(v.is<int>());
        v.asInteger<safe>();
        v.asInteger<unsafe>();
        v.get<int, safe>();
        v.get<int, unsafe>();
    }
    { // Unsigned integer
        Value v(123u);
        EXPECT_EQ(Type::unsigner, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isNumber());
        EXPECT_TRUE(v.isUnsigner());
        EXPECT_TRUE(v.is<unsigned int>());
        v.asUnsigner<safe>();
        v.asUnsigner<unsafe>();
        v.get<int, safe>();
        v.get<int, unsafe>();
    }
    { // Floater
        Value v(123.0);
        EXPECT_EQ(Type::floater, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isNumber());
        EXPECT_TRUE(v.isFloater());
        EXPECT_TRUE(v.is<float>());
        v.asFloater<safe>();
        v.asFloater<unsafe>();
        v.get<float, safe>();
        v.get<float, unsafe>();
    }
    { // Boolean
        Value v(false);
        EXPECT_EQ(Type::boolean, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isBoolean());
        EXPECT_TRUE(v.is<bool>());
        v.asBoolean<safe>();
        v.asBoolean<unsafe>();
        v.get<bool, safe>();
        v.get<bool, unsafe>();
    }
    { // Null
        Value v(nullptr);
        EXPECT_EQ(Type::null, v.type());
        EXPECT_EQ(Density::unspecified, v.density());
        EXPECT_TRUE(v.isNull());
        v.get<nullptr_t, safe>();
        v.get<nullptr_t, unsafe>();
    }
}

template <typename T>
void testNumber(T v, bool isS64, bool isS32, bool isS16, bool isS08, bool isU64, bool isU32, bool isU16, bool isU08, bool isF64, bool isF32) {
    Value val(v);

    EXPECT_EQ(isS64, val.is< int64_t>());
    EXPECT_EQ(isS32, val.is< int32_t>());
    EXPECT_EQ(isS16, val.is< int16_t>());
    EXPECT_EQ(isS08, val.is<  int8_t>());
    EXPECT_EQ(isU64, val.is<uint64_t>());
    EXPECT_EQ(isU32, val.is<uint32_t>());
    EXPECT_EQ(isU16, val.is<uint16_t>());
    EXPECT_EQ(isU08, val.is< uint8_t>());
    EXPECT_EQ(isF64, val.is<  double>());
    EXPECT_EQ(isF32, val.is<   float>());

    if (isS64) EXPECT_EQ( int64_t(v), val.get< int64_t>()); else EXPECT_THROW(val.get< int64_t>(), TypeError);
    if (isS32) EXPECT_EQ( int32_t(v), val.get< int32_t>()); else EXPECT_THROW(val.get< int32_t>(), TypeError);
    if (isS16) EXPECT_EQ( int16_t(v), val.get< int16_t>()); else EXPECT_THROW(val.get< int16_t>(), TypeError);
    if (isS08) EXPECT_EQ(  int8_t(v), val.get<  int8_t>()); else EXPECT_THROW(val.get<  int8_t>(), TypeError);
    if (isU64) EXPECT_EQ(uint64_t(v), val.get<uint64_t>()); else EXPECT_THROW(val.get<uint64_t>(), TypeError);
    if (isU32) EXPECT_EQ(uint32_t(v), val.get<uint32_t>()); else EXPECT_THROW(val.get<uint32_t>(), TypeError);
    if (isU16) EXPECT_EQ(uint16_t(v), val.get<uint16_t>()); else EXPECT_THROW(val.get<uint16_t>(), TypeError);
    if (isU08) EXPECT_EQ( uint8_t(v), val.get< uint8_t>()); else EXPECT_THROW(val.get< uint8_t>(), TypeError);
    if (isF64) EXPECT_EQ(  double(v), val.get<  double>()); else EXPECT_THROW(val.get<  double>(), TypeError);
    if (isF32) EXPECT_EQ(   float(v), val.get<   float>()); else EXPECT_THROW(val.get<   float>(), TypeError);
}

TEST(json, valueNumbers)
{
    // Zero, given as signed integer
    testNumber(0, true, true, true, true, true, true, true, true, true, true);
    // Zero, given as unsigned integer
    testNumber(0u, true, true, true, true, true, true, true, true, true, true);
    // Zero, given as floater
    testNumber(0.0, true, true, true, true, true, true, true, true, true, true);
    // Positive integer, given as signed integer
    testNumber(127, true, true, true, true, true, true, true, true, true, true);
    // Positive integer, given as unsigned integer
    testNumber(127u, true, true, true, true, true, true, true, true, true, true);
    // Positive integer, given as floater
    testNumber(127.0, true, true, true, true, true, true, true, true, true, true);
    // Positive integer too big for int8_t, given as signed integer
    testNumber(128, true, true, true, false, true, true, true, true, true, true);
    // Positive integer too big for int8_t, given as unsigned integer
    testNumber(128u, true, true, true, false, true, true, true, true, true, true);
    // Positive integer too big for int8_t, given as floater
    testNumber(128.0, true, true, true, false, true, true, true, true, true, true);
    // Positive integer too big for uint8_t, given as signed integer
    testNumber(256, true, true, true, false, true, true, true, false, true, true);
    // Positive integer too big for uint8_t, given as unsigned integer
    testNumber(256u, true, true, true, false, true, true, true, false, true, true);
    // Positive integer too big for uint8_t, given as floater
    testNumber(256.0, true, true, true, false, true, true, true, false, true, true);
    // Positive integer too big for int16_t, given as signed integer
    testNumber(32768, true, true, false, false, true, true, true, false, true, true);
    // Positive integer too big for int16_t, given as unsigned integer
    testNumber(32768u, true, true, false, false, true, true, true, false, true, true);
    // Positive integer too big for int16_t, given as floater
    testNumber(32768.0, true, true, false, false, true, true, true, false, true, true);
    // Positive integer too big for uint16_t, given as signed integer
    testNumber(65536, true, true, false, false, true, true, false, false, true, true);
    // Positive integer too big for uint16_t, given as unsigned integer
    testNumber(65536u, true, true, false, false, true, true, false, false, true, true);
    // Positive integer too big for uint16_t, given as floater
    testNumber(65536.0, true, true, false, false, true, true, false, false, true, true);
    // Positive integer too big for int32_t, given as signed integer
    testNumber(2147483648LL, true, false, false, false, true, true, false, false, true, true);
    // Positive integer too big for int32_t, given as unsigned integer
    testNumber(2147483648u, true, false, false, false, true, true, false, false, true, true);
    // Positive integer too big for int32_t, given as floater
    testNumber(2147483648.0, true, false, false, false, true, true, false, false, true, true);
    // Positive integer too big for uint32_t, given as signed integer
    testNumber(4294967296, true, false, false, false, true, false, false, false, true, true);
    // Positive integer too big for uint32_t, given as unsigned integer
    testNumber(4294967296u, true, false, false, false, true, false, false, false, true, true);
    // Positive integer too big for uint32_t, given as floater
    testNumber(4294967296.0, true, false, false, false, true, false, false, false, true, true);
    // Positive integer too big for int64_t, given as unsigned integer
    testNumber(9223372036854775808u, false, false, false, false, true, false, false, false, true, true);
    // Positive integer too big for int64_t, given as floater
    testNumber(9223372036854775808.0, false, false, false, false, true, false, false, false, true, true);
    // Positive integer too big for uint64_t, given as floater
    testNumber(20000000000000000000.0, false, false, false, false, false, false, false, false, true, true);
    // Negative integer, given as signed integer
    testNumber(-128, true, true, true, true, false, false, false, false, true, true);
    // Negative integer, given as floater
    testNumber(-128.0, true, true, true, true, false, false, false, false, true, true);
    // Negative integer too small for int8_t, given as signed integer
    testNumber(-129, true, true, true, false, false, false, false, false, true, true);
    // Negative integer too small for int8_t, given as floater
    testNumber(-129.0, true, true, true, false, false, false, false, false, true, true);
    // Negative integer too small for int16_t, given as signed integer
    testNumber(-32769, true, true, false, false, false, false, false, false, true, true);
    // Negative integer too small for int16_t, given as floater
    testNumber(-32769.0, true, true, false, false, false, false, false, false, true, true);
    // Negative integer too small for int32_t, given as signed integer
    testNumber(-2147483649LL, true, false, false, false, false, false, false, false, true, true);
    // Negative integer too small for int32_t, given as floater
    testNumber(-2147483649.0, true, false, false, false, false, false, false, false, true, true);
    // Negative integer too small for int64_t, given as floater
    testNumber(-10000000000000000000.0, false, false, false, false, false, false, false, false, true, true);
    // Floating point number
    testNumber(123.4, false, false, false, false, false, false, false, false, true, true);
}

TEST(json, wrongValueType)
{
    // Safe
    EXPECT_THROW((Value().asObject<safe>()), TypeError);
    EXPECT_THROW((Value().asArray<safe>()), TypeError);
    EXPECT_THROW((Value().asString<safe>()), TypeError);
    EXPECT_THROW((Value().get<std::string, safe>()), TypeError);
    EXPECT_THROW((Value().get<std::string_view, safe>()), TypeError);
    EXPECT_THROW((Value().get<const char *, safe>()), TypeError);
    EXPECT_THROW((Value().get<char, safe>()), TypeError);
    EXPECT_THROW((Value().asInteger<safe>()), TypeError);
    EXPECT_THROW((Value().asUnsigner<safe>()), TypeError);
    EXPECT_THROW((Value().asFloater<safe>()), TypeError);
    EXPECT_THROW((Value().get<int64_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<int32_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<int16_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<int8_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<uint64_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<uint32_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<uint16_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<uint8_t, safe>()), TypeError);
    EXPECT_THROW((Value().get<double, safe>()), TypeError);
    EXPECT_THROW((Value().get<float, safe>()), TypeError);
    EXPECT_THROW((Value().asBoolean<safe>()), TypeError);
    EXPECT_THROW((Value().get<bool, safe>()), TypeError);
    EXPECT_THROW((Value(1).get<nullptr_t, safe>()), TypeError);

    // Unsafe
    Value().asObject<unsafe>();
    Value().asArray<unsafe>();
    Value().asString<unsafe>();
    // Will cause SEH exception
    //Value().get<std::string, unsafe>();
    //Value().get<std::string_view, unsafe>();
    //Value().get<const char *, unsafe>();
    //Value().get<char, unsafe>();
    Value().asInteger<unsafe>();
    Value().asUnsigner<unsafe>();
    Value().asFloater<unsafe>();
    Value(true).get<int64_t, unsafe>();
    Value(true).get<int32_t, unsafe>();
    Value(true).get<int16_t, unsafe>();
    Value(true).get<int8_t, unsafe>();
    Value(true).get<uint64_t, unsafe>();
    Value(true).get<uint32_t, unsafe>();
    Value(true).get<uint16_t, unsafe>();
    Value(true).get<uint8_t, unsafe>();
    Value(true).get<double, unsafe>();
    Value(true).get<float, unsafe>();
    Value().asBoolean<unsafe>();
    Value(1).get<bool, unsafe>();
    Value(1).get<nullptr_t, unsafe>();
}

TEST(json, density)
{
    EXPECT_EQ(R"([
    1,
    2,
    3
])"s, encode(qc::json::makeArray(1, 2, 3), Density::multiline));
    EXPECT_EQ("[ 1, 2, 3 ]"s, encode(qc::json::makeArray(1, 2, 3), Density::uniline));
    EXPECT_EQ("[1,2,3]"s, encode(qc::json::makeArray(1, 2, 3), Density::nospace));
}

TEST(json, makeObject)
{
    { // Generic
        Object obj1{makeObject("a", 1, "b"s, 2.0, "c"sv, true)};
        Object obj2{makeObject("d", std::move(obj1))};
        EXPECT_EQ(1u, obj2.size());
        const Object & innerObj{obj2.at("d").asObject()};
        EXPECT_EQ(3u, innerObj.size());
        EXPECT_EQ(1, innerObj.at("a").asInteger());
        EXPECT_EQ(2.0, innerObj.at("b").asFloater());
        EXPECT_EQ(true, innerObj.at("c").asBoolean());
    }
    { // Empty array
        Object obj{makeObject()};
        EXPECT_TRUE(obj.empty());
    }
}

TEST(json, makeArray)
{
    { // Generic
        Array arr1{qc::json::makeArray(1, 2.0, true)};
        Array arr2{qc::json::makeArray("ok", std::move(arr1))};
        EXPECT_EQ(2u, arr2.size());
        EXPECT_EQ(2u, arr2.capacity());
        EXPECT_EQ("ok", arr2[0].asString());
        EXPECT_EQ(3u, arr2[1].asArray().size());
        EXPECT_EQ(3u, arr2[1].asArray().capacity());
        EXPECT_EQ(1, arr2[1].asArray()[0].asInteger());
        EXPECT_EQ(2.0, arr2[1].asArray()[1].asFloater());
        EXPECT_EQ(true, arr2[1].asArray()[2].asBoolean());
    }
    { // Empty array
        Array arr{qc::json::makeArray()};
        EXPECT_TRUE(arr.empty());
        EXPECT_EQ(0u, arr.capacity());
    }
}

TEST(json, comments)
{
    { // Encode object
        Value json{makeObject("a", 1, "b", 2, "c", 3)};
        json.setComment("Yada yada");
        Object & obj{json.asObject()};
        obj.at("a").setComment("How fascinating...");
        obj.at("c").setComment("Wow,\nso\n  incredible");
        EXPECT_EQ(R"(// Yada yada
{
    // How fascinating...
    "a": 1,
    "b": 2,
    // Wow,
    // so
    //   incredible
    "c": 3
})"s, qc::json::encode(json, Density::multiline));
        EXPECT_EQ(R"(/* Yada yada */ { /* How fascinating... */ "a": 1, "b": 2, /* Wow,
so
  incredible */ "c": 3 })"s, qc::json::encode(json, Density::uniline));
        EXPECT_EQ(R"(/*Yada yada*/{/*How fascinating...*/"a":1,"b":2,/*Wow,
so
  incredible*/"c":3})"s, qc::json::encode(json, Density::nospace));
    }
    { // Encode array
        Value json{qc::json::makeArray(1, 2, 3)};
        json.setComment("Yada yada");
        Array & arr{json.asArray()};
        arr[0].setComment("How fascinating...");
        arr[2].setComment("Wow,\nso\n  incredible");
        EXPECT_EQ(R"(// Yada yada
[
    // How fascinating...
    1,
    2,
    // Wow,
    // so
    //   incredible
    3
])"s, qc::json::encode(json, Density::multiline));
        EXPECT_EQ(R"(/* Yada yada */ [ /* How fascinating... */ 1, 2, /* Wow,
so
  incredible */ 3 ])"s, qc::json::encode(json, Density::uniline));
        EXPECT_EQ(R"(/*Yada yada*/[/*How fascinating...*/1,2,/*Wow,
so
  incredible*/3])"s, qc::json::encode(json, Density::nospace));
    }
    { // Decode
        const Value json{decode(R"(// AAAAA
// BBBBB
/* CCCCC */
[
    /* DDDDD */
    // EEEEE
    [ /* FFFFF */ /* GGGGG */ 0, /* HHHHH */ 1, /* IIIII */ ], // JJJJJ
    { /* KKKKK */ /* LLLLL */ "k": /* MMMMM */ "v" /* NNNNN */ } // OOOOO
    /* PPPPP */
] // QQQQQ)"sv)};
        EXPECT_EQ("AAAAA\nBBBBB", *json.comment());
        const Array & rootArr{json.asArray()};
        EXPECT_EQ(2u, rootArr.size());
        EXPECT_EQ("DDDDD", *rootArr.at(0).comment());
        const Array & innerArr{rootArr.at(0).asArray()};
        EXPECT_EQ(2u, innerArr.size());
        EXPECT_EQ("FFFFF", *innerArr.at(0).comment());
        EXPECT_EQ("HHHHH", *innerArr.at(1).comment());
        EXPECT_EQ("JJJJJ", *rootArr.at(1).comment());
        const Object & innerObj{rootArr.at(1).asObject()};
        EXPECT_EQ(1u, innerObj.size());
        EXPECT_EQ("KKKKK", *innerObj.at("k").comment());
    }
}

TEST(json, encoderOptions)
{
    EXPECT_EQ(R"({
  k: [
    'v'
  ]
})", encode(makeObject("k", makeArray("v")), Density::multiline, 2u, true, true));
}

TEST(json, numberEquality)
{
    { // Signed integer
        Value val{10};
        EXPECT_TRUE(val == 10);
        EXPECT_TRUE(val == 10u);
        EXPECT_TRUE(val == 10.0);
        EXPECT_FALSE(val != 10);
        EXPECT_FALSE(val != 10u);
        EXPECT_FALSE(val != 10.0);
        EXPECT_TRUE(10 == val);
        EXPECT_TRUE(10u == val);
        EXPECT_TRUE(10.0 == val);
        EXPECT_FALSE(10 != val);
        EXPECT_FALSE(10u != val);
        EXPECT_FALSE(10.0 != val);
        EXPECT_FALSE(val == 11);
        EXPECT_FALSE(val == 11u);
        EXPECT_FALSE(val == 11.0);
        EXPECT_TRUE(val != 11);
        EXPECT_TRUE(val != 11u);
        EXPECT_TRUE(val != 11.0);
        EXPECT_FALSE(11 == val);
        EXPECT_FALSE(11u == val);
        EXPECT_FALSE(11.0 == val);
        EXPECT_TRUE(11 != val);
        EXPECT_TRUE(11u != val);
        EXPECT_TRUE(11.0 != val);
    }
    { // Unsigned integer
        Value val{10u};
        EXPECT_TRUE(val == 10);
        EXPECT_TRUE(val == 10u);
        EXPECT_TRUE(val == 10.0);
        EXPECT_FALSE(val != 10);
        EXPECT_FALSE(val != 10u);
        EXPECT_FALSE(val != 10.0);
        EXPECT_TRUE(10 == val);
        EXPECT_TRUE(10u == val);
        EXPECT_TRUE(10.0 == val);
        EXPECT_FALSE(10 != val);
        EXPECT_FALSE(10u != val);
        EXPECT_FALSE(10.0 != val);
        EXPECT_FALSE(val == 11);
        EXPECT_FALSE(val == 11u);
        EXPECT_FALSE(val == 11.0);
        EXPECT_TRUE(val != 11);
        EXPECT_TRUE(val != 11u);
        EXPECT_TRUE(val != 11.0);
        EXPECT_FALSE(11 == val);
        EXPECT_FALSE(11u == val);
        EXPECT_FALSE(11.0 == val);
        EXPECT_TRUE(11 != val);
        EXPECT_TRUE(11u != val);
        EXPECT_TRUE(11.0 != val);
    }
    { // Floater
        Value val{10.0};
        EXPECT_TRUE(val == 10);
        EXPECT_TRUE(val == 10u);
        EXPECT_TRUE(val == 10.0);
        EXPECT_FALSE(val != 10);
        EXPECT_FALSE(val != 10u);
        EXPECT_FALSE(val != 10.0);
        EXPECT_TRUE(10 == val);
        EXPECT_TRUE(10u == val);
        EXPECT_TRUE(10.0 == val);
        EXPECT_FALSE(10 != val);
        EXPECT_FALSE(10u != val);
        EXPECT_FALSE(10.0 != val);
        EXPECT_FALSE(val == 11);
        EXPECT_FALSE(val == 11u);
        EXPECT_FALSE(val == 11.0);
        EXPECT_TRUE(val != 11);
        EXPECT_TRUE(val != 11u);
        EXPECT_TRUE(val != 11.0);
        EXPECT_FALSE(11 == val);
        EXPECT_FALSE(11u == val);
        EXPECT_FALSE(11.0 == val);
        EXPECT_TRUE(11 != val);
        EXPECT_TRUE(11u != val);
        EXPECT_TRUE(11.0 != val);
    }
    { // Special cases
        Value val{std::numeric_limits<uint64_t>::max()};
        EXPECT_FALSE(val == -1);

        val = -1;
        EXPECT_FALSE(val == std::numeric_limits<uint64_t>::max());

        val = std::numeric_limits<int64_t>::max();
        EXPECT_FALSE(val == double(std::numeric_limits<int64_t>::max()));

        val = std::numeric_limits<uint64_t>::max();
        EXPECT_FALSE(val == double(std::numeric_limits<uint64_t>::max()));

        val = std::numeric_limits<double>::infinity();
        EXPECT_TRUE(val == std::numeric_limits<double>::infinity());
        EXPECT_FALSE(val == std::numeric_limits<int64_t>::max());
        EXPECT_FALSE(val == std::numeric_limits<uint64_t>::max());

        val = std::numeric_limits<double>::quiet_NaN();
        EXPECT_FALSE(val == std::numeric_limits<double>::quiet_NaN());
        EXPECT_TRUE(val != std::numeric_limits<double>::quiet_NaN());
    }
}

TEST(json, general)
{
    std::string json(R"(// Third quarter summary document
// Protected information, do not propagate!
{
    "Dishes": [
        {
            "Gluten Free": false,
            "Ingredients": [ "\"Salt\"", "Barnacles" ],
            "Name": "Basket o' Barnacles",
            "Price": 5.45
        },
        {
            "Gluten Free": true,
            "Ingredients": [ /* It's actually cod lmao */ "Tuna" ],
            "Name": "Two Tuna",
            "Price": -inf
        },
        {
            "Gluten Free": false,
            "Ingredients": [ "\"Salt\"", "Octopus", "Crab" ],
            "Name": "18 Leg Bouquet",
            "Price": nan
        }
    ],
    // Not necessarily up to date
    "Employees": [
        { "Age": 69, "Name": "Ol' Joe Fisher", "Title": "Fisherman" },
        { "Age": 41, "Name": "Mark Rower", "Title": "Cook" },
        { "Age": 19, "Name": "Phineas", "Title": "Server Boy" }
    ],
    "Founded": 1964,
    "Green Eggs and Ham": "I do not like them in a box\nI do not like them with a fox\nI do not like them in a house\nI do not like them with a mouse\nI do not like them here or there\nI do not like them anywhere\nI do not like green eggs and ham\nI do not like them Sam I am\n",
    "Ha\x03r Name": "M\0\0n",
    // What could they mean?!
    "Magic Numbers": [777,777,777],
    "Name": "Salt's Crust",
    // Pay no heed
    "Profit Margin": null
})"s);
    EXPECT_EQ(json, encode(decode(json)));
}
