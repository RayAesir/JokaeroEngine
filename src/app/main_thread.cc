#include "main_thread.h"

// global
#include <atomic>
#include <mutex>
#include <queue>
// local
#include "mi_types.h"

template <typename T>
using MiQueue = std::queue<T, std::deque<T, mi_stl_allocator<T>>>;

namespace app {

namespace main_thread {

namespace {

std::thread::id gId = std::this_thread::get_id();
std::mutex gMutex;
MiQueue<std::packaged_task<void()>> gTasks;
std::atomic<int> gTasksTotal = 0;

bool PopTask(std::packaged_task<void()>& task) {
  std::scoped_lock lock(gMutex);
  if (gTasks.empty()) return false;

  task = std::move(gTasks.front());
  gTasks.pop();
  return true;
}

}  // namespace

bool IsMainThread() {
  // get_id() is very fast
  return (gId == std::this_thread::get_id()) ? true : false;
}

int GetTasksTotal() { return gTasksTotal; }

std::future<void> PushTask(std::packaged_task<void()>& package) {
  gTasksTotal++;
  std::future<void> future = package.get_future();
  {
    std::scoped_lock lock(gMutex);
    gTasks.push(std::move(package));
  }
  return future;
}

void ExecuteTasks() {
  while (gTasksTotal) {
    std::packaged_task<void()> task;
    if (PopTask(task)) {
      task();
      --gTasksTotal;
    }
  }
}

}  // namespace main_thread

}  // namespace app