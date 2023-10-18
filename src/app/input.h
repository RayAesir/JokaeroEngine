#pragma once

// deps
#include <glm/vec2.hpp>
// global
#include <array>
#include <bit>
// local
#include "mi_types.h"
// fwd
struct GLFWwindow;

// main thread only
namespace input {

struct Kbm {
  enum Enum : uint8_t {
    kLMB,
    kRMB,
    kMMB,
    kMB4,
    kMB5,
    kMB6,
    kMB7,
    kMB8,
    kSpace,
    kApostrophe,
    kComma,
    kMinus,
    kPeriod,
    kSlash,
    k0,
    k1,
    k2,
    k3,
    k4,
    k5,
    k6,
    k7,
    k8,
    k9,
    kSemicolon,
    kEqual,
    kA,
    kB,
    kC,
    kD,
    kE,
    kF,
    kG,
    kH,
    kI,
    kJ,
    kK,
    kL,
    kM,
    kN,
    kO,
    kP,
    kQ,
    kR,
    kS,
    kT,
    kU,
    kV,
    kW,
    kX,
    kY,
    kZ,
    kLeftBracket,
    kBackslash,
    kRightBracket,
    kGraveAccent,
    kEscape,
    kEnter,
    kTab,
    kBackspace,
    kInsert,
    kDelete,
    kRight,
    kLeft,
    kDown,
    kUp,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,
    kCapsLock,
    kScrollLock,
    kNumLock,
    kPrintScreen,
    kPause,
    kF1,
    kF2,
    kF3,
    kF4,
    kF5,
    kF6,
    kF7,
    kF8,
    kF9,
    kF10,
    kF11,
    kF12,
    kF13,
    kF14,
    kF15,
    kF16,
    kF17,
    kF18,
    kF19,
    kF20,
    kF21,
    kF22,
    kF23,
    kF24,
    kF25,
    kKp0,
    kKp1,
    kKp2,
    kKp3,
    kKp4,
    kKp5,
    kKp6,
    kKp7,
    kKp8,
    kKp9,
    kKpDecimal,
    kKpDivide,
    kKpMultiply,
    kKpSubtract,
    kKpAdd,
    kKpEnter,
    kKpEqual,
    kLeftShift,
    kLeftControl,
    kLeftAlt,
    kLeftSuper,
    kRightShift,
    kRightControl,
    kRightAlt,
    kRightSuper,
    kMenu,
    kTotal
  };
};

struct Action {
  enum Enum : uint8_t {
    kRelease,  // tap (keyboard and mouse)
    kPress,    // tap (keyboard and mouse)
    kRepeat,   // text input repeat (no mouse)
    kHold      // not present in glfw, poll state
  };
};

struct Mods {
  enum Enum : uint8_t {
    // GLFW
    kShift = (1 << 0),
    kCtrl = (1 << 1),
    kAlt = (1 << 2),
    // probably not so useful
    kSuper = (1 << 3),
    kCaps = (1 << 4),
    kNumLock = (1 << 5)
  };
};

// small struct, pass by value
struct Signal {
  Signal() = default;
  Signal(uint32_t value) { CastFields(value); }
  // GLFW callbacks, int
  Signal(int state, int kbm_code, int action, int mods)
      : f{static_cast<uint8_t>(state),     //
          static_cast<uint8_t>(kbm_code),  //
          static_cast<uint8_t>(action),    //
          static_cast<uint8_t>(mods)} {}

  struct Fields {
    uint8_t state;
    uint8_t kbm_code;
    uint8_t action;
    uint8_t mods;
  } f;

  // type-punning uint32_t <=> uint8_t[4]
  // union: UB, no
  // reinterpret_cast: violate strict aliasing, UB, no
  // std::memcpy: safe, before C++20, endianness, no
  // bitshift: safe but overhead, yes
  // std::bit_cast: safe, C++20, readable, yes

  // uint8_t[4] as POD aggregate struct (aligned)
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0476r2.html
  // as far as both types have the same size and are trivially copyable
  // the cast should be success

  // unique value to find corresponding Event
  // state[0, 255], code[0, 126], action[0, 3], mods[00111111]

  uint32_t CastUnique() { return std::bit_cast<uint32_t>(f); }
  uint32_t ShiftUnique() {
    return (f.mods << 24) | (f.action << 16) | (f.kbm_code << 8) | f.state;
  }
  void CastFields(uint32_t value) { f = std::bit_cast<Fields>(value); }
  void ShiftFields(uint32_t value) {
    f = Fields{.state = static_cast<uint8_t>(value),
               .kbm_code = static_cast<uint8_t>(value >> 8),
               .action = static_cast<uint8_t>(value >> 16),
               .mods = static_cast<uint8_t>(value >> 24)};
  }
};

struct Mouse {
  enum Enum : uint8_t { kMove, kScroll, kTotal };
};

void SetWindow(GLFWwindow* window);

MiVector<Signal>& GetSignals();

// GLFW's event process functions for release and press,
// set the window states and call other callbacks (key, mouse)
bool GetKbmState(uint8_t key);
uint8_t GetModsState();

// mouse emits multiple signals per frame
// set state and process offsets just once
bool GetMouseState(uint8_t mouse_code);

// position is relative to bottom-left (OpenGL)
glm::vec2 GetMousePos();
// offset is relative to top-left, easy Camera control
// offsets are cumulative per frame
glm::vec2 GetMouseOffset();
float GetScrollOffset();

// reset state and offset
void ResetMouse();
void ResetScroll();

}  // namespace input
