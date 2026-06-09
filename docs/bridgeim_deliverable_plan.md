# BridgeIM 可投递交付计划 (Deliverable Plan)

> 本文件是 BridgeIM 的**正式交付计划**,与总体重构路线 [bridgeim_refactor_plan.md](bridgeim_refactor_plan.md) 互补:
> - `bridgeim_refactor_plan.md` = 长期愿景(Stage 0–7,完整微服务体系)
> - 本文件 = **面向"可投递"的有限收口计划**,决定我们实际做到哪、怎么做、在哪停。

**目标(一句话):** 把 BridgeIM 收口成一个**随时可交付、可持续完善**的 C++ 后端简历项目,核心卖点是"集群 IM + 连接池 + 线程池 + 异步日志 + RPC 拆服务"。

**核心理念:** 不赌一个一次性终点,而是把交付拆成**一串各自独立可交付的台阶**。每完成一阶,项目都处于"可投递、不烂尾"状态;时间用完就在当前台阶停,之后可随时继续往上加。

**当前结论(2026-06-09,已核对代码):** 按 `bridgeim_refactor_plan.md` 自己的验收标准,**Stage 5(mprpc 已库化引入)与 Stage 6(至少一个域服务独立运行,实际有两个)已经达标**。因此剩下的不是"补功能",而是"补可信度收口"。

---

## 一、范围决策:做到哪、不做哪

| 台阶 | 内容 | 状态 | 是否纳入可投递 |
| --- | --- | --- | --- |
| **Phase 0 — 地板线** | Stage 5/6 收口(可复现构建 + 两处代码可信度修复 + 文档刷新 + 端到端 demo + Stage 7 设计文档) | 待做 | ✅ **必做**,做完即"随时可交" |
| **Phase 1 — +user-service** | 把登录/注册/状态拆成 RPC 服务(套好友服务模板) | 待做 | 🟡 时间够就做(3 个服务) |
| **Phase 2 — +group-service** | 把群组/成员/群聊拆成 RPC 服务 | 待做 | 🟡 时间更够再做(所有数据域皆服务化) |
| **Descoped — Stage 7 真实现** | presence + message 跨节点路由的**真实分布式实现** | **主动不做** | ❌ 仅写设计文档,不实现 |

**为什么 descope Stage 7 的真实现:** 活的 TCP 连接(`_userConnMap` 里的 `muduo::net::TcpConnectionPtr`)物理上只能待在持有 socket 的网关进程里。一旦业务判断("谁该收消息")移出网关,就必须重新设计"网关节点 ↔ 后端服务"之间的 presence/路由契约 —— 这是会吃掉数周到约两个月、且方差极大的真正硬骨头(详见 [stage7-design-notes.md](stage7-design-notes.md),由 Phase 0 T6 产出)。其面试边际价值远低于成本与烂尾风险。**看清硬点在哪并做出有意识的范围取舍,本身就是要展示的工程判断。**

---

## 二、Phase 0 — 地板线(必做,做完即可交)

> 完成本阶段后,项目从"本机能跑"升级为"别人 clone 能跑、文档能讲、一问不穿帮"。
> 任务建议顺序执行;每个任务结束都应是一个干净可编译的提交。

### Task 1:构建可复现(消除 repo 外硬依赖)

**问题:** [cmake/Mprpc.cmake](../cmake/Mprpc.cmake) 默认把 mprpc 指向 repo 外的 `../03_rpc_framework/dist/mprpc`。别人单独 clone BridgeIM 时该目录不存在,RPC 目标直接 `FATAL_ERROR`。

**Files:**
- Modify: `cmake/Mprpc.cmake`(增加 `third_party/mprpc` 本地回退查找路径)
- Modify: `README.md`(新增"构建前置:mprpc 依赖"小节)
- Create: `docs/build-prerequisites.md`(可选,详细前置说明)

