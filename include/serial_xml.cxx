module;

#include <simd> // GCC 16.1 does not have in this in the std module

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
export constexpr const skip_ skip;

struct attribute_ {};
export constexpr const attribute_ attribute;

struct no_unpack_ {};
export constexpr const no_unpack_ no_unpack;

struct unpack_ {};
export constexpr const unpack_ unpack;

struct no_iter_ {};
export constexpr const no_iter_ no_iter;

struct raw_ {};
export constexpr const raw_ raw;

constexpr bool is_alpha(const char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool is_num(const char c) { return (c >= '0' && c <= '9'); }

constexpr bool is_alnum(const char c) { return is_alpha(c) || is_num(c); }

export template <std::size_t N> struct format {
  char value[N];

  constexpr format(const char (&str)[N]) {
    for (std::size_t i = 0; i < N; ++i)
      value[i] = str[i];
  }
};

export template <std::size_t N1 = 1, std::size_t N2 = 1> struct iter {
  char single[N1] = "";
  char multiple[N2] = "";

  constexpr iter(const char (&s)[N1]) {
    for (std::size_t i = 0; i < N1; ++i)
      single[i] = s[i];
  }
  constexpr iter(const char (&s)[N1], const char (&m)[N2]) {
    for (std::size_t i = 0; i < N1; ++i)
      single[i] = s[i];
    for (std::size_t i = 0; i < N2; ++i)
      multiple[i] = m[i];
  }
};

template <std::meta::info m> consteval auto get_annotations() {
  static constexpr auto annotations =
      std::define_static_array(std::meta::annotations_of(m));

  bool is_attribute = false;
  bool is_no_iter = false;
  bool is_raw = false;
  bool is_skip = false;
  bool is_unpack = std::meta::is_class_type(std::meta::type_of(m)) &&
                   !std::formattable<typename[:std::meta::type_of(m):], char>;

  std::optional<std::string> custom_format;
  std::optional<std::pair<std::string, std::string>> iter_names;

  std::string name;

  template for (constexpr auto a : annotations) {
    static constexpr auto a_t = std::meta::type_of(a);
    if constexpr (a_t == ^^decltype(::serial_xml::attribute)) {
      is_attribute = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::no_iter)) {
      is_no_iter = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::raw)) {
      is_raw = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::skip)) {
      is_skip = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::unpack)) {
      is_unpack = true;
    } else if constexpr (a_t == ^^decltype(::serial_xml::no_unpack)) {
      is_unpack = false;
    } else if constexpr (std::meta::template_of(a_t) ==
                         ^^::serial_xml::format) {
      static constexpr auto format_value =
          std::meta::extract<typename[:a_t:]>(a);
      custom_format = std::string(format_value.value);
    } else if constexpr (std::meta::template_of(a_t) == ^^::serial_xml::iter) {
      static constexpr auto iter_value = std::meta::extract<typename[:a_t:]>(a);

      std::string multiple_name;
      std::string single_name;

      if constexpr (sizeof(iter_value.multiple) == 1) {
        if constexpr (std::meta::has_identifier(m)) {
          static constexpr auto temp_name = std::meta::identifier_of(m);
          multiple_name = std::string(temp_name);
        } else {
          multiple_name = "elements";
        }
      } else {
        static constexpr auto temp_name = iter_value.multiple;
        multiple_name = std::string(temp_name);
      }

      if constexpr (sizeof(iter_value.single) == 1) {
        single_name = "element";
      } else {
        static constexpr auto temp_name = iter_value.single;
        single_name = std::string(temp_name);
      }

      iter_names =
          std::make_pair(std::move(single_name), std::move(multiple_name));
    } else if constexpr (std::meta::template_of(a_t) == ^^::serial_xml::name) {
      static constexpr auto name_value = std::meta::extract<typename[:a_t:]>(a);
      name = std::string(name_value.value);
    }
  }

  if (name.empty()) {
    if constexpr (std::meta::has_identifier(m)) {
      static constexpr auto temp_name = std::meta::identifier_of(m);
      name = std::string(temp_name);
    } else {
      name = "field";
    }
  }

  return std::tuple{
      is_attribute,
      is_no_iter,
      is_raw,
      is_skip,
      is_unpack,
      (custom_format.has_value()
           ? std::optional(std::define_static_string(custom_format.value()))
           : std::optional<char const *>(std::nullopt)),
      (iter_names.has_value())
          ? std::optional(
                std::make_pair(std::define_static_string(iter_names->first),
                               std::define_static_string(iter_names->second)))
          : std::optional<std::pair<char const *, char const *>>(std::nullopt),
      std::define_static_string(name)};
}

