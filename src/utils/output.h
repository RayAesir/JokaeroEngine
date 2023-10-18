#pragma once

// deps
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
// local
#include "math/collision_types.h"

// vec[2,3,4](x, y, z) style
template <glm::length_t S, typename T, glm::qualifier P>
struct fmt::formatter<glm::vec<S, T, P>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const glm::vec<S, T, P>& vec, FormatContext& ctx) {
    auto&& out = ctx.out();
    constexpr char len[3]{'2', '3', '4'};
    out = fmt::format_to(out, "vec{}", len[S - 2]);
    *out = '(';
    for (int i = 0; i < S; ++i) {
      ctx.advance_to(out);
      out = fmt::formatter<T>::format(vec[i], ctx);
      if (i < (S - 1)) out = fmt::format_to(out, ", ");
    }
    *out = ')';
    return out;
  }
};

// force quat(w, x, y, z) style
// GLM_FORCE_QUAT_DATA_WXYZ can be used too
template <typename T, glm::qualifier P>
struct fmt::formatter<glm::qua<T, P>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const glm::qua<T, P>& qua, FormatContext& ctx) {
    auto&& out = ctx.out();
    out = fmt::format_to(out, "quat");
    *out = '(';
    ctx.advance_to(out);
    out = fmt::formatter<T>::format(qua.w, ctx);
    out = fmt::format_to(out, ", ");
    ctx.advance_to(out);
    out = fmt::formatter<T>::format(qua.x, ctx);
    out = fmt::format_to(out, ", ");
    ctx.advance_to(out);
    out = fmt::formatter<T>::format(qua.y, ctx);
    out = fmt::format_to(out, ", ");
    ctx.advance_to(out);
    out = fmt::formatter<T>::format(qua.z, ctx);
    *out = ')';
    return out;
  }
};

// GLM is column major
template <glm::length_t C, glm::length_t R, typename T, glm::qualifier P>
struct fmt::formatter<glm::mat<C, R, T, P>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const glm::mat<C, R, T, P>& mat, FormatContext& ctx) {
    auto&& out = ctx.out();
    constexpr char len[3]{'2', '3', '4'};
    out = fmt::format_to(out, "mat{}x{}\n", len[C - 2], len[R - 2]);
    for (int r = 0; r < R; ++r) {
      *out = '[';
      for (int c = 0; c < C; ++c) {
        ctx.advance_to(out);
        out = fmt::formatter<T>::format(mat[c][r], ctx);
        if (c < (C - 1)) out = fmt::format_to(out, ", ");
      }
      *out = ']';
      *out = '\n';
    }
    return out;
  }
};

// use without format args, fmt::print("{:.3f}", ray) -> error
std::ostream& operator<<(std::ostream& os, const Ray& ray);
std::ostream& operator<<(std::ostream& os, const Plane& plane);
std::ostream& operator<<(std::ostream& os, const Sphere& sphere);
std::ostream& operator<<(std::ostream& os, const AABB& aabb);
std::ostream& operator<<(std::ostream& os, const OBB& obb);

template <>
struct fmt::formatter<Ray> : ostream_formatter {};
template <>
struct fmt::formatter<Plane> : ostream_formatter {};
template <>
struct fmt::formatter<Sphere> : ostream_formatter {};
template <>
struct fmt::formatter<AABB> : ostream_formatter {};
template <>
struct fmt::formatter<OBB> : ostream_formatter {};