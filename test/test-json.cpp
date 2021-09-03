#include <cmath>

#include <gtest/gtest.h>

#include <qc-json/qc-json.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using qc::json::Value;
using qc::json::Object;
using qc::json::Array;
using qc::json::String;
using qc::json::decode;
using qc::json::encode;
using qc::json::Type;
using qc::json::TypeError;
using namespace qc::json::tokens;
using qc::json::Safety::safe;
using qc::json::Safety::unsafe;

struct CustomVal { int x, y; };

bool operator==(const CustomVal & cv1, const CustomVal & cv2) {
    return cv1.x == cv2.x && cv1.y == cv2.y;
}

template <qc::json::Safety isSafe>
struct qc::json::valueTo<CustomVal, isSafe> {
    CustomVal operator()(const Value & val) const {
        const Array & arr(val.asArray<isSafe>());
        return {arr.at(0).as<int, isSafe>(), arr.at(1).as<int, isSafe>()};
    }
};

template <>
struct qc::json::valueFrom<CustomVal> {
    Value operator()(const CustomVal & v) const {
        return Array(v.x, v.y);
    }
};

TEST(json, encodeDecodeString) {
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

TEST(json, encodeDecodeSignedInteger) {
    { // Zero
        int val{0};
        EXPECT_EQ(val, decode(encode(val)).as<int>());
    }
    { // Typical
        int val{123};
        EXPECT_EQ(val, decode(encode(val)).as<int>());
    }
    { // Max 64
        int64_t val{std::numeric_limits<int64_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<int64_t>());
    }
    { // Min 64
        int64_t val{std::numeric_limits<int64_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).as<int64_t>());
    }
    { // Max 32
        int32_t val{std::numeric_limits<int32_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<int32_t>());
    }
    { // Min 32
        int32_t val{std::numeric_limits<int32_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).as<int32_t>());
    }
    { // Max 16
        int16_t val{std::numeric_limits<int16_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<int16_t>());
    }
    { // Min 16
        int16_t val{std::numeric_limits<int16_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).as<int16_t>());
    }
    { // Max 8
        int8_t val{std::numeric_limits<int8_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<int8_t>());
    }
    { // Min 8
        int8_t val{std::numeric_limits<int8_t>::min()};
        EXPECT_EQ(val, decode(encode(val)).as<int8_t>());
    }
}

TEST(json, encodeDecodeUnsignedInteger) {
    { // Zero
        unsigned int val{0u};
        EXPECT_EQ(val, decode(encode(val)).as<unsigned int>());
    }
    { // Typical
        unsigned int val{123u};
        EXPECT_EQ(val, decode(encode(val)).as<unsigned int>());
    }
    { // Max 64
        uint64_t val{std::numeric_limits<uint64_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<uint64_t>());
    }
    { // Max 32
        uint32_t val{std::numeric_limits<uint32_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<uint32_t>());
    }
    { // Max 16
        uint16_t val{std::numeric_limits<uint16_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<uint16_t>());
    }
    { // Max 8
        uint8_t val{std::numeric_limits<uint8_t>::max()};
        EXPECT_EQ(val, decode(encode(val)).as<uint8_t>());
    }
}

