# BridgeIM Thread Pool Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move BridgeIM ChatService business handler execution off Muduo IO threads and onto the imported ThreadPool, while preserving the current network framing and message dispatch behavior.

**Architecture:** ChatServer remains responsible for Muduo callbacks, per-connection newline framing, JSON parsing, and msgid dispatch. ChatServer owns a business ThreadPool and enqueues the selected ChatService handler as a task. ChatService remains unaware of the ThreadPool and continues to own business logic, Redis routing, online connection maps, and model calls.

**Tech Stack:** C++11, Muduo TcpServer/EventLoop, nlohmann_json, BridgeIM common/concurrency ThreadPool, CMake static library target, MySQL ConnectionPool, Redis pub/sub.

---

## File Map

- `include/common/concurrency/ThreadPool.hpp`
  - Imported thread pool public API.
  - Before ChatServer integration, confirm it is the hardened version synced from `../01_thread_pool/include/ThreadPool.h`.

- `src/common/concurrency/ThreadPool.cpp`
  - Imported thread pool worker loop and shutdown implementation.
  - Before ChatServer integration, confirm it is the hardened version synced from `../01_thread_pool/src/ThreadPool.cpp`.

- `src/common/CMakeLists.txt`
  - Already builds `bridgeim_common_concurrency` as a static target from `concurrency/ThreadPool.cpp`.
  - No further change expected unless the source filename changes.

- `include/server/chatserver.hpp`
  - Add `#include "common/concurrency/ThreadPool.hpp"`.
  - Add `ThreadPool _threadPool;` as a ChatServer member.

- `src/server/chatserver.cpp`
  - Initialize `_threadPool` in the ChatServer constructor.
  - Replace direct `msgHandler(conn, js, time);` call with `_threadPool.enqueue(...)`.
  - Catch `enqueue()` failure in the IO thread.
  - Catch handler exceptions inside the worker task.

- `src/server/CMakeLists.txt`
  - Confirm ChatServer links a target that includes `bridgeim_common_concurrency`.
  - If it only links `bridgeim_common_db`, update it to link `bridgeim_common` or add `bridgeim_common_concurrency`.

---

### Task 1: Verify ThreadPool Source Is Ready

**Files:**
- Inspect: `include/common/concurrency/ThreadPool.hpp`
- Inspect: `src/common/concurrency/ThreadPool.cpp`
- Inspect source project: `../01_thread_pool/include/ThreadPool.h`
- Inspect source project: `../01_thread_pool/src/ThreadPool.cpp`

- [ ] **Step 1: Compare BridgeIM copy with source thread pool**

Run:

```bash
rtk diff --no-index ../01_thread_pool/include/ThreadPool.h include/common/concurrency/ThreadPool.hpp
rtk diff --no-index ../01_thread_pool/src/ThreadPool.cpp src/common/concurrency/ThreadPool.cpp
```

Expected:
- Differences should be limited to BridgeIM include path and any intentional C++11 compatibility changes.
- If `01_thread_pool` has just been hardened, sync those changes into BridgeIM before continuing.

- [ ] **Step 2: Build BridgeIM common targets**

Run:

```bash
rtk cmake --build build --target bridgeim_common_concurrency
```

Expected:
- `bridgeim_common_concurrency` builds successfully.

---

### Task 2: Add ThreadPool Ownership to ChatServer

**Files:**
- Modify: `include/server/chatserver.hpp`
- Modify: `src/server/chatserver.cpp`

- [ ] **Step 1: Add ThreadPool include and member**

In `include/server/chatserver.hpp`, add:

```cpp
#include "common/concurrency/ThreadPool.hpp"
```

Then add this private member near `_server` and `_loop`:

```cpp
ThreadPool _threadPool;
```

Expected shape:

```cpp
  muduo::net::TcpServer  _server;
  muduo::net::EventLoop *_loop;
  ThreadPool _threadPool;
```

- [ ] **Step 2: Initialize ThreadPool in ChatServer constructor**

In `src/server/chatserver.cpp`, change the constructor initializer from:

```cpp
    : _server(loop, listenAddr, nameArg), _loop(loop) {
```

to C++11-compatible config initialization. Use a helper in an unnamed namespace:

```cpp
namespace {
ThreadPool::Config makeBusinessThreadPoolConfig() {
  ThreadPool::Config config;
  config.thread_count = 4;
  config.max_queue_size = 10000;
  return config;
}
} // namespace
```

Then initialize:

