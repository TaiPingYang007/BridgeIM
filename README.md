# BridgeIM

> 基于 Muduo + Redis + Nginx 的 Mode A 基线（本地开发 + Docker 跑依赖）的 IM 后端项目

## 当前模式

本项目当前采用 **模式 A**：

- 本地写代码
- 本地编译 `ChatServer` / `ChatClient`
- 本地运行 `ChatServer` / `ChatClient`
- Docker 只负责依赖服务

当前约定：

- 当前仓库身份是 `BridgeIM`
- 当前代码基线来自 `ChatServer`
- 因此现阶段的可执行文件名仍保持 `ChatServer` / `ChatClient`

当前依赖服务：

- 共享 `mysql_db`
- 项目内 `redis`
- 项目内 `nginx`

## 重构蓝图

- 当前总目标是以 `02_cluster_chat_server` 为业务骨架，逐步吸收 `06_connection_pool`、`01_thread_pool`、`03_rpc_framework` 的公共能力，最终演化成更清晰分层的 IM 后端体系
- 当前仓库已经完成 Stage 1 的公共层骨架预留：`src/common` / `include/common` 已建立，后续会优先从数据库基础设施层开始整合
- 当前已进入 Stage 2 早期落地：`common/db` 已接入主构建，部分 model 已改走连接池版数据库访问链路

## 目录说明

```text
04_BridgeIM/
├── autobuild.sh                    # 本地构建脚本
├── compose.yaml                    # 只编排依赖服务
├── .env.example                    # 环境变量模板
├── cmake/ProjectDependencies.cmake # 本地依赖接入
├── docker/
│   ├── mysql/init/01-init-chatserver.sql
│   ├── nginx/nginx.conf
│   └── redis/redis.conf
├── scripts/bootstrap-shared-mysql.sh
├── graphify-out/                   # 本地代码图谱产物，不作为主线源码
├── include/
└── src/
```

## 当前数据库层状态

- 当前主数据库基础设施位于 `include/common/db` 和 `src/common/db`
- 已落地的核心组件包括：
  - `ConnectionPool`
  - `ConnectionPoolConfig`
  - `MySQL`
  - `DbSession`
  - `QueryResult`
- 当前配置入口优先兼容 `.env`，不再要求日常开发显式准备 `connection_pool.conf`
- `src/server/db` 暂时保留作历史对照，但当前主路径已经优先走 `common/db`

## 本地开发前提

本机建议准备：

- `g++`
- `cmake`
- `default-libmysqlclient-dev`
- `libhiredis-dev`
- `libboost-dev`
- `libboost-test-dev`
- `muduo`

推荐 `muduo` 本地安装到：

```text
/usr/local
```

如果不是这个位置，请显式设置：

```bash
export MUDUO_ROOT=/你的/muduo/安装目录
```

## MySQL 约定

本项目继续复用共享 `mysql_db`，采用：

- 独立数据库 `chatserver`
- 独立用户 `chatserver_app`
- 独立权限

第一次运行前，先执行：

```bash
cp .env.example .env
MYSQL_ROOT_PASSWORD=你的_mysql_root_密码 ./scripts/bootstrap-shared-mysql.sh
```

## Docker 依赖服务启动

进入项目目录：

```bash
cd ~/projects/cpp_project/04_BridgeIM
```

启动依赖服务：

```bash
docker compose up -d
```

查看状态：

```bash
docker compose ps
```

当前 compose 只负责：

- `redis`
- `nginx`

## 构建前置：mprpc 依赖

`FriendService` / `OfflineMessageService` 等 RPC 目标依赖 mprpc 静态库，外加 protobuf 与 zookeeper_mt 开发库。mprpc 源码已作为 **git submodule** 内置在 `third_party/mprpc`，因此从干净 clone 即可构建：

```bash
# 1. 拉取含子模块的源码
git clone --recursive <repo-url>
# 若已普通 clone，则补拉子模块：
git submodule update --init --recursive

# 2. 构建（autobuild 在 mprpc 未就绪时会自动构建它）
./autobuild.sh
```

`autobuild.sh` 检测不到 mprpc 时，会自动调用 `scripts/build-mprpc.sh` 把子模块编译进 `third_party/mprpc/dist`。也可单独执行：`./scripts/build-mprpc.sh`。

CMake 解析 mprpc 的优先级（见 `cmake/Mprpc.cmake`）：

