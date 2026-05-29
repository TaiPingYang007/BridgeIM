# BridgeIM Refactor Plan

## 目标

把以下四个来源项目整理为同一个 `BridgeIM` 演化路线：

- `01_thread_pool`
- `02_cluster_chat_server`
- `03_rpc_framework`
- `06_connection_pool`

最终形态不是“四份代码拼盘”，而是一个以 IM 业务为核心、带有公共基础设施层和可拆分服务边界的微服务体系。

## 总原则

1. 以 `02_cluster_chat_server` 为业务主干，不反过来让线程池、连接池、RPC 框架主导业务结构。
2. 优先复用“抽象和能力”，不要整仓库直接搬运。
3. 先在单体内完成分层，再拆服务；先替换实现，再改变拓扑。
4. 对客户端保持外部协议稳定，优先保留当前 JSON + `\n` 的接入方式。
5. 内部服务通信可以逐步转向 Protobuf + RPC，但外部客户端协议不必一开始就改。

## 四个来源项目的角色定位

| 来源项目 | 提供的核心能力 | 在 BridgeIM 的角色 | 建议落点 |
| --- | --- | --- | --- |
| `02_cluster_chat_server` | 登录、注册、单聊、群聊、好友、群组、离线消息、Redis 跨节点投递、Nginx 入口 | 业务主骨架 | 保留当前 `src/server` / `include/server` 作为起点，后续逐步拆分 |
| `06_connection_pool` | MySQL 连接池、配置解析、连接借还、生产者/消费者补充连接 | 公共数据访问基础设施 | 新增 `src/common/db`、`include/common/db`，先替换当前直接建连逻辑 |
| `01_thread_pool` | 固定线程池、有界任务队列、异步结果回传、优雅关闭 | 公共并发执行基础设施 | 新增 `src/common/concurrency`、`include/common/concurrency`，先服务于单体内部异步任务 |
| `03_rpc_framework` | 服务注册发现、Provider/Channel、配置读取、ZooKeeper 发现、Protobuf RPC | 微服务通信基础设施 | 新增 `src/common/rpc`、`include/common/rpc`，先库化，再用于服务拆分 |

## 当前单体到未来服务的映射

当前 `ChatServer` 单体里，最重要的未来拆分边界大致如下：

| 当前代码/职责 | 未来模块 | 未来服务归属 |
| --- | --- | --- |
| `src/server/chatserver.cpp` 收包、分帧、连接回调 | 接入层 / 网关层 | `access-gateway` |
| `ChatService::login`、`ChatService::reg`、`UserModel` | 账号与用户域 | `user-service` |
| 在线连接表 `_userConnMap`、登录态、Redis 订阅/退订、在线状态维护 | 会话与在线状态域 | `presence-service` |
| `addFriend`、`addFriendHandle`、`FriendModel`、`FriendRequestModel` | 社交关系域 | `relation-service` |
| `createGroup`、`addGroup`、`addGroupHandle`、`GroupModel`、`GroupRequestModel` | 群组与成员关系域 | `group-service` |
| `oneChat`、`groupChat`、`deliverMessage`、`OfflineMsgModel` | 消息投递与离线消息域 | `message-service` |

这里的关键点不是立刻拆成 5 个进程，而是先把这些边界在单体里拆成清晰模块，之后再决定哪些边界值得独立成服务。

## 建议目录落点

先不要大改当前目录，而是分两步走：

### 第一步：在现有目录旁边抽公共库

```text
04_BridgeIM/
├── include/
│   ├── common/
│   │   ├── concurrency/
│   │   ├── db/
│   │   └── rpc/
│   └── server/
├── src/
│   ├── common/
│   │   ├── concurrency/
│   │   ├── db/
│   │   └── rpc/
│   ├── client/
│   └── server/
```

### 第二步：业务边界稳定后再分应用

```text
04_BridgeIM/
├── src/
│   ├── apps/
│   │   ├── access_gateway/
│   │   ├── user_service/
│   │   ├── presence_service/
│   │   ├── relation_service/
│   │   ├── group_service/
│   │   └── message_service/
│   ├── common/
│   └── proto/
```

## 分阶段重构表

