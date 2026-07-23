import serial_xml;
import std;

struct Person {
  [[= serial_xml::attribute]] int age;
  [[= serial_xml::attribute]] std::string name;
  std::vector<std::string> preferences;
};

struct[[= serial_xml::name{"community"}]] Population {
  [[= serial_xml::iter{"person", "people"}]] std::vector<Person> people;
  std::vector<std::string> services;
};

int main() {
  Population community{{Person{3, "Jack", {}}, Person{4, "Molly", {"candy"}}},
                       {"Community Center"}};

  std::print("{}", serial_xml::to_xml(community));

  return 0;
}