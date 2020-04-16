# QC Json
###### Clean, quick, and simple JSON library for C++17

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
qc::json::Value jsonVal(qc::json::decode(jsonStr));
```

Let's get a reference to our top level object for easy access

```c++
qc::json::Object & obj(jsonVal.asObject());
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
std::string newJsonStr(qc::json::encode(jsonVal));
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
qc::json::Encoder encoder;
encoder.object();
    encoder.key("Name").val("18 Leg Bouquet");
    encoder.key("Price").val(17.99);
    encoder.key("Ingredients).array(true).val("Crab").val("Octopus").end();
    encoder.key("Gluten Free").val(true);
    encoder.key("Sold").val(69);
encoder.end();
std::string altJsonStr(encoder.finish());
```

`altJsonStr` will contain:

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

## Features (in no particular order)

### Remember, `char` is a different type from `signed char` or `unsigned char`

In C++, there are three distinct character types, `char`, `signed char`, and `unsigned char`. This is in contrast to the other integer types which only have two, e.g. `int` is just shorthand for `signed int`.

This is important because we treat `signed char` and `unsigned char` as integer types (`int8_t` and `uint8_t` respectively), but treats straight `char` as an actual character, that is, a length 1 string.

So `qc::json::encode('A')` produces `"A"`, whereas `qc::json::encode(signed char('A'))` produces `65`.

### Internal Number Storage

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
