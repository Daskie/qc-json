#include "CppUnitTest.h"

#include "qc-json.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

struct CustomVal { int x, y; };

bool operator==(const CustomVal & cv1, const CustomVal & cv2) {
    return cv1.x == cv2.x && cv1.y == cv2.y;
}

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {
            template <> static std::wstring ToString<uint16_t>(const uint16_t & v) { return std::to_wstring(v); }
            template <> static std::wstring ToString<nullptr_t>(const nullptr_t & v) { return L"null"; }
            template <> static std::wstring ToString<qc::json::Type>(const qc::json::Type & type) { return std::to_wstring(std::underlying_type_t<qc::json::Type>(type)); }
            template <> static std::wstring ToString<CustomVal>(const CustomVal & v) { return L"(" + std::to_wstring(v.x) + L", " + std::to_wstring(v.y) + L")"; }
        }
    }
}

template <bool safe>
struct qc_json_valueTo<CustomVal, safe> {
    CustomVal operator()(const qc::json::Value & val) const {
        const qc::json::Array & arr(val.asArray<safe>());
        return {arr.at(0).as<int, safe>(), arr.at(1).as<int, safe>()};
    }
};

template <>
struct qc_json_valueFrom<CustomVal> {
    qc::json::Value operator()(const CustomVal & v) const {
        return qc::json::Array(v.x, v.y);
    }
};

