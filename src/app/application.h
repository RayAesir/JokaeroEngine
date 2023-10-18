#pragma once

// global
#include <functional>

namespace app {

namespace init {

bool Application();

}

void Run(const std::function<void()>& render);

void CloseWindow();
void ToggleInputMode(bool ui_active);
void SetResizeableWindow(bool resizeable);
void SetWindowMode(bool windowed);
void ToggleOpenGlDebug(bool enable);
void ApplyOpenGlMessageSeverity();

}  // namespace app
