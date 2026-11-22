# `cap:a2a` — Agent-to-Agent (A2A) 多智能体协作协议标准库

## 1. 概述

`cap:a2a`（亦可通过 `import cap from "cap"` 访问 `cap.a2a`）是 CP 语言纯原生实现的工业级 **Agent-to-Agent (A2A)** 多智能体协作协议标准库，提供：
- **AgentCard 与能力发现**：标准化描述 Agent 名称、端点 URL、版本、技能（`AgentSkill`）与流式/推送能力，支持 RFC 8259 JSON 序列化与摘要解析。
- **Task 生命周期状态机与多模态载荷**：严格校验 `Submitted` / `Working` / `InputRequired` / `Completed` / `Failed` / `Canceled` / `Rejected` 状态转移，终态不可逆；支持 `Text`、`DataJson`、`FileUri` 载荷及 `Artifact` 产物分片。
- **JSON-RPC 2.0 任务路由与 SSE 流式事件**：内置 `A2aServer`（支持 `agent/card`、`tasks/send`、`tasks/get`、`tasks/cancel`，含 `historyLength` 截断与容量上限防护）及 `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` SSE 编解码。
- **零堆分配多智能体能力协商与 SLA 契约择优 (`negotiation.cp`)**：通过 `A2aModalityMask` / `A2aFeatureMask` 位掩码进行输入/输出模态与特性交集计算，`checkPeerCompatibility` 精确分类 7 类硬约束拒绝原因，`negotiateBestPeers` 基于可靠性、偏好特性命中、延迟与成本综合打分零堆分配输出 Top-K 最优协作节点。

## 2. 核心类型

- `AgentSkill` / `AgentCapabilities` / `AgentCard` / `AgentCardSummary`：Agent 元数据与能力声明。
- `TaskState`：`Submitted`、`Working`、`InputRequired`、`Completed`、`Failed`、`Canceled`、`Rejected`（含 `toString()`、`fromString()`、`isTerminal()`、`canTransitionTo()`）。
- `PartKind` / `Part` / `MessageRole` / `Message` / `Artifact`：任务消息与产物结构。
- `A2aServerConfig` / `A2aTask` / `A2aServer`：有界容量的任务路由器与状态机管理器。
- `SseEventKind` / `TaskStatusUpdateEvent` / `TaskArtifactUpdateEvent` / `SseTaskEvent` / `ParsedSseTaskEvent`：SSE 流式事件模型。
- `A2aError`：协议错误集（`InvalidJson`、`TaskNotFound`、`TaskNotCancelable`、`InvalidStateTransition`、`CapacityExceeded`、`InvalidSseEvent` 等）。
- `A2aModalityMask` / `A2aFeatureMask` / `A2aRequirement` / `A2aPeerOffer` / `NegotiationRejectReason` / `NegotiationMatch`：多智能体能力协商位掩码、需求/报价模型、硬约束拒绝原因与 Top-K 匹配结果（`negotiation.cp`）。

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
| `A2aEventStreamRing.init(slots)` | 初始化零堆分配任务事件流环形回放缓冲（`event_stream.cp`） |
| `ring.publish(*_, task_id, kind, timestamp_ms, is_final, payload_hash, payload_len)` | 分配单调递增 `event_id` 并写入事件，环满自动覆盖最旧槽位 |
| `ring.replayAfter(last_event_id, task_filter, out_events)` | 按 `Last-Event-ID` 游标与可选 `task_id` 过滤回放事件，返回 `ReplayResult`（含 `gap_detected` 覆盖缺口检测与截断安全） |
| `ring.latestEventId()` / `ring.oldestEventId()` / `ring.count()` / `ring.clear()` | 查询最新/最旧保留事件 ID、有效数量与清空缓冲 |
| `checkPeerCompatibility(req, offer)` | 按严格优先级校验离线、协议版本、输入/输出模态、必选特性、延迟与成本上限，返回 `NegotiationRejectReason` |
| `scoreCompatiblePeer(req, offer)` | 按可靠性(4000) + 偏好特性命中(2500) + 延迟效率(2000) + 成本效率(1500) 计算 `[0, 10000]` 整数综合分 |
| `negotiateBestPeers(req, offers, *out_matches)` | 零堆分配过滤不兼容节点并原地降序输出全局 Top-K 最优 `NegotiationMatch`（含模态/特性位掩码交集） |

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

