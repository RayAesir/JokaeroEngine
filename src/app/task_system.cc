#include "task_system.h"

// windows
// <GLFW/glfw3native.h> conflicts with glm templates, bug
#define _AMD64_
#include <errhandlingapi.h>
#include <windef.h>
#include <wingdi.h>
// deps
#include <spdlog/spdlog.h>
// global
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
// local
#include "app/parameters.h"
#include "mi_types.h"

template <typename T>
using MiQueue = std::queue<T, std::deque<T, mi_stl_allocator<T>>>;

namespace app {

namespace {

MiVector<std::thread> gWorkers;
std::mutex gMutex;
MiQueue<std::function<void(unsigned int)>> gTasks;
std::atomic<int> gTasksTotal = 0;
std::atomic<bool> gRunning = true;
std::atomic<bool> gPaused = false;
int gSleepDurationMs = 500;
// OpenGL threads
void* gDeviceContext;
MiVector<void*> gRendergingContextWorkers;

// private part
void GetContextHandlersWGL() {
  // query the already created context
  HDC device_context = wglGetCurrentDC();
  HGLRC rendering_context_main = wglGetCurrentContext();
  // create the new context for a OpenGL thread
  MiVector<HGLRC> rendering_context_workers;
  rendering_context_workers.reserve(app::cpu.task_threads);
  for (unsigned int t = 0; t < app::cpu.task_threads; ++t) {
    rendering_context_workers.push_back(wglCreateContext(device_context));
  }
  // share OpenGL objects with main context
  for (const auto& ctx : rendering_context_workers) {
    if (!wglShareLists(rendering_context_main, ctx)) {
      DWORD dw = GetLastError();
      spdlog::error("{}: Unable to share context", __FUNCTION__);
      spdlog::error("GetLastError DWORD: {}", dw);
    }
  }
  // convert to void* to pass further
  gDeviceContext = static_cast<void*>(device_context);
  for (const auto& ctx : rendering_context_workers) {
    gRendergingContextWorkers.push_back(static_cast<void*>(ctx));
  }
}

void MakeCurrentWGL(unsigned int tid) {
  std::scoped_lock lock(gMutex);
  BOOL success =
      wglMakeCurrent(static_cast<HDC>(gDeviceContext),
                     static_cast<HGLRC>(gRendergingContextWorkers[tid]));
  if (success == false) {
    DWORD dw = GetLastError();
    spdlog::error("{}: Can't make OpenGL context current to thread",
                  __FUNCTION__);
    spdlog::error("worker_index: {}", tid);
    spdlog::error("HDC: {}", fmt::ptr(gDeviceContext));
    spdlog::error("HGLRC: {}", fmt::ptr(gRendergingContextWorkers[tid]));
    spdlog::error("GetLastError DWORD: {}", dw);
  }
}

void DeleteContextWGL(unsigned int tid) {
  std::scoped_lock lock(gMutex);
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(static_cast<HGLRC>(gRendergingContextWorkers[tid]));
}

bool PopTask(std::function<void(int)>& task) {
  std::scoped_lock lock(gMutex);
  if (gTasks.empty()) return false;

  task = std::move(gTasks.front());
  gTasks.pop();
  return true;
}

void SleepOrYield() {
  if (gSleepDurationMs) {
    std::this_thread::sleep_for(std::chrono::microseconds(gSleepDurationMs));
  } else {
    std::this_thread::yield();
  }
}

// std::this_thread::get_id() uses
// Windows TEB/TIB that is stored in FS CPU register
// as "current thread ID" at FS:[0x24] which is fast but
// sync access to [thread_id, index] container is slow
// better to pass the index as argument directly
void ExecuteTask(unsigned int tid) {
  MakeCurrentWGL(tid);

  while (gRunning) {
    std::function<void(int)> task;
    if (!gPaused && PopTask(task)) {
      task(tid);
      --gTasksTotal;
    } else {
      SleepOrYield();
    }
  }

  DeleteContextWGL(tid);
}

}  // namespace

namespace init {

void GetCpuCores() {
  app::set::cpu.threads_total = std::thread::hardware_concurrency();
  if (app::cpu.threads_total > 1) {
    // limit by 8 threads (too many OpenGL context)
    app::set::cpu.task_threads = max(8U, app::cpu.threads_total);
  }
}

void CreateWorkers() {
  GetCpuCores();
  GetContextHandlersWGL();

  gWorkers.reserve(app::cpu.task_threads);
  for (unsigned int tid = 0; tid < app::cpu.task_threads; ++tid) {
    gWorkers.emplace_back(ExecuteTask, tid);
  }
}

void DestroyWorkers() {
  task::WaitForTasks();
  gRunning = false;

  for (auto& worker : gWorkers) {
    worker.join();
  }
}

}  // namespace init

namespace task {

void Pause() { gPaused = true; }

void Unpause() { gPaused = false; }

int GetTasksQueued() {
  std::scoped_lock lock(gMutex);
  return static_cast<int>(gTasks.size());
}

int GetTasksRunning() { return gTasksTotal - GetTasksQueued(); }

int GetTasksTotal() { return gTasksTotal; }

void PushTask(std::function<void(int)> task) {
  gTasksTotal++;
  {
    std::scoped_lock lock(gMutex);
    gTasks.push(std::move(task));
  }
}

void WaitForTasks() {
  while (true) {
    if (!gPaused) {
      if (gTasksTotal == 0) break;
    } else {
      if (GetTasksRunning() == 0) break;
    }
    SleepOrYield();
  }
}

}  // namespace task

}  // namespace app