```cpp
    : _server(loop, listenAddr, nameArg),
      _loop(loop),
      _threadPool(makeBusinessThreadPoolConfig()) {
```

- [ ] **Step 3: Build after ownership change**

Run:

```bash
rtk cmake --build build
```

Expected:
- `ChatServer` and `ChatClient` build successfully.
- Behavior is unchanged because `onMessage()` still calls handler directly.

---

### Task 3: Enqueue ChatService Handler in onMessage

**Files:**
- Modify: `src/server/chatserver.cpp`

- [ ] **Step 1: Replace direct handler call**

Replace:

```cpp
      // 回调消息绑定好的事件处理器，来执行相应的业务逻辑
      msgHandler(conn, js, time);
```

with:

```cpp
      try {
        _threadPool.enqueue([conn, js, time, msgHandler]() mutable {
          try {
            msgHandler(conn, js, time);
          } catch (const std::exception &e) {
            LOG_ERROR("business handler exception: %s\n", e.what());
          } catch (...) {
            LOG_ERROR("unknown business handler exception\n");
          }
        });
      } catch (const std::exception &e) {
        LOG_ERROR("failed to enqueue business task: %s\n", e.what());
        nlohmann::json response;
        response["errno"] = 1;
        response["errmsg"] = "server busy";
        conn->send(response.dump() + "\n");
      }
```

Reason:
- `enqueue()` runs on the Muduo IO thread and may throw when the queue is full or the pool is stopped.
- The worker task must catch handler exceptions because the returned `future` is intentionally not consumed.
- The lambda is `mutable` because current `MsgHandler` takes `nlohmann::json &js`, so the captured JSON copy must be passed as a non-const lvalue.

- [ ] **Step 2: Build after async handler change**

Run:

```bash
rtk cmake --build build
```

Expected:
- Build succeeds.
- Any compile error about `js` reference means the lambda needs `mutable` or the handler signature must be revisited.

---

### Task 4: Confirm Linkage Includes common/concurrency

**Files:**
- Inspect/modify: `src/server/CMakeLists.txt`
- Inspect: `src/common/CMakeLists.txt`

- [ ] **Step 1: Inspect ChatServer link target**

Run:

```bash
rtk grep -n "target_link_libraries" -A20 src/server/CMakeLists.txt
```

Expected:
- ChatServer should link either `bridgeim_common` or `bridgeim_common_concurrency`.

- [ ] **Step 2: Fix link target if needed**

If ChatServer links only `bridgeim_common_db`, change it to link the aggregate target:

```cmake
target_link_libraries(ChatServer PRIVATE
    bridgeim_common
    ${MUDUO_NET_LIBRARY}
    ${MUDUO_BASE_LIBRARY}
    nlohmann_json::nlohmann_json
    hiredis
)
```

Expected:
- The exact surrounding libraries may differ; preserve existing Muduo/JSON/Redis libraries and replace only the common target choice.

- [ ] **Step 3: Build final target**

Run:

```bash
rtk cmake --build build
```

Expected:
- Full project builds.

---

### Task 5: Manual Smoke Test the Message Path

**Files:**
- No source changes expected.

- [ ] **Step 1: Start dependencies if needed**

Run only if Redis/Nginx dependencies are not already running:

```bash
rtk docker compose up -d
```

Expected:
- Redis and Nginx containers are running.

- [ ] **Step 2: Start one ChatServer node**

Use the project environment flow from `PROJECT_CONTEXT.md`:

```bash
rtk proxy bash -lc 'set -a; source .env; set +a; ./bin/ChatServer 0.0.0.0 6000'
```

Expected:
- Server starts and listens on port 6000.
- No immediate thread pool shutdown or enqueue errors.

- [ ] **Step 3: Start ChatClient and test basic flow**

In another terminal:

```bash
rtk proxy bash -lc './bin/ChatClient 127.0.0.1 6000'
```

Expected:
- Client connects.
- Registration/login flow still works.
- Server responses still include newline-delimited JSON messages.

- [ ] **Step 4: Watch for async-specific failures**

Check server output/logs for:

```text
failed to enqueue business task
business handler exception
unknown business handler exception
```

Expected:
- These should not appear during normal smoke flow.

---

## Self-Review

- Spec coverage: This plan covers source readiness, ChatServer ownership, handler enqueue, linkage, and smoke testing.
- Placeholder scan: No placeholder tasks remain; each code-changing step includes concrete code.
- Type consistency: `ThreadPool`, `ThreadPool::Config`, `_threadPool`, `MsgHandler`, `conn`, `js`, and `time` names match current BridgeIM code.
