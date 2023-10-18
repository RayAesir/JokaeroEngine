#pragma once

// global
#include <future>

namespace app {

namespace main_thread {

bool IsMainThread();
int GetTasksTotal();
std::future<void> PushTask(std::packaged_task<void()>& package);
void ExecuteTasks();

}  // namespace main_thread

}  // namespace app
