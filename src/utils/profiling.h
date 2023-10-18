#pragma once

// global
#include <chrono>

namespace prof {

using namespace std::chrono;

using fsec = duration<float>;
using fms = duration<float, std::milli>;

class Counter {
 public:
  Counter() {
    start_ = steady_clock::now();
    end_ = start_;
  }

  // reusable
  void Start() { start_ = steady_clock::now(); }
  void End() { end_ = steady_clock::now(); }

  template <typename T>
  auto GetTime() const {
    return duration_cast<T>(end_ - start_).count();
  }

  template <typename T>
  auto GetElapsed() const {
    return duration_cast<T>(steady_clock::now() - start_).count();
  }

 private:
  steady_clock::time_point start_;
  steady_clock::time_point end_;
};

}  // namespace prof
