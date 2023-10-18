#pragma once

// global
#include <functional>

namespace app {

namespace init {

void CreateWorkers();
void DestroyWorkers();

}  // namespace init

namespace task {

void Pause();
void Unpause();
int GetTasksQueued();
int GetTasksRunning();
int GetTasksTotal();
void PushTask(std::function<void(int)> task);
void WaitForTasks();

}  // namespace task

}  // namespace app
