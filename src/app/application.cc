#include "application.h"

// clang-format off
// first, ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// guizmo, after imgui
#include <ImGuizmo.h>
// second, function loader GLAD
#include <glad/glad.h>
// third, GLFW
#include <GLFW/glfw3.h>
// clang-format on

// deps
#include <spdlog/spdlog.h>

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
// local
#include "app/input.h"
#include "app/main_thread.h"
#include "app/parameters.h"
#include "app/task_system.h"

namespace app {

namespace {

// glfw
GLFWwindow *gWindow{nullptr};
GLFWmonitor *gMonitor{nullptr};
const GLFWvidmode *gVideomode{nullptr};

// C-style GLFW callbacks
void CallbackWindowScale(GLFWwindow *window, float scale_x, float scale_y) {
  app::set::glfw.window_scale.x = scale_x;
  app::set::glfw.window_scale.y = scale_y;
}

void CallbackFramebufferSize(GLFWwindow *window, int width, int height) {
  app::set::glfw.window_size.x = width;
  app::set::glfw.window_size.y = height;
}

void CallbackOnCursorEnter(GLFWwindow *window, int entered) {
  app::set::glfw.cursor_entered = static_cast<bool>(entered);
}

void CallbackError(int error, const char *description) {
  spdlog::error("GLFW: Error code: {}, Description: {}", error, description);
}

void DebugOpenGL(GLenum source, GLenum type, GLuint id, GLenum severity,
                 GLsizei length, const GLchar *message,
                 const void *user_param) {
  auto source_str = [source]() {
    switch (source) {
      case GL_DEBUG_SOURCE_API:
        return "API";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW SYSTEM";
      case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER COMPILER";
      case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD PARTY";
      case GL_DEBUG_SOURCE_APPLICATION:
        return "APPLICATION";
      case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
      default:
        return "UNKNOWN";
    }
  }();

  auto type_str = [type]() {
    switch (type) {
      case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED_BEHAVIOR";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED_BEHAVIOR";
      case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
      case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
      case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
      case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
      default:
        return "UNKNOWN";
    }
  }();

  auto severity_str = [severity]() {
    switch (severity) {
      case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "NOTIFICATION";
      case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
      case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
      case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
      default:
        return "UNKNOWN";
    }
  }();

  spdlog::info("{}, {}, {}, {}:\n, {}", source_str, type_str, severity_str, id,
               message);
}

}  // namespace

namespace set {

// OpenGL and GPU parameters (for GTX 1080 and RTX 3070Ti)
void GetOpenGlParameters() {
  gpu.vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
  gpu.renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
  // max number of SSBO bindings is 96 (min 8)
  glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &gpu.max_ssbo_bindings);
  // max number of UBO bindings is 84 (min 36)
  glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &gpu.max_ubo_bindings);
  // max size of UBO is 64KB (default 16KB)
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &gpu.max_ubo_block_size);
  // minimal offset alignment between two binding points is 256 bytes
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &gpu.ubo_offset_alignment);
  // x:2^32, y:64k, z:64k
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0,
                  &gpu.max_workgroup_count[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1,
                  &gpu.max_workgroup_count[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2,
                  &gpu.max_workgroup_count[2]);
  // x:1536, y:1024, z:64
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0,
                  &gpu.max_workgroup_size[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1,
                  &gpu.max_workgroup_size[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2,
                  &gpu.max_workgroup_size[2]);
  // 1536
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                &gpu.max_workgroup_invocations);
  // 48KB
  glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,
                &gpu.max_compute_shared_memory_size);
  // 8
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &gpu.max_color_attachments);
  // 8
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &gpu.max_drawbuffers);
  // 32k
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gpu.max_texture_size);
  // 32 by the fragment shader
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gpu.max_texture_units);
  // 32 NVIDIA
  glGetIntegerv(GL_SUBGROUP_SIZE_KHR, &gpu.subgroup_size);
}

}  // namespace set

namespace init {

class CloseApplication {
 public:
  ~CloseApplication() {
    app::init::DestroyWorkers();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(gWindow);
    glfwTerminate();
  }
};

bool Application() {
  // the same as default but without date-time
  spdlog::set_pattern("[%^%l%$] %v");

  // GLFW the first
  if (!glfwInit()) {
    spdlog::error("{}: Failed to initialize GLFW", __FUNCTION__);
    glfwTerminate();
    return false;
  }

  // monitor
  gMonitor = glfwGetPrimaryMonitor();
  app::set::glfw.monitor_name = glfwGetMonitorName(gMonitor);
  glfwGetMonitorContentScale(gMonitor, &app::set::glfw.window_scale.x,
                             &app::set::glfw.window_scale.y);

  // video mode
  gVideomode = glfwGetVideoMode(gMonitor);
  app::set::glfw.videomode_size.x = gVideomode->width;
  app::set::glfw.videomode_size.y = gVideomode->height;
  app::set::glfw.refresh_rate = gVideomode->refreshRate;

  // windowed full screen (borderless full screen)
  // monitor hints
  glfwWindowHint(GLFW_REFRESH_RATE, gVideomode->refreshRate);
  // framebuffer hints
  glfwWindowHint(GLFW_RED_BITS, gVideomode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, gVideomode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, gVideomode->blueBits);

  // window hints
  if (app::opengl.resizeable) {
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  } else {
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  }
  glfwWindowHint(GLFW_CENTER_CURSOR, GL_TRUE);
  glfwWindowHint(GLFW_FOCUSED, GL_TRUE);

  // context hints
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // framebuffer hints
  glfwWindowHint(GLFW_SAMPLES, app::glfw.samples);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

  // create window and OpenGL context
  if (app::glfw.fullscreen) {
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    gWindow = glfwCreateWindow(gVideomode->width, gVideomode->height,
                               app::glfw.title.c_str(), gMonitor, nullptr);
  } else {
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    // gWindow = glfwCreateWindow(app::glfw.window_size.x,
    // app::glfw.window_size.y,
    //                            app::glfw.title.c_str(), nullptr, nullptr);
    gWindow = glfwCreateWindow(app::opengl.framebuffer_size.x,
                               app::opengl.framebuffer_size.y,
                               app::glfw.title.c_str(), nullptr, nullptr);
  }
  if (!gWindow) {
    spdlog::error("{}: Failed to create GLFW window", __FUNCTION__);
    glfwTerminate();
    return false;
  }

  // makes the OpenGL context of the specified window
  // current on the calling thread
  glfwMakeContextCurrent(gWindow);
  // create OpenGL workers
  app::init::CreateWorkers();

  // vertical synchronization
  if (app::glfw.vsync) {
    glfwSwapInterval(1);
  } else {
    glfwSwapInterval(0);
  }

  // callbacks
  glfwSetWindowContentScaleCallback(gWindow, CallbackWindowScale);
  glfwSetFramebufferSizeCallback(gWindow, CallbackFramebufferSize);
  glfwSetCursorEnterCallback(gWindow, CallbackOnCursorEnter);
  glfwSetErrorCallback(CallbackError);
  input::SetWindow(gWindow);

  // hide cursor, 3D camera control
  glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // GLAD manages function pointer for OpenGL, load before we call OpenGL
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    spdlog::error("{}: Failed to initialize GLAD", __FUNCTION__);
    return false;
  }

