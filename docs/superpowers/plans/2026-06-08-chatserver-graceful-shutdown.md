# ChatServer Graceful Shutdown Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move ChatServer Ctrl+C cleanup out of the signal handler so `ChatService::reset()` and `Logger::Shutdown()` run in normal `main()` control flow after `EventLoop::loop()` returns.

**Architecture:** Replace the current heavy SIGINT handler (`reset + exit`) with a lightweight notification handler that asks the Muduo `EventLoop` to quit. After `loop.loop()` returns, `main()` performs business reset and logger shutdown in a safe ordinary execution context. This is a teaching-project compromise: safer than the current handler, but not the strict POSIX `sigwait`/self-pipe design.

**Tech Stack:** C++11, Muduo `EventLoop`, POSIX `SIGINT`, BridgeIM `Logger`, `ChatService`, existing CMake build.

---

## File Structure

- Modify: `src/server/main.cpp`
  - Replace `resetHandler` behavior.
  - Store the main `EventLoop` pointer for SIGINT notification.
  - Move `ChatService::reset()` and `Logger::Shutdown()` after `loop.loop()`.
- Create: `test/chatserver_graceful_shutdown_smoke.sh`
  - Temporary/diagnostic smoke script that starts ChatServer, sends SIGINT, and checks for a stopping log line.
  - This test documents expected behavior. If the project owner does not want to keep smoke scripts under `test/`, remove it after the implementation and rely on the manual command in Task 3.

---

### Task 1: Write a failing graceful-shutdown smoke check

**Files:**
- Create: `test/chatserver_graceful_shutdown_smoke.sh`

- [ ] **Step 1: Create the smoke script**

Create `test/chatserver_graceful_shutdown_smoke.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${CHATSERVER_GRACEFUL_SHUTDOWN_PORT:-6000}"
LOG_FILE="${ROOT}/bin/$(date +%Y-%m-%d)-chatserver-${PORT}.log"

rm -f "${LOG_FILE}"

set -a
if [[ -f "${ROOT}/.env" ]]; then
  # shellcheck disable=SC1091
  source "${ROOT}/.env"
fi
set +a

"${ROOT}/bin/ChatServer" 0.0.0.0 "${PORT}" &
server_pid=$!

cleanup() {
  if kill -0 "${server_pid}" 2>/dev/null; then
    kill -TERM "${server_pid}" 2>/dev/null || true
    wait "${server_pid}" 2>/dev/null || true
  fi
}
trap cleanup EXIT

# Give ChatServer time to initialize Redis and enter EventLoop.
sleep 1

kill -INT "${server_pid}"

for _ in $(seq 1 20); do
  if ! kill -0 "${server_pid}" 2>/dev/null; then
    wait "${server_pid}" || true
    break
  fi
  sleep 0.2
done

if kill -0 "${server_pid}" 2>/dev/null; then
  echo "FAIL: ChatServer did not exit after SIGINT" >&2
  exit 1
fi

if [[ ! -f "${LOG_FILE}" ]]; then
  echo "FAIL: expected log file not found: ${LOG_FILE}" >&2
  exit 1
fi

if ! grep -q "ChatServer stopping" "${LOG_FILE}"; then
  echo "FAIL: stopping log line not found in ${LOG_FILE}" >&2
  exit 1
fi

if ! grep -q "ChatServer stopped" "${LOG_FILE}"; then
  echo "FAIL: stopped log line not found in ${LOG_FILE}" >&2
  exit 1
fi

printf 'PASS: ChatServer graceful shutdown log observed in %s\n' "${LOG_FILE}"
```

- [ ] **Step 2: Make the smoke script executable**

Run:

```bash
rtk chmod +x test/chatserver_graceful_shutdown_smoke.sh
```

Expected: command exits 0.

- [ ] **Step 3: Build ChatServer before running the smoke script**

Run:

```bash
rtk cmake --build build --target ChatServer --parallel
```

Expected: build exits 0.

- [ ] **Step 4: Run the smoke script and verify RED**

Run:

```bash
rtk ./test/chatserver_graceful_shutdown_smoke.sh
```