**Steps:**
- [ ] **Step 1:** 在 `cmake/Mprpc.cmake` 的 `MPRPC_ROOT` 解析顺序里追加第三优先级:`${CMAKE_SOURCE_DIR}/third_party/mprpc`(顺序为 `-DMPRPC_ROOT` → 环境变量 `MPRPC_ROOT` → `third_party/mprpc` → 现有 sibling 默认),保留现有的友好 `FATAL_ERROR` 文案。
- [ ] **Step 2:** 在 `README.md` 新增"构建前置:mprpc 依赖"小节,写清三条获取路径:(a) 设 `MPRPC_ROOT` 指向已构建的 03_rpc_framework dist;(b) 把构建产物放到 `third_party/mprpc`;(c) 从源码构建 03_rpc_framework 的精确命令。
- [ ] **Step 3(验收):** 模拟"干净 clone"验证:`rm -rf build && rtk cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMPRPC_ROOT=<path>` 应配置成功;不带任何 MPRPC_ROOT 且无 third_party 时,应给出可读的 `FATAL_ERROR` 而非 CMake 内部错误。
- [ ] **Step 4:** `rtk cmake --build build --parallel` 全量构建通过(`ChatServer`/`ChatClient`/`FriendService`/`OfflineMessageService`)。
- [ ] **Step 5:** 提交 `chore: make mprpc dependency reproducible from a clean checkout`。

**Acceptance:** 一个只拿到本 repo 的人,按 README 能成功配置并构建出全部目标。

---

### Task 2:修复 RPC channel 生命周期(每次调用 `new MprpcChannel()`)

**问题:** [chatservice.cpp](../src/server/chatservice.cpp) 每个 RPC wrapper 都 `stub(new MprpcChannel())`(如 L18/L34/L83…)。单参 Stub 构造**不接管 channel** → 每次调用疑似泄漏一个 channel,且每次都重做 ZooKeeper 服务发现 + 新建 TCP 连接。**面试必问点。**

**Files:**
- Modify: `src/server/chatservice.cpp`(RPC wrapper 区域 L15–L210 附近)

**Steps:**
- [ ] **Step 1(先调研):** 阅读 mprpc 的 `MprpcChannel` 实现(`MPRPC_ROOT/include/mprpcchannel.h` 及其源码),确认:(a) 是否每次构造都做 ZK 发现 + 连接;(b) 单个 channel 的 `CallMethod` 是否线程安全。ChatServer 业务用的是多 worker 线程池(`include/common/concurrency/ThreadPool.hpp`),所以并发安全是硬约束。
- [ ] **Step 2(选型):** 依据 Step 1 结论三选一并记录理由:(a) channel 线程安全 → 每服务一个进程级单例 channel;(b) 不安全 → `thread_local` channel(每 worker 一个)或小型 channel 池;(c) 维持每调用新建但显式回收(仅止血,不推荐,因 ZK/重连开销仍在)。优先 (a)/(b)。
- [ ] **Step 3:** 按选型重写 wrapper:消除每调用 `new`,确保无泄漏、无每调用 ZK 重发现、且在业务线程池下并发安全。
- [ ] **Step 4(验收):** 全量构建通过;启动 FriendService/OfflineMessageService + ChatServer,跑 Task 4 的 demo,RPC 路径正常;在 RPC 服务日志中确认连接/发现不再每请求重复发生。
- [ ] **Step 5:** 提交 `fix: reuse mprpc channels instead of constructing one per RPC call`。

**Acceptance:** 单次业务流程不再为每个 RPC 调用新建并丢弃 channel;并发调用安全。

---

### Task 3:修复虚假的 "fallback to local"

