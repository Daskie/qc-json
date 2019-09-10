#include "CppUnitTest.h"

#include "QJsonRead.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::JsonError;
using qjson::JsonReadError;
using qjson::JsonTypeError;
using qjson::Value;
using qjson::Type;

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
    template <> static std::wstring ToString<Type>(const Type & type) { return std::to_wstring(std::underlying_type_t<Type>(type)); }
    template <> static std::wstring ToString<uint16_t>(const uint16_t & v) { return std::to_wstring(v); }
}}}

struct CustomVal { int x, y; };

template <>
struct qjson_decode<CustomVal> {
    CustomVal operator()(const Value & val) {
        return {val[0].as<int>(), val[1].as<int>()};
    }
};

TEST_CLASS(Read) {

    public:

    TEST_METHOD(Empty) {
        Value val(qjson::read(R"({})"sv));
        Assert::AreEqual(0U, val.size());
    }

    TEST_METHOD(String) {
        { // Empty string
            Value val(qjson::read(R"({ "v": "" })"sv));
            Assert::AreEqual(""sv, val["v"].as<std::string_view>());
        }
        { // All printable
            Value val(qjson::read(R"({ "v": " !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"sv));
            Assert::AreEqual(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"sv, val["v"].as<std::string_view>());
        }
        { // Escape characters
            Value val(qjson::read(R"({ "v": "\b\f\n\r\t" })"sv));
            Assert::AreEqual("\b\f\n\r\t"sv, val["v"].as<std::string_view>());
        }
        { // Unicode
            Value val(qjson::read(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"sv));
            Assert::AreEqual("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"sv, val["v"].as<std::string_view>());
        }
        { // Non-ascii unicode
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"(\u0080)"); });
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            Value val(qjson::read(R"({ "v": 0 })"sv));
            Assert::AreEqual(0, val["v"].as<int>());
        }
        { // Normal
            Value val(qjson::read(R"({ "v": 123 })"sv));
            Assert::AreEqual(123, val["v"].as<int>());
        }
        { // Min
            Value val(qjson::read(R"({ "v": -9223372036854775808 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::min(), val["v"].as<int64_t>());
        }
        { // Max
            Value val(qjson::read(R"({ "v": 9223372036854775807 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::max(), val["v"].as<int64_t>());
        }
        { // Overflow
            Value val(qjson::read(R"({ "v": 18446744073709551615 })"sv));
            Assert::AreEqual(-1LL, val["v"].as<int64_t>());
        }
    }

    TEST_METHOD(Hex) {
        { // Zero
            Value val(qjson::read(R"({ "v": 0x0 })"sv));
            Assert::AreEqual(0x0U, val["v"].as<unsigned int>());
        }
        { // Uppercase
            Value val(qjson::read(R"({ "v": 0x0123456789ABCDEF })"sv));
            Assert::AreEqual(0x0123456789ABCDEFULL, val["v"].as<uint64_t>());
        }
        { // Lowercase
            Value val(qjson::read(R"({ "v": 0x0123456789abcdef })"sv));
            Assert::AreEqual(0x0123456789ABCDEFULL, val["v"].as<uint64_t>());
        }
        { // Max
            Value val(qjson::read(R"({ "v": 0xFFFFFFFFFFFFFFFF })"sv));
            Assert::AreEqual(0xFFFFFFFFFFFFFFFFULL, val["v"].as<uint64_t>());
        }
    }

    TEST_METHOD(Float) {
        { // Zero
            Value val(qjson::read(R"({ "v": 0.0 })"));
            Assert::AreEqual(0.0, val["v"].as<double>());
        }
        { // Whole
            Value val(qjson::read(R"({ "v": 123.0 })"));
            Assert::AreEqual(123.0, val["v"].as<double>(), 1.0e-6);
        }
        { // Fractional
            Value val(qjson::read(R"({ "v": 123.456 })"));
            Assert::AreEqual(123.456, val["v"].as<double>(), 1.0e-6);
        }
        { // Exponent
            Value val(qjson::read(R"({ "v": 123.456e17 })"));
            Assert::AreEqual(123.456e17, val["v"].as<double>(), 1.0e11);
        }
        { // Positive exponent
            Value val(qjson::read(R"({ "v": 123.456e+17 })"));
            Assert::AreEqual(123.456e17, val["v"].as<double>(), 1.0e11);
        }
        { // Negative
            Value val(qjson::read(R"({ "v": -123.456e-17 })"));
            Assert::AreEqual(-123.456e-17, val["v"].as<double>(), 1.0e-23);
        }
        { // Uppercase exponent without fraction
            Value val(qjson::read(R"({ "v": 123E34 })"));
            Assert::AreEqual(123.0e34, val["v"].as<double>(), 1.0e28);
        }
    }

    TEST_METHOD(Bool) {
        { // True
            Value val(qjson::read(R"({ "v": true })"sv));
            Assert::IsTrue(val["v"].as<bool>());
        }
        { // False
            Value val(qjson::read(R"({ "v": false })"sv));
            Assert::IsFalse(val["v"].as<bool>());
        }
    }

    TEST_METHOD(Null) {
        Value val(qjson::read(R"({ "v": null })"sv));
        Assert::AreEqual(Type::null, val["v"].type());
    }

    TEST_METHOD(As) {
        Value val(qjson::read(R"({ "s": "!", "i": 0, "f": 0.0, "b": true })"));
        Assert::AreEqual("!"sv, val["s"].as<std::string_view>());
        Assert::AreEqual('!', val["s"].as<char>());
        Assert::AreEqual(int64_t(0), val["i"].as<int64_t>());
        Assert::AreEqual(int32_t(0), val["i"].as<int32_t>());
        Assert::AreEqual(int16_t(0), val["i"].as<int16_t>());
        Assert::AreEqual(int8_t(0), val["i"].as<int8_t>());
        Assert::AreEqual(uint64_t(0), val["i"].as<uint64_t>());
        Assert::AreEqual(uint32_t(0), val["i"].as<uint32_t>());
        Assert::AreEqual(uint16_t(0), val["i"].as<uint16_t>());
        Assert::AreEqual(uint8_t(0), val["i"].as<uint8_t>());
        Assert::AreEqual(0.0, val["f"].as<double>());
        Assert::AreEqual(0.0f, val["f"].as<float>());
        Assert::AreEqual(true, val["b"].as<bool>());
    }

    TEST_METHOD(Custom) {
        Value val(qjson::read(R"({ "v": [ 1, 2 ] })"sv));
        CustomVal customVal(val["v"].as<CustomVal>());
        Assert::AreEqual(1, customVal.x);
        Assert::AreEqual(2, customVal.y);
    }

    TEST_METHOD(General) {
        Value val(qjson::read(
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
})"sv));

        Assert::AreEqual(4U, val.size());
        Assert::AreEqual("Salt's Crust"sv, val["Name"].as<std::string_view>());
        Assert::AreEqual(1964, val["Founded"].as<int>());
        
        const Value & employees(val["Employees"]);
        Assert::AreEqual(3U, employees.size());
        {
            const Value & employee(employees[0]);
            Assert::AreEqual(3U, employee.size());
            Assert::AreEqual("Ol' Joe Fisher"sv, employee["Name"].as<std::string_view>());
            Assert::AreEqual("Fisherman"sv, employee["Title"].as<std::string_view>());
            Assert::AreEqual(69, employee["Age"].as<int>());
        }
        {
            const Value & employee(employees[1]);
            Assert::AreEqual(3U, employee.size());
            Assert::AreEqual("Mark Rower"sv, employee["Name"].as<std::string_view>());
            Assert::AreEqual("Cook"sv, employee["Title"].as<std::string_view>());
            Assert::AreEqual(41, employee["Age"].as<int>());
        }
        {
            const Value & employee(employees[2]);
            Assert::AreEqual(3U, employee.size());
            Assert::AreEqual("Phineas"sv, employee["Name"].as<std::string_view>());
            Assert::AreEqual("Server Boy"sv, employee["Title"].as<std::string_view>());
            Assert::AreEqual(19, employee["Age"].as<int>());
        }

        const Value & dishes(val["Dishes"]);
        Assert::AreEqual(3U, dishes.size());
        {
            const Value & dish(dishes[0]);
            Assert::AreEqual(3U, dish.size());
            Assert::AreEqual("Basket o' Barnacles"sv, dish["Name"].as<std::string_view>());
            Assert::AreEqual(5.45, dish["Price"].as<double>(), 0.001);
            const Value & ingredients(dish["Ingredients"]);
            Assert::AreEqual(2U, ingredients.size());
            Assert::AreEqual("Salt"sv, ingredients[0].as<std::string_view>());
            Assert::AreEqual("Barnacles"sv, ingredients[1].as<std::string_view>());
        }
        {
            const Value & dish(dishes[1]);
            Assert::AreEqual(3U, dish.size());
            Assert::AreEqual("Two Tuna"sv, dish["Name"].as<std::string_view>());
            Assert::AreEqual(14.99, dish["Price"].as<double>(), 0.001);
            const Value & ingredients(dish["Ingredients"]);
            Assert::AreEqual(1U, ingredients.size());
            Assert::AreEqual("Tuna"sv, ingredients[0].as<std::string_view>());
        }
        {
            const Value & dish(dishes[2]);
            Assert::AreEqual(3U, dish.size());
            Assert::AreEqual("18 Leg Bouquet"sv, dish["Name"].as<std::string_view>());
            Assert::AreEqual(18.0, dish["Price"].as<double>(), 0.001);
            const Value & ingredients(dish["Ingredients"]);
            Assert::AreEqual(3U, ingredients.size());
            Assert::AreEqual("Salt"sv, ingredients[0].as<std::string_view>());
            Assert::AreEqual("Octopus"sv, ingredients[1].as<std::string_view>());
            Assert::AreEqual("Crab"sv, ingredients[2].as<std::string_view>());
        }
    }

    TEST_METHOD(NoWhitespace) {
        Value val(qjson::read(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv));
        Assert::AreEqual(1U, val.size());
        const Value & arr(val["a"]);
        Assert::AreEqual(5U, arr.size());
        Assert::AreEqual("abc"sv, arr[0].as<std::string_view>());
        Assert::AreEqual(-123, arr[1].as<int>());
        Assert::AreEqual(-123.456e-78, arr[2].as<double>(), 1.0e-82);
        Assert::IsTrue(arr[3].as<bool>());
        Assert::AreEqual(Type::null, arr[4].type());
    }

    TEST_METHOD(Types) {
        Value val(qjson::read(R"({ "o": {}, "a": [], "s": "", "i": 0, "h": 0x0, "f": 0.0, "b": true, "n": null })"));
        const Value & oval(val["o"]);
        const Value & aval(val["a"]);
        const Value & sval(val["s"]);
        const Value & ival(val["i"]);
        const Value & hval(val["h"]);
        const Value & fval(val["f"]);
        const Value & bval(val["b"]);
        const Value & nval(val["n"]);

        Assert::AreEqual(Type::  object, oval.type());
        Assert::AreEqual(Type::   array, aval.type());
        Assert::AreEqual(Type::  string, sval.type());
        Assert::AreEqual(Type:: integer, ival.type());
        Assert::AreEqual(Type:: integer, hval.type());
        Assert::AreEqual(Type::floating, fval.type());
        Assert::AreEqual(Type:: boolean, bval.type());
        Assert::AreEqual(Type::    null, nval.type());

        //Assert::ExpectException<JsonTypeError>([&]() { oval.asArray();    });
        //Assert::ExpectException<JsonTypeError>([&]() { oval.asString();   });
        //Assert::ExpectException<JsonTypeError>([&]() { oval.asInteger();  });
        //Assert::ExpectException<JsonTypeError>([&]() { oval.asFloating(); });
        //Assert::ExpectException<JsonTypeError>([&]() { oval.asBoolean();  });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { aval.asObject();   });
        //Assert::ExpectException<JsonTypeError>([&]() { aval.asString();   });
        //Assert::ExpectException<JsonTypeError>([&]() { aval.asInteger();  });
        //Assert::ExpectException<JsonTypeError>([&]() { aval.asFloating(); });
        //Assert::ExpectException<JsonTypeError>([&]() { aval.asBoolean();  });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { sval.asObject();   });
        //Assert::ExpectException<JsonTypeError>([&]() { sval.asArray();    });
        //Assert::ExpectException<JsonTypeError>([&]() { sval.asInteger();  });
        //Assert::ExpectException<JsonTypeError>([&]() { sval.asFloating(); });
        //Assert::ExpectException<JsonTypeError>([&]() { sval.asBoolean();  });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { ival.asObject();   });
        //Assert::ExpectException<JsonTypeError>([&]() { ival.asArray();    });
        //Assert::ExpectException<JsonTypeError>([&]() { ival.asString();   });
        //Assert::ExpectException<JsonTypeError>([&]() { ival.asFloating(); });
        //Assert::ExpectException<JsonTypeError>([&]() { ival.asBoolean();  });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { fval.asObject();  });
        //Assert::ExpectException<JsonTypeError>([&]() { fval.asArray();   });
        //Assert::ExpectException<JsonTypeError>([&]() { fval.asString();  });
        //Assert::ExpectException<JsonTypeError>([&]() { fval.asInteger(); });
        //Assert::ExpectException<JsonTypeError>([&]() { fval.asBoolean(); });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { bval.asObject();   });
        //Assert::ExpectException<JsonTypeError>([&]() { bval.asArray();    });
        //Assert::ExpectException<JsonTypeError>([&]() { bval.asString();   });
        //Assert::ExpectException<JsonTypeError>([&]() { bval.asInteger();  });
        //Assert::ExpectException<JsonTypeError>([&]() { bval.asFloating(); });
        //
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asObject();   });
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asArray();    });
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asString();   });
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asInteger();  });
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asFloating(); });
        //Assert::ExpectException<JsonTypeError>([&]() { nval.asBoolean();  });
    }

    TEST_METHOD(Exceptions) {
        { // Content outside root object
            qjson::read(R"(   {}   )"sv);
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({}, "hi")"); });
        }
        { // Key not within quotes
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ a: 0 })"); });
        }
        { // No colon after key
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a" 0 })"); });
        }
        { // No comma between object elements
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0 "b": 1 })"); });
        }
        { // Comma after last object element
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0, })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0, "b": 1, })"); });
        }
        { // No comma between array elements
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": [ 0 1 ] })"); });
        }
        { // Comma after last array element
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": [ 0, ] })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": [ 0, 1, ] })"); });
        }
        { // Missing value
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": })"); });
        }
        { // Unknown value
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": v })"); });
        }
        { // Missing escape sequence
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\" })"); });
        }
        { // Unknown escape sequence
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\v" })"); });
        }
        { // Missing unicode
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\u" })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\u0" })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\u00" })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": "\u000" })"); });
        }
        { // Unknown string content
            Assert::ExpectException<JsonReadError>([]() { qjson::read("{ \"a\": \"\n\" }"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read("{ \"a\": \"\t\" }"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read("{ \"a\": \"\0\" }"); });
        }
        { // Missing hex
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0x })"); });
        }
        { // Invalid hex digit
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0xG })"); });
        }
        { // Hex too long
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0x00000000000000000 })"); });
        }
        { // Plus sign
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": +0 })"); });
        }
        { // Missing number after minus
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": - })"); });
        }
        { // Invalid digit
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": -A })"); });
        }
        { // Integer value too large
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 18446744073709551616 })"); });
        }
        { // Too many integer digits
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 000000000000000000000 })"); });
        }
        { // Missing fractional component
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0. })"); });
        }
        { // Fractional component too large
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0.18446744073709551616 })"); });
        }
        { // Missing exponent
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e+ })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e- })"); });
        }
        { // Exponent too large
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e1001 })"); });
        }
    }

};
