#include <gtest/gtest.h>

import std;

import serial_xml;

std::string clean_to_xml(auto obj) { return serial_xml::to_xml(obj, false); }

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

TEST(Basic, FormattedAttribute) {
  struct FormattedAttribute {
    [[ = serial_xml::attribute, = serial_xml::format{"03d"} ]] int x;
  };
  FormattedAttribute obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<FormattedAttribute x=\"042\"/>");
}

TEST(Basic, Child) {
  struct[[= serial_xml::name{"MyStruct"}]] Child {
    int x;
  };
  Child obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<MyStruct><x>42</x></MyStruct>");
}

TEST(Basic, NamedChild) {
  struct NamedChild {
    [[= serial_xml::name{"field"}]] int x;
  };
  NamedChild obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<NamedChild><field>42</field></NamedChild>");
}

TEST(Basic, FormattedChild) {
  struct FormattedChild {
    [[= serial_xml::format{"03d"}]] int x;
  };
  FormattedChild obj{42};

  ASSERT_EQ(clean_to_xml(obj), "<FormattedChild><x>042</x></FormattedChild>");
}

TEST(Basic, Children) {
  struct Children {
    int x;
    int y;
  };
  Children obj{42, 100};
  ASSERT_EQ(clean_to_xml(obj), "<Children><x>42</x><y>100</y></Children>");
}

TEST(Basic, Attributes) {
  struct Attributes {
    [[= serial_xml::attribute]] int x;
    [[= serial_xml::attribute]] int y;
  };
  Attributes obj{42, 100};
  ASSERT_EQ(clean_to_xml(obj), "<Attributes x=\"42\" y=\"100\"/>");
}

TEST(Basic, AttributesAndChildren) {
  struct AttributesAndChildren {
    [[= serial_xml::attribute]] int x;
    int y;
  };
  AttributesAndChildren obj{42, 100};
  ASSERT_EQ(
      clean_to_xml(obj),
      "<AttributesAndChildren x=\"42\"><y>100</y></AttributesAndChildren>");
}

TEST(Nesting, NestedStruct) {
  struct Inner {
    int x;
  };
  struct Outer {
    Inner inner;
  };
  Outer obj{{42}};
  ASSERT_EQ(clean_to_xml(obj), "<Outer><inner><x>42</x></inner></Outer>");
}

struct NoUnpackInner {
  int x;
};

template <> struct std::formatter<NoUnpackInner> : std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const NoUnpackInner &value, FormatContext &ctx) const {
    return std::formatter<std::string>::format(std::to_string(value.x), ctx);
  }
};

TEST(Nesting, NoUnpack) {
  struct NoUnpackOuter {
    [[= serial_xml::no_unpack]] NoUnpackInner inner;
  };
  NoUnpackOuter obj{{42}};
  ASSERT_EQ(clean_to_xml(obj),
            "<NoUnpackOuter><inner>42</inner></NoUnpackOuter>");
}

TEST(STL, Vector) {
  struct Vector {
    std::vector<int> values;
  };
  Vector obj{{1, 2, 3}};
  ASSERT_EQ(clean_to_xml(obj),
            "<Vector><values><element>1</element><element>2</"
            "element><element>3</element>"
            "</values></Vector>");
}

TEST(STL, Optional) {
  struct Optional {
    std::optional<int> value;
  };
  Optional obj{{42}};
  ASSERT_EQ(clean_to_xml(obj), "<Optional><value>42</value></Optional>");

  Optional obj2{std::nullopt};
  ASSERT_EQ(clean_to_xml(obj2), "<Optional/>");
}

TEST(STL, NoIter) {
  struct NoIter {
    [[= serial_xml::no_iter]] std::vector<int> values;
  };
  NoIter obj{{1, 2, 3}};
  ASSERT_EQ(clean_to_xml(obj), "<NoIter><values>[1, 2, 3]</values></NoIter>");
}

TEST(STL, NestedVector) {
  struct NestedVector {
    std::vector<std::vector<int>> values;
  };
  NestedVector obj{{{1, 2}, {3, 4}}};
  ASSERT_EQ(clean_to_xml(obj),
            "<NestedVector><values><element>[1, 2]</element><element>[3, "
            "4]</element></values></NestedVector>");
}

template <typename T> struct CustomList {
  T data[16];
  std::size_t sz = 0;

  constexpr CustomList() = default;
  constexpr CustomList(std::initializer_list<T> init) {
    for (auto v : init) {
      if (sz < 16)
        data[sz++] = v;
    }
  }

  using iterator = const T *;
  using const_iterator = const T *;
  using value_type = T;
  using size_type = std::size_t;

  constexpr iterator begin() const { return data; }
  constexpr iterator end() const { return data + sz; }
  constexpr const_iterator cbegin() const { return data; }
  constexpr const_iterator cend() const { return data + sz; }

  constexpr size_type size() const { return sz; }
  constexpr bool empty() const { return sz == 0; }

  constexpr const T &operator[](std::size_t i) const { return data[i]; }
};

TEST(Basic, Iter) {
  struct Iter {
    [[= serial_xml::iter{"value", "values"}]] CustomList<int> values;
  };

  Iter obj{{1, 2, 3}};
  ASSERT_EQ(clean_to_xml(obj),
            "<Iter><values><value>1</value><value>2</value><value>3</"
            "value></values></Iter>");
}

TEST(Basic, InvalidNames) {
  struct InvalidName1 {
    int _x;
  } unnamed{3};

  ASSERT_THROW(clean_to_xml(unnamed), std::logic_error);

  struct InvalidName2 {
    int xml_x;
  } unnamed2{3};

  ASSERT_THROW(clean_to_xml(unnamed2), std::logic_error);
}

TEST(Basic, InvalidAnnotationCombinations) {
  struct InvalidCombination1 {
    [[ = serial_xml::unpack, = serial_xml::attribute ]] int x;
  } unnamed{3};

  ASSERT_THROW(clean_to_xml(unnamed), std::logic_error);
}