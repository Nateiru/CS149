#include "tasksys.h"
#include "itasksys.h"
#include <iostream>


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
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }

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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
    const std::vector<TaskID> &deps) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }

  return 0;
}

void TaskSystemParallelSpawn::sync() {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(
  int num_threads): ITaskSystem(num_threads) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable,
    int num_total_tasks,
    const std::vector<TaskID> &deps) {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }

  return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
  // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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
  int num_threads): ITaskSystem(num_threads) , num_threads_(num_threads) {
  threads_.resize(num_threads_);
  for(int i = 0; i < num_threads_; ++i) {
    threads_[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::threadLoop, this);
  }
}

void TaskSystemParallelThreadPoolSleeping::threadLoop() {
  for (;;) {
    // 等待 ready 对了有可以执行的任务
    std::unique_lock<std::mutex> lk(mtx_);
    not_empty_.wait(lk, [this]{
      return !ready_.empty() || terminate_;
    });
    if (terminate_) {
      break;
    }
    Task* task = ready_.front();
    int stage = -1, finished = -1;
    {
      // 当前任务执行的阶段时 stage，进入下一阶段
      // 如果不需要执行下一阶段从 ready_ 中删除
      // 此时任务并不一定完成，不能 notify done
      std::unique_lock<std::mutex> tlk(task->tmtx_);
      stage = task->stage_;
      if (++task->stage_ == task->total_tasks_) {
        ready_.pop();
      }
    }
    lk.unlock();

    // 运行 task 任务的 stage 阶段
    task->runnable_->runTask(stage, task->total_tasks_);

    // 需要 finish 的原因是可能有多个线程通知执行 task 的不同阶段
    // 但是都还没有完成任务
    {
      std::unique_lock<std::mutex> tlk(task->tmtx_);
      finished = ++task->finished_;
    }

    // 任务完成后有两件事
    // 1. 尝试将依赖任务从 block 移动到 ready
    // 2. 尝试唤醒 sync
    if (finished == task->total_tasks_) {
      lk.lock();
      finished_[task->id_] =  task;

      // 没有任务依赖当前完成的任务
      if(!gdep_.count(task->id_)) {
        // 每当任务完成时，才可能唤醒 done
        if (block_.empty() && ready_.empty()) {
          done_.notify_one();
        }
        continue;
      }

      for(auto t: gdep_[task->id_]) {
        std::unique_lock<std::mutex> tlk(t->tmtx_);
        if (--t->dep_cnt_ == 0) {
          block_.erase(t);
          ready_.push(t);
          not_empty_.notify_all();
        }
      }
      // 每当任务完成时，才可能唤醒 done
      if (block_.empty() && ready_.empty()) {
        done_.notify_one();
      }
    }
  }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {

  {
    std::unique_lock<std::mutex> lk(mtx_);
    terminate_ = true;
    not_empty_.notify_all();
  }

  for(int i = 0; i < num_threads_; ++i) {
    threads_[i].join();
  }

  // we need delete all task pointer that construct by `new`
  for (auto task : finished_) {
    delete task.second;
  }

  while (!ready_.empty()) {
    delete ready_.front();
    ready_.pop();
  }

  while (!block_.empty()) {
    delete *block_.begin();
    block_.erase(block_.begin());
  }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks) {
  runAsyncWithDeps(runnable, num_total_tasks, {});
  sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable,
    int num_total_tasks,
    const std::vector<TaskID> &deps) {

  // 当前任务依赖的任务数量，类比拓扑排序 
  // 如果依赖的任务已经完成，忽略这个依赖
  std::unique_lock<std::mutex> lk(mtx_);
  size_t dep_cnt = 0;
  for (auto dep : deps) {
    if (!finished_.count(dep)) {
      dep_cnt++;
    }
  }

  // TODO: replace new with std::shared_ptr 
  Task* task = new Task(id_, runnable, num_total_tasks, dep_cnt);

  if (!task->dep_cnt_) {
    ready_.push(task);
    not_empty_.notify_all();
  } else {
    for (auto dep : deps) {
      if (finished_.count(dep)) continue;
      if(gdep_.count(dep)) {
        gdep_[dep].insert(task);
      } else {
        gdep_[dep] = std::unordered_set<Task*>{task};
      }
    }
    block_.insert(task);
  }
  return id_++;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
  std::unique_lock<std::mutex> lk(mtx_);
  done_.wait(lk, [this]{
    return block_.empty() && ready_.empty();
  });
}