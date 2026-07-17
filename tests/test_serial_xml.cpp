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

TEST(Basic, EmptyStructWithName) {
  struct[[= serial_xml::name{"MyEmptyStruct"}]] EmptyStructWithName {};
  EmptyStructWithName obj;

  ASSERT_EQ(clean_to_xml(obj), "<MyEmptyStruct/>");
}

TEST(Basic, StructWithAttribute) {
  struct StructWithAttribute {
    [[= serial_xml::attribute]] int x;
  };
  StructWithAttribute obj{42};
  ASSERT_EQ(clean_to_xml(obj), "<StructWithAttribute x=\"42\"/>");
}

TEST(Basic, StructWithNameAndAttribute) {
  struct[[= serial_xml::name{"MyStruct"}]] StructWithNameAndAttribute {
    [[= serial_xml::attribute]] int x;
  };
  StructWithNameAndAttribute obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct x=\"42\"/>");
}

TEST(Basic, StructWithNameAndAttributeAndSkip) {
  struct[[= serial_xml::name{"MyStruct"}]] StructWithNameAndAttributeAndSkip {
    [[= serial_xml::attribute]] int x;
    [[= serial_xml::skip]] int y;
  };
  StructWithNameAndAttributeAndSkip obj{42, 100};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct x=\"42\"/>");
}

TEST(Basic, StructWithNameAndChild) {
  struct[[= serial_xml::name{"MyStruct"}]] StructWithNameAndChild {
    int x;
  };
  StructWithNameAndChild obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct><x>42</x></MyStruct>");
}

TEST(Basic, StructWithChild) {
  struct StructWithChild {
    int x;
  };
  StructWithChild obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<StructWithChild><x>42</x></StructWithChild>");
}