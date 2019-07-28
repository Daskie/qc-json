#include "CppUnitTest.h"

#include "QJsonRead.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std::string_literals;
using namespace std::string_view_literals;

using qjson::JsonReadError;
using qjson::Value;
using qjson::Object;
using qjson::Array;

TEST_CLASS(Read) {

    public:

    TEST_METHOD(Empty) {
        auto val(qjson::read(R"({})"sv));
        Assert::IsTrue(val->asObject()->empty());
    }

    TEST_METHOD(String) {
        { // Empty string
            auto val(qjson::read(R"({ "v": "" })"sv));
            Assert::AreEqual(""s, *val->asObject()->at("v")->asString());
        }
        { // All printable
            auto val(qjson::read(R"({ "v": " !\"#$%&'()*+,-./\/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" })"sv));
            Assert::AreEqual(R"( !"#$%&'()*+,-.//0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)"s, *val->asObject()->at("v")->asString());
        }
        { // Escape characters
            auto val(qjson::read(R"({ "v": "\b\f\n\r\t" })"sv));
            Assert::AreEqual("\b\f\n\r\t"s, *val->asObject()->at("v")->asString());
        }
        { // Unicode
            auto val(qjson::read(R"({ "v": "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F" })"sv));
            Assert::AreEqual("\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u000B\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F\u007F"s, *val->asObject()->at("v")->asString());
        }
        { // Non-ascii unicode
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"(\u0080)"); });
        }
    }

    TEST_METHOD(Integer) {
        { // Zero
            auto val(qjson::read(R"({ "v": 0 })"sv));
            Assert::AreEqual(0ll, *val->asObject()->at("v")->asInt());
        }
        { // Normal
            auto val(qjson::read(R"({ "v": 123 })"sv));
            Assert::AreEqual(123ll, *val->asObject()->at("v")->asInt());
        }
        { // Min
            auto val(qjson::read(R"({ "v": -9223372036854775808 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::min(), *val->asObject()->at("v")->asInt());
        }
        { // Max
            auto val(qjson::read(R"({ "v": 9223372036854775807 })"sv));
            Assert::AreEqual(std::numeric_limits<int64_t>::max(), *val->asObject()->at("v")->asInt());
        }
    }

    TEST_METHOD(Hex) {
        { // Zero
            auto val(qjson::read(R"({ "v": 0x0 })"sv));
            Assert::AreEqual(0x0ull, *val->asObject()->at("v")->asHex());
        }
        { // Uppercase
            auto val(qjson::read(R"({ "v": 0x0123456789ABCDEF })"sv));
            Assert::AreEqual(0x0123456789ABCDEFull, *val->asObject()->at("v")->asHex());
        }
        { // Lowercase
            auto val(qjson::read(R"({ "v": 0x0123456789abcdef })"sv));
            Assert::AreEqual(0x0123456789ABCDEFull, *val->asObject()->at("v")->asHex());
        }
        { // Max
            auto val(qjson::read(R"({ "v": 0xFFFFFFFFFFFFFFFF })"sv));
            Assert::AreEqual(0xFFFFFFFFFFFFFFFFull, *val->asObject()->at("v")->asHex());
        }
    }

    TEST_METHOD(Float) {
        { // Zero
            auto val(qjson::read(R"({ "v": 0.0 })"));
            Assert::AreEqual(0.0, *val->asObject()->at("v")->asFloat());
        }
        { // Whole
            auto val(qjson::read(R"({ "v": 123.0 })"));
            Assert::AreEqual(123.0, *val->asObject()->at("v")->asFloat(), 1.0e-6);
        }
        { // Fractional
            auto val(qjson::read(R"({ "v": 123.456 })"));
            Assert::AreEqual(123.456, *val->asObject()->at("v")->asFloat(), 1.0e-6);
        }
        { // Exponent
            auto val(qjson::read(R"({ "v": 123.456e17 })"));
            Assert::AreEqual(123.456e17, *val->asObject()->at("v")->asFloat(), 1.0e11);
        }
        { // Positive exponent
            auto val(qjson::read(R"({ "v": 123.456e+17 })"));
            Assert::AreEqual(123.456e17, *val->asObject()->at("v")->asFloat(), 1.0e11);
        }
        { // Negative
            auto val(qjson::read(R"({ "v": -123.456e-17 })"));
            Assert::AreEqual(-123.456e-17, *val->asObject()->at("v")->asFloat(), 1.0e-23);
        }
        { // Uppercase exponent without fraction
            auto val(qjson::read(R"({ "v": 123E34 })"));
            Assert::AreEqual(123.0e34, *val->asObject()->at("v")->asFloat(), 1.0e28);
        }
    }

    TEST_METHOD(Bool) {
        { // True
            auto val(qjson::read(R"({ "v": true })"sv));
            Assert::IsTrue(*val->asObject()->at("v")->asBool());
        }
        { // False
            auto val(qjson::read(R"({ "v": false })"sv));
            Assert::IsFalse(*val->asObject()->at("v")->asBool());
        }
    }

    TEST_METHOD(Null) {
        auto val(qjson::read(R"({ "v": null })"sv));
        Assert::IsFalse(val->asObject()->at("v").get());
    }

    TEST_METHOD(General) {
        auto val(qjson::read(
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

        const Object * root(val->asObject());
        Assert::AreEqual(4ull, root->size());
        Assert::AreEqual("Salt's Crust"s, *root->at("Name")->asString());
        Assert::AreEqual(1964ll, *root->at("Founded")->asInt());
        
        const Array * employees(root->at("Employees")->asArray());
        Assert::AreEqual(3ull, employees->size());
        {
            const Object * employee(employees->at(0)->asObject());
            Assert::AreEqual(3ull, employee->size());
            Assert::AreEqual("Ol' Joe Fisher"s, *employee->at("Name")->asString());
            Assert::AreEqual("Fisherman"s, *employee->at("Title")->asString());
            Assert::AreEqual(69ll, *employee->at("Age")->asInt());
        }
        {
            const Object * employee(employees->at(1)->asObject());
            Assert::AreEqual(3ull, employee->size());
            Assert::AreEqual("Mark Rower"s, *employee->at("Name")->asString());
            Assert::AreEqual("Cook"s, *employee->at("Title")->asString());
            Assert::AreEqual(41ll, *employee->at("Age")->asInt());
        }
        {
            const Object * employee(employees->at(2)->asObject());
            Assert::AreEqual(3ull, employee->size());
            Assert::AreEqual("Phineas"s, *employee->at("Name")->asString());
            Assert::AreEqual("Server Boy"s, *employee->at("Title")->asString());
            Assert::AreEqual(19ll, *employee->at("Age")->asInt());
        }

        const Array * dishes(root->at("Dishes")->asArray());
        Assert::AreEqual(3ull, dishes->size());
        {
            const Object * dish(dishes->at(0)->asObject());
            Assert::AreEqual(3ull, dish->size());
            Assert::AreEqual("Basket o' Barnacles"s, *dish->at("Name")->asString());
            Assert::AreEqual(5.45, *dish->at("Price")->asFloat(), 0.001);
            const Array * ingredients(dish->at("Ingredients")->asArray());
            Assert::AreEqual(2ull, ingredients->size());
            Assert::AreEqual("Salt"s, *ingredients->at(0)->asString());
            Assert::AreEqual("Barnacles"s, *ingredients->at(1)->asString());
        }
        {
            const Object * dish(dishes->at(1)->asObject());
            Assert::AreEqual(3ull, dish->size());
            Assert::AreEqual("Two Tuna"s, *dish->at("Name")->asString());
            Assert::AreEqual(14.99, *dish->at("Price")->asFloat(), 0.001);
            const Array * ingredients(dish->at("Ingredients")->asArray());
            Assert::AreEqual(1ull, ingredients->size());
            Assert::AreEqual("Tuna"s, *ingredients->at(0)->asString());
        }
        {
            const Object * dish(dishes->at(2)->asObject());
            Assert::AreEqual(3ull, dish->size());
            Assert::AreEqual("18 Leg Bouquet"s, *dish->at("Name")->asString());
            Assert::AreEqual(18.0, *dish->at("Price")->asFloat(), 0.001);
            const Array * ingredients(dish->at("Ingredients")->asArray());
            Assert::AreEqual(3ull, ingredients->size());
            Assert::AreEqual("Salt"s, *ingredients->at(0)->asString());
            Assert::AreEqual("Octopus"s, *ingredients->at(1)->asString());
            Assert::AreEqual("Crab"s, *ingredients->at(2)->asString());
        }
    }

    TEST_METHOD(NoWhitespace) {
        auto val(qjson::read(R"({"a":["abc",-123,-123.456e-78,true,null]})"sv));
        const Object * obj(val->asObject());
        Assert::AreEqual(1ull, obj->size());
        const Array * arr(obj->at("a")->asArray());
        Assert::AreEqual(5ull, arr->size());
        Assert::AreEqual("abc"s, *arr->at(0)->asString());
        Assert::AreEqual(-123ll, *arr->at(1)->asInt());
        Assert::AreEqual(-123.456e-78, *arr->at(2)->asFloat(), 1.0e-82);
        Assert::IsTrue(*arr->at(3)->asBool());
        Assert::IsFalse(arr->at(4).get());
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
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 9999999999999999999 })"); });
        }
        { // Too many integer digits
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 00000000000000000000 })"); });
        }
        { // Missing fractional component
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0. })"); });
        }
        { // Missing exponent
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e+ })"); });
            Assert::ExpectException<JsonReadError>([]() { qjson::read(R"({ "a": 0e- })"); });
        }
    }

};