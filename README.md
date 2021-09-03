# QC JSON
###### Clean, quick, and simple JSON library for C++20

### Some JSON

```json
{
    "Name": "18 Leg Bouquet",
    "Price": 17.99,
    "Ingredients": [ "Crab", "Octopus", "Breadcrumbs" ],
    "Sold": 68
}
```

Let's say it's in a string, `jsonStr`

### Decode some JSON

```c++
qc::json::Value jsonVal{qc::json::decode(jsonStr)};
```

Let's get a reference to our top level object for easy access

```c++
qc::json::Object & obj{jsonVal.asObject()};
```

Print the name

```c++
std::cout << "Name: " << obj.at("Name").asString() << std::endl;
```

> Name: 18 Leg Bouquet

Print the price

```c++
std::cout << "Price: " << obj.at("Price").asFloater() << std::endl;
```

> Price: 17.99

List the ingredients

```c++
std::cout << "Ingredients:";
for (qc::json::Value & val : obj.at("Ingredients").asArray()) {
    std::cout << " " << val.asString();
}
std::cout << std::endl;
```

> Ingredients: Crab Octopus Breadcrumbs

Print the quantity sold

```c++
std::cout << "Sold: " << obj.at("Sold").asInteger() << std::endl;
```

> Sold: 68

### Modify some JSON

Increment the quantity sold

```c++
++obj.at("Sold").asInteger();
```

Remove breadcrumbs from the ingredients list

```c++
obj.at("Ingredients").asArray().remove(2); // Here, 2 is the array index
```

Add a new "Gluten Free" field

```c++
obj.add("Gluten Free", false);
```

### Encode some JSON

```c++
std::string newJsonStr{qc::json::encode(jsonVal)};
```

`newJsonStr` will contain:

```json
{
    "Gluten Free": true,
    "Ingredients": [
        "Crab",
        "Octopus"
    ],
    "Name": "18 Leg Bouquet",
    "Price": 17.99,
    "Sold": 69
}
```

### Alternatively, encode some JSON directly

```c++
using namespace qc::json::tokens; // Provides shorthand for `object`, `array`, `end`, and density control

qc::json::Encoder encoder{};
encoder << object;
    encoder << "Name" << "18 Leg Bouquet";
    encoder << "Price" << 17.99;
    encoder << "Ingredients" << array << uniline << "Crab" << "Octopus" << end;
    encoder << "Gluten Free" << true;
    encoder << "Sold" << 69;
encoder << end;
std::string jsonStr{encoder.finish()};
```

`jsonStr` will contain:

```json
{
    "Name": "18 Leg Bouquet",
    "Price": 17.99,
    "Ingredients": [ "Crab", "Octopus" ],
    "Gluten Free": true,
    "Sold": 69
}
```

---

## Value Types

### Object

TODO

The key can be either a string or an identifier, where an identifier is an unquoted string of alphanumeric characters
and underscores.

Example:

```json5
{
     abc  : "letters",    
     123  : "numbers",    
     ___  : "underscores",    
    "! -" : "any string value"
}
```

The last element in the object may have a trailing comma.

### Array

The last element in the array may have a trailing comma.

TODO

### String

TODO

