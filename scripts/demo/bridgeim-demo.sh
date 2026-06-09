#!/usr/bin/env bash
# Start the BridgeIM deliverable-1.0 smoke demo stack.
#
# This is intentionally semi-automated: it launches the dependency/process stack
# and then prints the ChatClient commands to run manually. ChatClient is an
# interactive terminal client, so keeping the business flow manual makes the demo
# easier to observe and explain during interviews.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LOG_DIR="${PROJECT_ROOT}/logs/demo"
ENV_FILE="${PROJECT_ROOT}/.env"

PIDS=()
NAMES=()

cleanup() {
  local rc=$?
  if ((${#PIDS[@]} > 0)); then
    echo
    echo "[demo] stopping BridgeIM demo processes..."
    for pid in "${PIDS[@]}"; do
      if kill -0 "${pid}" 2>/dev/null; then
        kill "${pid}" 2>/dev/null || true
      fi
    done
    sleep 1
    for pid in "${PIDS[@]}"; do
      if kill -0 "${pid}" 2>/dev/null; then
        kill -9 "${pid}" 2>/dev/null || true
      fi
    done
  fi
  exit "${rc}"
}
trap cleanup EXIT INT TERM

fail() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" ]] || fail "missing required file: $1"
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "missing command: $1"
}

start_process() {
  local name="$1"
  shift
  local stdout_log="${LOG_DIR}/${name}.stdout.log"
  local stderr_log="${LOG_DIR}/${name}.stderr.log"

  echo "[demo] starting ${name}: $*"
  "$@" >"${stdout_log}" 2>"${stderr_log}" &
  local pid=$!
  PIDS+=("${pid}")
  NAMES+=("${name}")
  sleep 1
  if ! kill -0 "${pid}" 2>/dev/null; then
    echo "[demo] ${name} exited early. stderr:" >&2
    sed -n '1,80p' "${stderr_log}" >&2 || true
    exit 1
  fi
  echo "[demo] ${name} pid=${pid} (stdout=${stdout_log}, stderr=${stderr_log})"
}

check_tcp_port() {
  local host="$1"
  local port="$2"
  timeout 2 bash -c "</dev/tcp/${host}/${port}" >/dev/null 2>&1
}

require_command docker
require_command cmake
require_command timeout

cd "${PROJECT_ROOT}"
mkdir -p "${LOG_DIR}"

if [[ ! -f "${ENV_FILE}" ]]; then
  fail ".env not found. Copy .env.example to .env and fill MySQL/Redis/ZooKeeper settings first."
fi

set -a
# shellcheck source=/dev/null
source "${ENV_FILE}"
set +a

: "${ZK_ENDPOINTS:=127.0.0.1:2181}"
: "${RPC_ADVERTISE_HOST:=127.0.0.1}"
: "${MPRPC_LOG_MODE:=file}"
: "${MPRPC_LOG_NAME:=mprpc}"
# Keep demo logs out of bin/ even if an older .env still says MPRPC_LOG_DIR=bin.
if [[ -z "${MPRPC_LOG_DIR:-}" || "${MPRPC_LOG_DIR}" == "bin" ]]; then
  MPRPC_LOG_DIR="${PROJECT_ROOT}/logs"
fi

export ZK_ENDPOINTS RPC_ADVERTISE_HOST MPRPC_LOG_MODE MPRPC_LOG_DIR MPRPC_LOG_NAME

# ZK is currently provided by the vendored mprpc docker compose file.
if ! docker ps --format '{{.Names}}' | grep -qx 'mprpc-zookeeper'; then
  echo "[demo] mprpc-zookeeper is not running; starting third_party/mprpc compose..."
  docker compose -f "${PROJECT_ROOT}/third_party/mprpc/compose.yaml" up -d
fi

# BridgeIM compose starts Redis and Nginx. MySQL is the shared mysql_db container.
echo "[demo] ensuring BridgeIM dependency services (redis/nginx) are running..."
docker compose up -d redis nginx

if ! docker ps --format '{{.Names}}' | grep -qx 'mysql_db'; then
  fail "shared MySQL container mysql_db is not running. Start/bootstrap it before the demo."
fi

if ! check_tcp_port 127.0.0.1 2181; then
  fail "ZooKeeper is not reachable at 127.0.0.1:2181. Check mprpc-zookeeper."
fi
if ! check_tcp_port 127.0.0.1 6379; then
  fail "Redis is not reachable at 127.0.0.1:6379. Check docker compose ps."
fi
if ! check_tcp_port 127.0.0.1 3306; then
  fail "MySQL is not reachable at 127.0.0.1:3306. Check mysql_db."
fi

echo "[demo] building BridgeIM..."
"${PROJECT_ROOT}/scripts/build/all.sh"

require_file "${PROJECT_ROOT}/bin/OfflineMessageService"
require_file "${PROJECT_ROOT}/bin/FriendService"
require_file "${PROJECT_ROOT}/bin/ChatServer"
require_file "${PROJECT_ROOT}/bin/ChatClient"

# Each RPC provider reads RPC_PORT from the environment. They must differ.
start_process OfflineMessageService env \
  RPC_PORT=7000 \
  MPRPC_LOG_NAME=offline-message-rpc \
  "${PROJECT_ROOT}/bin/OfflineMessageService"

start_process FriendService env \
  RPC_PORT=7001 \
  MPRPC_LOG_NAME=friend-service-rpc \
  "${PROJECT_ROOT}/bin/FriendService"

start_process ChatServer-6000 "${PROJECT_ROOT}/bin/ChatServer" 0.0.0.0 6000
start_process ChatServer-6002 "${PROJECT_ROOT}/bin/ChatServer" 0.0.0.0 6002

echo
cat <<EOF
[demo] BridgeIM smoke demo stack is running.

Run clients in separate terminals:

  # Through Nginx cluster entry (recommended for cluster demo)
  ./bin/ChatClient 127.0.0.1 8000

  # Or direct node connections for controlled cross-node testing
  ./bin/ChatClient 127.0.0.1 6000
  ./bin/ChatClient 127.0.0.1 6002

Suggested business flow is documented in:
  docs/demo.md

Runtime logs:
  BridgeIM app logs: logs/
  Demo stdout/stderr: logs/demo/

Press ENTER in this terminal to stop all demo processes.
EOF

read -r _