template <auto name> consteval auto get_attribute_prefix() {
  std::string prefix;
  prefix.reserve(64);

  prefix += ' ';
  prefix += name;
  prefix += "=\"";

  return std::make_tuple(std::define_static_string(prefix), prefix.size());
}

std::tuple<std::unique_ptr<bool[]>, int>
get_escape_bitmask(std::string_view input) {
  using simd_t = std::simd::vec<char>;

  std::size_t padded_size [[indeterminate]];
  if (input.size() % sizeof(std::int64_t) != 0) {
    padded_size = (input.size() / sizeof(std::int64_t)) + 1;
  } else {
    padded_size = input.size() / sizeof(std::int64_t);
  }

  auto escape_flags = std::make_unique_for_overwrite<bool[]>(
      padded_size * sizeof(std::int64_t) / sizeof(bool));
  std::size_t i [[indeterminate]];
  for (i = 0; i < input.size(); i += simd_t::size()) {
    auto v =
        std::simd::unchecked_load<simd_t>(input.data() + i, simd_t::size());

    auto mask = (v == '<') | (v == '>') | (v == '&') | (v == '"') | (v == '\'');
    const auto &mask_bitset = mask.to_bitset();

    std::memcpy(escape_flags.get() + i, &mask_bitset,
                mask.size() * sizeof(bool));
  }

  const auto remaining = simd_t::size() - (i - input.size());
  auto v = std::simd::partial_load<simd_t>(
      input.data() + (input.size() - remaining), remaining);

  auto mask = (v == '<') | (v == '>') | (v == '&') | (v == '"') | (v == '\'');
  const auto &mask_bitset = mask.to_bitset();

  std::memcpy(escape_flags.get() + (input.size() - remaining), &mask_bitset,
              remaining * sizeof(bool));

  return std::tuple{std::move(escape_flags), padded_size};
}

int count_escapes(const std::unique_ptr<bool[]> &escape_flags,
                  int padded_size) {
  auto *escape_flags_ptr =
      reinterpret_cast<std::uint64_t *>(escape_flags.get());
  int count = 0;
  for (std::size_t i = 0; i < padded_size; ++i) {
    count += std::popcount(escape_flags_ptr[i]);
  }
  return count;
}

std::size_t copy_with_escapes(char *buf, std::string_view input,
                              std::unique_ptr<bool[]> escape_flags,
                              int padded_size) {
  const char *original_buf = buf;

  auto *escape_flags_ptr =
      reinterpret_cast<std::uint64_t *>(escape_flags.get());

  int last = 0;

  for (std::size_t i = 0; i < padded_size; ++i) {
    while (escape_flags_ptr[i] != 0) {
      const int idx = std::countr_zero(escape_flags_ptr[i]) / sizeof(bool);

      std::memcpy(buf, input.data() + last, idx - last);
      buf += (idx - last);

      last = idx;

      switch (input[last + idx]) {
      case '<':
        std::memcpy(buf, "&lt;", 4);
        buf += 4;
        ++last;
        break;
      case '>':
        std::memcpy(buf, "&gt;", 4);
        buf += 4;
        ++last;
        break;
      case '&':
        std::memcpy(buf, "&amp;", 5);
        buf += 5;
        ++last;
        break;
      case '"':
        std::memcpy(buf, "&quot;", 6);
        buf += 6;
        ++last;
        break;
      case '\'':
        std::memcpy(buf, "&apos;", 6);
        buf += 6;
        ++last;
        break;
      default:
        throw std::logic_error("Unexpected character for escaping");
      }

      escape_flags_ptr[i] &= (escape_flags_ptr[i] - 1);
    }
  }

  std::memcpy(buf, input.data() + last, input.size() - last);

  return (buf + (input.size() - last)) - original_buf;
}