### 4.3 零堆分配 A2A 任务生命周期状态机与租约优先级队列 (`task_queue.cp`)

- **核心类型**：`A2aTaskState`（`Submitted | Working | InputRequired | Completed | Failed | Canceled | DeadLetter`）、`A2aTaskItem`、`A2aQueueStats`
- **状态机校验**：`canTransition(from, to): bool`（终态 `Completed`/`Failed`/`Canceled`/`DeadLetter` 严格不可逆）
- **`A2aTaskQueue`**：
  - `init(slots)` / `enqueue(task_id, idempotency_key_hash, priority, created_ms, deadline_ms, max_attempts)`（幂等键去重，防重复派发）
  - `claimNext(worker_id, now_ms, lease_ms)`（按高优先级 -> 最早 `deadline_ms` -> `created_ms` 认领并授租约）
  - `heartbeat(task_id, worker_id, now_ms, lease_ms)`（租约续期）
  - `reclaimExpiredLeases(now_ms)`（超时自动重入队或超 `max_attempts` 转 `DeadLetter`）
  - `complete` / `fail` / `requestInput` / `resumeInput` / `cancel` / `findTask` / `stats`

```typescript
import { A2aTaskItem, A2aTaskQueue } from "cap:a2a";

pub fn demoA2aLeaseQueue() {
    let slots_arr: [8]A2aTaskItem = undefined;
    let slots: []A2aTaskItem = slots_arr[0..];
    let q = A2aTaskQueue.init(slots);

    * = q.enqueue(101, 0xCAFE, 10, 1000, 5000, 3);
    if (tid := q.claimNext(1, 1100, 2000)) {
        * = q.heartbeat(tid, 1, 2000, 2000);
        * = q.complete(tid, 1, 2);
    }
    * = q.stats();
}
```

### 4.4 零堆分配任务事件流环形回放与 `Last-Event-ID` 断线续传示例 (`event_stream.cp`)

```cp
import {
    A2aEventKind,
    A2aStreamEvent,
    A2aEventStreamRing,
} from "cap:a2a";

pub fn demoA2aEventStreamResume() {
    let slots_buf: [16]A2aStreamEvent = [A2aStreamEvent{}; 16];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    const id1 = ring.publish(42, A2aEventKind.StatusUpdate, 1000, false, 0x101, 32);
    * = ring.publish(42, A2aEventKind.ArtifactAppend, 1050, false, 0x102, 64);
    * = ring.publish(42, A2aEventKind.ArtifactComplete, 1100, true, 0x103, 128);

    // 客户端携带 Last-Event-ID = id1 断线重连，按 task_id = 42 精确续传后续事件
    let out_buf: [8]A2aStreamEvent = [A2aStreamEvent{}; 8];
    let out: []A2aStreamEvent = out_buf[0..];
    const res = ring.replayAfter(id1, 42, out);
    if (res.gap_detected) {
        // 游标已被环形覆盖，通知客户端拉取任务全量快照
    }
    * = res;
}
```

### 4.5 零堆分配多智能体能力协商与 SLA 择优示例 (`negotiation.cp`)