  // OpenGL GPU parameters after GLAD
  app::set::GetOpenGlParameters();

  // create ImGui's context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // init the ImGui's backends
  ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
  ImGui_ImplOpenGL3_Init("#version 460");

  // default font is ProggyClean.ttf, monospace, size 13
  ImGuiIO io = ImGui::GetIO();
  ImFontConfig cfg;
  cfg.SizePixels = std::ceil(13.0f * app::glfw.window_scale.x);
  io.Fonts->AddFontDefault(&cfg);

  // scale all but font
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(std::ceil(app::glfw.window_scale.x));
  style.FrameRounding = 10.0f;
  ImGui::StyleColorsDark();

  // connect Guizmo
  ImGuiContext *ctx = ImGui::GetCurrentContext();
  ImGuizmo::SetImGuiContext(ctx);
  ImGuizmo::SetOrthographic(false);

  // enable OpenGL debug
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(DebugOpenGL, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                        GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

  // OpenGL's Zd range is [-1.0, 1.0]
  // DirectX, Metal and Vulkan operate with [0.0, 1.0]
  // Engine use the Reversed-Z [1.0, 0.0]
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

  // better cubemaps
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // guard object to close Application
  static CloseApplication close_app;

  return true;
};

}  // namespace init

void Run(const std::function<void()> &render) {
  while (!glfwWindowShouldClose(gWindow)) {
    // check every cycle for tasks
    app::main_thread::ExecuteTasks();

    // actually glfwPollEvents() starts after glfwSwapBuffers()
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // after ImGui::NewFrame()
    ImGuizmo::BeginFrame();
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    float time = static_cast<float>(glfwGetTime());
    ++app::set::glfw.frame_count;
    app::set::glfw.time = time;
    app::set::glfw.delta_time = ImGui::GetIO().DeltaTime;
    app::set::imgui.want_text_input = ImGui::GetIO().WantTextInput;
    app::set::imgui.cursor_over_ui = ImGui::GetIO().WantCaptureMouse;
    app::set::glfw.sin_half_speed = glm::abs(glm::sin(time * 0.5f));
    app::set::glfw.sin_full_speed = glm::abs(glm::sin(time));
    app::set::glfw.cursor_state = glfwGetInputMode(gWindow, GLFW_CURSOR);

    render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(gWindow);
  }
}

void CloseWindow() { glfwSetWindowShouldClose(gWindow, true); }

void ToggleInputMode(bool ui_active) {
  if (ui_active) {
    glfwSetInputMode(gWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(gWindow,
                     static_cast<double>(app::glfw.window_size.x) * 0.5,
                     static_cast<double>(app::glfw.window_size.y) * 0.5);
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  } else {
    ImGui::SetWindowFocus(nullptr);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(gWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
}

void SetResizeableWindow(bool resizeable) {
  glfwSetWindowAttrib(gWindow, GLFW_RESIZABLE, resizeable);
}

void SetWindowMode(bool windowed) {
  if (windowed) {
    glfwSetWindowMonitor(gWindow, nullptr, gVideomode->width / 16,
                         gVideomode->height / 16,
                         app::opengl.framebuffer_size.x,
                         app::opengl.framebuffer_size.y, GLFW_DONT_CARE);
  } else {
    glfwSetWindowMonitor(gWindow, gMonitor, 0, 0, gVideomode->width,
                         gVideomode->height, gVideomode->refreshRate);
  }
}

void ToggleOpenGlDebug(bool enable) {
  if (enable) {
    glEnable(GL_DEBUG_OUTPUT);
  } else {
    glDisable(GL_DEBUG_OUTPUT);
  }
}

void ApplyOpenGlMessageSeverity() {
  for (const auto &msg : app::opengl.msg_severity) {
    constexpr GLsizei all_messages = 0;
    glDebugMessageControl(GL_DONT_CARE,  // all
                          GL_DONT_CARE,  // all
                          msg.severity,  //
                          all_messages,  // all
                          nullptr,       // all
                          msg.enable     //
    );
  }
}

}  // namespace app