TEST_CLASS(Json) {

  public:

    template <typename T>
    void thereAndBackAgain(T v) {
        Assert::AreEqual(v, qc::json::decode(qc::json::encode(v)).as<T>());
    }

    TEST_METHOD(EncodeDecodeString) {
        // Empty
        thereAndBackAgain(""sv);
        // Typical
        thereAndBackAgain("abc"sv);
        // Printable characters
        thereAndBackAgain(R"( !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv);
        // Escape characters
        thereAndBackAgain("\b\f\n\r\t"sv);
        // Unicode
        thereAndBackAgain("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv);
    }

    TEST_METHOD(EncodeDecodeSignedInteger) {
        // Zero
        thereAndBackAgain(0);
        // Typical
        thereAndBackAgain(123);
        // Max 64
        thereAndBackAgain(std::numeric_limits<int64_t>::max());
        // Min 64
        thereAndBackAgain(std::numeric_limits<int64_t>::min());
        // Max 32
        thereAndBackAgain(std::numeric_limits<int32_t>::max());
        // Min 32
        thereAndBackAgain(std::numeric_limits<int32_t>::min());
        // Max 16
        thereAndBackAgain(std::numeric_limits<int16_t>::max());
        // Min 16
        thereAndBackAgain(std::numeric_limits<int16_t>::min());
        // Max 8
        thereAndBackAgain(std::numeric_limits<int8_t>::max());
        // Min 8
        thereAndBackAgain(std::numeric_limits<int8_t>::min());
    }

    TEST_METHOD(EncodeDecodeUnsignedInteger) {
        // Zero
        thereAndBackAgain(0u);
        // Typical
        thereAndBackAgain(123u);
        // Max 64
        thereAndBackAgain(std::numeric_limits<uint64_t>::max());
        // Max 32
        thereAndBackAgain(std::numeric_limits<uint32_t>::max());
        // Max 16
        thereAndBackAgain(std::numeric_limits<uint16_t>::max());
        // Max 8
        thereAndBackAgain(std::numeric_limits<uint8_t>::max());
    }

    TEST_METHOD(EncodeDecodeFloater) {
        uint64_t val64;
        uint32_t val32;

        // Zero
        thereAndBackAgain(0.0);
        // Typical
        thereAndBackAgain(123.45);
        // Max integer 64
        thereAndBackAgain(reinterpret_cast<const double &>(val64 = 0b0'10000110011'1111111111111111111111111111111111111111111111111111u));
        // Max integer 32
        thereAndBackAgain(reinterpret_cast<const float &>(val32 = 0b0'10010110'11111111111111111111111u));
        // Max 64
        thereAndBackAgain(reinterpret_cast<const double &>(val64 = 0b0'11111111110'1111111111111111111111111111111111111111111111111111u));
        // Max 32
        thereAndBackAgain(reinterpret_cast<const float &>(val32 = 0b0'11111110'11111111111111111111111u));
        // Min normal 64
        thereAndBackAgain(reinterpret_cast<const double &>(val64 = 0b0'00000000001'0000000000000000000000000000000000000000000000000000u));
        // Min normal 32
        thereAndBackAgain(reinterpret_cast<const float &>(val32 = 0b0'00000001'00000000000000000000000u));
        // Min subnormal 64
        thereAndBackAgain(reinterpret_cast<const double &>(val64 = 0b0'00000000000'0000000000000000000000000000000000000000000000000001u));
        // Min subnormal 32
        thereAndBackAgain(reinterpret_cast<const float &>(val32 = 0b0'00000000'00000000000000000000001u));
        // Positive infinity
        thereAndBackAgain(std::numeric_limits<double>::infinity());
        // Negative infinity
        thereAndBackAgain(-std::numeric_limits<double>::infinity());
        // NaN
        Assert::IsTrue(std::isnan(qc::json::decode(qc::json::encode(std::numeric_limits<double>::quiet_NaN())).as<double>()));
    }

    TEST_METHOD(EncodeDecodeBoolean) {
        // true
        thereAndBackAgain(true);
        // false
        thereAndBackAgain(false);
    }

    TEST_METHOD(EncodeDecodeNull) {
        Assert::IsTrue(qc::json::decode(qc::json::encode(nullptr)).isNull());
    }

    TEST_METHOD(EncodeDecodeCustom) {
        thereAndBackAgain(CustomVal{1, 2});
    }

    TEST_METHOD(ValueConstruction) {
        // Default
        Assert::AreEqual(qc::json::Type::null, qc::json::Value().type());
        // Object
        Assert::AreEqual(qc::json::Type::object, qc::json::Value(qc::json::Object()).type());
        // Array
        Assert::AreEqual(qc::json::Type::array, qc::json::Value(qc::json::Array()).type());
        // String
        Assert::AreEqual(qc::json::Type::string, qc::json::Value("abc"sv).type());
        Assert::AreEqual(qc::json::Type::string, qc::json::Value("abc"s).type());
        Assert::AreEqual(qc::json::Type::string, qc::json::Value("abc").type());
        Assert::AreEqual(qc::json::Type::string, qc::json::Value(const_cast<char *>("abc")).type());
        Assert::AreEqual(qc::json::Type::string, qc::json::Value('a').type());
        // Number
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(int64_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(int32_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(int16_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(int8_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(uint64_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(uint32_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(uint16_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(uint8_t(0)).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(0.0).type());
        Assert::AreEqual(qc::json::Type::number, qc::json::Value(0.0f).type());
        // Boolean
        Assert::AreEqual(qc::json::Type::boolean, qc::json::Value(false).type());
        // Null
        Assert::AreEqual(qc::json::Type::null, qc::json::Value(nullptr).type());
    }

    TEST_METHOD(ValueMove) {
        qc::json::Value v1("abc"sv);
        Assert::AreEqual(qc::json::Type::string, v1.type());
        Assert::AreEqual("abc"sv, v1.asString());

        qc::json::Value v2(std::move(v1));
        Assert::AreEqual(qc::json::Type::null, v1.type());
        Assert::AreEqual(qc::json::Type::string, v2.type());
        Assert::AreEqual("abc"sv, v2.asString());

        v1 = std::move(v2);
        Assert::AreEqual(qc::json::Type::string, v1.type());
        Assert::AreEqual("abc"sv, v1.asString());
        Assert::AreEqual(qc::json::Type::null, v2.type());
    }

    TEST_METHOD(ValueTypes) {
        { // Object
            qc::json::Value v(qc::json::Object{});
            Assert::AreEqual(qc::json::Type::object, v.type());
            Assert::IsTrue(v.isObject());
            Assert::IsTrue(v.is<qc::json::Object>());
            v.asObject<false>();
            v.asObject<true>();
        }
        { // Array
            qc::json::Value v(qc::json::Array{});
            Assert::AreEqual(qc::json::Type::array, v.type());
            Assert::IsTrue(v.isArray());
            Assert::IsTrue(v.is<qc::json::Array>());
            v.asArray<false>();
            v.asArray<true>();
        }
        { // String
            qc::json::Value v("abc"sv);
            Assert::AreEqual(qc::json::Type::string, v.type());
            Assert::IsTrue(v.isString());
            Assert::IsTrue(v.is<std::string_view>());
            v.asString<false>();
            v.asString<true>();
            v.as<std::string_view, true>();
            v.as<std::string_view, false>();
        }
        { // Character
            qc::json::Value v('a');
            Assert::AreEqual(qc::json::Type::string, v.type());
            Assert::IsTrue(v.isString());
            Assert::IsTrue(v.is<std::string_view>());
            Assert::IsTrue(v.is<char>());
            v.asString<false>();
            v.asString<true>();
            v.as<std::string_view, true>();
            v.as<std::string_view, false>();
            v.as<char, true>();
            v.as<char, false>();
        }
        { // Number
            qc::json::Value v(123);
            Assert::AreEqual(qc::json::Type::number, v.type());
            Assert::IsTrue(v.isNumber());
            Assert::IsTrue(v.is<int>());
            v.asNumber<true>();
            v.asNumber<false>();
            v.as<int, true>();
            v.as<int, false>();
        }
        { // Boolean
            qc::json::Value v(false);
            Assert::AreEqual(qc::json::Type::boolean, v.type());
            Assert::IsTrue(v.isBoolean());
            Assert::IsTrue(v.is<bool>());
            v.asBoolean<true>();
            v.asBoolean<false>();
            v.as<bool, true>();
            v.as<bool, false>();
        }
        { // Null
            qc::json::Value v(nullptr);
            Assert::AreEqual(qc::json::Type::null, v.type());
            Assert::IsTrue(v.isNull());
        }
    }

    template <typename T>
    void testNumber(T v, bool isS64, bool isS32, bool isS16, bool isS08, bool isU64, bool isU32, bool isU16, bool isU08, bool isF64, bool isF32) {
        qc::json::Value val(v);
        Assert::AreEqual(isS64, val.is< int64_t>());
        Assert::AreEqual(isS32, val.is< int32_t>());
        Assert::AreEqual(isS16, val.is< int16_t>());
        Assert::AreEqual(isS08, val.is<  int8_t>());
        Assert::AreEqual(isU64, val.is<uint64_t>());
        Assert::AreEqual(isU32, val.is<uint32_t>());
        Assert::AreEqual(isU16, val.is<uint16_t>());
        Assert::AreEqual(isU08, val.is< uint8_t>());
        Assert::AreEqual(isF64, val.is<  double>());
        Assert::AreEqual(isF32, val.is<   float>());

        if (isS64) Assert::AreEqual( int64_t(v), val.as< int64_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as< int64_t>(); });
        if (isS32) Assert::AreEqual( int32_t(v), val.as< int32_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as< int32_t>(); });
        if (isS16) Assert::AreEqual( int16_t(v), val.as< int16_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as< int16_t>(); });
        if (isS08) Assert::AreEqual(  int8_t(v), val.as<  int8_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<  int8_t>(); });
        if (isU64) Assert::AreEqual(uint64_t(v), val.as<uint64_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<uint64_t>(); });
        if (isU32) Assert::AreEqual(uint32_t(v), val.as<uint32_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<uint32_t>(); });
        if (isU16) Assert::AreEqual(uint16_t(v), val.as<uint16_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<uint16_t>(); });
        if (isU08) Assert::AreEqual( uint8_t(v), val.as< uint8_t>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as< uint8_t>(); });
        if (isF64) Assert::AreEqual(  double(v), val.as<  double>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<  double>(); });
        if (isF32) Assert::AreEqual(   float(v), val.as<   float>()); else Assert::ExpectException<qc::json::TypeError>([&val]() { val.as<   float>(); });
    }

    TEST_METHOD(ValueNumbers) {
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

    TEST_METHOD(ValueAs) {
        // Safe
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asObject<true>(); } );
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asArray<true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asString<true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<std::string_view, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asNumber<true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int64_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int32_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int16_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int8_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint64_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint32_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint16_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint8_t, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<double, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<float, true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asBoolean<true>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<bool, true>(); });

        // Unsafe
        qc::json::Value().asObject<false>();
        qc::json::Value().asArray<false>();
        qc::json::Value().asString<false>();
        qc::json::Value().as<std::string_view, false>();
        qc::json::Value().asNumber<false>();
        qc::json::Value().as<int64_t, false>();
        qc::json::Value().as<int32_t, false>();
        qc::json::Value().as<int16_t, false>();
        qc::json::Value().as<int8_t, false>();
        qc::json::Value().as<uint64_t, false>();
        qc::json::Value().as<uint32_t, false>();
        qc::json::Value().as<uint16_t, false>();
        qc::json::Value().as<uint8_t, false>();
        qc::json::Value().as<double, false>();
        qc::json::Value().as<float, false>();
        qc::json::Value().asBoolean<false>();
        qc::json::Value().as<bool, false>();
    }

    TEST_METHOD(Object) {
        qc::json::Object obj;
        { // Initial state
            Assert::AreEqual(0u, obj.size());
            Assert::AreEqual(0u, obj.capacity());
            Assert::IsTrue(obj.empty());
            Assert::IsFalse(obj.contains("key"sv));
            Assert::IsTrue(obj.find("key"sv) == obj.end());
            Assert::ExpectException<std::out_of_range>([&obj]() { obj.at("key"sv); });
        }
        { // When adding a single element
            obj.add("k0"s, 0);
            Assert::AreEqual(1u, obj.size());
            Assert::AreEqual(8u, obj.capacity());
            Assert::IsFalse(obj.empty());
            Assert::IsTrue(obj.contains("k0"sv));
            auto it(obj.find("k0"sv));
            Assert::IsTrue(it != obj.end());
            Assert::AreEqual("k0"s, it->first);
            Assert::AreEqual(0, it->second.as<int>());
            Assert::AreEqual(0, obj.at("k0"sv).as<int>());
        }
        { // Filling up its capacity
            obj.add("k1"s, 1);
            obj.add("k2"s, 2);
            obj.add("k3"s, 3);
            obj.add("k4"s, 4);
            obj.add("k5"s, 5);
            obj.add("k6"s, 6);
            obj.add("k7"s, 7);
            Assert::AreEqual(8u, obj.size());
            Assert::AreEqual(8u, obj.capacity());
        }
        { // Exceeding its capacity
            obj.add("k8"s, 8);
            Assert::AreEqual(9u, obj.size());
            Assert::AreEqual(16u, obj.capacity());
        }
        { // Expected contents in order
            auto it(obj.cbegin());
            Assert::AreEqual("k0"s, it->first);
            Assert::AreEqual(0, it->second.as<int>());
            ++it;
            Assert::AreEqual("k1"s, it->first);
            Assert::AreEqual(1, it->second.as<int>());
            ++it;
            Assert::AreEqual("k2"s, it->first);
            Assert::AreEqual(2, it->second.as<int>());
            ++it;
            Assert::AreEqual("k3"s, it->first);
            Assert::AreEqual(3, it->second.as<int>());
            ++it;
            Assert::AreEqual("k4"s, it->first);
            Assert::AreEqual(4, it->second.as<int>());
            ++it;
            Assert::AreEqual("k5"s, it->first);
            Assert::AreEqual(5, it->second.as<int>());
            ++it;
            Assert::AreEqual("k6"s, it->first);
            Assert::AreEqual(6, it->second.as<int>());
            ++it;
            Assert::AreEqual("k7"s, it->first);
            Assert::AreEqual(7, it->second.as<int>());
            ++it;
            Assert::AreEqual("k8"s, it->first);
            Assert::AreEqual(8, it->second.as<int>());
            ++it;
            Assert::IsTrue(it == obj.cend());
        }
        { // Accessing some element
            Assert::IsTrue(obj.contains("k7"sv));
            auto it(obj.find("k7"sv));
            Assert::IsTrue(it != obj.end());
            Assert::AreEqual("k7"s, it->first);
            Assert::AreEqual(7, it->second.as<int>());
            Assert::AreEqual(7, obj.at("k7"sv).as<int>());
        }
        { // Moving
            qc::json::Object obj2(std::move(obj));
            Assert::AreEqual(0u, obj.size());
            Assert::AreEqual(0u, obj.capacity());
            Assert::AreEqual(9u, obj2.size());
            Assert::AreEqual(16u, obj2.capacity());
            Assert::IsTrue(obj2.contains("k7"sv));
            obj = std::move(obj2);
        }
        { // Removing middle
            Assert::AreEqual(3, obj.remove(obj.find("k3"sv)).second.as<int>());
            Assert::AreEqual(8u, obj.size());
            Assert::AreEqual(4, obj.cbegin()[3].second.as<int>());
        }
        { // Removing first
            Assert::AreEqual(0, obj.remove(obj.begin()).second.as<int>());
            Assert::AreEqual(7u, obj.size());
            Assert::AreEqual(1, obj.cbegin()->second.as<int>());
        }
        { // Removing last
            Assert::AreEqual(8, obj.remove(obj.end() - 1).second.as<int>());
            Assert::AreEqual(6u, obj.size());
        }
        { // Clear
            obj.clear();
            Assert::AreEqual(0u, obj.size());
            Assert::AreEqual(16u, obj.capacity());
        }
    }

    TEST_METHOD(Array) {
        qc::json::Array arr;
        { // Initial state
            Assert::AreEqual(0u, arr.size());
            Assert::AreEqual(0u, arr.capacity());
            Assert::IsTrue(arr.empty());
            Assert::ExpectException<std::out_of_range>([&arr]() { arr.at(0); });
        }
        { // When adding a single element
            arr.add(0);
            Assert::AreEqual(1u, arr.size());
            Assert::AreEqual(8u, arr.capacity());
            Assert::IsFalse(arr.empty());
            Assert::AreEqual(0, arr.at(0).as<int>());
            Assert::ExpectException<std::out_of_range>([&arr]() { arr.at(1); });
        }
        { // Filling up its capacity
            arr.add(1);
            arr.add(2);
            arr.add(3);
            arr.add(4);
            arr.add(5);
            arr.add(6);
            arr.add(7);
            Assert::AreEqual(8u, arr.size());
            Assert::AreEqual(8u, arr.capacity());
        }
        { // Exceeding its capacity
            arr.add(8);
            Assert::AreEqual(9u, arr.size());
            Assert::AreEqual(16u, arr.capacity());
        }
        { // Expected contents in order
            auto it(arr.cbegin());
            Assert::AreEqual(0, it->as<int>());
            ++it;
            Assert::AreEqual(1, it->as<int>());
            ++it;
            Assert::AreEqual(2, it->as<int>());
            ++it;
            Assert::AreEqual(3, it->as<int>());
            ++it;
            Assert::AreEqual(4, it->as<int>());
            ++it;
            Assert::AreEqual(5, it->as<int>());
            ++it;
            Assert::AreEqual(6, it->as<int>());
            ++it;
            Assert::AreEqual(7, it->as<int>());
            ++it;
            Assert::AreEqual(8, it->as<int>());
            ++it;
            Assert::IsTrue(it == arr.cend());
        }
        { // Accessing some element
            Assert::AreEqual(7, arr.at(7).as<int>());
            Assert::ExpectException<std::out_of_range>([&arr]() { arr.at(9); });
        }
        { // Moving
            qc::json::Array arr2(std::move(arr));
            Assert::AreEqual(0u, arr.size());
            Assert::AreEqual(0u, arr.capacity());
            Assert::AreEqual(9u, arr2.size());
            Assert::AreEqual(16u, arr2.capacity());
            Assert::AreEqual(7, arr2.at(7).as<int>());
            arr = std::move(arr2);
        }
        { // Removing middle
            Assert::AreEqual(3, arr.remove(3).as<int>());
            Assert::AreEqual(8u, arr.size());
            Assert::AreEqual(4, arr.at(3).as<int>());
        }
        { // Removing first
            Assert::AreEqual(0, arr.remove(arr.begin()).as<int>());
            Assert::AreEqual(7u, arr.size());
            Assert::AreEqual(1, arr.at(0).as<int>());
        }
        { // Removing last
            Assert::AreEqual(8, arr.remove(arr.end() - 1).as<int>());
            Assert::AreEqual(6u, arr.size());
        }
        { // Removing a section
            arr.remove(arr.begin() + 1, arr.begin() + 4);
            Assert::AreEqual(3u, arr.size());
            Assert::AreEqual(6, arr.at(1).as<int>());
        }
        { // Clear
            arr.clear();
            Assert::AreEqual(0u, arr.size());
            Assert::AreEqual(16u, arr.capacity());
            Assert::ExpectException<std::out_of_range>([&arr]() { arr.remove(0u); });
        }
        { // Explicit instantiation
            arr = qc::json::Array(true, 6, "wow");
            Assert::AreEqual(3u, arr.size());
            Assert::AreEqual(8u, arr.capacity());
            Assert::AreEqual(true, arr.at(0).asBoolean());
            Assert::AreEqual(6, arr.at(1).as<int>());
            Assert::AreEqual("wow"sv, arr.at(2).asString());

            arr = qc::json::Array(nullptr);
            Assert::AreEqual(1u, arr.size());
            Assert::AreEqual(8u, arr.capacity());
            Assert::IsTrue(arr.at(0).isNull());

            arr = qc::json::Array(1, 2, 3, 4, 5, 6, 7, 8, 9);
            Assert::AreEqual(9u, arr.size());
            Assert::AreEqual(16u, arr.capacity());
        }
    }

    TEST_METHOD(String) {
        { // Standard
            qc::json::String str("abc"sv);
            Assert::AreEqual(3u, str.size());
            Assert::AreEqual("abc"sv, str.view());
        }
        { // Inline
            qc::json::String str("123456123456"sv);
            Assert::AreEqual(12u, str.size());
            Assert::AreEqual("123456123456"sv, str.view());
            Assert::AreEqual(reinterpret_cast<const char *>(&str) + 4, str.view().data());

            // Moving
            qc::json::String str2(std::move(str));
            Assert::AreEqual(""sv, str.view());
            Assert::AreEqual("123456123456"sv, str2.view());
        }
        { // Dynamic
            qc::json::String str("1234561234561"sv);
            Assert::AreEqual(13u, str.size());
            Assert::AreEqual("1234561234561"sv, str.view());
            Assert::AreNotEqual(reinterpret_cast<const char *>(&str) + 4, str.view().data());

            // Moving
            qc::json::String str2(std::move(str));
            Assert::AreEqual(""sv, str.view());
            Assert::AreEqual("1234561234561"sv, str2.view());
        }
    }

    TEST_METHOD(General) {
        std::string json(R"({
    "Dishes": [
        {
            "Gluten Free": false,
            "Ingredients": [
                "Salt",
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
            "Price": 14.99
        },
        {
            "Gluten Free": false,
            "Ingredients": [
                "Salt",
                "Octopus",
                "Crab"
            ],
            "Name": "18 Leg Bouquet",
            "Price": 18.18
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
    "Name": "Salt's Crust",
    "Profit Margin": null
})"s);
        Assert::AreEqual(json, qc::json::encode(qc::json::decode(json)));
    }

};
