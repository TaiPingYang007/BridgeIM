#ifndef LOCKQUEUE_H
#define LOCKQUEUE_H

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

// 异步写日志使用的线程安全队列。
//
// 有界 + 可关闭：
//   - 满时丢弃“最旧”一条并计数，生产者永不阻塞，适合日志这种不该拖慢业务
//     热路径的场景。
//   - Stop() 置停止标志并唤醒消费者；消费者把剩余消息排空后 Pop 返回 false，
//     从而优雅退出。
template <class T> class LockQueue {
public:
  // capacity 为队列容量上限；0 表示不设上限（退化为无界）。
  explicit LockQueue(std::size_t capacity = 10000)
      : _capacity(capacity), _dropped(0), _stop(false) {}

  void Push(const T &msg) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stop) {
      return;
    }
    if (_capacity != 0 && _queue.size() >= _capacity) {
      _queue.pop();
      ++_dropped;
    }
    _queue.push(msg);
    _condvar.notify_one();
  }

  // 返回 true 并通过 out 带出一条消息；
  // 仅当“已关闭且队列已排空”时返回 false，作为消费者退出信号。
  bool Pop(T &out) {
    std::unique_lock<std::mutex> lock(_mutex);
    _condvar.wait(lock, [this] { return _stop || !_queue.empty(); });
    if (_queue.empty()) {
      return false;
    }

    out = _queue.front();
    _queue.pop();
    return true;
  }

  void Stop() {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _stop = true;
    }
    _condvar.notify_all();
  }

  void SetCapacity(std::size_t capacity) {
    std::lock_guard<std::mutex> lock(_mutex);
    _capacity = capacity;
    while (_capacity != 0 && _queue.size() > _capacity) {
      _queue.pop();
      ++_dropped;
    }
  }

  std::size_t DroppedCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _dropped;
  }

private:
  std::queue<T> _queue;
  mutable std::mutex _mutex;
  std::condition_variable _condvar;
  std::size_t _capacity;
  std::size_t _dropped;
  bool _stop;
};

#endif
