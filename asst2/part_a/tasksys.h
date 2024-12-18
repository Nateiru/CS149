#ifndef _TASKSYS_H
#define _TASKSYS_H

#include <mutex>
#include <queue>
#include <vector>
#include <thread>
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
private:
  int num_threads_;
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
private:
  int num_threads_;                       // to store the threads
  std::vector<std::thread> threads_;      // thread poll
  unsigned int jobs_ = 0x00;              // bitmap value for indicating whether there is a job
  unsigned int bitmap_init_value_ = 0x00; // initialized bitmap value with 0x1111
  IRunnable* runnable_;                   // we need to record the runnable
  std::mutex mtx_;                        // the big lock
  bool terminate_ = false;                // whether we should terminate the thread
  int total_tasks_ = 0;                   // we should record the total task
  void start();                           // start the thread pool
  void threadLoop(int i);                 // thread functionaility
  bool busy();                            // whether the threads are busy doing their jobs
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

public:
  using task_id_t = int;

private:
  int num_threads_{0};
  std::vector<std::thread> threads_;
  std::mutex mtx_;
  std::queue<task_id_t> tasks_;
  std::condition_variable not_empty_;
  bool terminate_{false};

  IRunnable* runnable_{nullptr};
  int total_tasks_{0};
  int completed_tasks_{0};
  std::condition_variable completed_;

  void threadLoop();
};

#endif