# QJson
Simple, lightweight JSON endoder/decoder for C++17

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
