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

template <bool unsafe>
struct qc_json_valueTo<CustomVal, unsafe> {
    CustomVal operator()(const qc::json::Value & val) const {
        const qc::json::Array & arr(val.asArray<unsafe>());
        return {arr.at(0).as<int, unsafe>(), arr.at(1).as<int, unsafe>()};
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

    TEST_METHOD(EncodeDecodeInteger) {
        // Zero
        thereAndBackAgain(0);
        // Typical
        thereAndBackAgain(123);
        // Max 64
        thereAndBackAgain(int64_t(9223372036854775807));
        // Min 64
        thereAndBackAgain(int64_t(9223372036854775808));
        // Max 64 unsigned
        thereAndBackAgain(uint64_t(0xFFFFFFFFFFFFFFFFu));
        // Max 32
        thereAndBackAgain(int32_t(2147483647));
        // Min 32
        thereAndBackAgain(int32_t(2147483648));
        // Max 32 unsigned
        thereAndBackAgain(uint32_t(0xFFFFFFFFu));
        // Max 16
        thereAndBackAgain(int16_t(32767));
        // Min 16
        thereAndBackAgain(int16_t(-32768));
        // Max 16 unsigned
        thereAndBackAgain(uint16_t(0xFFFFu));
        // Max 8
        thereAndBackAgain(int8_t(127));
        // Min 8
        thereAndBackAgain(int8_t(128));
        // Max 8 unsigned
        thereAndBackAgain(uint8_t(0xFFu));
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
        Assert::IsTrue(std::isnan(qc::json::decode(qc::json::encode(std::numeric_limits<double>::quiet_NaN())).asFloater()));
    }

    TEST_METHOD(EncodeDecodeBoolean) {
        // true
        thereAndBackAgain(true);
        // false
        thereAndBackAgain(false);
    }

    TEST_METHOD(EncodeDecodeNull) {
        thereAndBackAgain(nullptr);
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
        // Integer
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(int64_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(uint64_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(int32_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(uint32_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(int16_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(uint16_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(int8_t(0)).type());
        Assert::AreEqual(qc::json::Type::integer, qc::json::Value(uint8_t(0)).type());
        // Floater
        Assert::AreEqual(qc::json::Type::floater, qc::json::Value(0.0).type());
        Assert::AreEqual(qc::json::Type::floater, qc::json::Value(0.0f).type());
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

    TEST_METHOD(ValueTyping) {
        { // Object
            qc::json::Value v(qc::json::Object{});
            Assert::AreEqual(qc::json::Type::object, v.type());
            Assert::IsTrue(v.is(qc::json::Type::object));
            v.asObject<false>();
            v.asObject<true>();
        }
        { // Array
            qc::json::Value v(qc::json::Array{});
            Assert::AreEqual(qc::json::Type::array, v.type());
            Assert::IsTrue(v.is(qc::json::Type::array));
            v.asArray<false>();
            v.asArray<true>();
        }
        { // String
            qc::json::Value v("abc"sv);
            Assert::AreEqual(qc::json::Type::string, v.type());
            Assert::IsTrue(v.is(qc::json::Type::string));
            v.asString<false>();
            v.asString<true>();
            v.as<std::string_view, false>();
            v.as<std::string_view, true>();
        }
        { // Integer
            qc::json::Value v(0);
            Assert::AreEqual(qc::json::Type::integer, v.type());
            Assert::IsTrue(v.is(qc::json::Type::integer));
            v.asInteger<false>();
            v.asInteger<true>();
            v.as<int64_t, false>();
            v.as<int64_t, true>();
            v.as<uint64_t, false>();
            v.as<uint64_t, true>();
            v.as<int32_t, false>();
            v.as<int32_t, true>();
            v.as<uint32_t, false>();
            v.as<uint32_t, true>();
            v.as<int16_t, false>();
            v.as<int16_t, true>();
            v.as<uint16_t, false>();
            v.as<uint16_t, true>();
            v.as<int8_t, false>();
            v.as<int8_t, true>();
            v.as<uint8_t, false>();
            v.as<uint8_t, true>();
        }
        { // Floater
            qc::json::Value v(0.0);
            Assert::AreEqual(qc::json::Type::floater, v.type());
            Assert::IsTrue(v.is(qc::json::Type::floater));
            v.asFloater<false>();
            v.asFloater<true>();
            v.as<double, false>();
            v.as<double, true>();
            v.as<float, false>();
            v.as<float, true>();
        }
        { // Boolean
            qc::json::Value v(false);
            Assert::AreEqual(qc::json::Type::boolean, v.type());
            Assert::IsTrue(v.is(qc::json::Type::boolean));
            v.asBoolean<false>();
            v.asBoolean<true>();
            v.as<bool, false>();
            v.as<bool, true>();
        }
        { // Null
            qc::json::Value v;
            Assert::AreEqual(qc::json::Type::null, v.type());
            Assert::IsTrue(v.is(qc::json::Type::null));
            v.as<nullptr_t, false>();
            v.as<nullptr_t, true>();
        }
    }

    TEST_METHOD(ValueAs) {
        // Safe
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asObject<false>(); } );
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asArray<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asString<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<std::string_view, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asInteger<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int64_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint64_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int32_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint32_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int16_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint16_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<int8_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<uint8_t, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asFloater<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<double, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<float, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().asBoolean<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value().as<bool, false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value(0).asNull<false>(); });
        Assert::ExpectException<qc::json::TypeError>([]() { qc::json::Value(0).as<nullptr_t, false>(); });

        // Unsafe
        qc::json::Value().asObject<true>();
        qc::json::Value().asArray<true>();
        qc::json::Value().asString<true>();
        qc::json::Value().as<std::string_view, true>();
        qc::json::Value().asInteger<true>();
        qc::json::Value().as<int64_t, true>();
        qc::json::Value().as<uint64_t, true>();
        qc::json::Value().as<int32_t, true>();
        qc::json::Value().as<uint32_t, true>();
        qc::json::Value().as<int16_t, true>();
        qc::json::Value().as<uint16_t, true>();
        qc::json::Value().as<int8_t, true>();
        qc::json::Value().as<uint8_t, true>();
        qc::json::Value().asFloater<true>();
        qc::json::Value().as<double, true>();
        qc::json::Value().as<float, true>();
        qc::json::Value().asBoolean<true>();
        qc::json::Value().as<bool, true>();
        qc::json::Value(0).asNull<true>();
        qc::json::Value(0).as<nullptr_t, true>();
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
            Assert::AreEqual(nullptr, arr.at(0).asNull());

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
            "Price": 18.0
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