template <char const *name, char const *format>
void add_attribute(std::string &result, const auto &value) {
  using T = std::decay_t<decltype(value)>;
  static constexpr auto m_t = ^^std::decay_t<decltype(value)>;

  static constexpr auto prefix_result = get_attribute_prefix<name>();
  static constexpr auto prefix = std::get<0>(prefix_result);
  static constexpr auto prefix_size = std::get<1>(prefix_result);
  static constexpr auto gen_format = std::string("{:") + format + "}";

  if constexpr (std::is_arithmetic_v<T> && sizeof(T) <= 64 && format == "") {
    if constexpr (std::is_floating_point_v<T>) {
      static constexpr auto format_resize = 311 + prefix_size + 1;

      const auto original_size = result.size();

      result.resize_and_overwrite(
          result.size() + format_resize, [&](char *buf, std::size_t max_size) {
            buf += original_size;

            std::memcpy(buf, prefix, prefix_size);

            buf += prefix_size;

            auto [ptr, _] = std::to_chars(buf, buf + 311, value);

            *ptr = '"';

            return original_size + prefix_size + ((ptr + 1) - buf);
          });
    } else if constexpr (std::is_integral_v<T>) {
      static constexpr auto format_resize = 20 + prefix_size + 1;

      const auto original_size = result.size();

      result.resize_and_overwrite(
          result.size() + format_resize, [&](char *buf, std::size_t max_size) {
            buf += original_size;

            std::memcpy(buf, prefix, prefix_size);

            buf += prefix_size;

            auto [ptr, _] = std::to_chars(buf, buf + 20, value);

            *ptr = '"';

            return original_size + prefix_size + ((ptr + 1) - buf);
          });
    }
  } else if constexpr (m_t == ^^std::string || m_t == ^^std::string_view) {
    auto escape_result = get_escape_bitmask(value);

    auto num_escapes =
        count_escapes(std::get<0>(escape_result), std::get<1>(escape_result));

    const auto original_size = result.size();

    result.resize_and_overwrite(
        result.size() + prefix_size + 1 + value.size() + (num_escapes * 5),
        [&](char *buf, std::size_t max_size) {
          buf += original_size;

          const char *original_buf = buf;

          std::memcpy(buf, prefix, prefix_size);

          buf += prefix_size;

          buf += copy_with_escapes(buf, value,
                                   std::move(std::get<0>(escape_result)),
                                   std::get<1>(escape_result));

          *buf = '"';
          return original_size + (buf + 1) - original_buf;
        });
  } else {
    static constexpr auto gen_format =
        std::define_static_string(std::string("{:") + format + '}');

    std::string buffer [[indeterminate]];
    buffer.reserve(256);
    std::format_to(std::back_inserter(buffer), gen_format, value);

    auto escape_result = get_escape_bitmask(buffer);

    auto num_escapes =
        count_escapes(std::get<0>(escape_result), std::get<1>(escape_result));

    const auto original_size = result.size();

    result.resize_and_overwrite(
        original_size + prefix_size + buffer.size() + 1 + (num_escapes * 5),
        [&](char *buf, std::size_t max_size) {
          buf += original_size;

          const char *original_buf = buf;

          std::memcpy(buf, prefix, prefix_size);

          buf += prefix_size;

          buf += copy_with_escapes(buf, buffer,
                                   std::move(std::get<0>(escape_result)),
                                   std::get<1>(escape_result));

          *buf = '"';

          return original_size + (buf + 1) - original_buf;
        });
  }
}

template <auto name> consteval auto get_tags() {

  std::string opening_tag;
  opening_tag.reserve(64);
  opening_tag += '<';
  opening_tag += name;
  opening_tag += '>';
  std::string closing_tag;
  closing_tag.reserve(64);
  closing_tag += "</";
  closing_tag += name;
  closing_tag += '>';

  return std::tuple{std::define_static_string(opening_tag), opening_tag.size(),
                    std::define_static_string(closing_tag), closing_tag.size()};
}

