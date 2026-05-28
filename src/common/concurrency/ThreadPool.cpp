#include "common/concurrency/ThreadPool.hpp"

// 构造函数：线程池开始工作，threads指定线程数量
ThreadPool::ThreadPool(const Config &config) : stop(false), config_(config)
{
    if (config_.thread_count == 0)
    {
        throw std::invalid_argument("thread count must be greater than 0");
    }

    if (config_.max_queue_size == 0)
    {
        throw std::invalid_argument("max queue size must be greater than 0");
    }

    workers.reserve(config_.thread_count);

    try
    {
        for (size_t i = 0; i < config_.thread_count; i++)
        {
            workers.emplace_back([this]() {
                while (true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });

                        if (this->stop && this->tasks.empty())
                        {
                            return;
                        }

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
        }
    }
    catch (...)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }

        condition.notify_all();

        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        throw;
    }
}

// 线程池关闭：通知所有消费者停止工作，并且等待他们结束
void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        // 幂等：如果已经关闭，直接返回
        if (stop)
        {
            return;
        }

        // 禁止 worker 线程内部调用 shutdown
        for (auto &worker : workers)
        {
            if (worker.get_id() == std::this_thread::get_id())
            {
                throw std::runtime_error("shutdown() must not be called from a worker thread");
            }
        }

        stop = true;
    }

    condition.notify_all();

    for (auto &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

ThreadPool::~ThreadPool()
{
    shutdown();
}
