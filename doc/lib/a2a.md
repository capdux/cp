# `cap:a2a` — Agent-to-Agent (A2A) 多智能体协作协议标准库

## 1. 概述

`cap:a2a`（亦可通过 `import cap from "cap"` 访问 `cap.a2a`）是 CP 语言纯原生实现的工业级 **Agent-to-Agent (A2A)** 多智能体协作协议标准库，提供：
- **AgentCard 与能力发现**：标准化描述 Agent 名称、端点 URL、版本、技能（`AgentSkill`）与流式/推送能力，支持 RFC 8259 JSON 序列化与摘要解析。
- **Task 生命周期状态机与多模态载荷**：严格校验 `Submitted` / `Working` / `InputRequired` / `Completed` / `Failed` / `Canceled` / `Rejected` 状态转移，终态不可逆；支持 `Text`、`DataJson`、`FileUri` 载荷及 `Artifact` 产物分片。
- **JSON-RPC 2.0 任务路由与 SSE 流式事件**：内置 `A2aServer`（支持 `agent/card`、`tasks/send`、`tasks/get`、`tasks/cancel`，含 `historyLength` 截断与容量上限防护）及 `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` SSE 编解码。

## 2. 核心类型

- `AgentSkill` / `AgentCapabilities` / `AgentCard` / `AgentCardSummary`：Agent 元数据与能力声明。
- `TaskState`：`Submitted`、`Working`、`InputRequired`、`Completed`、`Failed`、`Canceled`、`Rejected`（含 `toString()`、`fromString()`、`isTerminal()`、`canTransitionTo()`）。
- `PartKind` / `Part` / `MessageRole` / `Message` / `Artifact`：任务消息与产物结构。
- `A2aServerConfig` / `A2aTask` / `A2aServer`：有界容量的任务路由器与状态机管理器。
- `SseEventKind` / `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` / `SseTaskEvent` / `ParsedSseTaskEvent`：SSE 流式事件模型。
- `A2aError`：协议错误集（`InvalidJson`、`TaskNotFound`、`TaskNotCancelable`、`InvalidStateTransition`、`CapacityExceeded`、`InvalidSseEvent` 等）。

## 3. API 速查

| 符号 | 说明 |
| :--- | :--- |
| `encodeAgentCardJson(alloc, card)` | 将 `AgentCard` 序列化为符合 RFC 8259 转义的 JSON |
| `parseAgentCardSummary(alloc, json)` | 解析 `AgentCard` JSON 摘要（需调用 `summary.deinit(alloc)` 释放） |
| `A2aServer.init(alloc, card, config)` | 初始化 A2A 任务路由器 |
| `server.sendTask(task_id, ctx_id, msg)` | 创建任务或对 `InputRequired` 任务续传追问 |
| `server.transitionTask(task_id, next, msg)` | 执行受控状态转移并可选追加 Agent 回复 |
| `server.addArtifact(task_id, artifact)` | 为任务追加输出产物 |
| `server.cancelTask(task_id)` | 取消非终态任务；终态返回 `error.TaskNotCancelable` |
| `server.handleJsonRpc(raw_req)` | 路由 `agent/card`、`tasks/send`、`tasks/get`、`tasks/cancel` |
| `encodeTaskEventSse(alloc, event)` | 编码 `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` 为 SSE 帧 |
| `parseSseTaskEvent(alloc, raw_sse)` | 解析 SSE 帧为 `ParsedSseTaskEvent`（需调用 `deinit(alloc)`） |
| `AgentRouter.init(peers, heartbeat_timeout_ms)` | 初始化零堆分配固定池多智能体能力路由器（`router.cp`） |
| `router.registerPeer(*_, id, name, skill_mask, priority, max_concurrency, now_ms)` | 注册智能体节点（拒绝空名、重复 ID 与池满） |
| `router.heartbeat(*_, id, now_ms, health)` | 刷新节点心跳时间戳与健康状态（`Healthy`/`Degraded`/`Unhealthy`/`Draining`） |
| `router.routeTask(*_, required_skills, now_ms, exclude_peer_id)` | 64 位技能位掩码 O(1) 匹配，按健康优先→最低负载率→最高优先级→最小 ID 路由并递增并发数 |
| `router.releaseTask(*_, id)` | 任务完成后递减节点 `active_tasks` |
| `router.listCapablePeers(required_skills, now_ms, out_ids)` | 列出满足技能且存活可路由的节点 ID |
| `HandoffTrace.init(buf, max_hops)` | 初始化零堆分配交接链防环与跳数守卫（`recordHop` / `hasVisited` / `hops`） |

## 4. 示例

### 4.1 A2A Server JSON-RPC 路由示例

```typescript
import { AgentCard, AgentSkill, A2aServer, encodeAgentCardJson } from "cap:a2a";

const skills: [1]AgentSkill = [AgentSkill{ .id = "code-review", .name = "Code Review" }];
const card = AgentCard{ .name = "Reviewer", .url = "https://agent.local/a2a", .skills = &skills };
let srv = A2aServer.init(alloc, card, .{});
defer srv.deinit();

const resp = try srv.handleJsonRpc("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tasks/send\",\"params\":{\"id\":\"t-1\",\"message\":{\"text\":\"check diff\"}}}");
defer alloc.free(resp);
```

### 4.2 零堆分配多智能体技能路由与 Handoff 防环示例 (`router.cp`)

```typescript
import { AgentPeer, AgentRouter, PeerHealth, HandoffTrace, HandoffStatus } from "cap:a2a";

pub fn demoHandoffRouting() {
    const SKILL_CODE: u64 = 1;
    const SKILL_SQL: u64 = 2;

    let pool: [4]AgentPeer = undefined;
    let pool_slice: []AgentPeer = pool[0..];
    let router = AgentRouter.init(pool_slice, 5000);

    * = router.registerPeer(1, "planner", SKILL_CODE, 10, 4, 1000);
    * = router.registerPeer(2, "sql-worker", SKILL_CODE | SKILL_SQL, 5, 4, 1000);

    let trace_buf: [8]u32 = undefined;
    let trace_slice: []u32 = trace_buf[0..];
    let trace = HandoffTrace.init(trace_slice, 4);
    * = trace.recordHop(1);

    // 排除当前节点 1，路由至具备 SKILL_SQL 的最优健康节点
    if (next_peer := router.routeTask(SKILL_SQL, 2000, 1)) {
        if (trace.recordHop(next_peer) == HandoffStatus.Ok) {
            // 执行跨 Agent 交接后释放并发槽位
            * = router.releaseTask(next_peer);
        }
    }
}
```
