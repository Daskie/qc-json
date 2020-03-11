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
obj.at("Ingredients").asArray().remove(2);
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

Now for all the specifics...
