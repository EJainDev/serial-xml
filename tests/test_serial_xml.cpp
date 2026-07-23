#include <gtest/gtest.h>

import std;

import serial_xml;

std::string clean_to_xml(auto obj) {
  return serial_xml::to_xml(obj).substr(
      std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>").size());
}

TEST(Basic, EmptyStruct) {
  struct EmptyStruct {};
  EmptyStruct obj;

  std::string xml = serial_xml::to_xml(obj);
  ASSERT_EQ(xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><EmptyStruct/>");
}

TEST(Basic, EmptyName) {
  struct[[= serial_xml::name{"MyEmptyStruct"}]] EmptyName {};
  EmptyName obj;

  ASSERT_EQ(clean_to_xml(obj), "<MyEmptyStruct/>");
}

TEST(Basic, Attribute) {
  struct Attribute {
    [[= serial_xml::attribute]] int x;
  };
  Attribute obj{42};
  ASSERT_EQ(clean_to_xml(obj), "<Attribute x=\"42\"/>");
}

TEST(Basic, NamedAttribute) {
  struct[[= serial_xml::name{"MyStruct"}]] NamedAttribute {
    [[= serial_xml::attribute]] int x;
  };
  NamedAttribute obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct x=\"42\"/>");
}

TEST(Basic, AttributeAndSkip) {
  struct[[= serial_xml::name{"MyStruct"}]] AttributeAndSkip {
    [[= serial_xml::attribute]] int x;
    [[= serial_xml::skip]] int y;
  };
  AttributeAndSkip obj{42, 100};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct x=\"42\"/>");
}

TEST(Basic, Child) {
  struct[[= serial_xml::name{"MyStruct"}]] Child {
    int x;
  };
  Child obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct><x>42</x></MyStruct>");
}

TEST(Basic, NamedChild) {
  struct Child {
    int x;
  };
  Child obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<Child><x>42</x></Child>");
}