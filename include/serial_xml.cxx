export module serial_xml;

import std;

namespace serial_xml {
export template <std::size_t N> struct name {
  char value[N];

  constexpr name(const char (&str)[N]) {
    for (std::size_t i = 0; i < N; ++i)
      value[i] = str[i];
  }

  static constexpr bool is_empty() { return N == 1; }
};

struct skip_ {};
export constexpr const skip_ skip = skip_{};

struct attribute_ {};
export constexpr const attribute_ attribute = attribute_{};

struct no_iter_ {};
export constexpr const no_iter_ no_iter = no_iter_{};

struct add_ {};
export constexpr const add_ add = add_{};

struct raw_ {};
export constexpr const raw_ raw = raw_{};

export template <std::size_t N> struct format {
  char value[N];

  constexpr format(const char (&str)[N]) {
    for (std::size_t i = 0; i < N; ++i)
      value[i] = str[i];
  }
};

export template <std::size_t N1 = 1, std::size_t N2 = 1> struct unpack {
  char single[N1] = "";
  char multiple[N2] = "";

  constexpr unpack(const char (&s)[N1]) {
    for (std::size_t i = 0; i < N1; ++i)
      single[i] = s[i];
  }
  constexpr unpack(const char (&s)[N1], const char (&m)[N2]) {
    for (std::size_t i = 0; i < N1; ++i)
      single[i] = s[i];
    for (std::size_t i = 0; i < N2; ++i)
      multiple[i] = m[i];
  }
};

template <typename T>
T replace_char(const T &input, char target, const auto &replacement) {
  T result;

  if constexpr (std::is_same_v<std::decay_t<decltype(replacement)>,
                               const char *>) {
    result.reserve(input.size() + std::strlen(replacement) * 4);
  } else {
    result.reserve(input.size() + replacement.size() * 4);
  }

  for (const auto c : input) {
    if (c == target) {
      result.append(replacement);
    } else {
      result.push_back(c);
    }
  }

  return result;
}

void add_escapes(auto &input) {
  input = replace_char(input, '&', "&amp;");
  input = replace_char(input, '<', "&lt;");
  input = replace_char(input, '>', "&gt;");
  input = replace_char(input, '"', "&quot;");
  input = replace_char(input, '\'', "&apos;");
}

template <std::meta::info m> consteval auto get_annotations() {
  static constexpr auto annotations =
      std::define_static_array(std::meta::annotations_of(m));

  bool is_raw = false;
  bool is_skip = false;
  bool is_no_iter = false;
  bool is_add = false;
  bool is_attribute = false;

  std::optional<std::string> custom_format;
  std::optional<std::pair<std::string, std::string>> unpack_names;

  template for (constexpr auto a : annotations) {
    static constexpr auto a_t = std::meta::type_of(a);
    if constexpr (a_t == ^^decltype(::serial_xml::raw)) {
      is_raw = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::skip)) {
      is_skip = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::no_iter)) {
      is_no_iter = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::add)) {
      is_add = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::attribute)) {
      is_attribute = true;
    } else if constexpr (std::meta::template_of(a_t) ==
                         ^^::serial_xml::format) {
      static constexpr auto format_value =
          std::meta::extract<typename[:a_t:]>(a);
      custom_format = std::string(format_value.value);
    } else if constexpr (std::meta::template_of(a_t) ==
                         ^^::serial_xml::unpack) {
      static constexpr auto unpack_value =
          std::meta::extract<typename[:a_t:]>(a);

      std::string multiple_name;
      std::string single_name;

      if constexpr (sizeof(unpack_value.multiple) == 1) {
        if constexpr (std::meta::has_identifier(m)) {
          static constexpr auto temp_name = std::meta::identifier_of(m);
          multiple_name = std::string(temp_name);
        } else {
          multiple_name = "items";
        }
      } else {
        static constexpr auto temp_name = unpack_value.multiple;
        multiple_name = std::string(temp_name);
      }

      if constexpr (sizeof(unpack_value.single) == 1) {
        if constexpr (std::meta::has_identifier(m)) {
          static constexpr auto temp_name = std::meta::identifier_of(m);
          single_name = std::string(temp_name);
        } else {
          single_name = "item";
        }
      } else {
        static constexpr auto temp_name = unpack_value.single;
        single_name = std::string(temp_name);
      }

      unpack_names =
          std::make_pair(std::move(single_name), std::move(multiple_name));
    }
  }

  return std::tuple{
      is_skip,
      is_attribute,
      is_no_iter,
      is_add,
      is_raw,
      (custom_format.has_value()
           ? std::optional(std::define_static_string(custom_format.value()))
           : std::optional<char const *>(std::nullopt)),
      (unpack_names.has_value())
          ? std::optional(
                std::make_pair(std::define_static_string(unpack_names->first),
                               std::define_static_string(unpack_names->second)))
          : std::optional<std::pair<char const *, char const *>>(std::nullopt)};
}

