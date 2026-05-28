// 进行类和函数声明
#pragma once

#include <condition_variable>
#include <functional>
#include <future> // 用于获取异步返回值的取餐小票
#include <memory> // 用于智能指针 shared_ptr
#include <mutex>
#include <queue>
#include <stdexcept> // 用于抛出异常
#include <thread>
#include <vector>

class ThreadPool // 线程池类声明
{
public:
  struct Config
  {
    size_t thread_count = 4;
    size_t max_queue_size = 10000;
  };

  // 线程池创建：初始化几个消费者线程
  explicit ThreadPool(const Config &config);

  // 线程池关闭：清理资源，释放消费者线程
  ~ThreadPool();

  // 生产者往任务缓冲队列里塞新任务
  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::result_of<F(Args...)>::type>
  {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    {
      std::unique_lock<std::mutex> lock(queue_mutex);

      if (stop)
      {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      if (tasks.size() >= config_.max_queue_size)
      {
        throw std::runtime_error("task queue is full");
      }

      tasks.emplace([task]() { (*task)(); });
    }

    condition.notify_one();
    return res;
  }

  // 线程池关闭：通知所有消费者停止工作，并且等待他们结束
  void shutdown();

private:
  // 1、消费者集群 (Workers/Consumers)
  std::vector<std::thread> workers;

  // 2、任务缓冲队列 (Task Queue / Buffer)
  std::queue<std::function<void()>> tasks;

  // 3、并发同步原语 (Synchronization Primitives)
  std::mutex queue_mutex;
  std::condition_variable condition;

  // 4、线程池的状态：如果线程池要关闭了（stop = true）, 通知所有的消费者停止工作
  bool stop;

  // 5、线程池的配置
  Config config_;
};
