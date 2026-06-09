# BridgeIM: C++ Instant Messaging Backend

BridgeIM 是一个 C++11 即时通信后端项目，围绕 Muduo 网络库、Nginx 多节点入口、Redis 跨节点消息分发、MySQL 连接池、业务线程池、异步日志和内部 mprpc 服务拆分构建。

项目当前保留外部客户端协议为 **JSON + newline (`\n`)**，同时把部分后端领域能力拆成 Protobuf/mprpc 内部服务，形成一个可本地运行、可演示、可继续演进的 IM 后端工程样例。

## Highlights

- **Muduo TCP gateway**：`ChatServer` 负责连接管理、换行分帧、JSON 协议解析和业务分发。
- **Nginx cluster entry**：客户端可通过 `127.0.0.1:8000` 进入两个本地 ChatServer 节点。
- **Redis cross-node fanout**：跨 ChatServer 节点的在线消息通过 Redis pub/sub 投递。
- **MySQL connection pool**：数据库访问通过 `common/db` 连接池、`DbSession` 和 `QueryResult` 封装。
- **Bounded business thread pool**：网络 IO 与业务/数据库操作通过有界线程池隔离。
- **Async logging**：业务日志写入 `logs/`，避免与 `bin/` 可执行文件混在一起。
- **Internal RPC services**：好友域和离线消息域已拆成独立 mprpc 服务：
  - `FriendService`
  - `OfflineMessageService`
- **Reproducible mprpc dependency**：mprpc 作为 `third_party/mprpc` git submodule 引入，并可由构建脚本自动编译成本地静态库。

## Runtime Topology

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

ZooKeeper 只负责 RPC 服务注册发现；Redis 负责 ChatServer 节点之间的消息 fanout。

## Repository Layout

```text
04_BridgeIM/
├── CMakeLists.txt
├── cmake/                 # CMake dependency/protobuf/model target modules
├── compose.yaml           # Redis + Nginx local dependency stack
├── docker/                # Nginx / Redis configuration
├── docs/                  # Public demo and architecture roadmap
├── include/               # Public headers
├── proto/                 # Protobuf service definitions
├── scripts/               # Build, demo, DB bootstrap, and checks
├── src/                   # Client, gateway, common libraries, RPC services
├── test/                  # Standalone learning/smoke programs
└── third_party/mprpc      # mprpc git submodule
```

Generated/local artifacts are intentionally ignored:

- `build/`
- `bin/`
- `logs/`
- `.env`
- `.codegraph/`
- `.vscode/`

## Prerequisites

Recommended local dependencies:

- `g++`
- `cmake >= 3.16`
- `default-libmysqlclient-dev`
- `libhiredis-dev`
- `libboost-dev`
- `libboost-test-dev`
- protobuf development libraries
- ZooKeeper C client development library (`zookeeper_mt`)
- Muduo, recommended under `/usr/local`
- Docker / Docker Compose

If Muduo is installed outside `/usr/local`, configure with:

```bash
export MUDUO_ROOT=/path/to/muduo
```

## Dependency Services

BridgeIM expects these runtime dependencies:

| Dependency | Source | Default |
| --- | --- | --- |
| MySQL | shared external container | `mysql_db`, `127.0.0.1:3306` |
| Redis | root `compose.yaml` | `127.0.0.1:6379` |
| Nginx | root `compose.yaml` | `127.0.0.1:8000` |
| ZooKeeper | `third_party/mprpc/compose.yaml` | `mprpc-zookeeper`, `127.0.0.1:2181` |

Root `compose.yaml` intentionally starts only BridgeIM's local dependency stack (`redis` and `nginx`). ZooKeeper is provided by the vendored mprpc compose file, and MySQL is shared as `mysql_db`.

## First-Time Setup

Clone with submodules:

```bash
git clone --recursive <repo-url>
cd 04_BridgeIM
```

If the repository was cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

Prepare environment variables:

```bash
cp .env.example .env
# Edit .env if your MySQL/Redis/ZooKeeper settings differ from the defaults.
```

