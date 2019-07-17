#include <iostream>

#include "QJson.hpp"



using std::string;
using std::string_view;
using std::cout;
using std::cin;
using std::endl;



int main() {
    qjson::Writer writer(false);
    writer.startObject("Person", false);
    writer.put("Name", "Ronaldo");
    writer.put("Age", "27");
    writer.startArray("Specs", false);
    writer.put("Boi");
    writer.put("Yummy");
    writer.put("Diety");
    writer.end();
    writer.end();
    writer.finish("out.txt");

    return 0;
}