A string may only contain [printable](https://en.cppreference.com/w/cpp/string/byte/isprint) characters, with the exception of escaped newlines.

A string may be surrounded in either single or double quotes. Any of the same quotes within a string must be escaped
with a backslash.

Example:

```json5
[
    "This is valid", 'and so is this', "but not this mismatch',
    'And don\'t forget to', "escape your \"quotes\""
]
```

#### Escape Sequences

The following escape sequences are replaced with their corresponding code point:

| Sequence | Name | Code Point |
|:---:|:---:|:---:|
| `\0` | Null | `U+0000` |
| `\b` | Backspace | `U+0008` |
| `\t` | Horizontal Tab | `U+0009` |
| `\n` | New Line | `U+000A` |
| `\v` | Vertical Tab | `U+000B` |
| `\f` | Form Feed | `U+000C` |
| `\r` | Carriage Return | `U+000D` |

In addition, a specific code point can be specified with `\x` followed by two hex digits, or `\u` followed by four hex digits.
Currently, only extended ASCII is supported, i.e.`U+0000` through `U+00FF`. 

Examples:
- `\x5A` yields `U+005A` or `Z`
- `\u007E` yields `U+007E` or `~`

Finally, newlines can be escaped; either `\n` or `\r\n`. This allows long string to be split accross multiple lines
without introducing newlines into the data.

Example:
```json5
"This is a longer string. \
Yep. \
Sure is that."
```
...would decode into `This is a longer string. Yep. Sure is that.`.

In all other circumstances, a backslash followed by character simply yeilds that character.

Examples:
- `\A` yields `A`
- `\\` yields `\ `

### Number

TODO

Numbers may have leading or trailing decimal points in the coefficient.

Valid: `1.` `.1` `1.e2` `.1e2`

Invalid: `.` `.e2` `1e.2` `1e2.`

Numbers may also have leading zeroes.

Valid: `0123` `00` `00.0` `01e01` `00e00`

Positive infinity can be represented as `inf`, `Infinity`, `+inf`, or `+Infinity`

Negative infinity can be represented as `-inf` or `-Infinity`

NaN can be represented as `nan`, `NaN`, `+nan`, `+NaN`, `-nan`, or `-NaN`

Hexadecimal can be represented using the `0x` or `0X` prefix, e.g. `0x1A`.

Octal can be represented using the `0o` or `0OX` prefix, e.g. `0o17`.

Binary can be represented using the `0b` or `0B` prefix. `0b1101`.

Hexadecimal, octal, and binary numbers must be positive and may not have a sign. 

### Null

TODO

## Features (in no particular order)

### Remember, `char` is a different type from `signed char` or `unsigned char`

In C++, there are three distinct character types, `char`, `signed char`, and `unsigned char`. This is in contrast to the other integer types which only have two, e.g. `int` is just shorthand for `signed int`.

This is important because we treat `signed char` and `unsigned char` as integer types (`int8_t` and `uint8_t` respectively), but treats straight `char` as an actual character, that is, a length 1 string.

So `qc::json::encode('A')` produces `"A"`, whereas `qc::json::encode(signed char('A'))` produces `65`.

### Internal number storage

In JSON, all numbers have the same type. That is, `10`, `10.000`, and `1e1` are all simply `Number`s. In C++ however, numbers come in three "flavors": signed integral, unsigned integral, and floating point. Since we want the greatest precision possible, we use the `int64_t`, `uint64_t`, and `double` types respectively. Unfortunately, no one type can capture all possible values of the others. This means, if we want to correctly encode/decode all possible numeric values, which we do, we need to utilize all three of these types under the hood.

Encoding is easy. The encoder is passed some value, which already has a numeric type, and it goes from there. The only thing to note here is that other numeric types are promoted to their largest variants first, so a `int16_t` will be promoted to `int64_t`, a `float` will be promoted to a `double`, etc. Since we strictly upcast and don't cross "flavor" boundaries, there is no funny business.

Decoding is another story. We start with just a string, and no trivial method of testing what type is the best choice. For example:

- `"1"` can be stored as all three
- `"1.000000"` can also be stored as all three
- `"1.000001"` can only be stored as `double`, however
- `"1000000000000000"` can be stored as all three, as it is less than 2<sup>53</sup>
- `"10000000000000000"` can only be stored as `int64_t` or `uint64_t`, as it is greater than 2<sup>53</sup>
- `"10000000000000000000"` can only be stored as `uint64_t`, as it is greater than 2<sup>63</sup>
- `"100000000000000000000"` can't be stored exactly by any, as it is greater than 2<sup>64</sup>

Long story short, the decoder prefers `int64_t` over `uint64_t` over `double`. If the number can be stored exactly as `int64_t`, it will be. Otherwise, if it can be stored exactly as `uint64_t`, it will be. Otherwise, it will be stored as `double`.

Then, when the user accesses that number as a certain arithmetic type, we check if it can be exactly represented by that type. If so, we convert from our internal type to that type (if necessary), and everyone's happy. If not, we throw a `TypeError`. This includes lower precision types, so accessing a value of `10` as `uint8_t` is fine, but acessing a value of `1000` as `uint8_t` is not.

This seems like a good time to point out that every. single. edge case involving these numeric conversions and machinations has been tested. This can be found in the number portion of each test file.

### New C++17 [charconv](https://abseil.io/about/design/charconv) library used for number/string conversion

It's the new best-on-the-block solution for converting strings to numbers and vice-versa. Much faster than existing solutions, while being fully correct/reliable.

### Unsafe `Value` accessors

The `Value::as` set of methods take a `qc::json::Safety` enum template value `isSafe`. If this is `qc::json::safe`, then the type is checked before the underlying data is returned, and if the type does not match, a `TypeError` is thrown. If this is instead `qc::json::unsafe` then no check is performed.

Checking the type is the default behavior, and generally a good idea, but there are situations in which it is only an uneccessary slowdown. Take for example:

```c++
// Here, `val` is a `qc::json::Value`
if (val.isString()) {
    name = val.asString();
}
else if (val.is<int>()) {
    name = "#" + std::to_string(val.as<int>());
}
```

In this example, the type is already checked, rendering the internal type check in the `as` methods redundant. This code can be safely changed to the following to be just a bit more performant:

```c++
// Not necessary, but may help cut down on verbosity in JSON-heavy code
using qc::json::unsafe;

// Here, `val` is a `qc::json::Value`
if (val.isString()) {
    name = val.asString<unsafe>();
}
else if (val.is<int>()) {
    name = "#" + std::to_string(val.as<int, unsafe>());
}
```

Beware, when using unsafe accessors, **NO** checks are done. There is nothing stopping you from reading a `Boolean` as an `Array`, or a `String` as an `int`. So be certain your assumptions are correct, lest you arrive at segfault city.
