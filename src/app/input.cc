#include "input.h"

// deps
#include <GLFW/glfw3.h>
// local
#include "app/parameters.h"
#include "global.h"

namespace input {

namespace {

GLFWwindow *gWindow = nullptr;

// enum to glfw key code, lookup tables
static constexpr std::array<int, Kbm::kTotal> gKbmToGlfw{
    GLFW_MOUSE_BUTTON_1,
    GLFW_MOUSE_BUTTON_2,
    GLFW_MOUSE_BUTTON_3,
    GLFW_MOUSE_BUTTON_4,
    GLFW_MOUSE_BUTTON_5,
    GLFW_MOUSE_BUTTON_6,
    GLFW_MOUSE_BUTTON_7,
    GLFW_MOUSE_BUTTON_8,
    GLFW_KEY_SPACE,
    GLFW_KEY_APOSTROPHE,
    GLFW_KEY_COMMA,
    GLFW_KEY_MINUS,
    GLFW_KEY_PERIOD,
    GLFW_KEY_SLASH,
    GLFW_KEY_0,
    GLFW_KEY_1,
    GLFW_KEY_2,
    GLFW_KEY_3,
    GLFW_KEY_4,
    GLFW_KEY_5,
    GLFW_KEY_6,
    GLFW_KEY_7,
    GLFW_KEY_8,
    GLFW_KEY_9,
    GLFW_KEY_SEMICOLON,
    GLFW_KEY_EQUAL,
    GLFW_KEY_A,
    GLFW_KEY_B,
    GLFW_KEY_C,
    GLFW_KEY_D,
    GLFW_KEY_E,
    GLFW_KEY_F,
    GLFW_KEY_G,
    GLFW_KEY_H,
    GLFW_KEY_I,
    GLFW_KEY_J,
    GLFW_KEY_K,
    GLFW_KEY_L,
    GLFW_KEY_M,
    GLFW_KEY_N,
    GLFW_KEY_O,
    GLFW_KEY_P,
    GLFW_KEY_Q,
    GLFW_KEY_R,
    GLFW_KEY_S,
    GLFW_KEY_T,
    GLFW_KEY_U,
    GLFW_KEY_V,
    GLFW_KEY_W,
    GLFW_KEY_X,
    GLFW_KEY_Y,
    GLFW_KEY_Z,
    GLFW_KEY_LEFT_BRACKET,
    GLFW_KEY_BACKSLASH,
    GLFW_KEY_RIGHT_BRACKET,
    GLFW_KEY_GRAVE_ACCENT,
    GLFW_KEY_ESCAPE,
    GLFW_KEY_ENTER,
    GLFW_KEY_TAB,
    GLFW_KEY_BACKSPACE,
    GLFW_KEY_INSERT,
    GLFW_KEY_DELETE,
    GLFW_KEY_RIGHT,
    GLFW_KEY_LEFT,
    GLFW_KEY_DOWN,
    GLFW_KEY_UP,
    GLFW_KEY_PAGE_UP,
    GLFW_KEY_PAGE_DOWN,
    GLFW_KEY_HOME,
    GLFW_KEY_END,
    GLFW_KEY_CAPS_LOCK,
    GLFW_KEY_SCROLL_LOCK,
    GLFW_KEY_NUM_LOCK,
    GLFW_KEY_PRINT_SCREEN,
    GLFW_KEY_PAUSE,
    GLFW_KEY_F1,
    GLFW_KEY_F2,
    GLFW_KEY_F3,
    GLFW_KEY_F4,
    GLFW_KEY_F5,
    GLFW_KEY_F6,
    GLFW_KEY_F7,
    GLFW_KEY_F8,
    GLFW_KEY_F9,
    GLFW_KEY_F10,
    GLFW_KEY_F11,
    GLFW_KEY_F12,
    GLFW_KEY_F13,
    GLFW_KEY_F14,
    GLFW_KEY_F15,
    GLFW_KEY_F16,
    GLFW_KEY_F17,
    GLFW_KEY_F18,
    GLFW_KEY_F19,
    GLFW_KEY_F20,
    GLFW_KEY_F21,
    GLFW_KEY_F22,
    GLFW_KEY_F23,
    GLFW_KEY_F24,
    GLFW_KEY_F25,
    GLFW_KEY_KP_0,
    GLFW_KEY_KP_1,
    GLFW_KEY_KP_2,
    GLFW_KEY_KP_3,
    GLFW_KEY_KP_4,
    GLFW_KEY_KP_5,
    GLFW_KEY_KP_6,
    GLFW_KEY_KP_7,
    GLFW_KEY_KP_8,
    GLFW_KEY_KP_9,
    GLFW_KEY_KP_DECIMAL,
    GLFW_KEY_KP_DIVIDE,
    GLFW_KEY_KP_MULTIPLY,
    GLFW_KEY_KP_SUBTRACT,
    GLFW_KEY_KP_ADD,
    GLFW_KEY_KP_ENTER,
    GLFW_KEY_KP_EQUAL,
    GLFW_KEY_LEFT_SHIFT,
    GLFW_KEY_LEFT_CONTROL,
    GLFW_KEY_LEFT_ALT,
    GLFW_KEY_LEFT_SUPER,
    GLFW_KEY_RIGHT_SHIFT,
    GLFW_KEY_RIGHT_CONTROL,
    GLFW_KEY_RIGHT_ALT,
    GLFW_KEY_RIGHT_SUPER,
    GLFW_KEY_MENU,
};

// glfw key code to enum, unordered map has search O(1)
MiUnMap<int, uint8_t> gGlfwToKbm = ([]() {
  MiUnMap<int, uint8_t> map;
  map.reserve(Kbm::kTotal);
  for (uint8_t kbm_code = 0; kbm_code < Kbm::kTotal; ++kbm_code) {
    int glfw_code = gKbmToGlfw[kbm_code];
    map.try_emplace(glfw_code, kbm_code);
  }
  return map;
})();

MiVector<Signal> gSignals;

std::array<bool, Mouse::kTotal> gMouseState{};
uint8_t gModsState = 0;

glm::vec2 gMousePos{0.0f};
glm::vec2 gMouseOffset{0.0f};
glm::vec2 gMouseLast{glm::vec2(app::glfw.window_size) * 0.5f};
float gScrollOffset = 0.0f;

}  // namespace

// async GLFW callbacks
void CallbackKeyboardPress(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {
  if (app::imgui.want_text_input) return;

  uint8_t kbm_code = gGlfwToKbm.at(key);
  gSignals.emplace_back(global::gState, kbm_code, action, mods);
  gModsState = static_cast<uint8_t>(mods);
}

void CallbackMousePress(GLFWwindow *window, int button, int action, int mods) {
  if (app::imgui.cursor_over_ui) return;

  uint8_t kbm_code = gGlfwToKbm.at(button);
  gSignals.emplace_back(global::gState, kbm_code, action, mods);
}

// the position is relative to the top-left corner
void CallbackMouseMove(GLFWwindow *window, double pos_x, double pos_y) {
  // top-left (GLFW) -> bottom-left (OpenGL)
  gMousePos = glm::vec2(pos_x, app::glfw.window_size.y - pos_y);
  // show the mouse pointer when UI is active
  if (!app::imgui.ui_active) {
    // not reversed for Camera
    gMouseOffset += glm::vec2(static_cast<float>(pos_x) - gMouseLast.x,
                              static_cast<float>(pos_y) - gMouseLast.y);
    gMouseState[Mouse::kMove] = true;
  }
  // position after the offset calculation
  gMouseLast = glm::vec2(static_cast<float>(pos_x), static_cast<float>(pos_y));
}

void CallbackMouseWheel(GLFWwindow *window, double offset_x, double offset_y) {
  if (app::imgui.ui_active) return;

  gScrollOffset += static_cast<float>(offset_y);
  gMouseState[Mouse::kScroll] = true;
}

void SetWindow(GLFWwindow *window) {
  gWindow = window;
  glfwSetKeyCallback(gWindow, CallbackKeyboardPress);
  glfwSetMouseButtonCallback(gWindow, CallbackMousePress);
  glfwSetCursorPosCallback(gWindow, CallbackMouseMove);
  glfwSetScrollCallback(gWindow, CallbackMouseWheel);
}

MiVector<Signal> &GetSignals() { return gSignals; }

bool GetKbmState(uint8_t key) { return glfwGetKey(gWindow, gKbmToGlfw[key]); }
uint8_t GetModsState() { return gModsState; }

bool GetMouseState(uint8_t mouse_code) { return gMouseState[mouse_code]; }

glm::vec2 GetMousePos() { return gMousePos; }

glm::vec2 GetMouseOffset() { return gMouseOffset; }
float GetScrollOffset() { return gScrollOffset; }

void ResetMouse() {
  gMouseOffset = glm::vec2(0.0f);
  gMouseState[Mouse::kMove] = false;
}
void ResetScroll() {
  gScrollOffset = 0.0f;
  gMouseState[Mouse::kScroll] = false;
}

}  // namespace input