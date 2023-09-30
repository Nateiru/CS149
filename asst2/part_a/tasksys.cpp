#include "tasksys.h"
#include <mutex>


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char *TaskSystemSerial::name() {
  return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
    const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemSerial::sync() {
  return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name() {
  return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads):
    ITaskSystem(num_threads), num_threads_(num_threads) {}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
  auto thread_func = [runnable_ = runnable, num = num_threads_, total = num_total_tasks](int i) {
    while(i < total) {
      runnable_->runTask(i, total);
      i += num;
    }
  };
  std::vector<std::thread> threads(num_threads_);
  for (int i = 0; i < num_threads_; ++i) {
    threads[i] = std::thread(thread_func, i);
  }

  for (int i = 0; i < num_threads_; ++i) {
    threads[i].join();
  }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
    const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemParallelSpawn::sync() {
  return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name() {
  return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads)
  : ITaskSystem(num_threads), num_threads_(num_threads) {
  unsigned int init = 0x01;
  for(int i = 0; i < num_threads_; ++i) {
    bitmap_init_value_ |= init;
    init <<= 1;
  }
  start();
}

void TaskSystemParallelThreadPoolSpinning::start() {
  threads_.resize(num_threads_);
  for(int i = 0; i < num_threads_; ++i) {
    threads_[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::threadLoop, this, i);
  }
}

void TaskSystemParallelThreadPoolSpinning::threadLoop(int i) {
  for(;;) {
    if (terminate_)
      break; 
    bool flag = false;
    {
      std::lock_guard<std::mutex> guard{mtx_};
      flag = (jobs_ >> i) & 0x01;
    }
    if(flag) {
      int taskId = i;
      while(taskId < total_tasks_) {
        runnable_->runTask(taskId, total_tasks_);
        taskId += num_threads_;
      }
      {
        std::lock_guard<std::mutex> guard{mtx_};
        jobs_ &= ~(0x01 << i);
      }
    }
  }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
  /*
    * Here, we don't need to synchronize the code, because
    * the thread will never write `terminate`. No matter
    * the thread may read some corrupted value, this doesn't matter.
  */
  terminate_ = true;
  for(int i = 0; i < num_threads_; ++i) {
    threads_[i].join();
  }
}

bool TaskSystemParallelThreadPoolSpinning::busy() {
  bool is_busy;
  {
    std::lock_guard<std::mutex> guard{mtx_};
    is_busy = jobs_;
  }
  return is_busy;
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks) {
  total_tasks_ = num_total_tasks;
  runnable_ = runnable;
  {
    std::lock_guard<std::mutex> guard{mtx_};
    jobs_ = bitmap_init_value_;
  }
  while(busy());
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable,
    int num_total_tasks,
    const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
  return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name() {
  return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(
  int num_threads): ITaskSystem(num_threads) {
  //
  // TODO: CS149 student implementations may decide to perform setup
  // operations (such as thread pool construction) here.
  // Implementations are free to add new class member variables
  // (requiring changes to tasksys.h).
  //
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
  //
  // TODO: CS149 student implementations may decide to perform cleanup
  // operations (such as thread pool shutdown construction) here.
  // Implementations are free to add new class member variables
  // (requiring changes to tasksys.h).
  //
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks) {


  //
  // TODO: CS149 students will modify the implementation of this
  // method in Parts A and B.  The implementation provided below runs all
  // tasks sequentially on the calling thread.
  //

  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable,
    int num_total_tasks,
    const std::vector<TaskID> &deps) {


  //
  // TODO: CS149 students will implement this method in Part B.
  //

  return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

  //
  // TODO: CS149 students will modify the implementation of this method in Part B.
  //

  return;
}