export template <typename T>
  requires(std::is_class_v<T>)
std::string to_xml(const T &value, bool first = true,
                   const std::string &fixed_name = "") {
  std::string result;
  result.reserve(4096); // Arbitrary size to avoid too many reallocations
  if (first) {
    result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  }

  static constexpr auto M = ^^T;

  static constexpr auto annotations =
      std::define_static_array(std::meta::annotations_of(M));

  std::string name;
  if (!fixed_name.empty()) {
    name = fixed_name;
  } else {
    template for (constexpr auto a : annotations) {
      if constexpr (std::meta::template_of(std::meta::type_of(a)) ==
                    ^^::serial_xml::name) {
        static constexpr auto temp_name =
            std::meta::extract<typename[:std::meta::type_of(a):]>(a);
        name = std::string(temp_name.value);
      }
    }
    if (name.empty()) {
      if constexpr (std::meta::has_identifier(M)) {
        static constexpr auto temp_name = std::meta::identifier_of(M);
        name = std::string(temp_name);
      } else {
        name = "object";
      }
    }
  }

  result += std::format("<{}", name);

  std::string body;
  static constexpr auto members =
      std::define_static_array(std::meta::nonstatic_data_members_of(
          M, std::meta::access_context::current()));

  template for (constexpr auto m : members) {
    static constexpr auto m_annotations = get_annotations<m>();

    static constexpr auto is_raw = std::get<0>(m_annotations);
    static constexpr auto is_skip = std::get<1>(m_annotations);
    static constexpr auto is_no_iter = std::get<2>(m_annotations);
    static constexpr auto is_add = std::get<3>(m_annotations);
    static constexpr auto is_attribute = std::get<4>(m_annotations);

    static constexpr auto custom_format = std::get<5>(m_annotations);
    static constexpr auto unpack_names = std::get<6>(m_annotations);

    if constexpr (is_skip) {
      continue;
    }

    if constexpr (std::meta::is_class_type(std::meta::type_of(m)) &&
                  (!std::formattable<typename[:std::meta::type_of(m):], char> ||
                   is_add) &&
                  !is_attribute && !is_no_iter) {
      body += to_xml(value.[:m:], false);
    } else if constexpr (unpack_names.has_value() &&
                         std::meta::is_class_type(std::meta::type_of(m)) &&
                         std::ranges::range<
                             typename[:std::meta::type_of(m):]>) {
      body += std::format("<{}>", unpack_names->second);

      if constexpr (is_attribute &&
                    std::meta::is_class_type(std::meta::type_of(m)) &&
                    !is_no_iter &&
                    !std::formattable<typename[:std::meta::type_of(m):],
                                                                       char>) {
        throw std::logic_error(
            "Cannot use [[serial_xml::attribute]] on a class "
            "type (i.e. a type that is broken down into its "
            "members) without [[serial_xml::no_iter]]. Either add "
            "[[serial_xml::no_iter]], remove [[serial_xml::attribute]], or add "
            "support for std::format.");
      }

      for (const auto &item : value.[:m:]) {
        if constexpr ((!std::formattable<
                           typename[:std::meta::type_of(m):], char> ||
                       is_add) &&
                      !is_attribute && !is_no_iter) {
          body += to_xml<decltype(item)>(item, false,
                                         std::string(unpack_names->first));
        } else {
          std::string val;
          if (custom_format) {
            val = std::format(
                std::dynamic_format(std::format("{{:{}}}", *custom_format)),
                value.[:m:]);
          } else {
            val = std::format("{}", value.[:m:]);
          }
          add_escapes(val);

          body += std::format("<{0}>{1}</{0}>", unpack_names->first, val);
        }
      }
      body += std::format("</{}>", unpack_names->second);
    } else {
      std::string temp_result;

      if (is_attribute) {
        static constexpr auto temp_name = std::meta::identifier_of(m);
        if (custom_format) {
          temp_result = std::format(std::dynamic_format(std::format(
                                        " {{}}=\"{{:{}}}\"", *custom_format)),
                                    temp_name, value.[:m:]);
        } else {
          temp_result = std::format(" {}=\"{}\"", temp_name, value.[:m:]);
        }
        result += temp_result;
      } else {
        std::string converted_temp_name;
        if constexpr (std::meta::has_identifier(m)) {
          static constexpr auto temp_name = std::meta::identifier_of(m);
          converted_temp_name = std::string(temp_name);
        } else {
          converted_temp_name = "field";
        }

        static constexpr auto type = std::meta::type_of(m);

        std::string val;
        if (custom_format) {
          val = std::format(
              std::dynamic_format(std::format("{{:{}}}", *custom_format)),
              value.[:m:]);
        } else {
          val = std::format("{}", value.[:m:]);
        }
        add_escapes(val);

        if (is_raw) {
          temp_result += std::format("{}", val);
        } else {
          temp_result +=
              std::format("<{0}>{1}</{0}>", converted_temp_name, val);
        }

        body += temp_result;
      }
    }
  }

  if (body.empty()) {
    result += "/>";
  } else {
    result += std::format(">{}</{}>", body, name);
  }

  return result;
}

