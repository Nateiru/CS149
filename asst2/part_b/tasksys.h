#ifndef _TASKSYS_H
#define _TASKSYS_H

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <cstddef>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>

#include "itasksys.h"
/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
public:
  TaskSystemSerial(int num_threads);
  ~TaskSystemSerial();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
public:
  TaskSystemParallelSpawn(int num_threads);
  ~TaskSystemParallelSpawn();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
public:
  TaskSystemParallelThreadPoolSpinning(int num_threads);
  ~TaskSystemParallelThreadPoolSpinning();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

struct Task {
  TaskID id_{-1};                 // 任务编号
  IRunnable *runnable_{nullptr};  // 执行任务
  int stage_{0};                  // 该任务有很多子任务，当前执行第 stage_ 子任务
  int finished_{0};               // 子任务完成的数量
  int total_tasks_{0};            // 子任务总数量
  size_t dep_cnt_{0};             // 依赖任务数量，当 dep_cnt_ = 0 时加入 ready
  std::mutex tmtx_;               // task 互斥锁
  Task(TaskID id, IRunnable *runnable, int total_tasks, size_t dep_cnt):
    id_(id), runnable_(runnable), total_tasks_(total_tasks), dep_cnt_(dep_cnt) {}
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
public:
  TaskSystemParallelThreadPoolSleeping(int num_threads);
  ~TaskSystemParallelThreadPoolSleeping();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();

private:
  int num_threads_{0};
  std::vector<std::thread> threads_;
  std::queue<std::shared_ptr<Task>> ready_;            // ready_ 中的任务此时依赖的任务已经全部完成
  std::unordered_set<std::shared_ptr<Task>> block_;    // block_ 依赖任务没被完成暂时被阻塞
  std::condition_variable not_empty_;                  // 通知 ready_ 已经有任务能够执行
  std::mutex mtx_;                                     // 一把大锁保平安
  std::condition_variable done_;                       // 同步任务全部完成
  TaskID id_{0};                                       // 全局的任务 id 分配，从 0 开始
  bool terminate_{false};                              // 是否终止

  // 依赖关系图：哪些任务依赖于 task_id 
  std::unordered_map<TaskID, std::unordered_set<std::shared_ptr<Task>>> gdep_;
  // 已经完成的任务，有可能任务 b 虽然依赖 a，但是 a 已经完成，此时需要注意任务 dep_cnt 的维护
  std::unordered_set<TaskID> finished_;
  void threadLoop();
};

#endif