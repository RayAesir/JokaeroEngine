#pragma once

// deps
#include <magic_enum.hpp>
// global
#include <array>

// Google style Enum to strings
template <typename N, typename M>
constexpr N EnumHelper(const M& magic_enum) {
  N temp{};
  // remove kTotal
  for (size_t i = 0; i < magic_enum.size() - 1; ++i) {
    // remove 'k'
    temp[i] = magic_enum[i].data() + 1;
  }
  return temp;
}

// e.g. NamedEnum<MeshType::Enum>::ToStr()
template <typename T>
struct NamedEnum {
  static constexpr auto enum_names = magic_enum::enum_names<T>();
  using Names = std::array<const char*, T::kTotal>;
  static const Names& ToStr() {
    static constexpr Names names = EnumHelper<Names>(enum_names);
    return names;
  }
};
