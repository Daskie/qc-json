#include "CppUnitTest.h"

#include "QJson.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using std::string;
using std::string_view;
using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::Value;
using qjson::Type;
using qjson::encode;
using qjson::decode;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {
            template <> static std::wstring ToString<Type>(const Type & type) { return std::to_wstring(std::underlying_type_t<Type>(type)); }
            template <> static std::wstring ToString<uint16_t>(const uint16_t & v) { return std::to_wstring(v); }
        }
    }
}

struct CustomVal { int x, y; };

template <bool unsafe>
struct qjson_valueTo<CustomVal, unsafe> {
    CustomVal operator()(const Value & val) const {
        const qjson::Array & arr(val.asArray<unsafe>());
        return {arr[0].as<int, unsafe>(), arr[1].as<int, unsafe>()};
    }
};

template <>
struct qjson_valueFrom<CustomVal> {
    Value operator()(const CustomVal & v) const {
        return qjson::Array(v.x, v.y);
    }
};

TEST_CLASS(Json) {

    public:

    TEST_METHOD(DecodeString) {
        { // String view
            Value val(decode(R"("Heh, greetings!")"sv));
            Assert::AreEqual(Type::string, val.type());
            Assert::AreEqual("Heh, greetings!"sv, val.as<string_view>());
        }
        { // String
            Value val(decode(R"("Heh, greetings!")"s));
            Assert::AreEqual(Type::string, val.type());
            Assert::AreEqual("Heh, greetings!"sv, val.as<string_view>());
        }
        { // Const C string
            Value val(decode(R"("Heh, greetings!")"));
            Assert::AreEqual(Type::string, val.type());
            Assert::AreEqual("Heh, greetings!"sv, val.as<string_view>());
        }
        { // Mutable C string
            Value val(decode(const_cast<char *>(R"("Heh, greetings!")")));
            Assert::AreEqual(Type::string, val.type());
            Assert::AreEqual("Heh, greetings!"sv, val.as<string_view>());
        }
    }

    TEST_METHOD(EncodeString) {
        Assert::AreEqual(R"("Heh, greetings!")"s, encode("Heh, greetings!"sv));
    }

    TEST_METHOD(DecodeInteger) {
        Value val(decode(R"(123)"sv));
        Assert::AreEqual(Type::integer, val.type());
        Assert::AreEqual(int64_t(123), val.as<int64_t>());
        Assert::AreEqual(int32_t(123), val.as<int32_t>());
        Assert::AreEqual(int16_t(123), val.as<int16_t>());
        Assert::AreEqual( int8_t(123), val.as< int8_t>());
    }

    TEST_METHOD(EncodeInteger) {
        Assert::AreEqual(R"(123)"s, encode(123LL));
    }

    TEST_METHOD(DecodeHex) {
        Value val(decode(R"(0xABC)"sv));
        Assert::AreEqual(Type::hex, val.type());
        Assert::AreEqual(uint64_t(0xABC), val.as<uint64_t>());
        Assert::AreEqual(uint32_t(0xABC), val.as<uint32_t>());
        Assert::AreEqual(uint16_t(0xABC), val.as<uint16_t>());
        Assert::AreEqual( uint8_t(0xABC), val.as< uint8_t>());
    }

    TEST_METHOD(EncodeHex) {
        Assert::AreEqual(R"(0xABC)"s, encode(0xABCULL));
    }

    TEST_METHOD(DecodeFloater) {
        Value val(decode(R"(1.23)"sv));
        Assert::AreEqual(Type::floater, val.type());
        Assert::AreEqual(1.23, val.as<double>(), 1.0e-6);
        Assert::AreEqual(1.23f, val.as<float>(), 1.0e-6f);
    }

    TEST_METHOD(EncodeFloater) {
        Assert::AreEqual(R"(1.23)"s, encode(1.23));
    }

    TEST_METHOD(DecodeBoolean) {
        Value val(decode(R"(true)"sv));
        Assert::AreEqual(Type::boolean, val.type());
        Assert::AreEqual(true, val.as<bool>());
    }

    TEST_METHOD(EncodeBoolean) {
        Assert::AreEqual(R"(true)"s, encode(true));
    }

    TEST_METHOD(DecodeNull) {
        Value val(decode(R"(null)"sv));
        Assert::AreEqual(Type::null, val.type());
    }

    TEST_METHOD(EncodeNull) {
        Assert::AreEqual(R"(null)"s, encode(nullptr));
    }

    TEST_METHOD(DecodeObject) {
        Value val(decode(R"({})"sv));
        Assert::AreEqual(Type::object, val.type());
        Assert::IsTrue(val.asObject().empty());
    }

    TEST_METHOD(EncodeObject) {
        Assert::AreEqual(R"({})"s, encode(qjson::Object()));
    }

    TEST_METHOD(DecodeArray) {
        Value val(decode(R"([])"sv));
        Assert::AreEqual(Type::array, val.type());
        Assert::IsTrue(val.asArray().empty());
    }

    TEST_METHOD(EncodeArray) {
        Assert::AreEqual(R"([])"s, encode(qjson::Array()));
    }

    TEST_METHOD(DecodeCustom) {
        Value val(decode(R"([ 1, 2 ])"sv));
        CustomVal v(val.as<CustomVal>());
        Assert::AreEqual(1, v.x);
        Assert::AreEqual(2, v.y);
    }

    TEST_METHOD(EncodeCustom) {
        Assert::AreEqual(R"([ 1, 2 ])"s, encode(CustomVal{1, 2}));
    }

    TEST_METHOD(General) {
        string json(
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
            "Ingredients": [ "Salt", "Barnacles" ]
        },
        {
            "Name": "Two Tuna",
            "Price": 14.99,
            "Ingredients": [ "Tuna" ]
        },
        {
            "Name": "18 Leg Bouquet",
            "Price": 18.00,
            "Ingredients": [ "Salt", "Octopus", "Crab" ]
        }
    ]
})"s);
        Assert::AreEqual(json, encode(decode(json)));
    }

};
