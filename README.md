# QC JSON

###### Quick and clean JSON5 header library for C++20

### Contents
- [Description](#description)
- [Setup](#setup)
- [SAX Encoding](#qc-json-encodehpp)
  - [Simple Example](#simple-example)
  - [The Six Types](#the-six-types)
  - [Density](#density)
  - [Indentation Spaces](#indentation-spaces)
  - [Single Quote Strings](#single-quote-strings)
  - [Identifiers](#identifiers)
  - [Comments](#comments)
  - [Binary, Octal, and Hexadecimal](#binary-octal-and-hexadecimal)
  - [Infinity and NaN](#infinity-and-nan)
  - [Standalone Values](#standalone-values)
  - [Encoder Reuse](#encoder-reuse)
  - [Encode Errors](#encode-errors)
- [SAX Decoding](#qc-json-decodehpp)
  - [Decode Function](#decode-function)
  - [State](#state)
  - [Composer](#composer)
  - [Decode Errors](#decode-errors)
- [DOM Encoding and Decoding](#qc-jsonhpp)
  - [DOM Example](#dom-example)
  - [DOM Decoding](#dom-decoding)
  - [DOM Encoding](#dom-encoding)
  - [Value Creation](#value-creation)
  - [Value Modification](#value-modification)
  - [Checking Type](#checking-type)
  - [Value Access](#value-access)
  - [Value Equality](#value-equality)
  - [Custom Type Conversion](#custom-type-conversion)
  - [Handling Comments](#handling-comments)
  - [Handling Density](#handling-density)
- [Miscellaneous](#miscellaneous)
  - [Optimizations](#optimizations)
  - [Supported Characters and Escape Sequences](#supported-characters-and-escape-sequences)
  - [Number Formats](#number-formats)
  - [Number Storage](#number-storage)
  - [`char` is Special](#char-is-special)
- [TODO](#todo)

---

## Description

QC JSON is a [JSON5](https://json5.org) header-only library for C++20 emphasizing usability, functionality, and
simplicity.

What sets QC JSON apart from other JSON libraries?
- Supports all JSON5 features
  - Comments
  - Dangling commas
  - Hexadecimal numbers
  - Infinity and NaN
  - Single quote strings
  - Unquoted keys
  - Additional escape sequences
- DOM and SAX interfaces
- Header-only for super easy setup
- The concept of container "density" to control whitespace formatting
- Comments and formatting are preserved when modifying a JSON document
- Binary, octal, and hexadecimal number formats
- Two, four, and eight byte unicode escape sequences (`\xHH`, `\uHHHH`, `\UHHHHHHHH`)
- Uses C++17's [\<charconv\>](https://en.cppreference.com/w/cpp/header/charconv) library for fast and lossless floating
  point encoding/decoding
- C++20 features such as concepts improves compile-time information
- Custom type conversion
- Support for 32 and 64 bit platforms

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

This standalone header provides a SAX-style interface for encoding [JSON5](https://json5.org). All standard features are
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
    
    // One more, an array, all on one line just for fun
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

There are six fundamental [JSON value types](https://www.json.org): Object, Array, String, Number, Boolean, and Null.

Here is an example of encoding each:

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
1. `unspecified`: the default, and equivalent to `multiline`
2. `multiline`: newlines and indentation
3. `uniline`: single spaces between elements
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

The option to drop the quotes from key strings that contain strictly word characters (alphanumeric and underscore) may be
provided as an option to the encoder.

If any non-word character is present, the key will be quoted.

```c++
using namespace qc::json::tokens;

qc::json::Encoder encoder{
    qc::json::Density::multiline, // Root density
    4u,    // Indent spaces
    false, // Use single quotes
    true   // Use identifiers <---
};

encoder << object;
    encoder << "abc" << "Valid";
    encoder << "10" << "Also valid";
    encoder << "_" << "Believe it or not, valid";
    encoder << "$" << "Still needs quotes";
encoder << end;

std::cout << encoder.finish();
```
```json5
{
    abc: "Valid",
    10: "Also valid",
    _: "Believe it or not, valid",
    "$": "Still needs quotes"
}
```

### Comments

Comments may be encoded using the `comment` token.

In a `multiline` density, a line comment is generated: `// A comment ...`

In a `uniline` density, or in a `multiline` density between a key and value, a block comment with spaces is
generated: `/* A comment ... */`

In a `nospace` density, a block comment without spaces is generated: `/*A comment ...*/`

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

Calling `finish()` on an `qc::json::Encoder` leaves the encoder in a valid, empty state, ready to be reused.

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
- Ending an object with a dangling key
- An identifier that is empty
- A comment containing unsupported characters
- A would-be block comment containing `*/`
- Finishing before all containers have been ended
- Attempting to add a second root value

---

## [qc-json-decode.hpp](include/qc-json-decode.hpp)

This standalone header provides a SAX-style interface for decoding [JSON5](https://spec.json5.org). All standard features are
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

`qc-json` provides a simple and efficient solution to this problem by keeping track of state for the user on the stack.
This simplifies the calling logic and removes the need for additional containers or memory allocation.

When calling `decode`, the user provides the initial state, which will be passed to the callback for the JSON root
element.

### Composer

The composer is a user-provided class with callback methods for each JSON element type. For each element decoded, the
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

---

## [qc-json.hpp](include/qc-json.hpp)

This header includes both `qc-json-encode.hpp` and `qc-json-decode.hpp` and provides a DOM-style interface for encoding,
decoding, and manipulating [JSON5](https://spec.json5.org).

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

### DOM Decoding

A JSON string is decoded to a `qc::json::Value` using the `qc::json::decode` function.

```json5
// Just a little JSON
{
    "key": "value"
}
```
```c++
qc::json::Value rootVal{qc::json::decode(jsonStr)};
```

### DOM Encoding

A JSON string is encoded from a `qc::json::Value` using the `qc::json::encode` function.

Four optional parameters may be provided to specify the output format.

Option | Description
:---:|:---:
`density` | The root density, will be overridden by containers of higher density
`indentSpaces` | The number of spaces per level of indentation
`singleQuotes` | Whether to use single or double quotes for strings
`identifiers` | Whether keys containing only alphanumeric and underscore characters should be unquoted

```c++
jsonStr = qc::json::encode(rootVal,
                           qc::json::Density::uniline, // Density
                           4,                          // Indent spaces
                           true,                       // Single quotes
                           true);                      // Identifiers
```
```json5
/* Just a little JSON */ { key: 'value' }
```

### Value Creation

Constructing a value via `qc::json::Value{...}` creates a new JSON value depending on the type passed:

 Constructed With Type | JSON Type | Stored Internally As
:---:|:---:|:---:
`qc::json::Object` | `object` | `qc::json::Object`
`qc::json::Array` | `array` | `qc::json::Array`
`std::string`, `std::string_view`, `const char *`, `char` | `string` | `std::string`
`int8_t`, `int16_t`, `int32_t`, `int64_t` | `integer` | `int64_t`
`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` | `unsigner` | `uint64_t`
`float`, `double` | `floater` | `double`
`bool` | `boolean` | `bool`
`nullptr_t` or nothing | `null` | `nullptr_t`

Note: `qc::json::Object` is an alias for `std::map<std::string, qc::json::Value>`, and  `qc::json::Array` is an alias
for `std::vector<qc::json::Value>`

Any other type may be passed to the constructor, in which case `qc::json::ValueFrom` is called.
See [Custom Type Conversion](#custom-type-conversion).

### `makeObject` and `makeArray`

Objects and arrays may be constructed "manually" using the standard `std::map`/`std::vector` API, but in certain cases
this proves tedious.

Two helper functions, `qc::json::makeObject` and `qc::json::makeArray` are provided as a short-hand alternatives.

```c++
// An even number of arguments alternating between key and value are forwarded to `std::map::emplace`
qc::json::Value objVal{qc::json::makeObject("a", 123, "b", true, "c", nullptr)};

// Arguments are each forwarded to `std::vector::emplace_back`
qc::json::Value arrVal{qc::json::makeArray(123, true, nullptr)};
```
```json5
// `objVal`
{
    "a": 123,
    "b": true,
    "c": null
}
```
```json5
// `arrVal`
[
    123,
    true,
    null
]
```

### Value Modification

An existing `qc::json::Value` may be modified in two ways.

A mutable reference to the contained value may be accessed and manipulated using the `as...` methods.

```c++
qc::json::Value val{0};
++val.asInteger(); // Directly increments the internal value to `1`
```

Alternatively, the `qc::json::Value` itself may be directly assigned a new value.

```c++
qc::json::Value val{0};
val = 1; // Assigns the internal value to `1`
val = "abc"; // The old internal value is destructed and a new internal value is constructed as the string "abc"
```

### Checking Type

Type may be checked using the `is` methods, of which there are direct and templated versions.

`qc::json::Type` | Direct Method | Template Method | Internal Type
:---:|:---:|:---:|:---:
`object` | `isObject` | `is<qc::json::Object>` | `qc::json::Object`
`array` | `isArray` | `is<qc::json::Array>` | `qc::json::Array`
`string` | `isString` | `is<std::string>`, `is<std::string_view>`, `is<const char *>`, `is<char>`* | `std::string`
`integer` | `isInteger` | ** | `int64_t`
`unsigner` | `isUnsigner` | ** | `uint64_t`
`floater` | `isFloater` | ** | `double`
`boolean` | `isBoolean` | `is<bool>` | `bool`
`null` | `isNull` | `is<nullptr_t>` | `nullptr_t`

\* `is<char>` checks not only that the type is `string`, but also that the length of the string is `1`.

** The direct numeric methods `isInteger`, `isUnsigner`, and `isFloater` simply check for a match in type. In contrast,
the template numeric method `is<...>` checks not just the type, but also the value of the number. Basically, it returns
true if the value can be exactly represented by the given type. For example, a value `13` may be stored internally as a
`int64_t` with type `integer`, but `is<int>`, `is<unsigned int>`, and `is<float>` would all return true, since `13` can
be exactly represented in each case. On the other hand, were the value `-13`, `is<int>` and `is<float>` would return
true, but `is<unsigned int>` would return false.

### Value Access

The internal value of a `qc::json::Value` can be retrieved by two means.

First, a reference to the value is returned via the `as...` methods: `asObject`, `asArray`, `asString`, `asInteger`,
`asUnsigner`, `asFloater`, and `asBoolean`.

Second, a copy of the value is returned via the `get<...>` method. For example, `get<short>` will return the value as a
`short`.

In both cases, if the type requested does not match the type of the value, or, in the case of numbers, the type
requested cannot exactly represent the value, a `qc::json::TypeError` is thrown.

If the type is known ahead of time, a little performance can be gained by bypassing these checks with the "unsafe"
specializations of these functions.

For example, this is a common pattern:
```c++
qc::json::Value val{...};

// Check the type of the value
if (val.isBoolean()) {
    // We know it's a boolean, so this is okay
    doSomething(val.asBoolean<qc::json::unsafe>());
}
else if (val.is<const char *>()) {
    // We know it's a string, so this is okay
    doSomethingElse(val.get<const char *, qc::json::unsafe>());
}
```

Be warned, using these unsafe methods erroneously is a first class ticket to segfault city. The code will
`reinterpret_cast` a `double` to a `std::string *` if you let it.

### Value Equality

Two values can be compared with `==` and `!=`.

```c++
qc::json::Value val1, val2{...};

val1 == val2; // Whether the values are equivalent
val1 != val2; // Whether the values are not equivalent
```

Alternatively, a value can be compared directly.

```c++
qc::json::Value val{...};
qc::json::Object obj{...};

val == true; // Whether `val` is a boolean with value `true`
val != obj; // Whether `val` isn't an object or is an object that isn't equivalent to `obj`
```

For numbers, the type does not need to match so long as the values are exactly the same.

```c++
qc::json::Value val{-1}; // A signed integer
val == -1.0; // True - different types but exactly the same number
val == -1.1; // False - the fractional component is considered
val == -1u; // False - the unsigned value has the same binary representation but is a different number
```

### Custom Type Conversion

User defined types can be implicitly converted to or from a `qc::json::Value` by means of the `qc::json::ValueFrom` and
`qc::json::ValueTo` struct specializations.

For example, let's enable automatic conversion between a `std::pair<int, int>` and an array of two numbers.

```c++
// Specializing `qc::json::ValueFrom` for `std::pair<int, int>`
template <>
struct qc::json::ValueFrom<std::pair<int, int>> {
    qc::json::Value operator()(const std::pair<int, int> & v) const {
        return qc::json::makeArray(v.first, f.second);
    }
};
```

```c++
// Specializing `qc::json::ValueTo` for `std::pair<int, int>`
template <>
struct qc::json::ValueTo<std::pair<int, int>> {
    std::pair<int, int> operator()(const qc::json::Value & v) const {
        const qc::json::Array & arr{v.asArray()};
        return {arr.at(0u)->get<int>(), arr.at(1u)->get<int>()};
    }
};
```

This now enables us to write code such as:

```c++
std::pair<int, int> pair{1, 2};

// `ValueFrom` is called to implicity convert `pair` to `qc::json::Value`
qc::json::Value pairVal{pair};

// The same implicit conversion happens and then the new `qc::json::Value` is assigned to `pairVal`
pairVal = pair;

// The same implicit conversion happens and then the new `qc::json::Value` is compared to `pairVal`
pairVal == pair;

// `ValueTo` is called to convert `pairVal` to `std::pair<int, int>`
pair = pairVal.get<pairVal>();
```

### Handling Comments

Each JSON value may have one comment "attached" to it.

#### Decoding Comments

Each value will pick up the comment immediately preceding it.

```json5
// A comment
[
    // Multiple, contiguous line comments
    // are combined into one
    "val1",
    
    /* In the case of multiple separate comments */
    // The last one is used
    {
        // A comment may precede the key
        "key1": "val2",
      
        "key2": /* Or the value */ "val3"
    }
    
    // But any comment not preceding a value is ignored
]
```
```c++
qc::json::Value rootVal{...}; // Decoded from the above string

// Simply printing the comments...

std::cout << *rootVal.comment() << std::endl;

qc::json::Array & rootArr{rootVal.asArray()};
std::cout << *rootArr.at(0).comment() << std::endl;
std::cout << *rootArr.at(1).comment() << std::endl;

qc::json::Object & innerObj{rootArr.at(1).asObject()};
std::cout << *innerObj.at("key1").comment() << std::endl;
std::cout << *innerObj.at("key2").comment() << std::endl;
```
> A comment<br>
> Multiple, contiguous line comments are combined into one<br>
> The last one is used<br>
> A comment may precede the key<br>
> Or the value

#### DOM Structure Comments

Use the `hasComment` method to check for the presence of a comment:
```c++
if (jsonVal.hasComment()) ...
```

Use the `comment` method to get a pointer to the comment string, or `nullptr` if the value has no comment:
```c++
std::string * commentStr{jsonVal.comment()};
```

Use the `setComment` method to add or override a comment:
```c++
jsonVal.setComment("Some incredible information")
```

Use the `removeComment` method to remove a comment and return its ownership:
```c++
std::unique_ptr<std::string> commentStr{jsonVal.removeComment()};
```

#### Encoding Comments

Comments are placed immediately before each value.

```json5
[
    // A line comment is used in multiline density
    // with newlines preserved
    "val1",
    [ /* A block comment is used in uniline density */ "val2" ],
    [/*A block comment without extra spaces is used in nospace density*/"val3"]
]
```

### Handling Density

Use the `density` method to retrieve the current density of an object or array.
```c++
qc::json::Density density{jsonObj.density()};
```

Use the `setDensity` method to set the density for an object or array.
```c++
jsonObj.setDensity(qc::json::Density::uniline);
```

When a JSON string is decoded, density is detected and assigned per-container based on the whitespace within.

```json5
[
    // Multiline density
    [ /* Uniline density */ 1, 2, 3 ],
    [/*Nospace density*/1,2,3]
]
```

See the [encoding density](#density) section for more info.

---

## Miscellaneous

### Optimizations

Although the focus of this library is not speed, performance is never truly out of mind. Here are ***some*** of the
optimizations that have been made thus far:
- The size of each `qc::json::Value` is only 16 bytes thanks to low-order bit packing of pointers
- Numbers, booleans, and null values are stored inline with no dynamic allocation
- Liberal preference for `std::string_view` over `std::string` cuts down on unecessary and unintended string copies
- Modern C++ move semantics and perfect forwarding where it makes sense
- Exclusive usage of the new C++17 [\<charconv\>](https://abseil.io/about/design/charconv) library for incredibly fast floating point encoding/decoding

### Supported Characters and Escape Sequences

Key and value strings may only contain [printable](https://en.cppreference.com/w/cpp/string/byte/isprint) characters,
or newline sequences (`\n`, `\r\n`) if escaped. Any other character must be represented with an escape sequence.

#### Specific Escape Sequences

Sequence | Name | Code Point
:---:|:---:|:---:
`\0` | Null | `U+0000`
`\b` | Backspace | `U+0008`
`\t` | Horizontal Tab | `U+0009`
`\n` | New Line | `U+000A`
`\v` | Vertical Tab | `U+000B`
`\f` | Form Feed | `U+000C`
`\r` | Carriage Return | `U+000D`

#### Generic Escape Sequences

Sequence | Code Point | Description
:---:|:---:|:---:
`\xHH` | `U+00HH` | 1 Byte Unicode code point, each H is a hex digit
`\uHHHH` | `U+HHHH` | 2 Byte Unicode code point, each H is a hex digit
`\UHHHHHHHH` | `U+HHHHHHHH` | 4 Byte Unicode code point, each H is a hex digit

#### Newline Escapes

JSON5 permits newlines to appear in strings if escaped, allowing long strings to be split accross several lines.

Both `\n` and `\r\n` newline sequences may be escaped.

```json5
{
    "k": "A long \
string with \
multiple lines",
    "Even works \
for keys!" : "v"
}
```

#### Other Escape Sequences

Any character not described above preceeded by a backslash simply evaluates to that character.

For example, `\A` is `A`, `\\` is `\ `, `\?` is `?`, etc.

### Number Formats

Numbers may have leading or trailing decimal points in the coefficient.
- Valid: `1.` `.1` `1.e2` `.1e2`
- Invalid: `.` `.e2` `1e.2` `1e2.`

Numbers may have leading zeroes, and this is not interpreted as octal.
- Valid: `0123` `00` `00.0` `01e01` `00e00`

Positive infinity can be represented as `inf`, `Infinity`, `+inf`, or `+Infinity`.

Negative infinity can be represented as `-inf` or `-Infinity`.

NaN can be represented as `nan`, `NaN`, `+nan`, `+NaN`, `-nan`, or `-NaN`.

Hexadecimal can be represented using the `0x` or `0X` prefix, and may contain uppercase or lowercase digits.
- Valid: `0x1A` `0x1a` `0X1A` `0X1a`

Octal can be represented using the `0o` or `0O` prefix.
- Valid: `0o17` `0O17`

Binary can be represented using the `0b` or `0B` prefix.
- Valid: `0b1101` `0B1101`

Hexadecimal, octal, and binary numbers must not have a sign, decimal, or exponent.

### Number Storage

In JSON, all numbers are considered to have the same type, large or small, positive or negative, fraction or no.
Additionally, there is no restriction on the size or precision of a number.

In C++ however, without using some "big number" library, which we don't, we really only have three choices: `int64_t`,
`uint64_t`, or `double`.

Deciding which to use when decoding a number essentially works as follows:
1. If the number is an integer and can fit in a `int64_t`, then use that
2. Otherwise, if the number is a positive integer and can fit in a `uint64_t`, then use that
3. Otherwise, use a `double`, and accept any loss in precision

In this way, `int64_t` is prefered to `uint64_t`, which is prefered to `double`.

As an example:
- `"1"` will be stored as a `int64_t`
- `"1.000000"` will be stored as a `int64_t` as it is still an integer despite the trailing zeroes
- `"1.000001"` will be stored as a `double` as it has a non-zero fractional component
- `"1000000000000000000"` will be stored as a `int64_t`
- `"10000000000000000000"` will be stored as a `uint64_t` as it is too large for `int64_t`
- `"-10000000000000000000"` will be stored as a `double` as it is too small for `int64_t`
- `"100000000000000000000"` will be stored as a `double` as it is too large for `uint64_t`

When the user accesses a number as a certain arithmetic type using the `get<...>` method, we check if it can be exactly
represented by the requested type. If so, we convert from our internal type to that type (if necessary). If not, we
throw a `TypeError`. This includes lower precision types, so accessing a value of `10` as `uint8_t` is fine, but
accessing a value of `1000` as `uint8_t` is not.

### `char` is Special

In C++, there are three distinct character types: `char`, `signed char`, and `unsigned char`. This is in contrast to the
other integer types which only have two, e.g. `int` is just shorthand for `signed int`.

This is important because this library treats `signed char` and `unsigned char` as integer types (`int8_t` and `uint8_t`
respectively), but treats `char` as an actual character, that is, a length 1 string.

So `qc::json::encode('A')` produces `"A"`, whereas `qc::json::encode(static_cast<signed char>('A'))` produces `65`.

---

## TODO

- Implement a stream-style decoder such that the user may do something like
```c++
std::string jsonStr{"[ 1, 2, 3 ]"};
StreamDecoder json{jsonStr};
int x, y, z;
json >> array >> x >> y >> z >> end;
```

- Full unicode support

- Fuzz testing

- Performance profiling

- Consider preserving order of elements in objects
