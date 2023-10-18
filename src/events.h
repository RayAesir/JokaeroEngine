#pragma once

// deps
#include <sigslot/signal.hpp>
// global
#include <array>
#include <functional>
// local
#include "mi_types.h"

struct Event {
  enum Enum : uint32_t {
    kCloseWindow,
    kToggleInputMode,
    kSetResizeableWindow,
    kSetWindowMode,
    kToggleOpenGlDebug,
    kApplyOpenGlMessageSeverity,

    kToggleSettingsWindow,
    kToggleActiveWindow,
    kToggleResourcesWindow,
    kToggleDemoWindow,
    kToggleGuizmo,

    kMouseMove,
    kScrollZoom,
    kMoveRight,
    kMoveLeft,
    kMoveUp,
    kMoveDown,
    kMoveForward,
    kMoveBackward,
    kRollLeft,
    kRollRight,
    kModsRun,
    kResetCamera,

    kRandomPunch,
    kRush,

    kApplyTextureSettings,

    kReloadShaders,
    kApplyResolution,
    kApplyDirShadowmapResolution,
    kApplyPointShadowmapResolution,
    kUpdateShadowsPoisson,
    kGenerateBRDFLut,

    kUpdateFx,
    kResetFx,

    kRequestTexturesCount,

    kSelectEntity,
    kSelectedObjectContextMenu,

    kUpdateParticlesInstances,
    kTotal,
  };
};

namespace event {

inline std::array<sigslot::signal<>, Event::kTotal> events;

// task:
// 1. create and bind events to new object
// 2. RAII connect/disconnect
// 3. plain custom constructors in derived classes
// 4. minimum typing (DRY)

// choices: std::function or MemFn wrapper
// reasons not to use std::function:
// construction/copy/calling overhead, even for non-capture lambda
// https://artificial-mind.net/blog/2019/09/07/std-function-performance

// can't simply use std::function<void()>([this](){...}) because
// object needs to be alive (move -> crash, copy -> incorrect 'this')
// therefore passing the object pointer [](T* self){self->Connect(...);}
// got ugly syntax, no automatization { init_ = [](){}; init_(); }

template <class T>
class AutoCall {
 public:
  AutoCall(void (T::*init)(), T* self) : init_(init) { operator()(self); }

  void operator()(T* self) { return (self->*init_)(); }

 private:
  void (T::*init_)();
};

// don't want v-table, CRTP
template <typename T>
class Base {
 public:
  Base(void (T::*init)(), T* self) : fn_(init, self) {}
  // no copy/move connections to disconnect automatically (RAII)
  Base(const Base& o) : fn_(o.fn_) { fn_(static_cast<T*>(this)); }
  Base& operator=(const Base& o) {
    if (o != *this) {
      fn_ = o.fn_;
      fn_(static_cast<T*>(this));
    }
    return *this;
  }
  // MemFn is simple pointer, copy
  Base(Base&& o) noexcept : fn_(o.fn_) { fn_(static_cast<T*>(this)); }
  Base& operator=(Base&& o) noexcept {
    if (o != *this) {
      fn_ = o.fn_;
      fn_(static_cast<T*>(this));
    }
    return *this;
  }

 public:
  MiVector<sigslot::scoped_connection> connections_;

  template <typename... Args>
  void Connect(int id, Args&&... args) {
    connections_.emplace_back(events[id].connect(std::forward<Args>(args)...));
  }

 private:
  AutoCall<T> fn_;
};

void Invoke(uint32_t event);

void SetKey(uint32_t event, uint8_t state, uint8_t kbm_code, uint8_t action,
            uint8_t mods = 0);
void SetMouse(uint32_t event, uint8_t state, uint8_t mouse_code,
              uint8_t mods = 0);

void SetKeybinds();

void ProcessInput();

// functors as custom (timed) events
// operator return true when event expired and should be removed
using CustomEvent = std::function<bool()>;
void PushCustomEvent(CustomEvent&& custom_event);
void ProcessCustomEvents();

}  // namespace event