template <char const *name, char const *format>
void add_child(std::string &result, const auto &value) {
  using T = typename std::decay_t<decltype(value)>;
  static constexpr auto m_t = ^^T;

  static constexpr auto tags = get_tags<name>();
  static constexpr auto opening_tag = std::get<0>(tags);
  static constexpr auto opening_tag_size = std::get<1>(tags);
  static constexpr auto closing_tag = std::get<2>(tags);
  static constexpr auto closing_tag_size = std::get<3>(tags);
  static constexpr auto combined_size = opening_tag_size + closing_tag_size;

  const auto original_size = result.size();

  if constexpr (std::is_arithmetic_v<T> && sizeof(T) <= 64 && format == "") {
    if constexpr (std::is_floating_point_v<T>) {
      static constexpr auto format_resize = 311 + combined_size;
      result.resize_and_overwrite(
          original_size + format_resize, [&](char *buf, std::size_t max_size) {
            buf += original_size;

            std::memcpy(buf, opening_tag, opening_tag_size);

            buf += opening_tag_size;

            auto [ptr, _] = std::to_chars(buf, buf + 311, value);

            std::memcpy(ptr, closing_tag, closing_tag_size);

            return (combined_size + (ptr - buf));
          });
    } else if constexpr (std::is_integral_v<T>) {
      static constexpr auto format_resize = 20 + combined_size;

      result.resize_and_overwrite(
          original_size + format_resize, [&](char *buf, std::size_t max_size) {
            buf += original_size;

            std::memcpy(buf, opening_tag, opening_tag_size);

            buf += opening_tag_size;

            auto [ptr, _] = std::to_chars(buf, buf + 20, value);

            std::memcpy(ptr, closing_tag, closing_tag_size);

            return (combined_size + (ptr - buf));
          });
    }
  } else if constexpr (m_t == ^^std::string || m_t == ^^std::string_view) {
    auto escape_result = get_escape_bitmask(value);

    auto num_escapes =
        count_escapes(std::get<0>(escape_result), std::get<1>(escape_result));

    result.resize_and_overwrite(
        original_size + combined_size + value.size() + (num_escapes * 5),
        [&](char *buf, std::size_t max_size) {
          const auto original_buf = buf;

          buf += original_size;

          std::memcpy(buf, opening_tag, opening_tag_size);

          buf += opening_tag_size;

          buf += copy_with_escapes(buf, value,
                                   std::move(std::get<0>(escape_result)),
                                   std::get<1>(escape_result));

          std::memcpy(buf, closing_tag, closing_tag_size);

          return (buf + closing_tag_size) - original_buf;
        });
  } else {
    static constexpr auto gen_format =
        std::define_static_string(std::string("{:") + format + '}');

    std::string buffer [[indeterminate]];
    buffer.reserve(256);
    std::format_to(std::back_inserter(buffer), gen_format, value);

    auto escape_result = get_escape_bitmask(buffer);

    auto num_escapes =
        count_escapes(std::get<0>(escape_result), std::get<1>(escape_result));

    result.resize_and_overwrite(
        original_size + combined_size + buffer.size() + (num_escapes * 5),
        [&](char *buf, std::size_t max_size) {
          const auto original_buf = buf;

          buf += original_size;

          std::memcpy(buf, opening_tag, opening_tag_size);

          buf += opening_tag_size;

          buf += copy_with_escapes(buf, buffer,
                                   std::move(std::get<0>(escape_result)),
                                   std::get<1>(escape_result));

          std::memcpy(buf, closing_tag, closing_tag_size);

          return (buf + closing_tag_size) - original_buf;
        });
  }
}

template <bool is_attribute, bool is_no_iter, char const *name,
          char const *format>
bool handle_stl(std::string &result, std::string &body, const auto &value) {
  using T = std::decay_t<decltype(value)>;

  static constexpr auto m_t = std::meta::template_of(std::meta::dealias(^^T));

  if constexpr (!is_attribute) {
    if constexpr (!is_no_iter &&
                  (m_t == ^^std::vector || m_t == ^^std::array ||
                   m_t == ^^std::inplace_vector || m_t == ^^std::deque ||
                   m_t == ^^std::forward_list || m_t == ^^std::span ||
                   m_t == ^^std::valarray || m_t == ^^std::set ||
                   m_t == ^^std::unordered_set || m_t == ^^std::multiset ||
                   m_t == ^^std::unordered_multiset)) {
      static constexpr auto tags = get_tags<name>();
      static constexpr auto start = std::get<0>(tags);
      static constexpr auto end = std::get<2>(tags);
      body += start;

      static constexpr auto single_name = std::define_static_string("element");
      static constexpr auto item_m_t = std::meta::dealias(^^decltype(value[0]));

      for (const auto &item : value) {
        add_child<single_name, format>(body, item);
      }
      body += end;

      return true;
    }
  }

  if constexpr (m_t == ^^std::optional) {
    if (!value.has_value()) {
      return true;
    }

    if constexpr (is_attribute) {
      add_attribute<name, format>(result, value.value());
    } else {
      add_child<name, format>(body, value.value());
    }

    return true;
  }

  return false;
}

template <typename T>
  requires(std::is_class_v<T>)
