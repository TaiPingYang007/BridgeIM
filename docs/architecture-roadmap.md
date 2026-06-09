# BridgeIM Architecture Roadmap

BridgeIM is a C++ instant-messaging backend that evolves a clustered chat server into a clearer, service-oriented architecture. The roadmap is intentionally staged: first stabilize the monolithic gateway and shared infrastructure, then split selected business domains behind internal RPC boundaries, and only then consider a fuller microservice topology.

## Design Principles

1. Keep the external client protocol stable: JSON text frames separated by `\n`.
2. Keep `ChatServer` as the access gateway while it owns live TCP connections.
3. Use Redis for cross-node message fanout; do not replace this responsibility with ZooKeeper.
4. Use ZooKeeper for RPC service registration/discovery only.
5. Prefer reusable infrastructure libraries over copying demo/example business code.
6. Split business domains only when the boundary is clear enough to run and explain.

## Current Delivered Architecture

```text
ChatClient
  -> Nginx :8000
  -> ChatServer node A :6000
  -> ChatServer node B :6002

ChatServer
  -> Redis pub/sub
  -> MySQL chatserver
  -> mprpc Channel
      -> FriendService :7001
      -> OfflineMessageService :7000

ZooKeeper :2181
  -> RPC service registration/discovery
```

Delivered capabilities in the 1.0 line:

- Muduo-based TCP server and interactive terminal client
- newline-delimited JSON external protocol
- Nginx stream load balancing across multiple ChatServer nodes
- Redis pub/sub for cross-node message delivery
- MySQL connection pool through `common/db`
- bounded business thread pool through `common/concurrency`
- async business logging under `logs/`
- vendored mprpc submodule with reproducible local build
- internal RPC services:
  - `FriendService` for friend and friend-request operations
  - `OfflineMessageService` for offline-message insert/query/remove

## Evolution Stages

| Stage | Goal | Status |
| --- | --- | --- |
| 0 | Stabilize local clustered ChatServer baseline | Done |
| 1 | Introduce common infrastructure layout | Done |
| 2 | Integrate MySQL connection pool | Done |
| 3 | Integrate bounded business thread pool | Done |
| 4 | Clarify monolith business boundaries | Partially done |
| 5 | Library-ize and integrate mprpc | Done |
| 6 | Split first backend domains into RPC services | Done (`FriendService`, `OfflineMessageService`) |
| 7 | Full IM microservice topology | Future work |

## Domain Boundary Map

| Current responsibility | Target domain/service |
| --- | --- |
| TCP accept, framing, client JSON protocol | access gateway (`ChatServer`) |
| login, register, user state | user domain / future `UserService` |
| live connections, online presence, Redis subscriptions | presence/session domain |
| friends and friend requests | `FriendService` |
| groups and group requests | future `GroupService` |
| offline messages | `OfflineMessageService` |
| direct message routing and group fanout | future message-routing domain |

## Why Full Stage 7 Is Deferred

The hard part of a complete distributed IM system is not simply creating more RPC services. The difficult boundary is **presence and message routing**.

Today, `ChatServer` owns live `muduo::net::TcpConnectionPtr` objects. Those live socket connections cannot be moved through RPC. When a message needs to reach user `X`, the system must know which gateway node owns `X`'s live connection, then route the message to that gateway for final delivery.

A production-grade Stage 7 design would need:

- presence records mapping `user_id -> gateway_node`
- gateway node registration and health checking
- stale presence cleanup after abnormal disconnects
- durable retry or queueing for offline-message persistence failures
- a strategy for routing messages to the gateway that owns the target socket
- careful handling of reconnects, duplicate sessions, and node failure

BridgeIM 1.0 deliberately stops before this larger distributed-systems redesign. The current version demonstrates clear service boundaries for selected data domains while keeping live connection ownership inside `ChatServer`, where it naturally belongs for this stage.

## Future Work

Reasonable next steps after 1.0:

1. Add `UserService` for registration, login lookup, and user state persistence.
2. Add `GroupService` for group metadata, membership, and request workflows.
3. Add durable retry/queueing around offline-message persistence.
4. Evolve mprpc from per-call ZooKeeper lookup/TCP connect toward cached discovery and persistent connections.
5. Design a presence service and gateway routing contract before attempting full message-service extraction.

The key rule for future work is to preserve the external JSON client protocol while moving only internal service-to-service communication behind Protobuf/RPC boundaries.
