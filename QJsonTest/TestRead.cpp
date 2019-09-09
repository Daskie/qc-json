#include "CppUnitTest.h"

#include "QJsonRead.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::JsonError;
using qjson::JsonReadError;
using qjson::JsonTypeError;
using qjson::Value;
using qjson::Object;
using qjson::Array;
using qjson::Type;

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
    template <> static std::wstring ToString<Type>(const Type & type) { return std::to_wstring(std::underlying_type_t<Type>(type)); }
}}}

TEST_CLASS(Read) {

    public:

    TEST_METHOD(Empty) {
        Object val(qjson::read(R"({})"sv));
        Assert::IsTrue(val.empty());
    }

    TEST_METHOD(String) {
        { // Empty string
            Object val(qjson::read(R"({ "v": "" })"sv));
            Assert::AreEqual(""s, val.at("v")->asString());
        }
        { // All printable
            Object val(qjson::read(R"({ "v": " !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"sv));
            Assert::AreEqual(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"s, val.at("v")->asString());
        }
        { // Escape characters
            Object val(qjson::read(R"({ "v": "\b\f\n\r\t" })"sv));
            Assert::AreEqual("\b\f\n\r\t"s, val.at("v")->asString());
        }
        { // Unicode
            Object val(qjson::read(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"sv));
            Assert::AreEqual("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"s, val.at("v")->asString());
        }
        { // Non-ascii unicode
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"(\u0080)"); });
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            Object val(qjson::read(R"({ "v": 0 })"sv));
            Assert::AreEqual(0LL, val.at("v")->asInteger());
        }
        { // Normal
            Object val(qjson::read(R"({ "v": 123 })"sv));
            Assert::AreEqual(123LL, val.at("v")->asInteger());
        }
        { // Min
            Object val(qjson::read(R"({ "v": -9223372036854775808 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::min(), val.at("v")->asInteger());
        }
        { // Max
            Object val(qjson::read(R"({ "v": 9223372036854775807 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::max(), val.at("v")->asInteger());
        }
        { // Overflow
            Object val(qjson::read(R"({ "v": 18446744073709551615 })"sv));
            Assert::AreEqual(-1LL, val.at("v")->asInteger());
        }
    }

    TEST_METHOD(Hex) {
        { // Zero
            Object val(qjson::read(R"({ "v": 0x0 })"sv));
            Assert::AreEqual(0x0ULL, uint64_t(val.at("v")->asInteger()));
        }
        { // Uppercase
            Object val(qjson::read(R"({ "v": 0x0123456789ABCDEF })"sv));
            Assert::AreEqual(0x0123456789ABCDEFULL, uint64_t(val.at("v")->asInteger()));
        }
        { // Lowercase
            Object val(qjson::read(R"({ "v": 0x0123456789abcdef })"sv));
            Assert::AreEqual(0x0123456789ABCDEFULL, uint64_t(val.at("v")->asInteger()));
        }
        { // Max
            Object val(qjson::read(R"({ "v": 0xFFFFFFFFFFFFFFFF })"sv));
            Assert::AreEqual(0xFFFFFFFFFFFFFFFFULL, uint64_t(val.at("v")->asInteger()));
        }
    }

    TEST_METHOD(Float) {
        { // Zero
            Object val(qjson::read(R"({ "v": 0.0 })"));
            Assert::AreEqual(0.0, val.at("v")->asFloating());
        }
        { // Whole
            Object val(qjson::read(R"({ "v": 123.0 })"));
            Assert::AreEqual(123.0, val.at("v")->asFloating(), 1.0e-6);
        }
        { // Fractional
            Object val(qjson::read(R"({ "v": 123.456 })"));
            Assert::AreEqual(123.456, val.at("v")->asFloating(), 1.0e-6);
        }
        { // Exponent
            Object val(qjson::read(R"({ "v": 123.456e17 })"));
            Assert::AreEqual(123.456e17, val.at("v")->asFloating(), 1.0e11);
        }
        { // Positive exponent
            Object val(qjson::read(R"({ "v": 123.456e+17 })"));
            Assert::AreEqual(123.456e17, val.at("v")->asFloating(), 1.0e11);
        }
        { // Negative
            Object val(qjson::read(R"({ "v": -123.456e-17 })"));
            Assert::AreEqual(-123.456e-17, val.at("v")->asFloating(), 1.0e-23);
        }
        { // Uppercase exponent without fraction
            Object val(qjson::read(R"({ "v": 123E34 })"));
            Assert::AreEqual(123.0e34, val.at("v")->asFloating(), 1.0e28);
        }
    }

    TEST_METHOD(Bool) {
        { // True
            Object val(qjson::read(R"({ "v": true })"sv));
            Assert::IsTrue(val.at("v")->asBoolean());
        }
        { // False
            Object val(qjson::read(R"({ "v": false })"sv));
            Assert::IsFalse(val.at("v")->asBoolean());
        }
    }

    TEST_METHOD(Null) {
        Object val(qjson::read(R"({ "v": null })"sv));
        Assert::AreEqual(Type::null, val.at("v")->type());
    }

    TEST_METHOD(General) {
        Object val(qjson::read(
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

        Assert::AreEqual(size_t(4), val.size());
        Assert::AreEqual("Salt's Crust"s, val.at("Name")->asString());
        Assert::AreEqual(1964LL, val.at("Founded")->asInteger());
        
        const Array & employees(val.at("Employees")->asArray());
        Assert::AreEqual(size_t(3), employees.size());
        {
            const Object & employee(employees.at(0)->asObject());
            Assert::AreEqual(size_t(3), employee.size());
            Assert::AreEqual("Ol' Joe Fisher"s, employee.at("Name")->asString());
            Assert::AreEqual("Fisherman"s, employee.at("Title")->asString());
            Assert::AreEqual(69LL, employee.at("Age")->asInteger());
        }
        {
            const Object & employee(employees.at(1)->asObject());
            Assert::AreEqual(size_t(3), employee.size());
            Assert::AreEqual("Mark Rower"s, employee.at("Name")->asString());
            Assert::AreEqual("Cook"s, employee.at("Title")->asString());
            Assert::AreEqual(41LL, employee.at("Age")->asInteger());
        }
        {
            const Object & employee(employees.at(2)->asObject());
            Assert::AreEqual(size_t(3), employee.size());
            Assert::AreEqual("Phineas"s, employee.at("Name")->asString());
            Assert::AreEqual("Server Boy"s, employee.at("Title")->asString());
            Assert::AreEqual(19LL, employee.at("Age")->asInteger());
        }

        const Array & dishes(val.at("Dishes")->asArray());
        Assert::AreEqual(size_t(3), dishes.size());
        {
            const Object & dish(dishes.at(0)->asObject());
            Assert::AreEqual(size_t(3), dish.size());
            Assert::AreEqual("Basket o' Barnacles"s, dish.at("Name")->asString());
            Assert::AreEqual(5.45, dish.at("Price")->asFloating(), 0.001);
            const Array & ingredients(dish.at("Ingredients")->asArray());
            Assert::AreEqual(size_t(2), ingredients.size());
            Assert::AreEqual("Salt"s, ingredients.at(0)->asString());
            Assert::AreEqual("Barnacles"s, ingredients.at(1)->asString());
        }
        {
            const Object & dish(dishes.at(1)->asObject());
            Assert::AreEqual(size_t(3), dish.size());
            Assert::AreEqual("Two Tuna"s, dish.at("Name")->asString());
            Assert::AreEqual(14.99, dish.at("Price")->asFloating(), 0.001);
            const Array & ingredients(dish.at("Ingredients")->asArray());
            Assert::AreEqual(size_t(1), ingredients.size());
            Assert::AreEqual("Tuna"s, ingredients.at(0)->asString());
        }
        {
            const Object & dish(dishes.at(2)->asObject());
            Assert::AreEqual(size_t(3), dish.size());
            Assert::AreEqual("18 Leg Bouquet"s, dish.at("Name")->asString());
            Assert::AreEqual(18.0, dish.at("Price")->asFloating(), 0.001);
            const Array & ingredients(dish.at("Ingredients")->asArray());
            Assert::AreEqual(size_t(3), ingredients.size());
            Assert::AreEqual("Salt"s, ingredients.at(0)->asString());
            Assert::AreEqual("Octopus"s, ingredients.at(1)->asString());
            Assert::AreEqual("Crab"s, ingredients.at(2)->asString());
        }
    }

    TEST_METHOD(NoWhitespace) {
        Object val(qjson::read(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv));
        Assert::AreEqual(size_t(1), val.size());
        const Array & arr(val.at("a")->asArray());
        Assert::AreEqual(size_t(5), arr.size());
        Assert::AreEqual("abc"s, arr.at(0)->asString());
        Assert::AreEqual(-123LL, arr.at(1)->asInteger());
        Assert::AreEqual(-123.456e-78, arr.at(2)->asFloating(), 1.0e-82);
        Assert::IsTrue(arr.at(3)->asBoolean());
        Assert::AreEqual(Type::null, arr.at(4)->type());
    }

    TEST_METHOD(Types) {
        Object val(qjson::read(R"({ "o": {}, "a": [], "s": "", "i": 0, "h": 0x0, "f": 0.0, "b": true, "n": null })"));
        const auto & oval(val.at("o"));
        const auto & aval(val.at("a"));
        const auto & sval(val.at("s"));
        const auto & ival(val.at("i"));
        const auto & hval(val.at("h"));
        const auto & fval(val.at("f"));
        const auto & bval(val.at("b"));
        const auto & nval(val.at("n"));

        Assert::AreEqual(Type::  object, oval->type());
        Assert::AreEqual(Type::   array, aval->type());
        Assert::AreEqual(Type::  string, sval->type());
        Assert::AreEqual(Type:: integer, ival->type());
        Assert::AreEqual(Type:: integer, hval->type());
        Assert::AreEqual(Type::floating, fval->type());
        Assert::AreEqual(Type:: boolean, bval->type());
        Assert::AreEqual(Type::    null, nval->type());

        Assert::ExpectException<JsonTypeError>([&]() { oval->asArray();    });
        Assert::ExpectException<JsonTypeError>([&]() { oval->asString();   });
        Assert::ExpectException<JsonTypeError>([&]() { oval->asInteger();  });
        Assert::ExpectException<JsonTypeError>([&]() { oval->asFloating(); });
        Assert::ExpectException<JsonTypeError>([&]() { oval->asBoolean();  });

        Assert::ExpectException<JsonTypeError>([&]() { aval->asObject();   });
        Assert::ExpectException<JsonTypeError>([&]() { aval->asString();   });
        Assert::ExpectException<JsonTypeError>([&]() { aval->asInteger();  });
        Assert::ExpectException<JsonTypeError>([&]() { aval->asFloating(); });
        Assert::ExpectException<JsonTypeError>([&]() { aval->asBoolean();  });

        Assert::ExpectException<JsonTypeError>([&]() { sval->asObject();   });
        Assert::ExpectException<JsonTypeError>([&]() { sval->asArray();    });
        Assert::ExpectException<JsonTypeError>([&]() { sval->asInteger();  });
        Assert::ExpectException<JsonTypeError>([&]() { sval->asFloating(); });
        Assert::ExpectException<JsonTypeError>([&]() { sval->asBoolean();  });

        Assert::ExpectException<JsonTypeError>([&]() { ival->asObject();   });
        Assert::ExpectException<JsonTypeError>([&]() { ival->asArray();    });
        Assert::ExpectException<JsonTypeError>([&]() { ival->asString();   });
        Assert::ExpectException<JsonTypeError>([&]() { ival->asFloating(); });
        Assert::ExpectException<JsonTypeError>([&]() { ival->asBoolean();  });

        Assert::ExpectException<JsonTypeError>([&]() { fval->asObject();  });
        Assert::ExpectException<JsonTypeError>([&]() { fval->asArray();   });
        Assert::ExpectException<JsonTypeError>([&]() { fval->asString();  });
        Assert::ExpectException<JsonTypeError>([&]() { fval->asInteger(); });
        Assert::ExpectException<JsonTypeError>([&]() { fval->asBoolean(); });

        Assert::ExpectException<JsonTypeError>([&]() { bval->asObject();   });
        Assert::ExpectException<JsonTypeError>([&]() { bval->asArray();    });
        Assert::ExpectException<JsonTypeError>([&]() { bval->asString();   });
        Assert::ExpectException<JsonTypeError>([&]() { bval->asInteger();  });
        Assert::ExpectException<JsonTypeError>([&]() { bval->asFloating(); });

        Assert::ExpectException<JsonTypeError>([&]() { nval->asObject();   });
        Assert::ExpectException<JsonTypeError>([&]() { nval->asArray();    });
        Assert::ExpectException<JsonTypeError>([&]() { nval->asString();   });
        Assert::ExpectException<JsonTypeError>([&]() { nval->asInteger();  });
        Assert::ExpectException<JsonTypeError>([&]() { nval->asFloating(); });
        Assert::ExpectException<JsonTypeError>([&]() { nval->asBoolean();  });
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