export std::string prettify(const std::string &xml) {
  std::string result;
  result.reserve(static_cast<std::size_t>(xml.size() * 1.5));

  int indent_level = 0;
  auto append_indent = [&](int level) {
    for (int i = 0; i < level; ++i) {
      result += "  ";
    }
  };

  for (std::size_t i = 0; i < xml.size();) {
    if (xml[i] == '<') {
      const std::size_t tag_end = xml.find('>', i);
      if (tag_end == std::string::npos) {
        if (!result.empty() && result.back() != '\n') {
          result += '\n';
        }
        append_indent(indent_level);
        result += xml.substr(i);
        break;
      }

      const std::string_view tag(xml.data() + i, tag_end - i + 1);
      const bool is_closing_tag = tag.size() > 1 && tag[1] == '/';
      const bool is_declaration = tag.size() > 1 && tag[1] == '?';
      const bool is_comment = tag.size() > 3 && tag.substr(1, 3) == "!--";
      const bool is_cdata = tag.size() > 8 && tag.substr(1, 8) == "![CDATA[";
      const bool is_self_closing = !is_closing_tag && !is_declaration &&
                                   !is_comment && !is_cdata && tag.size() > 2 &&
                                   tag[tag.size() - 2] == '/';

      if (is_closing_tag) {
        indent_level = std::max(0, indent_level - 1);
      }

      if (!result.empty() && result.back() != '\n') {
        result += '\n';
      }

      if (!is_declaration && !is_comment && !is_cdata) {
        append_indent(indent_level);
      }

      result.append(tag);

      if (!is_closing_tag && !is_declaration && !is_comment && !is_cdata &&
          !is_self_closing) {
        ++indent_level;
      }

      result += '\n';
      i = tag_end + 1;
      continue;
    }

    const std::size_t text_end = xml.find('<', i);
    const std::size_t length =
        text_end == std::string::npos ? xml.size() - i : text_end - i;
    const std::string_view text(xml.data() + i, length);
    const bool has_content = std::ranges::any_of(
        text, [](unsigned char ch) { return !std::isspace(ch); });

    if (has_content) {
      if (!result.empty() && result.back() != '\n') {
        result += '\n';
      }
      append_indent(indent_level);
      result.append(text);
      result += '\n';
    }

    if (text_end == std::string::npos) {
      break;
    }
    i = text_end;
  }

  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result;
}
} // namespace serial_xml