void to_xml(const T &value, std::string &result, bool first,
            const std::string &fixed_name = "") {
  if (first) {
    result.reserve(4096);
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

    static constexpr auto is_attribute = std::get<0>(m_annotations);
    static constexpr auto is_no_iter = std::get<1>(m_annotations);
    static constexpr auto is_raw = std::get<2>(m_annotations);
    static constexpr auto is_skip = std::get<3>(m_annotations);
    static constexpr auto is_unpack = std::get<4>(m_annotations);

    static constexpr auto custom_format = std::get<5>(m_annotations);
    static constexpr auto iter_names = std::get<6>(m_annotations);

    static constexpr auto name = std::get<7>(m_annotations);

    static constexpr auto view_name = std::string_view(name);

    if constexpr (is_skip) {
      continue;
    }

    if constexpr (!(std::ranges::all_of(view_name,
                                        [](char c) {
                                          return is_alnum(c) || c == '_' ||
                                                 c == '-' || c == '.';
                                        })) ||
                  view_name[0] == '_' || view_name[0] == '-' ||
                  view_name[0] == '.' || is_num(view_name[0]) ||
                  std::ranges::starts_with(view_name,
                                           std::string_view("xml"))) {
      throw std::logic_error(std::format("Invalid XML name: '{}'", view_name));
    } else if constexpr (is_unpack && is_attribute) {
      throw std::logic_error(std::format(
          "Cannot use [[serial_xml::unpack]] and [[serial_xml::attribute]] "
          "together. Either remove [[serial_xml::unpack]] or "
          "[[serial_xml::attribute]]. If you have not applied "
          "[[serial_xml::unpack]], please add [[serial_xml::no_unpack]] to the "
          "member with the name '{}'",
          view_name));
    } else {

      bool handled_stl = false;

      if constexpr (!iter_names.has_value() &&
                    std::meta::has_parent(std::meta::type_of(m))) {
        if constexpr (std::meta::parent_of(std::meta::type_of(m)) ==
                      std::meta::parent_of(^^std::optional)) {
          handled_stl =
              handle_stl<is_attribute, is_no_iter, name,
                         (custom_format) ? *custom_format
                                         : std::define_static_string("")>(
                  result, body, value.[:m:]);
        }
      }
      if (!handled_stl) {
        if constexpr (is_unpack) {
          to_xml(value.[:m:], body, false, name);
        } else if constexpr (iter_names.has_value() &&
                             std::meta::is_class_type(std::meta::type_of(m)) &&
                             std::ranges::range<
                                 typename[:std::meta::type_of(m):]>) {
          body += std::format("<{}>", iter_names->second);

          if constexpr (is_attribute &&
                        std::meta::is_class_type(std::meta::type_of(m)) &&
                        is_unpack &&
                        !std::formattable<
                            typename[:std::meta::type_of(m):], char>) {
            throw std::logic_error(
                "Cannot use [[serial_xml::attribute]] on a class "
                "type (i.e. a type that is broken down into its "
                "members) without [[serial_xml::no_unpack]]. Either add "
                "[[serial_xml::no_unpack]], remove [[serial_xml::attribute]], "
                "or "
                "add support for std::format.");
          }

          for (const auto &item : value.[:m:]) {
            if constexpr (is_unpack) {
              to_xml(item, body, false, iter_names->first);
            } else {
              if constexpr (custom_format.has_value()) {
                add_child<iter_names->first, *custom_format>(body, item);
              } else {
                add_child<iter_names->first, std::define_static_string("")>(
                    body, item);
              }
            }
          }
          body += std::format("</{}>", iter_names->second);
        } else {
          if constexpr (is_attribute) {
            if constexpr (custom_format.has_value()) {
              add_attribute<name, *custom_format>(result, value.[:m:]);
            } else {
              add_attribute<name, std::define_static_string("")>(result,
                                                                 value.[:m:]);
            }
          } else {
            if constexpr (is_raw) {
              body += std::format("{}", value.[:m:]);
            } else {
              if constexpr (custom_format.has_value()) {
                add_child<name, *custom_format>(body, value.[:m:]);
              } else {
                add_child<name, std::define_static_string("")>(body,
                                                               value.[:m:]);
              }
            }
          }
        }
      }
    }
  }

  if (body.empty()) {
    result += "/>";
  } else {
    result += std::format(">{}</{}>", body, name);
  }
}

export template <typename T>
  requires(std::is_class_v<T>)
std::string to_xml(const T &value, bool first = true) {
  std::string result;
  to_xml(value, result, first);
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