1. `-DMPRPC_ROOT=/path/to/mprpc`
2. 环境变量 `MPRPC_ROOT`
3. 内置子模块构建产物 `third_party/mprpc/dist`
4. 姊妹项目 `../03_rpc_framework/dist/mprpc`（本地并排开发便捷默认）

系统还需安装 protobuf 与 ZooKeeper C 客户端开发库（`zookeeper_mt`）；都找不到时 CMake 会给出明确报错。

## 本地构建

执行：

```bash
./autobuild.sh
```

或者手动执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

产物：

- `bin/ChatServer`
- `bin/ChatClient`

## 启动前先加载环境变量

`ChatServer` 和本地模式下的 `Redis` / `MySQL` 连接参数来自 `.env`。  
每次准备启动本地服务端前，建议先在当前终端执行：

```bash
set -a
source .env
set +a
```

## 本地运行服务端

启动节点 1：

```bash
./bin/ChatServer 0.0.0.0 6000
```

启动节点 2：

```bash
./bin/ChatServer 0.0.0.0 6002
```

说明：

- `redis` 走 Docker 内 `6379`
- `nginx` 在 Docker 内监听 `8000`
- `nginx` 会转发到宿主机本地的：
  - `6000`
  - `6002`
- 因为 `nginx` 是 Docker 容器，所以本地 `ChatServer` 必须绑定 `0.0.0.0`，不能只绑定 `127.0.0.1`

## 本地运行客户端

默认走集群入口：

```bash
./bin/ChatClient 127.0.0.1 8000
```

注意：

- `8000` 走的是 Docker 内 Nginx 集群入口
- Nginx 默认会在 `6000` 和 `6002` 两个本地后端之间转发
- 如果你只启动了一个 `ChatServer` 节点，不要连 `8000`，应该直接连对应节点端口

如果你想直连某个节点：

节点 1：

```bash
./bin/ChatClient 127.0.0.1 6000
```

节点 2：

```bash
./bin/ChatClient 127.0.0.1 6002
```

推荐测试习惯：

- 先做单节点验证：`6000 + client 直连 6000`
- 再做集群入口验证：`6000 + 6002 + client 连 8000`

## 推荐运行顺序

### 第一次

```bash
cd ~/projects/cpp_project/04_BridgeIM
cp .env.example .env
MYSQL_ROOT_PASSWORD=你的_mysql_root_密码 ./scripts/bootstrap-shared-mysql.sh
docker compose up -d
./autobuild.sh
set -a && source .env && set +a
./bin/ChatServer 0.0.0.0 6000
./bin/ChatServer 0.0.0.0 6002
./bin/ChatClient 127.0.0.1 8000
```

### 日常开发

```bash
cd ~/projects/cpp_project/04_BridgeIM
docker compose up -d
./autobuild.sh
set -a && source .env && set +a
./bin/ChatServer 0.0.0.0 6000
./bin/ChatServer 0.0.0.0 6002
./bin/ChatClient 127.0.0.1 8000
```

## 日志与排查

看依赖服务日志：

```bash
docker compose logs -f redis
docker compose logs -f nginx
```

如果客户端提示：

```text
this account is using
```

说明数据库里用户状态残留为 `online`，可以清理：

```bash
docker exec -it mysql_db mysql -uchatserver_app -pchatserver_dev_password -D chatserver -e "UPDATE User SET state='offline' WHERE state='online'; SELECT id,name,state FROM User;"
```

## 集群验证方式

1. 启动 `redis` 和 `nginx`
2. 本地开两个 `ChatServer` 节点：
   - `./bin/ChatServer 0.0.0.0 6000`
   - `./bin/ChatServer 0.0.0.0 6002`
3. 本地开两个客户端
4. 一个客户端连 `8000`
5. 再结合服务端日志，确认不同节点之间能通过 Redis 转发消息

## Graphify

- `graphify-out/` 是本地分析产物目录，不是主业务源码
- 当前已执行过一次 `graphify update .`，可用于继续看结构树和调用流

## 当前设计说明

现在的主路线是：

- 本地开发 / 本地构建 / 本地运行
- Docker 只作为依赖环境

这也是当前 `BridgeIM` 重新起步的运行基线：先沿用稳定的本地运行链路，再逐步演化后续 BridgeIM 能力。
