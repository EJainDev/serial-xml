import std;

import serial_xml;

struct[[= serial_xml::name{"person"}]] Person {
  std::string name;
  unsigned int age;
};

struct[[= serial_xml::name{"population"}]] Population {
  [[= serial_xml::attribute]] std::string name;
  [[= serial_xml::attribute]] int size;
  [[= serial_xml::unpack{"person"}]] std::vector<Person> people;
};

int main() {
  std::println("{}", serial_xml::to_xml(
                         Population{"Earth", 2, {{"Alice", 30}, {"Bob", 25}}}));
}