Bootstrap the shared MySQL database if needed:

```bash
MYSQL_ROOT_PASSWORD=<root-password> ./scripts/db/bootstrap-mysql.sh
```

In this local Claude/WSL environment, run shell commands through `rtk`:

```bash
MYSQL_ROOT_PASSWORD=<root-password> rtk ./scripts/db/bootstrap-mysql.sh
```

## Build

Use the canonical build script:

```bash
./scripts/build/all.sh
```

In this local Claude/WSL environment:

```bash
rtk ./scripts/build/all.sh
```

The build script will:

1. prepare `build/`
2. initialize/build vendored mprpc into `third_party/mprpc/dist` when needed
3. configure CMake
4. build all targets

Main outputs:

- `bin/ChatClient`
- `bin/ChatServer`
- `bin/FriendService`
- `bin/OfflineMessageService`

## Demo

Start the multi-process demo stack:

```bash
./scripts/demo/bridgeim-demo.sh
```

In this local Claude/WSL environment:

```bash
rtk ./scripts/demo/bridgeim-demo.sh
```

The demo script builds the project, starts dependency services as needed, launches both RPC services and two ChatServer nodes, then prints the client commands to run manually.

Detailed walkthrough: [docs/demo.md](docs/demo.md)

## Manual Run

Start Redis and Nginx:

```bash
docker compose up -d redis nginx
```

Start ZooKeeper for mprpc if it is not already running:

```bash
docker compose -f third_party/mprpc/compose.yaml up -d
```

Load `.env` before starting backend processes:

```bash
set -a
source .env
set +a
```

Start RPC services with distinct ports:

```bash
RPC_PORT=7000 MPRPC_LOG_NAME=offline-message-rpc ./bin/OfflineMessageService
RPC_PORT=7001 MPRPC_LOG_NAME=friend-service-rpc ./bin/FriendService
```

Start two ChatServer nodes:

```bash
./bin/ChatServer 0.0.0.0 6000
./bin/ChatServer 0.0.0.0 6002
```

Run the client through the cluster entry:

```bash
./bin/ChatClient 127.0.0.1 8000
```

Or connect directly to a specific node for debugging:

```bash
./bin/ChatClient 127.0.0.1 6000
./bin/ChatClient 127.0.0.1 6002
```

`ChatServer` should bind `0.0.0.0` when used behind Docker Nginx, because Nginx forwards from inside a container to the host.

## Checks

Run the offline-message RPC failure-handling check:

```bash
./scripts/checks/rpc-failure-handling.sh
```

Build from a clean output state:

```bash
rm -rf build bin logs
./scripts/build/all.sh
```

## Troubleshooting

### `this account is using`

A user may be left as `online` after an abnormal server exit. Reset states with:

```bash
docker exec -it mysql_db mysql -uchatserver_app -pchatserver_dev_password -D chatserver \
  -e "UPDATE User SET state='offline' WHERE state='online'; SELECT id,name,state FROM User;"
```

### mprpc not found

Ensure the submodule is initialized:

```bash
git submodule update --init --recursive
```

Then rebuild:

```bash
./scripts/build/all.sh
```

CMake resolves mprpc in this order:

1. `-DMPRPC_ROOT=/path/to/mprpc`
2. environment variable `MPRPC_ROOT`
3. vendored build output `third_party/mprpc/dist`
4. sibling project `../03_rpc_framework/dist/mprpc`

### ZooKeeper not reachable

Start the mprpc dependency stack:

```bash
docker compose -f third_party/mprpc/compose.yaml up -d
```

Expected container:

```text
mprpc-zookeeper -> 127.0.0.1:2181
```

## Architecture Roadmap

BridgeIM 1.0 deliberately keeps live TCP connection ownership inside `ChatServer` while selected data domains are split behind internal RPC services. Full distributed presence/message routing is future work because live socket ownership, gateway-node routing, stale session cleanup, and durable retry semantics require a larger distributed-systems design.

See [docs/architecture-roadmap.md](docs/architecture-roadmap.md).