Expected before implementation: FAIL with missing `ChatServer stopping` or `ChatServer stopped` log line, because the current SIGINT handler calls `exit(0)` directly and `loop.loop()` never returns to `main()` cleanup code.

---

### Task 2: Move Ctrl+C cleanup into normal `main()` flow

**Files:**
- Modify: `src/server/main.cpp`

- [ ] **Step 1: Replace the heavy reset handler with a lightweight loop-quit notification**

In `src/server/main.cpp`, add a small unnamed namespace near the includes and replace the current `resetHandler` with this implementation:

```cpp
namespace {
muduo::net::EventLoop *g_loop = nullptr;

void resetHandler(int) {
  if (g_loop != nullptr) {
    g_loop->quit();
  }
}
} // namespace
```

This intentionally removes these operations from signal context:

```cpp
ChatService::instance()->reset();
exit(0);
```

- [ ] **Step 2: Set `g_loop` after creating the EventLoop**

In `main()`, after:

```cpp
muduo::net::EventLoop loop;
```

add:

```cpp
g_loop = &loop;
```

Keep the `std::signal(SIGINT, resetHandler);` registration after `g_loop` is set, so the handler has a loop to notify.

- [ ] **Step 3: Add normal-flow cleanup after `loop.loop()`**

Replace the final:

```cpp
loop.loop();
```

with:

```cpp
loop.loop();

LOG_INFO("ChatServer stopping at %s:%d\n", ip, port);
ChatService::instance()->reset();
LOG_INFO("ChatServer stopped at %s:%d\n", ip, port);
Logger::GetInstance().Shutdown();

return 0;
```

Keep `Logger::Shutdown()` last so logs emitted during `ChatService::reset()` are still accepted and drained.

- [ ] **Step 4: Build ChatServer**

Run:

```bash
rtk cmake --build build --target ChatServer --parallel
```

Expected: build exits 0.

- [ ] **Step 5: Run the smoke script and verify GREEN**

Run:

```bash
rtk ./test/chatserver_graceful_shutdown_smoke.sh
```

Expected: PASS with output similar to:

```text
PASS: ChatServer graceful shutdown log observed in /home/taipingyang007/projects/cpp_project/04_BridgeIM/bin/YYYY-MM-DD-chatserver-6000.log
```

---

### Task 3: Final verification and cleanup decision

**Files:**
- Review: `src/server/main.cpp`
- Optional remove: `test/chatserver_graceful_shutdown_smoke.sh`

- [ ] **Step 1: Run full build**

Run:

```bash
rtk cmake --build build --parallel
```

Expected: all targets build successfully, including `ChatServer`, `ChatClient`, `FriendService`, and `OfflineMessageService`.

- [ ] **Step 2: Inspect the resulting log file**

Run:

```bash
rtk grep -n "ChatServer stopping\|ChatServer stopped" bin/$(date +%Y-%m-%d)-chatserver-6000.log
```

Expected: output contains both `ChatServer stopping` and `ChatServer stopped` lines.

- [ ] **Step 3: Decide whether to keep the smoke script**

If keeping the smoke script as project documentation/regression coverage:

```bash
rtk git add src/server/main.cpp test/chatserver_graceful_shutdown_smoke.sh
```

If the project owner does not want the smoke script committed, remove it after the green run:

```bash
rtk rm test/chatserver_graceful_shutdown_smoke.sh
rtk git add src/server/main.cpp
```

- [ ] **Step 4: Commit**

Run:

```bash
rtk git commit -m "refactor: make ChatServer shutdown graceful" -m "Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

Expected: commit succeeds.

---

## Self-Review

- Spec coverage: The plan covers moving reset out of the signal handler, notifying `EventLoop`, calling `ChatService::reset()` in normal control flow, calling `Logger::Shutdown()` after reset, and verifying logs flush.
- Placeholder scan: No placeholder steps remain; commands and code snippets are explicit.
- Scope check: This plan is intentionally scoped to the teaching-project version using `loop.quit()` from the handler. It does not implement strict POSIX `sigwait`/self-pipe handling.
- Type consistency: The plan uses existing symbols `muduo::net::EventLoop`, `ChatService::instance()->reset()`, and `Logger::GetInstance().Shutdown()` consistently.
