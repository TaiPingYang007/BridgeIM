# common/concurrency

Shared concurrency infrastructure lives here.

## ThreadPool

Fixed-size thread pool synced from `01_thread_pool`. C++11 compatible.

### Key features

- Bounded task queue with overflow rejection
- `std::packaged_task` + `std::future` for async result return
- Explicit `shutdown()` with graceful drain (idempotent)
- Worker threads must not call `shutdown()`
- RAII destructor calls `shutdown()` as fallback
