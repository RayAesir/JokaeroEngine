#include "engine.h"

// deps
#include <spdlog/spdlog.h>
// local
#include "app/input.h"
#include "app/parameters.h"
#include "global.h"

namespace event {

namespace {

// unite keys and mouse buttons
struct EventBehavior {
  enum Enum : bool { kTap = false, kHold = true };
  uint32_t event;
  // tap == false, hold == true
  bool behavior;
};

MiUnMap<uint32_t, EventBehavior> gSignalToEvent;
MiUnMap<uint32_t, uint32_t> gMouseToEvent;
MiVector<CustomEvent> gCustomEvent;

}  // namespace

void Invoke(uint32_t event) { events[event](); }

void SetKey(uint32_t event, uint8_t state, uint8_t kbm_code, uint8_t action,
            uint8_t mods) {
  input::Signal sig{state, kbm_code, action, mods};

  bool behavior = false;
  if (action == input::Action::kHold) {
    behavior = true;
    sig.f.action = input::Action::kPress;
  }

  auto [it, success] =
      gSignalToEvent.try_emplace(sig.CastUnique(), event, behavior);
  if (success == false) {
    spdlog::error("{}: Key code '{}' already used in state '{}'", __FUNCTION__,
                  kbm_code, state);
  }
}

void SetMouse(uint32_t event, uint8_t state, uint8_t mouse_code, uint8_t mods) {
  input::Signal sig{state, mouse_code, input::Action::kPress, mods};

  auto [it, success] = gMouseToEvent.try_emplace(sig.CastUnique(), event);
  if (success == false) {
    spdlog::error("{}: Mouse code '{}' already used in state '{}'",
                  __FUNCTION__, mouse_code, state);
  }
}

void SetKeybinds() {
  using namespace input;
  using enum Event::Enum;

  SetKey(kCloseWindow, State::kLoading, Kbm::kEscape, Action::kPress);
  SetKey(kToggleInputMode, State::kLoading, Kbm::kTab, Action::kPress);

  SetKey(kCloseWindow, State::kScene, Kbm::kEscape, Action::kPress);
  SetKey(kToggleInputMode, State::kScene, Kbm::kTab, Action::kPress);
  SetKey(kToggleSettingsWindow, State::kScene, Kbm::k1, Action::kPress);
  SetKey(kToggleActiveWindow, State::kScene, Kbm::k2, Action::kPress);
  SetKey(kToggleResourcesWindow, State::kScene, Kbm::k3, Action::kPress);
  SetKey(kToggleDemoWindow, State::kScene, Kbm::k4, Action::kPress);

  SetKey(kSelectEntity, State::kScene, Kbm::kLMB, Action::kPress);
  SetKey(kSelectedObjectContextMenu, State::kScene, Kbm::kRMB, Action::kPress);
  SetKey(kToggleGuizmo, State::kScene, Kbm::kG, Action::kPress);

  SetMouse(kMouseMove, State::kScene, Mouse::kMove);
  SetMouse(kScrollZoom, State::kScene, Mouse::kScroll);

  SetKey(kMoveRight, State::kScene, Kbm::kD, Action::kHold);
  SetKey(kMoveLeft, State::kScene, Kbm::kA, Action::kHold);
  SetKey(kMoveUp, State::kScene, Kbm::kSpace, Action::kHold);
  SetKey(kMoveDown, State::kScene, Kbm::kLeftControl, Action::kHold);
  SetKey(kMoveForward, State::kScene, Kbm::kW, Action::kHold);
  SetKey(kMoveBackward, State::kScene, Kbm::kS, Action::kHold);
  SetKey(kRollLeft, State::kScene, Kbm::kQ, Action::kHold);
  SetKey(kRollRight, State::kScene, Kbm::kE, Action::kHold);
  SetKey(kModsRun, State::kScene, Kbm::kLeftShift, Action::kHold);
  SetKey(kResetCamera, State::kScene, Kbm::kR, Action::kPress);

  SetKey(kRandomPunch, State::kScene, Kbm::kV, Action::kPress);
  SetKey(kRush, State::kScene, Kbm::kF, Action::kPress);
}

bool ProcessSignal(input::Signal sig) {
  auto use_mods = gSignalToEvent.find(sig.CastUnique());
  if (use_mods == gSignalToEvent.end()) {
    sig.f.mods = 0;
    auto no_mods = gSignalToEvent.find(sig.CastUnique());
    // not used key or mod
    if (no_mods == gSignalToEvent.end()) return true;

    EventBehavior eb = no_mods->second;
    if (eb.behavior == EventBehavior::kHold) {
      // chord/mod hold, release
      if (input::GetKbmState(sig.f.kbm_code) == false) return true;
      // chord/mod hold, press
      Invoke(eb.event);
      return false;
    }
    // single tap
    Invoke(eb.event);
    return true;
  }

  EventBehavior eb = use_mods->second;
  if (eb.behavior != EventBehavior::kHold) {
    // single tap, chord tap
    Invoke(eb.event);
    return true;
  }

  // single hold, release
  if (input::GetKbmState(sig.f.kbm_code) == false) return true;
  // single hold, press
  Invoke(eb.event);
  return false;
}

void ProcessInput() {
  std::erase_if(input::GetSignals(), ProcessSignal);

  // mouse emits multiple signals per frame
  // set state and process just once (offsets cumulative)
  for (uint8_t code = 0; code < input::Mouse::kTotal; ++code) {
    input::Signal check{
        global::gState,              //
        code,                        //
        input::GetMouseState(code),  //
        input::GetModsState()        //
    };
    // Scroll/MouseMove with Modifier key
    auto use_mods = gMouseToEvent.find(check.CastUnique());
    if (use_mods != gMouseToEvent.end()) {
      Invoke(use_mods->second);
    } else {
      // running with Shift and use MouseMove
      check.f.mods = 0;
      auto no_mods = gMouseToEvent.find(check.CastUnique());
      if (no_mods != gMouseToEvent.end()) {
        Invoke(no_mods->second);
      }
    }
  }

  input::ResetMouse();
  input::ResetScroll();
}

void PushCustomEvent(CustomEvent&& custom_event) {
  gCustomEvent.push_back(std::move(custom_event));
}

void ProcessCustomEvents() {
  std::erase_if(gCustomEvent, [](CustomEvent& ev) { return ev(); });
}

}  // namespace event