| 阶段 | 目标 | 主要来源项目 | 核心动作 | 完成标志 |
| --- | --- | --- | --- | --- |
| `Stage 0` | 稳住当前基线 | `02_cluster_chat_server` | 保持 Mode A 可编译、可运行、可双节点验证；补齐文档和恢复路由 | `ChatServer` / `ChatClient` 可本地运行，Redis/Nginx 依赖链稳定 |
| `Stage 1` | 抽公共基础设施目录 | `06_connection_pool`、`01_thread_pool`、`03_rpc_framework` | 在 `src/common` 和 `include/common` 预留落点，先不拆业务 | 代码库出现公共层，业务层仍可运行 |
| `Stage 2` | 接入连接池 | `06_connection_pool` | 把当前 MySQL 直连改造成连接池借还；统一配置入口 | Model 层不再自己临时建连，数据库访问稳定通过连接池 |
| `Stage 3` | 接入线程池 | `01_thread_pool` | 把适合异步化的耗时操作从网络回调中剥离，建立任务执行边界 | 网络线程不再承担全部慢操作，线程池可控运行 |
| `Stage 4` | 拆单体内部业务层 | `02_cluster_chat_server` | 把 `ChatService` 拆成用户、关系、群组、消息、在线状态几个内部模块 | `ChatService` 不再是超大编排类，边界可单测、可替换 |
| `Stage 5` | 库化 RPC 框架 | `03_rpc_framework` | 抽出 `mprpc` 的配置、Provider、Channel、ZooKeeper 接入，去掉 example 导向结构 | RPC 框架以公共库形式存在，不再是独立练手仓库形态 |
| `Stage 6` | 首次服务拆分 | `02_cluster_chat_server` + `03_rpc_framework` | 保留 `access-gateway` 对客户端说 JSON；把部分域能力改为内部 RPC 调用 | 至少有一个后端域服务脱离原单体独立运行 |
| `Stage 7` | 形成 IM 微服务体系 | 全部 | 完成网关、用户、在线状态、关系、群组、消息几类服务的稳定协作 | 业务链路跨服务可用，部署拓扑清晰 |

## 推荐整合顺序

推荐顺序不是 `01 -> 02 -> 03 -> 06`，而是：

1. `02_cluster_chat_server`
2. `06_connection_pool`
3. `01_thread_pool`
4. `03_rpc_framework`

原因如下：

- `02` 决定业务骨架，不先稳住它，后面所有基础设施都会没有落点。
- `06` 最容易先落地，因为它直接服务当前 `Model -> MySQL` 这一层。
- `01` 适合在仍是单体时接入，先改善执行模型，再谈拆服务。
- `03` 应该最后进场，因为 RPC 拆分过早会把复杂度成倍放大。

## 每个来源项目的整合注意点

### `01_thread_pool`

- 当前仓库使用 `C++17`，而 `BridgeIM` 当前顶层还是 `C++11`。
- `ThreadPool.h` 使用了 `std::invoke_result_t`，直接并入当前工程会触发语言级别不匹配。
- 整合前需要先做一个决策：
  - 要么把 `BridgeIM` 顶层标准升级到 `C++17`
  - 要么改写线程池模板接口以兼容 `C++11`
- 我的建议是优先评估整体升级到 `C++17`，因为后续公共基础设施通常也更适合用更现代的标准库能力。

### `06_connection_pool`

- 当前连接池实现默认从 `./config/connection_pool.conf` 读取配置，和 `BridgeIM` 现在的 `.env` 习惯不一致。
- 当前是单例生命周期模型，析构时会等待借出连接归还，这很好，但要注意和服务进程关闭流程对齐。
- 整合时不要把 `example/main.cpp` 和独立 Docker 编排照搬进来，只吸收连接池库本身。
- 第一落点应该是替换 `db/db.cpp` 周边的建连方式，而不是先改业务协议。

### `03_rpc_framework`

- 要复用的是 `Provider`、`Channel`、`Config`、`ZooKeeper` 发现，不是把整个 example 目录当成业务实现搬进来。
- 当前 RPC 框架也自带 `logger`、`lockqueue` 等组件，和 `BridgeIM` 已有实现存在重叠，整合时必须去重。
- ZooKeeper 的职责应该是“服务注册发现”，不要试图让它替代 Redis 的消息分发职责。
- 客户端外部协议和内部 RPC 协议应该分开：外部保持 JSON，内部服务之间逐步转为 Protobuf。

### `02_cluster_chat_server`

- 它是业务真身，不应该被“大重写”。
- 最先要做的是把当前 `ChatService` 的方法按域拆开，而不是急着把文件名、类名全部重命名。
- 只有当内部边界足够清晰时，RPC 拆分才不会把单体混乱直接放大成分布式混乱。

## 我对第一批落地任务的建议

如果按最稳的节奏推进，下一批任务建议是：

1. 在 `BridgeIM` 里新增 `common/db` 和 `common/concurrency` 的空目录与最小 CMake 落点。
2. 先把当前数据库访问路径梳理成 `main -> ChatServer -> ChatService -> Model -> db/db.cpp -> MySQL` 的明确调用链。
3. 再把 `06_connection_pool` 抽成可编译的库，优先接进 `db` 层。
4. 等数据库层稳定后，再决定线程池是全局升级到 `C++17` 还是做兼容改写。

## 当前不建议做的事

- 不建议一开始就把四个项目整仓库合并。
- 不建议一开始就拆成很多进程。
- 不建议一开始就把客户端协议改成 Protobuf。
- 不建议在业务边界没拆清楚前，把 `mprpc` 直接塞进 `ChatService`。
