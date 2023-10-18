#include "parameters.h"

// deps
#include <glad/glad.h>
// local
#include "app/ini.h"

namespace app {

namespace types {

Glfw::Glfw() {
  monitor_name = "Default Monitor";
  window_scale = glm::vec2(1.0f);
  videomode_size = glm::ivec2(1920, 1080);
  refresh_rate = 60;

  title = "Render Engine";
  fullscreen = false;
  cursor_state = 0;
  cursor_entered = true;

  vsync = false;
  window_size = glm::ivec2(1920, 1080);
  samples = 0;

  frame_count = 0;
  time = 0.0f;
  delta_time = 0.0f;
  sin_half_speed = 0.0f;
  sin_full_speed = 0.0f;
}

OpenGl::OpenGl() {
  window_mode = {
      .label = "Window Mode",                        //
      .list = {{0, "Fullscreen"}, {1, "Windowed"}},  //
      .current = 1                                   //
  };

  resolution = {
      .label = "Resolution",  //
      .list =
          {
              {glm::ivec2(1024, 576), "1024x576"},    //
              {glm::ivec2(1152, 648), "1152x648"},    //
              {glm::ivec2(1280, 720), "1280x720"},    //
              {glm::ivec2(1366, 768), "1366x768"},    //
              {glm::ivec2(1600, 900), "1600x900"},    //
              {glm::ivec2(1920, 1080), "1920x1080"},  //
              {glm::ivec2(2560, 1440), "2560x1440"},  //
              {glm::ivec2(3840, 2160), "3840x2160"},  //
          },
      .current = 5  //
  };

  framebuffer_size = glm::ivec2(1920, 1080);
  inv_framebuffer_size =
      glm::vec2(glm::vec2(1.0f) / glm::vec2(1920.0f, 1080.0f));
  aspect_ratio = 1920.0f / 1080.0f;
  resizeable = true;

  show_debug = true;
  msg_severity = {
      {
          {"Low##opengl", true, GL_DEBUG_SEVERITY_LOW},                    //
          {"Medium##opengl", true, GL_DEBUG_SEVERITY_MEDIUM},              //
          {"High##opengl", true, GL_DEBUG_SEVERITY_HIGH},                  //
          {"Notification##opengl", true, GL_DEBUG_SEVERITY_NOTIFICATION},  //
          {"All##opengl", true, GL_DONT_CARE},                             //
      },
  };
}

}  // namespace types

namespace set {

void UpdateScreenSize() {
  const glm::ivec2 framebuffer_size = opengl.resolution.Selected().res;
  const glm::vec2 size_float = glm::vec2(framebuffer_size);
  // integer
  opengl.framebuffer_size = framebuffer_size;
  // float
  opengl.inv_framebuffer_size = 1.0f / size_float;
  opengl.aspect_ratio = size_float.x / size_float.y;
}

ini::Description GetIniDescription() {
  ini::Description desc;
  desc.bools = {
      {"Display", "bFullscreen", &glfw.fullscreen},
      {"Display", "bVSync", &glfw.vsync},
      {"Debug", "bShowDebug", &opengl.show_debug},
      {"Display", "bResizeable", &opengl.resizeable},
  };
  desc.ints = {
      {"Display", "iWindowMode", &opengl.window_mode.current, 0, 1},
      {"Display", "iCurrentResolution", &opengl.resolution.current, 0, 7},
  };

  return desc;
}

}  // namespace set

}  // namespace app
