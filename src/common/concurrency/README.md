# common/concurrency

Source files for shared concurrency infrastructure.

## ThreadPool

Fixed-size thread pool with bounded task queue, `future`-based result return, and explicit `shutdown()`.

### shutdown semantics

- Stop accepting new tasks (`enqueue` throws)
- Execute already-queued tasks before exiting
- Join all worker threads before returning
- Idempotent: repeated calls return immediately
- Worker threads must not call `shutdown()` (throws `std::runtime_error`)

### Source

Synced from `01_thread_pool` project.