```cp
import {
    A2aModalityMask,
    A2aFeatureMask,
    A2aRequirement,
    A2aPeerOffer,
    NegotiationMatch,
    negotiateBestPeers,
} from "cap:a2a";

pub fn demoA2aCapabilityNegotiation() {
    const req = A2aRequirement{
        required_in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
        required_out_modalities: A2aModalityMask.JSON,
        required_features: A2aFeatureMask.STREAMING,
        preferred_features: A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
        max_latency_ms: 800,
        max_cost_micro_usd: 50000,
        min_protocol_minor: 2,
    };

    const offers: [2]A2aPeerOffer = [
        A2aPeerOffer{
            peer_id: 10,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE | A2aModalityMask.IMAGE,
            out_modalities: A2aModalityMask.JSON | A2aModalityMask.TEXT,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY,
            p95_latency_ms: 420,
            cost_micro_usd: 22000,
            protocol_minor: 3,
            reliability_bps: 9920,
            online: true,
        },
        A2aPeerOffer{
            peer_id: 20,
            in_modalities: A2aModalityMask.TEXT | A2aModalityMask.CODE,
            out_modalities: A2aModalityMask.JSON,
            features: A2aFeatureMask.STREAMING | A2aFeatureMask.STATE_HISTORY | A2aFeatureMask.AUTH_MTLS,
            p95_latency_ms: 180,
            cost_micro_usd: 15000,
            protocol_minor: 3,
            reliability_bps: 9980,
            online: true,
        },
    ];

    let top_buf: [2]NegotiationMatch = [NegotiationMatch{}; 2];
    let top: []NegotiationMatch = top_buf[0..];
    const matched = negotiateBestPeers(req, offers[0..], top);
    // matched == 2, top[0].peer_id == 20（更高可靠性、全偏好特性命中且更低延迟与成本）
    * = matched;
}
```

### 4.6 零堆分配跨 Agent 任务委派、租约续期、超时故障转移与 Fencing Token 校验示例 (`delegation.cp`)

- **核心类型**：`DelegationState`（`Pending | Active | Completed | Failed | TimedOut | Escalated | Cancelled`）、`DelegationRecord`、`DelegationStats`、`A2aDelegationManager`
- **核心能力**：
  - `delegate(*_, task_id, owner_agent_id, backup_agent_id, lease_ms, deadline_ms, now_ms): u64?`：分配单调递增 `fence_token`，防止重复委派与满槽越界；
  - `renewLease(*_, task_id, fence_token, extend_ms, now_ms): bool`：仅持有当前有效 `fence_token` 且未过租约/总截止时间允许续租，自动封顶至 `deadline_ms`；
  - `complete(*_, task_id, fence_token, result_summary, now_ms): bool` / `fail(...)` / `cancel(...)`：严格校验 `fence_token`，拒绝过期僵尸 Worker 延迟回写并统计 `stale_fence_rejections`；
  - `sweepTimeouts(*_, now_ms): usize`：租约超时自动故障转移至 `backup_agent_id` 并轮换 `fence_token` 使旧 Worker 失效；超过 `deadline_ms` 自动标记 `Escalated`；无备用或备用再超时标记 `TimedOut`；
  - `get(task_id)`、`verifyFence(task_id, fence_token)`、`activeCount()`、`stats()`、`releaseTerminal(task_id)`（零堆分配槽位复用）。

```cp
import {
    DelegationRecord,
    A2aDelegationManager,
} from "cap:a2a";

pub fn demoA2aDelegationQuickStart() {
    let slots: [8]DelegationRecord = [DelegationRecord{}; 8];
    let slots_s: []DelegationRecord = slots[0..];
    let mgr = A2aDelegationManager.init(slots_s) ?? return;

    // 1. 委派任务 1001 给主节点 10，备用节点 20，租约 2000ms，总截止 10000ms
    const tok1 = mgr.delegate(1001, 10, 20, 2000, 10000, 1000) ?? return;
    // 主节点续租至 4500ms
    * = mgr.renewLease(1001, tok1, 2500, 2000);

    // 2. 假设主节点 10 失联，t=5000ms 触发 sweepTimeouts 自动切换至备用节点 20 并递增 fence_token
    * = mgr.sweepTimeouts(5000);

    // 3. 主节点 10 复活后持旧 tok1 尝试回写 -> 被 Fencing Token 拒绝！
    * = mgr.complete(1001, tok1, "stale result", 5200); // false

    // 4. 备用节点 20 持新 fence_token 正常完成任务
    if (rec := mgr.get(1001)) {
        * = mgr.complete(1001, rec.fence_token, "backup ok", 5300); // true
    }
    * = mgr.stats();
}
```
