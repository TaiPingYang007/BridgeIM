#!/usr/bin/env bash
# Verify ChatService's RPC failure handling does not claim a fake local fallback
# and does not ignore offline-message persistence failures.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHAT_SERVICE="${PROJECT_ROOT}/src/server/chatservice.cpp"

if grep -q "fallback to local" "${CHAT_SERVICE}"; then
  echo "error: chatservice.cpp still claims 'fallback to local' without a real fallback." >&2
  exit 1
fi

if grep -Eq '^  insertOfflineMessageByRpc\([^;]+\);$' "${CHAT_SERVICE}"; then
  echo "error: insertOfflineMessageByRpc return value is ignored." >&2
  exit 1
fi

echo "chatservice RPC failure handling checks passed."