TEST(json, encodeDecodeFloater) {
    uint64_t val64;
    uint32_t val32;

    { // Zero
        double val{0.0};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Typical
        double val{123.45};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Max integer 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'10000110011'1111111111111111111111111111111111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Max integer 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'10010110'11111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Max 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'11111111110'1111111111111111111111111111111111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Max 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'11111110'11111111111111111111111u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Min normal 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'00000000001'0000000000000000000000000000000000000000000000000000u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Min normal 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'00000001'00000000000000000000000u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Min subnormal 64
        double val{reinterpret_cast<const double &>(val64 = 0b0'00000000000'0000000000000000000000000000000000000000000000000001u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Min subnormal 32
        double val{reinterpret_cast<const float &>(val32 = 0b0'00000000'00000000000000000000001u)};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Positive infinity
        double val{std::numeric_limits<double>::infinity()};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // Negative infinity
        double val{-std::numeric_limits<double>::infinity()};
        EXPECT_EQ(val, decode(encode(val)).as<double>());
    }
    { // NaN
        EXPECT_TRUE(std::isnan(decode(encode(std::numeric_limits<double>::quiet_NaN())).as<double>()));
    }
}

TEST(json, encodeDecodeBoolean) {
    // true
    EXPECT_EQ(true, decode(encode(true)).asBoolean());
    // false
    EXPECT_EQ(false, decode(encode(false)).asBoolean());
}

TEST(json, encodeDecodeNull) {
    EXPECT_TRUE(decode(encode(nullptr)).isNull());
}

TEST(json, encodeDecodeCustom) {
    CustomVal val{1, 2};
    EXPECT_EQ(val, decode(encode(val)).as<CustomVal>());
}

TEST(json, valueConstruction) {
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
    EXPECT_EQ(Type::number, Value(int64_t(0)).type());
    EXPECT_EQ(Type::number, Value(int32_t(0)).type());
    EXPECT_EQ(Type::number, Value(int16_t(0)).type());
    EXPECT_EQ(Type::number, Value(int8_t(0)).type());
    EXPECT_EQ(Type::number, Value(uint64_t(0)).type());
    EXPECT_EQ(Type::number, Value(uint32_t(0)).type());
    EXPECT_EQ(Type::number, Value(uint16_t(0)).type());
    EXPECT_EQ(Type::number, Value(uint8_t(0)).type());
    EXPECT_EQ(Type::number, Value(0.0).type());
    EXPECT_EQ(Type::number, Value(0.0f).type());
    // Boolean
    EXPECT_EQ(Type::boolean, Value(false).type());
    // Null
    EXPECT_EQ(Type::null, Value(nullptr).type());
}

TEST(json, valueMove) {
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

TEST(json, valueTypes) {
    { // Object
        Value v(Object{});
        EXPECT_EQ(Type::object, v.type());
        EXPECT_TRUE(v.isObject());
        EXPECT_TRUE(v.is<Object>());
        v.asObject<safe>();
        v.asObject<unsafe>();
    }
    { // Array
        Value v(Array{});
        EXPECT_EQ(Type::array, v.type());
        EXPECT_TRUE(v.isArray());
        EXPECT_TRUE(v.is<Array>());
        v.asArray<safe>();
        v.asArray<unsafe>();
    }
    { // String
        Value v("abc"sv);
        EXPECT_EQ(Type::string, v.type());
        EXPECT_TRUE(v.isString());
        EXPECT_TRUE(v.is<std::string_view>());
        v.asString<safe>();
        v.asString<unsafe>();
        v.as<std::string_view, safe>();
        v.as<std::string_view, unsafe>();
    }
    { // Character
        Value v('a');
        EXPECT_EQ(Type::string, v.type());
        EXPECT_TRUE(v.isString());
        EXPECT_TRUE(v.is<std::string_view>());
        EXPECT_TRUE(v.is<char>());
        v.asString<safe>();
        v.asString<unsafe>();
        v.as<std::string_view, safe>();
        v.as<std::string_view, unsafe>();
        v.as<char, safe>();
        v.as<char, unsafe>();
    }
    { // Number
        Value v(123);
        EXPECT_EQ(Type::number, v.type());
        EXPECT_TRUE(v.isNumber());
        EXPECT_TRUE(v.is<int>());
        v.asNumber<safe>();
        v.asNumber<unsafe>();
        v.as<int, safe>();
        v.as<int, unsafe>();
    }
    { // Boolean
        Value v(false);
        EXPECT_EQ(Type::boolean, v.type());
        EXPECT_TRUE(v.isBoolean());
        EXPECT_TRUE(v.is<bool>());
        v.asBoolean<safe>();
        v.asBoolean<unsafe>();
        v.as<bool, safe>();
        v.as<bool, unsafe>();
    }
    { // Null
        Value v(nullptr);
        EXPECT_EQ(Type::null, v.type());
        EXPECT_TRUE(v.isNull());
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

    if (isS64) EXPECT_EQ( int64_t(v), val.as< int64_t>()); else EXPECT_THROW(val.as< int64_t>(), TypeError);
    if (isS32) EXPECT_EQ( int32_t(v), val.as< int32_t>()); else EXPECT_THROW(val.as< int32_t>(), TypeError);
    if (isS16) EXPECT_EQ( int16_t(v), val.as< int16_t>()); else EXPECT_THROW(val.as< int16_t>(), TypeError);
    if (isS08) EXPECT_EQ(  int8_t(v), val.as<  int8_t>()); else EXPECT_THROW(val.as<  int8_t>(), TypeError);
    if (isU64) EXPECT_EQ(uint64_t(v), val.as<uint64_t>()); else EXPECT_THROW(val.as<uint64_t>(), TypeError);
    if (isU32) EXPECT_EQ(uint32_t(v), val.as<uint32_t>()); else EXPECT_THROW(val.as<uint32_t>(), TypeError);
    if (isU16) EXPECT_EQ(uint16_t(v), val.as<uint16_t>()); else EXPECT_THROW(val.as<uint16_t>(), TypeError);
    if (isU08) EXPECT_EQ( uint8_t(v), val.as< uint8_t>()); else EXPECT_THROW(val.as< uint8_t>(), TypeError);
    if (isF64) EXPECT_EQ(  double(v), val.as<  double>()); else EXPECT_THROW(val.as<  double>(), TypeError);
    if (isF32) EXPECT_EQ(   float(v), val.as<   float>()); else EXPECT_THROW(val.as<   float>(), TypeError);
}

TEST(json, valueNumbers) {
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

TEST(json, valueAs) {
    // Safe
    EXPECT_THROW((Value().asObject<safe>()), TypeError);
    EXPECT_THROW((Value().asArray<safe>()), TypeError);
    EXPECT_THROW((Value().asString<safe>()), TypeError);
    EXPECT_THROW((Value().as<std::string_view, safe>()), TypeError);
    EXPECT_THROW((Value().asNumber<safe>()), TypeError);
    EXPECT_THROW((Value().as<int64_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<int32_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<int16_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<int8_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<uint64_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<uint32_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<uint16_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<uint8_t, safe>()), TypeError);
    EXPECT_THROW((Value().as<double, safe>()), TypeError);
    EXPECT_THROW((Value().as<float, safe>()), TypeError);
    EXPECT_THROW((Value().asBoolean<safe>()), TypeError);
    EXPECT_THROW((Value().as<bool, safe>()), TypeError);

    // Unsafe
    Value().asObject<unsafe>();
    Value().asArray<unsafe>();
    Value().asString<unsafe>();
    Value().as<std::string_view, unsafe>();
    Value().asNumber<unsafe>();
    Value().as<int64_t, unsafe>();
    Value().as<int32_t, unsafe>();
    Value().as<int16_t, unsafe>();
    Value().as<int8_t, unsafe>();
    Value().as<uint64_t, unsafe>();
    Value().as<uint32_t, unsafe>();
    Value().as<uint16_t, unsafe>();
    Value().as<uint8_t, unsafe>();
    Value().as<double, unsafe>();
    Value().as<float, unsafe>();
    Value().asBoolean<unsafe>();
    Value().as<bool, unsafe>();
}

TEST(json, object) {
    Object obj;
    { // Initial state
        EXPECT_EQ(0u, obj.size());
        EXPECT_EQ(0u, obj.capacity());
        EXPECT_TRUE(obj.empty());
        EXPECT_FALSE(obj.contains("key"sv));
        EXPECT_TRUE(obj.find("key"sv) == obj.end());
        EXPECT_THROW(obj.at("key"sv), std::out_of_range);
    }
    { // When adding a single element
        obj.add("k0"s, 0);
        EXPECT_EQ(1u, obj.size());
        EXPECT_EQ(8u, obj.capacity());
        EXPECT_FALSE(obj.empty());
        EXPECT_TRUE(obj.contains("k0"sv));
        auto it(obj.find("k0"sv));
        EXPECT_TRUE(it != obj.end());
        EXPECT_EQ("k0"s, it->first);
        EXPECT_EQ(0, it->second.as<int>());
        EXPECT_EQ(0, obj.at("k0"sv).as<int>());
    }
    { // Filling up its capacity
        obj.add("k1"s, 1);
        obj.add("k2"s, 2);
        obj.add("k3"s, 3);
        obj.add("k4"s, 4);
        obj.add("k5"s, 5);
        obj.add("k6"s, 6);
        obj.add("k7"s, 7);
        EXPECT_EQ(8u, obj.size());
        EXPECT_EQ(8u, obj.capacity());
    }
    { // Exceeding its capacity
        obj.add("k8"s, 8);
        EXPECT_EQ(9u, obj.size());
        EXPECT_EQ(16u, obj.capacity());
    }
    { // Expected contents in order
        auto it(obj.cbegin());
        EXPECT_EQ("k0"s, it->first);
        EXPECT_EQ(0, it->second.as<int>());
        ++it;
        EXPECT_EQ("k1"s, it->first);
        EXPECT_EQ(1, it->second.as<int>());
        ++it;
        EXPECT_EQ("k2"s, it->first);
        EXPECT_EQ(2, it->second.as<int>());
        ++it;
        EXPECT_EQ("k3"s, it->first);
        EXPECT_EQ(3, it->second.as<int>());
        ++it;
        EXPECT_EQ("k4"s, it->first);
        EXPECT_EQ(4, it->second.as<int>());
        ++it;
        EXPECT_EQ("k5"s, it->first);
        EXPECT_EQ(5, it->second.as<int>());
        ++it;
        EXPECT_EQ("k6"s, it->first);
        EXPECT_EQ(6, it->second.as<int>());
        ++it;
        EXPECT_EQ("k7"s, it->first);
        EXPECT_EQ(7, it->second.as<int>());
        ++it;
        EXPECT_EQ("k8"s, it->first);
        EXPECT_EQ(8, it->second.as<int>());
        ++it;
        EXPECT_TRUE(it == obj.cend());
    }
    { // Accessing some element
        EXPECT_TRUE(obj.contains("k7"sv));
        auto it(obj.find("k7"sv));
        EXPECT_TRUE(it != obj.end());
        EXPECT_EQ("k7"s, it->first);
        EXPECT_EQ(7, it->second.as<int>());
        EXPECT_EQ(7, obj.at("k7"sv).as<int>());
    }
    { // Moving
        Object obj2(std::move(obj));
        EXPECT_EQ(0u, obj.size());
        EXPECT_EQ(0u, obj.capacity());
        EXPECT_EQ(9u, obj2.size());
        EXPECT_EQ(16u, obj2.capacity());
        EXPECT_TRUE(obj2.contains("k7"sv));
        obj = std::move(obj2);
    }
    { // Removing middle
        EXPECT_EQ(3, obj.remove(obj.find("k3"sv)).second.as<int>());
        EXPECT_EQ(8u, obj.size());
        EXPECT_EQ(4, obj.cbegin()[3].second.as<int>());
    }
    { // Removing first
        EXPECT_EQ(0, obj.remove(obj.begin()).second.as<int>());
        EXPECT_EQ(7u, obj.size());
        EXPECT_EQ(1, obj.cbegin()->second.as<int>());
    }
    { // Removing last
        EXPECT_EQ(8, obj.remove(obj.end() - 1).second.as<int>());
        EXPECT_EQ(6u, obj.size());
    }
    { // Clear
        obj.clear();
        EXPECT_EQ(0u, obj.size());
        EXPECT_EQ(16u, obj.capacity());
    }
}

TEST(json, array) {
    Array arr;
    { // Initial state
        EXPECT_EQ(0u, arr.size());
        EXPECT_EQ(0u, arr.capacity());
        EXPECT_TRUE(arr.empty());
        EXPECT_THROW(arr.at(0), std::out_of_range);
    }
    { // When adding a single element
        arr.add(0);
        EXPECT_EQ(1u, arr.size());
        EXPECT_EQ(8u, arr.capacity());
        EXPECT_FALSE(arr.empty());
        EXPECT_EQ(0, arr.at(0).as<int>());
        EXPECT_THROW(arr.at(1), std::out_of_range);
    }
    { // Filling up its capacity
        arr.add(1);
        arr.add(2);
        arr.add(3);
        arr.add(4);
        arr.add(5);
        arr.add(6);
        arr.add(7);
        EXPECT_EQ(8u, arr.size());
        EXPECT_EQ(8u, arr.capacity());
    }
    { // Exceeding its capacity
        arr.add(8);
        EXPECT_EQ(9u, arr.size());
        EXPECT_EQ(16u, arr.capacity());
    }
    { // Expected contents in order
        auto it(arr.cbegin());
        EXPECT_EQ(0, it->as<int>());
        ++it;
        EXPECT_EQ(1, it->as<int>());
        ++it;
        EXPECT_EQ(2, it->as<int>());
        ++it;
        EXPECT_EQ(3, it->as<int>());
        ++it;
        EXPECT_EQ(4, it->as<int>());
        ++it;
        EXPECT_EQ(5, it->as<int>());
        ++it;
        EXPECT_EQ(6, it->as<int>());
        ++it;
        EXPECT_EQ(7, it->as<int>());
        ++it;
        EXPECT_EQ(8, it->as<int>());
        ++it;
        EXPECT_TRUE(it == arr.cend());
    }
    { // Accessing some element
        EXPECT_EQ(7, arr.at(7).as<int>());
        EXPECT_THROW(arr.at(9), std::out_of_range);
    }
    { // Moving
        Array arr2(std::move(arr));
        EXPECT_EQ(0u, arr.size());
        EXPECT_EQ(0u, arr.capacity());
        EXPECT_EQ(9u, arr2.size());
        EXPECT_EQ(16u, arr2.capacity());
        EXPECT_EQ(7, arr2.at(7).as<int>());
        arr = std::move(arr2);
    }
    { // Removing middle
        EXPECT_EQ(3, arr.remove(3).as<int>());
        EXPECT_EQ(8u, arr.size());
        EXPECT_EQ(4, arr.at(3).as<int>());
    }
    { // Removing first
        EXPECT_EQ(0, arr.remove(arr.begin()).as<int>());
        EXPECT_EQ(7u, arr.size());
        EXPECT_EQ(1, arr.at(0).as<int>());
    }
    { // Removing last
        EXPECT_EQ(8, arr.remove(arr.end() - 1).as<int>());
        EXPECT_EQ(6u, arr.size());
    }
    { // Removing a section
        arr.remove(arr.begin() + 1, arr.begin() + 4);
        EXPECT_EQ(3u, arr.size());
        EXPECT_EQ(6, arr.at(1).as<int>());
    }
    { // Clear
        arr.clear();
        EXPECT_EQ(0u, arr.size());
        EXPECT_EQ(16u, arr.capacity());
        EXPECT_THROW(arr.remove(0u), std::out_of_range);
    }
    { // Explicit instantiation
        arr = Array(true, 6, "wow");
        EXPECT_EQ(3u, arr.size());
        EXPECT_EQ(8u, arr.capacity());
        EXPECT_EQ(true, arr.at(0).asBoolean());
        EXPECT_EQ(6, arr.at(1).as<int>());
        EXPECT_EQ("wow"sv, arr.at(2).asString());

        arr = Array(nullptr);
        EXPECT_EQ(1u, arr.size());
        EXPECT_EQ(8u, arr.capacity());
        EXPECT_TRUE(arr.at(0).isNull());

        arr = Array(1, 2, 3, 4, 5, 6, 7, 8, 9);
        EXPECT_EQ(9u, arr.size());
        EXPECT_EQ(16u, arr.capacity());
    }
}

TEST(json, string) {
    { // Standard
        String str("abc"sv);
        EXPECT_EQ(3u, str.size());
        EXPECT_EQ("abc"sv, str.view());
    }
    { // Inline
        String str("123456123456"sv);
        EXPECT_EQ(12u, str.size());
        EXPECT_EQ("123456123456"sv, str.view());
        EXPECT_EQ(reinterpret_cast<const char *>(&str) + 4, str.view().data());

        // Moving
        String str2(std::move(str));
        EXPECT_EQ(""sv, str.view());
        EXPECT_EQ("123456123456"sv, str2.view());
    }
    { // Dynamic
        String str("1234561234561"sv);
        EXPECT_EQ(13u, str.size());
        EXPECT_EQ("1234561234561"sv, str.view());
        EXPECT_NE(reinterpret_cast<const char *>(&str) + 4, str.view().data());

        // Moving
        String str2(std::move(str));
        EXPECT_EQ(""sv, str.view());
        EXPECT_EQ("1234561234561"sv, str2.view());
    }
}

TEST(json, density) {
    EXPECT_EQ(R"([
    1,
    2,
    3
])"s, encode(Array{1, 2, 3}, multiline));
    EXPECT_EQ("[ 1, 2, 3 ]"s, encode(Array{1, 2, 3}, uniline));
    EXPECT_EQ("[1,2,3]"s, encode(Array{1, 2, 3}, compact));
}

TEST(json, general) {
    std::string json(R"({
    "Dishes": [
        {
            "Gluten Free": false,
            "Ingredients": [
                "\"Salt\"",
                "Barnacles"
            ],
            "Name": "Basket o' Barnacles",
            "Price": 5.45
        },
        {
            "Gluten Free": true,
            "Ingredients": [
                "Tuna"
            ],
            "Name": "Two Tuna",
            "Price": -inf
        },
        {
            "Gluten Free": false,
            "Ingredients": [
                "\"Salt\"",
                "Octopus",
                "Crab"
            ],
            "Name": "18 Leg Bouquet",
            "Price": nan
        }
    ],
    "Employees": [
        {
            "Age": 69,
            "Name": "Ol' Joe Fisher",
            "Title": "Fisherman"
        },
        {
            "Age": 41,
            "Name": "Mark Rower",
            "Title": "Cook"
        },
        {
            "Age": 19,
            "Name": "Phineas",
            "Title": "Server Boy"
        }
    ],
    "Founded": 1964,
    "Green Eggs and Ham": "I do not like them in a box\nI do not like them with a fox\nI do not like them in a house\nI do not like them with a mouse\nI do not like them here or there\nI do not like them anywhere\nI do not like green eggs and ham\nI do not like them Sam I am\n",
    "Ha\x03r Name": "M\0\0n",
    "Name": "Salt's Crust",
    "Profit Margin": null
})"s);
    EXPECT_EQ(json, encode(decode(json)));
}
