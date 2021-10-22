# QC JSON

###### Quick and clean [JSON5](https://json5.org) header library for C++20

### Contents
- [Setup](#setup)
- [qc-json-encode.hpp (SAX encoding)](#qc-json-encodehppincludeqc-json-encodehpp)
  - [Simple Example](#simple-example)
  - [The Six Types](#the-six-types)
  - [Density](#density)
  - [Indentation Spaces](#indentation-spaces)
  - [Single Quote Strings](#single-quote-strings)
  - [Identifiers](#identifiers)
  - [Binary, Octal, and Hexadecimal](#binary-octal-and-hexadecimal)
  - [Infinity and NaN](#infinity-and-nan)
  - [Comments](#comments)
  - [Standalone Values](#standalone-values)
  - [Encoder Reuse](#encoder-reuse)
  - [Encode Errors](#encode-errors)
- [qc-json-decode.hpp (SAX decoding)](#qc-json-decodehppincludeqc-json-decodehpp)
  - [Decode Function](#decode-function)
  - [State](#state)
  - [Composer](#composer)
  - [Decode Errors](#decode-errors)
- [qc-json.hpp (DOM encoding and decoding)](#qc-jsonhppincludeqc-jsonhpp)

---

## Setup

### Method 1: Download the Header file(s)

As this is a header-only library with no dependencies outside the STL, you may simply copy/download the header(s) you
need into your project repo, `/usr/include`, or anywhere else your code can include from.

- [include/qc-json-encode.hpp](include/qc-json-encode.hpp) for SAX-style encoding
- [include/qc-json-decode.hpp](include/qc-json-decode.hpp) for SAX-style decoding
- [include/qc-json.hpp](include/qc-json.hpp) for DOM-style encoding and decoding (also requires the above two)

### Method 2: CMake via `FetchContent`

If using CMake, the latest version of this library can be automatically downloaded at config-time using the
`FetchContent` module.

```cmake
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/daskie/qc-json.git)
FetchContent_MakeAvailable(qc-json)
...
target_link_libraries(my-project PRIVATE qc-json::qc-json)
```

### Method 3: CMake via `find_package`

If using CMake, you can instead pre-install the library to avoid the config-time overhead and build directory bloat.

First install the package (there are many options for this) and then use `find_package` to link it into your build.

```cmake
find_package(qc-json REQUIRED)
...
target_link_libraries(my-project PRIVATE qc-json::qc-json)
```

---

## [qc-json-encode.hpp](include/qc-json-encode.hpp)

This standalone header provides a SAX-style interface for encoding JSON5. All standard features are
supported, plus a few extras.

### Simple Example

```c++
// Allows `qc::json::` to be omitted for code stream tokens such as `object`, `array`, and `end`
using namespace qc::json::tokens;

// Create an encoder to start. May specify certain options that will be described later
qc::json::Encoder encoder{};

    // Start an object
    encoder << object;
    
    // Insert a key/value pair
    encoder << "Name" << "18 Leg Bouquet";
    
    // And another, this time a number
    encoder << "Price" << 17.99;
    
    // And an array, all on one line just for fun
    encoder << "Ingredients" << array << "Crab" << "Octopus" << "Breadcrubs" << end;

// End the object
encoder << end;

// Now print the encoded string
std::cout << encoder.finish();
```
```json5
{
    "Name": "18 Leg Bouquet",
    "Price": 17.99,
    "Ingredients": [
        "Crab",
        "Octopus",
        "Breadcrubs"
    ]
}
```

### The Six Types

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{};
encoder << array;

    encoder << object << end; // Object
    encoder << array << end; // Array
    encoder << "abc"; // String
    encoder << 123; // Number
    encoder << true; // Boolean
    encoder << nullptr; // Null

encoder << end;
std::cout << encoder.finish();
```
```json5
[
  {},
  [],
  "abc",
  true,
  null
]
```

### Density

Objects and arrays may optionally specify a density which controls how much whitespace is generated.

Additionally, a root-level density may be provided as an option to the encoder.

There are four density levels:
1. `unspecified`: the default, same as `multiline`
2. `multiline`: newlines and indentation
3. `uniline`: single space between elements
4. `nospace`: no whitespace whatsoever

```c++
using namespace qc::json::tokens;
using qc::json::Density;

qc::json::Encoder encoder{Density::multiline}; // A root density may be specified to the encoder

encoder << array; // No density provided, defaults to `unspecified`

    // Objects
    encoder << object(Density::multiline) << "a" << 1 << "b" << 2 << end;
    encoder << object(Density::uniline)   << "c" << 3 << "d" << 4 << end;
    encoder << object(Density::nospace)   << "e" << 5 << "f" << 6 << end;
    
    // Arrays
    encoder << array(Density::multiline) << 1 << 2 << end;
    encoder << array(Density::uniline)   << 3 << 4 << end;
    encoder << array(Density::nospace)   << 5 << 6 << end;

encoder << end;

std::cout << encoder.finish();
```
```json5
[
    {
        "a": 1,
        "b": 2
    },
    { "c":  3, "d":  4 },
    {"e":5,"f":6},
    [
        1,
        2
    ],
    [ 3, 4 ],
    [5,6]
]
```

Density propagates into sub-containers maximally. For example, a `multiline` container inside a `nospace` parent
container will still be encoded as `nospace`, since `nospace` is "denser" than `multiline`.

```c++
using namespace qc::json::tokens;
using qc::json::Density;

qc::json::Encoder encoder{};

encoder << array(Density::uniline); // Parent container has `uniline` density

    // Objects
    encoder << object(Density::multiline) << "a" << 1 << "b" << 2 << end;
    encoder << object(Density::uniline)   << "c" << 3 << "d" << 4 << end;
    encoder << object(Density::nospace)   << "e" << 5 << "f" << 6 << end;
    
    // Arrays
    encoder << array(Density::multiline) << 1 << 2 << end;
    encoder << array(Density::uniline)   << 3 << 4 << end;
    encoder << array(Density::nospace)   << 5 << 6 << end;

encoder << end;

std::cout << encoder.finish();
```
```json5
[ { "a": 1, "b": 2 }, { "c":  3, "d":  4 }, {"e":5,"f":6}, [ 1, 2 ], [ 3, 4 ], [5,6] ]
```

### Indentation Spaces

The number of spaces per level of indentation may be provided as an option to the encoder. By default, four spaces are
used.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{
    qc::json::Density::multiline, // Root density
    2u // Indent spaces <---
};

encoder << object;
    encoder << "by" << array;
        encoder << "gones";
        encoder << "zantine";
        encoder << "ob";
    encoder << end;
encoder << end;

std::cout << encoder.finish();
```
```json5
{
  "by": [
    "gones",
    "zantine",
    "ob"
  ]
}
```

### Single Quote Strings

The option to use single quotes instead of double quotes for strings may be provided as an option to the encoder.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{
    qc::json::Density::multiline, // Root density
    4u,  // Indent spaces
    true // Use single quotes <---
};

encoder << object;
    encoder << "run" << "ran";
    encoder << "jump" << "jumped";
    encoder << "fly" << "flew";
encoder << end;

std::cout << encoder.finish();
```
```json5
{
    'run': 'ran',
    'jump': 'jumped',
    'fly': 'flew'
}
```

### Identifiers

The option to drop the quotes from key string that contain strictly word characters (alphanumeric and underscore) may be
provided as an option to the encoder.

If any other character appears in the key, it will be quoted.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{
    qc::json::Density::multiline, // Root density
    4u,    // Indent spaces
    false, // Use single quotes
    true   // Use identifiers <---
};

encoder << object;
    encoder << "abc" << "valid";
    encoder << "10" << "also valid";
    encoder << "_" << "believe it or not, valid";
    encoder << "$" << "still needs quotes";
encoder << end;

std::cout << encoder.finish();
```
```json5
{
    abc: "valid",
    10: "also valid",
    _: "believe it or not, valid",
    "$": "still needs quotes"
}
```

### Binary, Octal, and Hexadecimal

Unsigned integers may be encoded in binary, octal, or hexadecimal using the `bin`, `oct`, or `hex` tokens, respectively.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{};

const uint64_t val{7911u};

encoder << object;
    encoder << "dec" << val;
    encoder << "bin" << bin(val);
    encoder << "oct" << oct(val);
    encoder << "hex" << hex(val);
encoder << end;

std::cout << encoder.finish();
```
```json5
{
    "dec": 7911,
    "bin": 0b1111011100111,
    "oct": 0o17347,
    "hex": 0x1EE7
}
```

### Infinity and NaN

Positive infinity, negative infinity, and NaN are encoded to `inf`, `-inf`, and `nan`, respectively.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{};

encoder << array(qc::json::Density::uniline);
    encoder << std::numeric_limits<float>::infinity();
    encoder << -std::numeric_limits<float>::infinity();
    encoder << std::numeric_limits<float>::quiet_NaN();
encoder << end;

std::cout << encoder.finish();
```
```json5
[ inf, -inf, nan ]
```

### Comments

Comments may be encoded directly into the JSON using the `comment` token.

In a `multiline` context, a line comment is generated: `// A comment ...`

In a `uniline` context or in a `multiline` context between a key and value, a block comment with spaces is
generated: `/* A comment ... */`

In a `nospace` context, a block comment without spaces is generated: `/*A comment ...*/`

Note that commas are always placed after values followed by comments. This may generate dangling commas if the value is
the last in a container. As dangling commas are legal in JSON5, we accept this as an alternative to the extra code
complexity that would be necessary to avoid them.

```c++
using namespace qc::json::tokens;
using qc::json::Density;

qc::json::Encoder encoder{};

encoder << comment("Comments may occur anywhere in the JSON");
encoder << object;
    encoder << "Here's a comment before a value";
    encoder << "Of course, you can have multiple in succession";
    encoder << "Name" << "Odracir";
    encoder << "Age" << comment("This one's between a key and value") << 81;
    encoder << "Favorite foods" << array(Density::uniline);
        encoder << comment("Before");
        encoder << "Beer";
        encoder << comment("Between");
        encoder << "More beer";
        encoder << comment("After");
    encoder << end;
    encoder << comment("A comment with newlines\nis split into multiple lines\r\nand \\r\\n is supported");
encoder << end;
encoder << "One last comment, just to round it out";

std::cout << encoder.finish();
```
```json5
// Comments may occur anywhere in the JSON
{
    // Here's a comment before a value
    // Of course, you can have multiple in succession
    "Name": "Odracir",
    "Age": /* This one's between a key and value */ 81,
    "Favorite foods": [ /* Before */ "Beer", /* Between */ "More beer", /* After */ ],
    // A comment with newlines
    // is split into multiple lines
    // and \r\n is supported
},
// One last comment, just to round it out
```

### Standalone Values

A single json element may be encoded on its own without needing to be within an object or array.

Of course, trying to add more than one element to the root of the JSON is an error.

```c++
qc::json::Encoder encoder{};

encoder << "alone";

std::cout << encoder.finish();
```
```json5
"alone"
```

### Encoder Reuse

Calling `finish()` on an `qc::json::Encoder` leaves the encoder in valid, **empty** state, ready to be reused.

A single json element may be encoded on its own without needing to be within an object or array.

Of course, trying to add more than one element to the root of the JSON is an error.

```c++
using namespace qc::json::tokens;
using qc::json::Density;

qc::json::Encoder encoder{};

encoder << object;
    encoder << "the" << "first";
encoder << end;

std::cout << encoder.finish() << '\n';

encoder << array(Density::uniline) << "the" << "second" << end;

std::cout << encoder.finish() << '\n';

encoder << "third";

std::cout << encoder.finish();
```
```json5
{
    "the": "first"
}
[ "the", "second" ]
"third"
```

### Encode Errors

If streaming something to an encoder would cause an illegal state, a `qc::json::EncodeError` exception is thrown.

The following (at least) will cause an error:
- Trying to put two keys for a single object element
- Not putting a key for an object element
- Ending a container that doesn't exist
- Ending an object given a key before giving a value
- An identifier containing invalid characters
- An identifier that is empty
- Putting a comment between a key and value
- A comment containing unsupported characters
- A would-be block comment containing `*/`
- Finishing before all containers have been ended
- Attempting to add a second root value

---

## [qc-json-decode.hpp](include/qc-json-decode.hpp)

This standalone header provides a SAX-style interface for decoding JSON5. All standard features are
supported, plus a few extras.

### Decode Function

A JSON string may be decoded using the `qc::json::decode` function: 

```c++
template <typename Composer, typename State> void decode(string_view json, Composer & composer, State & initialState);
template <typename Composer, typename State> void decode(string_view json, Composer & composer, State && initialState);
```

`json` is the JSON5 string to decode. `composer` and `initialState` are explained below.

### State

When composing decoded JSON, it is useful, or even necessary, to track certain information relative to the scope of the
JSON being decoded. For example, when the JSON contains nested objects or arrays, it may be necessary to store and later
retrieve data of the parent container when starting/ending the child container.

`qc-json` provides a simple and efficient solution to this problem by keeping track of state for the user. In this way, the
data simply exists on the stack, without the need for additional containers or memory allocatoin.

When calling `decode`, the user provides the initial state, which will be passed to the callback for the JSON root
element.

### Composer

The composer is a user-provided class with callback methods for each JSON5 element type. For each element decoded, the
corresponding callback is called, along with the current state.

An example composer class:
```c++
class MyComposer
{
    // Need not be an inner class
    struct State { ... };
    
    public:
    
    ///
    /// Callback for an object
    ///
    /// @param outerState the state containing the object
    /// @return the new state to be used within the object
    ///
    State object(State & outerState);
    
    ///
    /// Callback for an array
    ///
    /// @param outerState the state containing the array
    /// @return the new state to be used within the array
    ///
    State array(State & outerState);
    
    ///
    /// Callback for the end of an object or array
    ///
    /// Density is determined by the whitespace found within the container or its children, not including the contents
    /// of comments and strings
    ///   - `multiline` if any newlines were encountered
    ///   - `uniline` if any other whitespace was encountered
    ///   - `compact` if no whitespace was encountered
    ///
    /// @param density the determined density of the array
    /// @param innerState the state of the array that ended
    /// @param outerState the state containing the array
    ///
    void end(const Density density, State && innerState, State & outerState);
    
    ///
    /// Callback for an object key
    ///
    /// @param key the key string. *Note: this view becomes invalid upon return*
    /// @param state the state containing the key
    ///
    void key(const std::string_view key, State & state);
    
    ///
    /// Callback for a string value
    ///
    /// @param val the value. *Note: this view becomes invalid upon return*
    /// @param state the state containing the value
    ///
    void val(const std::string_view val, State & state);
    
    ///
    /// Callback for a number that can be exactly represented as a 64 bit signed integer
    ///
    /// @param val the value
    /// @param state the state containing the value
    ///
    void val(const int64_t val, State & state);
    
    ///
    /// Callback for a number that can be exactly represented as an unsigned 64 bit integer, but is too large for a
    /// signed 64 bit integer
    ///
    /// @param val the value
    /// @param state the state containing the value
    ///
    void val(const uint64_t val, State & state);
    
    ///
    /// Callback for a number that cannot be exactly represented as a signed or unsigned 64 bit integer
    ///
    /// @param val the value
    /// @param state the state containing the value
    ///
    void val(const double val, State & state);
    
    ///
    /// Callback for a boolean
    ///
    /// @param val the value
    /// @param state the state containing the value
    ///
    void val(const bool val, State & state);
    
    ///
    /// Callback for null
    ///
    /// @param state the state containing the value
    ///
    void val(const std::nullptr_t, State & state);
    
    ///
    /// Callback for a comment
    ///
    /// Multiple line comments (`//` style) in succession will result in a single callback with their contents joined
    /// with newlines
    ///
    /// @param comment the comment string. *Note: this view becomes invalid upon return*
    /// @param state the state containing the comment
    ///
    void comment(const std::string_view comment, State & state);
}
```

The signatures of these callbacks for user provided composer classes are enforced using concepts, resulting in direct
and understandable error messages should they be ill-formed.

A dummy base class `qc::json::DummyComposer` is provided which the user may extend to avoid implementing all callbacks.

### Decode Errors

If the decoder encounters any issues decoding the JSON string, a `qc::json::DecodeError` will be thrown which contains
an index to approximately the first character which caused the problem.

## [qc-json.hpp](include/qc-json.hpp)

This header includes both `qc-json-encode.hpp` and `qc-json-decode.hpp` and provides a DOM-style interface for encoding,
decoding, and manipulating JSON5.

### DOM example

Let's say we have some JSON string `jsonStr`.

```json5
{
    "Name": "18 Leg Bouquet",
    "Price": 17.99,
    // Owner's note: make this gluten free
    "Ingredients":["Crab","Octopus","Breadcrumbs"]
}
```

To decode this, we pass it to the `qc::json::decode` function.

```c++
qc::json::Value rootVal{qc::json::decode(jsonStr)};
```

This returns us a `qc::json::Value`, which represents a single JSON "thing".

Next let's get a reference to our top level object.

```c++
// `qc::json::Object` is an alias for `std::map<std::string, qc::json::Value>`
qc::json::Object & rootObj{rootVal.asObject()};
```

We'll demonstrate basic access by type checking and printing the name.

```c++
qc::json::Value & nameVal{rootObj.at("Name")};

if (nameVal.type() == qc::json::Type::string) {
    std::cout << "Name: " << nameVal.asString();
}
```

> Name: 18 Leg Bouquet

Now let's modify some JSON by fulfilling the owner's request to make the dish gluten-free.

First we'll remove breadcrumbs from the ingredients list.

```c++
// We'll be reusing this later
qc::json::Value & ingredientsVal{rootObjec.at("Ingredients")};

// Get reference to ingredients array
// `qc::json::Array` is an alias for `std::vector<qc::json::Array>`
qc::json::Array & ingredients{ingredientsVal.asArray()};

// If "Breadcrumbs" is present, remove it
// This is just `std::vector` manipulation
auto it{ingredients.find("Breadcrumbs")};
if (it != ingredients.end()) {
    ingredients.erase(it);
}
```

Next, we update the price to reflect the change.

```c++
rootObj.at("Price").asFloater() -= 0.5;
```

We really shouldn't be storing prices as floating point, so let's add a comment.

```c++
rootObj.at("Price").setComment("Consider storing prices as cent integers");
```

Anyway, we'll next add a gluten-free tag.

```c++
// This constructs a new `qc::json::Value` defaulting to type Null, then assigns a bool converting it to type Boolean
rootObj["Gluten-free"] = true;

// Alternatively, we could directly create a Boolean value
rootObj.emplace("Gluten-free", true);
```

Last thing is to remove the gluten comment.

```c++
ingredientsVal.removeComment();
```

Finally, let's give the ingredients array a little breathing room.

```c++
ingredientsVal.setDensity(qc::json::Density::uniline);
```

Encoding back to a string yields our results.

```c++
jsonStr = qc::json::encode(rootVal);
```
```json5
{
    "Gluten-free": true,
    "Ingredients": [ "Crab", "Octopus" ],
    "Name": "18 Leg Bouquet",
    // Consider storing prices as cent integers
    "Price": 17.49
}
```

Note the reorganization of elements into alphabetical order. This is expected an currently unnavoidable due to using
`std::map` as the backing container. Consideration of alternatives is on the backlog.

### Safe vs Unsafe Access

### Type Exceptions

### Automatic Conversion between Custom Types and `qc::json::Value`

### Checking Type

### Retrieving Value

### Creating a Value

### Modifying a Value

### Equality of Values

### Handling Comments

### Handling Density

### `makeObject` and `makeArray`

### Decoding JSON string to `qc::json::Value`

### Encoding `qc::json::Value` to JSON string

---

## Miscellaneous

### Strings and `std::string_view`

### Supported Characters and Escape Sequences

### Number Formats

### Number Storage

### `char` is Special

---

## TODO

- Implement a stream-style decoder such that the user may do something like
```c++
std::string jsonStr{"[ 1, 2, 3 ]"};
StreamDecoder json{jsonStr};
int x, y, z;
json >> array >> x >> y >> z >> end;
```

- Fuzz testing

- Performance profiling

- Consider preserving order of elements in objects

---
---
---

### Alternatively, encode some JSON directly

```c++
using namespace qc::json::tokens; // Provides shorthand for `object`, `array`, `end`, and density control

qc::json::Encoder encoder{};
encoder << object;
    encoder << "Name" << "18 Leg Bouquet";
    encoder << "Price" << 17.99;
    encoder << "Ingredients" << array(uniline) << "Crab" << "Octopus" << end;
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

In addition, a specific code point can be specified with `\x` followed by two HexToken digits, or `\u` followed by four HexToken digits.
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

Hexadecimal, Octal, and Binary numbers must be positive and may not have a sign. 

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