**问题:** offline 三个 wrapper 的日志写着 `fallback to local`([chatservice.cpp:26](../src/server/chatservice.cpp#L26)/L41/L61),但代码只 `return false`/`return {}`,**根本没有回退本地 Model**。RPC 服务挂掉时离线消息会**静默丢失**(调用方 L313/L897 还忽略返回值)。

**Files:**
- Modify: `src/server/chatservice.cpp`(offline wrapper + 调用点 L313/L897)

**Steps:**
- [ ] **Step 1(定方向,二选一):**
  - 方案 A(诚实降级):真的实现回退 —— RPC 失败时调用本地 `_offlineMsgModel` 写库,保证不丢消息。
  - 方案 B(诚实标注):不做回退,但删除误导性 `fallback to local` 文案,改为明确的失败日志,并让调用点正确处理失败(至少记录,不静默吞)。
  - 推荐 A,因为"RPC 挂了也不丢离线消息"是更好的面试叙事。
- [ ] **Step 2:** 按所选方案修改三个 offline wrapper 与调用点 L313/L897,使返回值被正确处理。
- [ ] **Step 3(验收):** 构建通过;**故意不启动** OfflineMessageService,触发一次离线投递,确认行为符合所选方案(A:消息落本地库;B:有明确错误日志、无静默丢失)。
- [ ] **Step 4:** 提交 `fix: make offline-message RPC failure handling match its documented behavior`。

**Acceptance:** 代码行为与注释/日志一致;RPC 失败不再静默丢消息。

---

### Task 4:端到端 demo / 冒烟脚本

**问题:** 缺一个"一条命令把整条 RPC 链路跑给人看"的东西。面试可演示性 = 硬通货。

**Files:**
- Create: `scripts/smoke-rpc-demo.sh`
- Create: `docs/demo.md`(演示说明 + 预期输出截图位)

**Steps:**
- [ ] **Step 1:** 写 `scripts/smoke-rpc-demo.sh`:加载 `.env` → 启动 `OfflineMessageService`、`FriendService`、两个 `ChatServer` 节点(后台,记录 PID)→ 提示用 `ChatClient` 跑"注册两个用户 → 互加好友 → 登录拉好友列表 → 单聊 → 一方离线再收离线消息"。脚本结尾提供统一 `kill` 收尾。
- [ ] **Step 2:** 脚本健壮性:依赖未就绪(Redis/MySQL/ZK)时给出可读报错;`set -euo pipefail`;退出时清理后台进程(`trap`)。
- [ ] **Step 3:** 写 `docs/demo.md`:列出演示步骤、每步预期客户端输出、以及如何在服务端日志里看到"好友/离线走了 RPC、跨节点走了 Redis"。
- [ ] **Step 4(验收):** 全新终端执行 `rtk ./scripts/smoke-rpc-demo.sh`,按提示走完一遍,确认好友列表、单聊、离线消息、跨节点投递均正常。
- [ ] **Step 5:** 提交 `docs: add end-to-end RPC smoke demo script and walkthrough`。

**Acceptance:** 一个新人按 `docs/demo.md` 能独立把核心链路演示出来。

---

### Task 5:文档刷新(消除"卖便宜"的进度落差)

**问题:** [README.md](../README.md) 与 PROJECT_CONTEXT.md 仍停在"Stage 2 早期",严重低估实际进度(实为 Stage 5/6),审阅者会误判。

**Files:**
- Modify: `README.md`
- Modify: `PROJECT_CONTEXT.md`(本地上下文,见 §四)

**Steps:**
- [ ] **Step 1:** 更新 README"重构蓝图/当前阶段"段:如实写明已完成连接池、线程池、异步日志、RPC 库化 + 两个独立 RPC 服务(FriendService / OfflineMessageService)。
- [ ] **Step 2:** 在 README 增补一张"运行拓扑"图:`ChatClient → Nginx → ChatServer×2`,以及 `ChatServer → mprpc Channel → FriendService / OfflineMessageService`、`ChatServer → Redis(跨节点) / MySQL`。
- [ ] **Step 3:** README 增加指向本计划与 demo 的链接;明确标注"presence/跨节点消息路由的完整微服务化为有意识的未来工作(见 stage7-design-notes.md)"。
- [ ] **Step 4(验收):** 通读 README,确保一个陌生读者能在 5 分钟内看懂"这个项目做到了哪、亮点是什么、怎么跑起来"。
- [ ] **Step 5:** 提交 `docs: refresh README to reflect actual RPC integration progress`。

**Acceptance:** 文档不再低估实际工作;读者第一印象与代码实况一致。

---

### Task 6:Stage 7 设计文档(把"没做"转成"主动设计后收口")

**问题:** Stage 7 的真正难点(presence + 跨节点消息路由)不实现,但要**写清设计**,以展示判断力。

**Files:**
- Create: `docs/stage7-design-notes.md`

**Steps:**
- [ ] **Step 1:** 描述现状耦合:`deliverMessage`(本机连接 → Redis 跨节点 → 离线)与 `handleRedisSubscribeMessage` 如何把"活连接 + 在线状态 + 跨节点 fanout"绑在网关进程内([chatservice.cpp:300](../src/server/chatservice.cpp#L300)、[L888](../src/server/chatservice.cpp#L888))。
- [ ] **Step 2:** 写出核心矛盾:业务判断移出网关后,"消息如何到达活 socket"的分布式路由问题。
- [ ] **Step 3:** 给出候选设计(至少一种较完整):presence-service 记录"用户→网关节点"映射;message-service 路由到目标网关节点;网关节点持有 socket 负责最终推送;含一致性/掉线/重连/惊群等风险点。
- [ ] **Step 4:** 明确写下"为何当前主动不实现"的工程理由(成本、方差、烂尾风险、面试边际价值)。
- [ ] **Step 5:** 提交 `docs: add Stage 7 presence/routing design notes (deliberately descoped)`。

**Acceptance:** 文档能让读者相信"作者理解硬点、有方案、且是主动取舍"。

---

## 三、Phase 1 / Phase 2 — 可选增量(时间够再做)

> 这两阶是"同一套已验证的 RPC 拆服务模板"的复用,**低设计风险**。每阶完成都是一个独立可交付的更强版本。每阶仍遵循 Phase 0 的纪律:小步、可编译、勤提交、端到端验证、更新 demo 与文档。

### Phase 1:user-service(把登录/注册/状态 RPC 化)

- 参照 `src/services/friend_service/` 与 `proto/friend_service.proto` 的模板新建 `proto/user_service.proto` 与 `src/services/user_service/main.cpp`,封装 `UserModel` 的注册/登录/状态读写。
- 在 `chatservice.cpp` 增加 user RPC wrapper(复用 Task 2 的 channel 生命周期方案),把 `login`/`reg` 的 Model 调用切到 RPC。
- 注意:登录涉及在线状态与连接登记;**仅把数据库读写 RPC 化**,在线表/连接登记仍留在网关(与 descope 决策一致)。
- 验收:扩展 `scripts/smoke-rpc-demo.sh`,在 UserService 进程参与下跑通注册/登录;更新 demo 与 README。

### Phase 2:group-service(把群组/成员/群聊 RPC 化)

- 新建 `proto/group_service.proto` 与 `src/services/group_service/main.cpp`,封装 `GroupModel`/`GroupRequestModel`(建群、加群申请、成员与角色查询)。
- 在 `chatservice.cpp` 把 `createGroup`/`addGroup`/`addGroupHandle`/`groupChat` 的 Model 调用切到 RPC;群聊扩散的"逐成员投递"仍由网关 `deliverMessage` 负责(连接在网关)。
- group 比 friend 更绕(成员集合、角色、群发),预留更多调试时间。
- 验收:demo 增加建群 → 加群 → 群聊场景;更新 demo 与 README。
- **达成后的叙事:** 所有"数据/业务域"皆已服务化,presence + 消息路由按设计留在网关 —— 一个完整、自洽、且无分布式烂尾风险的故事。

---

## 四、维护约定

- **本计划(本文件)进 git**,作为可投递材料的一部分。
- **当前进度/下一步动作记录在 [PROJECT_CONTEXT.md](../PROJECT_CONTEXT.md)**(本地、不进 git)的"可投递交付计划与当前状态"小节,每次推进后更新。
- 每个 Task 完成 = 一次干净可编译提交;每个 Phase 完成 = 一个"可投递"快照。
- 触碰 ChatServer/线程池/日志时,保留既有安全约束(`enqueue` 满抛异常、worker 内业务异常需捕获、Logger `Shutdown()` drain)。

---

## 五、执行方式

本计划的 Phase 0 任务可逐个执行,每任务结束在本会话内复核。建议起点:**Task 1(构建可复现)** —— 它是 demo 与文档的地基。
