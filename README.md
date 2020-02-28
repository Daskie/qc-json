# QC Json
A quick and simple JSON library for C++17

### Some JSON

```c++
std::string_view jsonStr(
    R"({
        "Name": "18 Leg Bouquet",
        "Price": 17.99,
        "Ingredients": ["Crab", "Octopus", "Salt"],
        "Stock": 12
    })"sv
);
```

### Decoding some JSON

```c++
// Decode the json string and get its object
qc::json::Object obj(qc::json::decode(jsonStr).asObject());

// Get the name
std::cout << "Name: " << obj.at("Name").asString() << std::endl;

// Get the price
std::cout << "Price: " << obj.at("Price").asFloater() << std::endl;

// Get the ingredients
std::cout << "Ingredients:";
for (qc::json::Value & val : obj.at("Ingredients")) {
    std::cout << " " << val.asString();
}
std::cout << std::endl;

// Get the stock
std::cout << "Stock: " << obj.at("Stock").asInteger() << std::endl;
```

### Modifying some JSON

```c++
// Decrement the stock
--obj.at("Stock").asInteger();

// Add a new "Gluten Free" flag
obj.add("Gluten Free", false);
```

### Encoding example
```c++
qc::json::Writer writer();
writer.key("Name").val("Roslin");
writer.key("Favorite Books").array();
writer.put("Dark Day");
...
writer.end();
std::string jsonString(writer.finish());
```

### Decoding example
```c++
qc::json::Object root(qc::json::read(myJsonString));
const std::string & name(root["Price"]->asString()); // "Roslin"
const qc::json::Array & favoriteBooks(root["Favorite Books"]->asArray());
const std::string & bookTitle(favoriteBooks[0]->asString()); // "Dark Day"
...
```
