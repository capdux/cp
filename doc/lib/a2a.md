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
| `A2aEventStreamRing.init(slots)` | 初始化零堆分配任务事件流环形回放缓冲（`events.cp`） |
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

### 4.3 零堆分配 A2A 任务生命周期状态机与租约优先级队列 (`taskQueue.cp`)

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

### 4.4 零堆分配任务事件流环形回放与 `Last-Event-ID` 断线续传示例 (`events.cp`)

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

### 4.7 零堆分配 A2A 任务生命周期状态机与 Artifact 流式分片路由器 (`lifecycle.cp`)

面向 Google Agent2Agent (A2A) 多智能体异步任务编排与流式产物交付的 100% 零动态堆分配内核：

- **容量常量**：`A2A_LIFECYCLE_MAX_TASKS = 64`、`A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK = 4`、`A2A_ARTIFACT_ID_MAX_LEN = 64`、`A2A_ARTIFACT_MIME_MAX_LEN = 64`。
- **核心类型**：
  - `A2aTaskPhase`：`submitted | working | input_required | auth_required | completed | failed | canceled | rejected`，内置 `isTerminal()` 与 `canTransitionTo(next)`（严格拦截终态迁移、自环及跳过 `working` 直接 `completed` 等非法边）。
  - `A2aArtifactMeta`：定长内联 `artifact_id` / `mime_type`、`chunks_received`、`total_bytes`、`finalized`、`last_seq`。
  - `A2aTaskRecord`、`A2aTransitionOutcome`（`ok` / `prev_phase` / `next_phase` / `rejected_terminal` / `invalid_edge`）、`A2aLifecycleStats`。
- **核心管理器 `A2aTaskLifecycleManager`**：
  - `submit_task(task_id, context_id, now_ms): bool`：ID 去重与 64 槽位满槽守卫。
  - `transition(task_id, next_phase, now_ms): A2aTransitionOutcome`：按 A2A 规范严格校验状态转移边并原子维护阶段统计。
  - `request_cancel(task_id, now_ms): bool`：非终态标记 `cancel_requested` 并流转至 `canceled`；终态安全拒绝。
  - `append_artifact_chunk(task_id, artifact_id, mime_type, seq, chunk_bytes, is_last_chunk): bool`：仅 `working` / `input_required` 态允许写入，严格校验单调递增 `seq` 防乱序/重复，`finalized` 封口后拒绝继续追加。
  - `prune_terminal_before(cutoff_ms): usize`：回收 `updated_ms < cutoff_ms` 的终态任务及关联 Artifact 槽位供复用。
  - `get_task(task_id)`、`get_artifact(task_id, artifact_id)`、`stats()`。

```cp
import {
    A2aTaskPhase,
    A2aTaskLifecycleManager,
} from "cap:a2a";

pub fn demoA2aTaskLifecycle() {
    let mgr = A2aTaskLifecycleManager.init();
    * = mgr.submit_task(1001, 500, 1000);
    * = mgr.transition(1001, A2aTaskPhase.working, 1010);

    // 流式分片交付产物（单调序号 1 -> 2 并封口）
    * = mgr.append_artifact_chunk(1001, "report.md", "text/markdown", 1, 512, false);
    * = mgr.append_artifact_chunk(1001, "report.md", "text/markdown", 2, 256, true);

    * = mgr.transition(1001, A2aTaskPhase.completed, 1100);
    // 回收过期终态任务槽位
    * = mgr.prune_terminal_before(2000);
}
```

### 4.8 零堆分配 A2A AgentCard 能力发现注册表与健康加权路由器 (`cards.cp`)

面向多智能体网络（Agent2Agent Mesh）的 100% 纯 CP 零动态堆分配 AgentCard 服务发现、技能/模态位图过滤、TTL 心跳过期摘除与健康加权路由择优内核（固定 64 槽位内联数组）：

- **容量与能力位图常量**：
  - `A2A_REGISTRY_MAX_CARDS = 64`、`A2A_CARD_NAME_MAX_LEN = 48`、`A2A_CARD_URL_MAX_LEN = 128`
  - 模态位图：`A2A_MOD_TEXT`、`A2A_MOD_JSON`、`A2A_MOD_IMAGE`、`A2A_MOD_AUDIO`、`A2A_MOD_FILE`
  - 协议特性位图：`A2A_FEAT_STREAMING`、`A2A_FEAT_PUSH_NOTIFY`、`A2A_FEAT_STATE_HISTORY`
- **数据模型**：`A2aCardEntry`（`create`, `nameSlice`, `urlSlice`, `isValid`）、`A2aDiscoveryQuery`、`A2aDiscoveryHit`、`A2aCardRegistryStats`。
- **核心引擎 `A2aCardRegistry`**：
  - `init()` / `upsert_card(*_, entry): bool`（同 `agent_id` 原地刷新元数据与心跳，新 ID 分配空槽）
  - `heartbeat(*_, agent_id, health_score_bps, avg_latency_ms, now_ms): bool` / `deregister(*_, agent_id): bool`
  - `prune_expired(*_, now_ms): usize`（清理 `now_ms - last_heartbeat_ms > ttl_ms` 的失联节点）
  - `discover_topk(*_, query, now_ms, k, out_hits): usize`（自动跳过过期节点，按位与精确校验技能/输入输出模态/特性与最低健康分/延迟上限，按 `health_score_bps / 10000.0 / (1.0 + avg_latency_ms / 200.0)` 降序返回 Top-K，平局按 `agent_id` 升序稳定决胜）
  - `stats(): A2aCardRegistryStats`

```cp
import {
    A2A_MOD_TEXT,
    A2A_MOD_JSON,
    A2A_FEAT_STREAMING,
    A2aCardEntry,
    A2aDiscoveryQuery,
    A2aDiscoveryHit,
    A2aCardRegistry,
} from "cap:a2a";

pub fn demoA2aCardRegistry() {
    let reg = A2aCardRegistry.init();
    const SKILL_SQL: u64 = 1 << 0;
    const c1 = A2aCardEntry.create(
        101, "sql-fast", "https://sql1.local/a2a",
        SKILL_SQL, A2A_MOD_TEXT, A2A_MOD_JSON, A2A_FEAT_STREAMING,
        9800, 80, 1_000, 10_000,
    ) ?? return;
    * = reg.upsert_card(c1);

    let hits: [4]A2aDiscoveryHit = [A2aDiscoveryHit{}; 4];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    const n = reg.discover_topk(
        A2aDiscoveryQuery{
            required_skills: SKILL_SQL,
            required_in_mods: A2A_MOD_TEXT,
            required_out_mods: A2A_MOD_JSON,
            required_features: A2A_FEAT_STREAMING,
            min_health_bps: 9000,
            max_latency_ms: 200,
        },
        2_000,
        4,
        hits_s,
    );
    * = n; // 1 (agent_id == 101)
}
```


---

### 4.9 零堆分配 A2A 多智能体加权法定人数投票与离群估计聚合引擎 (`quorum.cp`)

面向多智能体委员会评审、共识表决与多模型数值估计融合的 100% 纯 CP 零动态堆分配投票裁决引擎：

- **容量与策略常量/枚举**：
  - `QUORUM_MAX_VOTES = 64`、`QUORUM_MAX_OPTIONS = 16`、`QUORUM_MAX_ESTIMATES = 64`、`QUORUM_NO_OPTION = 0xFFFFFFFF`
  - `QuorumPolicy`（`simple_majority | supermajority_two_thirds | weighted_threshold_bps | ranked_borda`）
  - `QuorumDuplicatePolicy`（`reject_duplicate | allow_revote`）、`QuorumVerdict`（`pending | reached | unreachable_early_stop | tie`）、`QuorumVoteStatus`
- **多智能体投票会话（`A2aQuorumSession`）**：
  - `init(cfg)` / `reset(*_)` / `castVote(*_, vote)` / `evaluateQuorum(total_eligible_weight)`：支持按 `agent_id` 防重/改票、可选置信度加权、以及数学可证明的实时早停（当领先者提前过线返回 `reached`，或剩余未投权重已无法逆转达标时返回 `unreachable_early_stop`）。
- **稳健数值估计聚合（`aggregateNumericEstimates`）**：
  - 零堆计算多 Agent 数值估计的 `median`、稳健离散度 `mad`、对称去极值 + `3*MAD` 离群剔除后的 `trimmed_mean` 以及 `consensus_tight` 收敛判定。

```cp
import {
    QuorumPolicy,
    QuorumVerdict,
    QuorumConfig,
    QuorumVote,
    A2aQuorumSession,
} from "cap:a2a";

pub fn demoA2aConsensusQuorum() {
    let session = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 1,
        policy: QuorumPolicy.supermajority_two_thirds,
    }) ?? return;

    // 3 个等权 Agent（总权 30000），前 2 票均投选项 7 即刻达到 2/3 早停，无需等待第 3 个 LLM 推理！
    * = session.castVote(QuorumVote{ agent_id: 1, option_id: 7, weight_bps: 10000, confidence_bps: 9500 });
    * = session.castVote(QuorumVote{ agent_id: 2, option_id: 7, weight_bps: 10000, confidence_bps: 9200 });
    const ev = session.evaluateQuorum(30000);
    if (ev.verdict == QuorumVerdict.reached) {
        * = ev.winning_option_id; // 7 (remaining_weight == 10000 提前早停)
    }
}
```


---

### 4.10 零堆分配 A2A 多智能体子任务 DAG 依赖调度与失败传播引擎 (`taskGraph.cp`)

面向多智能体协同任务编排的 100% 纯 CP 零动态堆分配有向无环图（DAG）调度器、并发波次分派与失败级联/取消引擎：

- **容量常量与枚举**：
  - `TASK_GRAPH_MAX_NODES = 64`、`TASK_GRAPH_MAX_EDGES = 128`
  - `TaskGraphNodeState`（`pending | ready | running | succeeded | failed | skipped_dep_failed | cancelled`）
  - `TaskGraphStatus`（`in_progress | completed_ok | completed_partial | failed`）
  - `TaskGraphOpStatus`（`ok | duplicate_task_id | task_not_found | capacity_full | self_loop | cycle_detected | duplicate_edge | invalid_transition`）
- **数据模型与引擎 `A2aTaskGraph`**：
  - `addTask(*_, task_id, assigned_agent_id, priority, allow_partial_fail)` / `addDependency(*_, dep_task_id, target_task_id)`：插入边时内置纯迭代 BFS 环路检测，严拒自环与多跳环。
  - `collectReadyTasks(max_concurrency, out_task_ids: *[]u64)`：扣除当前在途 `running` 槽位后按 `(priority 降序, task_id 升序)` 返回就绪任务。
  - `markRunning` / `markSucceeded` / `markFailed`：任务失败时自动纯迭代级联将未设置 `allow_partial_fail` 的下游全部标记为 `skipped_dep_failed`；若下游设置 `allow_partial_fail = true`，待其所有前置终结后仍可推进为 `ready` 执行降级聚合。
  - `cancelSubtree(*_, root_task_id)` / `getTask(task_id)` / `summary()` / `clear(*_)`。

```cp
import {
    TaskGraphStatus,
    A2aTaskGraph,
} from "cap:a2a";

pub fn demoA2aTaskGraph() {
    let dag = A2aTaskGraph.init();
    * = dag.addTask(1, 101, 10, false);
    * = dag.addTask(2, 102, 20, false);
    * = dag.addTask(3, 103, 5, false);
    * = dag.addDependency(1, 3);
    * = dag.addDependency(2, 3);

    let wave: [4]u64 = [0; 4];
    let wave_s: []u64 = wave[0..];
    const n = dag.collectReadyTasks(2, *wave_s); // [2, 1] 按优先级降序
    * = n;
    * = dag.summary().overall == TaskGraphStatus.in_progress;
}
```


---

### 4.11 零堆分配 A2A 多智能体零信任委派链校验器 (`trustChain.cp`)

面向多智能体跨边界协作的 100% 纯 CP 零动态堆分配 **Capability-Attenuated Delegation Trust Chain**（能力单调衰减委派链与零信任准入校验器）：

- **常量与枚举**：
  - `TRUST_CHAIN_MAX_HOPS = 16`
  - `TrustVerifyStatus`（`ok | empty_chain | privilege_escalation | hop_limit_exceeded | cycle_detected | expired | not_yet_valid | revoked | cost_exceeded | missing_capability | broken_subject_link | invalid_time_window | invalid_agent_id`）
- **数据模型**：
  - `TrustDelegationLink`（`issuer_agent_id`, `subject_agent_id`, `capability_mask: u64`, `max_cost_micro_usd: u64`, `not_before_ms`, `expires_ms`, `max_further_hops: u8`, `revocation_epoch: u32`）
  - `TrustChainResult`（`status`, `failed_hop_index`, `effective_capabilities`, `effective_max_cost_micro_usd`, `effective_not_before_ms`, `effective_expires_ms`, `remaining_further_hops`, `root_issuer_id`, `leaf_subject_id`, `hop_count`, `isOk()`）
- **核心函数与构建器**：
  - `delegateLink(parent, subject, caps, cost, nbf, exp, max_hops, epoch): TrustDelegationLink?`：强制执行严格单调衰减（能力子集、成本不增、有效期收紧、跳数递减、防自环），违规返回 `null`。
  - `verifyTrustChain(chain, required_caps, est_cost, now_ms, revoked_ids): TrustChainResult` / `verifyTrustChainWithEpoch(...)`：端到端校验断链、多跳环路、单调衰减、吊销列表/纪元、有效期与最终任务能力/预算准入。
  - `TrustChain`：固定 16 跳栈容器（`initRoot`, `appendDelegation`, `asSlice`, `len`, `verify`）。

```cp
import {
    TrustDelegationLink,
    TrustChain,
} from "cap:a2a";

pub fn demoA2aTrustChain() {
    const root = TrustDelegationLink{
        issuer_agent_id: 1,
        subject_agent_id: 2,
        capability_mask: 0b1111,
        max_cost_micro_usd: 500_000,
        not_before_ms: 1000,
        expires_ms: 9000,
        max_further_hops: 3,
        revocation_epoch: 1,
    };
    let chain = TrustChain.initRoot(root) ?? return;
    * = chain.appendDelegation(3, 0b0011, 100_000, 1500, 6000, 1, 1);

    const no_revoked: [0]u64 = [];
    const res = chain.verify(0b0010, 50_000, 2000, no_revoked[0..]);
    * = res.isOk(); // true
}
```


---

### 4.12 零堆分配多智能体共享黑板与乐观并发 CAS 协同引擎 (`blackboard.cp`)

面向多智能体黑板架构（Blackboard Architecture）协同编辑、任务状态同步与分布式锁控制的 100% 纯 CP 零动态堆分配共享工作区引擎：

- **常量与枚举**：
  - `BLACKBOARD_MAX_ENTRIES = 64`、`BLACKBOARD_MAX_KEY_LEN = 48`、`BLACKBOARD_MAX_VAL_LEN = 256`、`BLACKBOARD_EVENT_RING_CAP = 128`、`BLACKBOARD_NO_OWNER = 0`、`BLACKBOARD_NO_TTL = 0`
  - `BlackboardOpStatus`（`ok | not_found | version_conflict | lease_held_by_other | table_full | invalid_key | value_too_large | invalid_writer`）
  - `BlackboardEventKind`（`put | cas | delete | lease_expired | ttl_expired`）
- **数据模型**：
  - `BlackboardEntry`（定长 `key`, `value`, `version`, `writer_id`, `lease_owner_id`, `lease_expires_ms`, `expires_ms`, `updated_ms`；提供 `keySlice()`, `valueSlice()`, `hasActiveLease(now_ms)`, `isExpired(now_ms)`）
  - `BlackboardWriteResult`、`BlackboardEvent`、`BlackboardPollResult`（含 `lagged` 环形覆盖落后检测）、`BlackboardStats`
- **核心引擎 `SharedBlackboard`**：
  - `init()` / `clear(*_)` / `stats()` / `sweepExpired(*_, now_ms)`
  - `put(*_, key, value, writer_id, ttl_ms, now_ms)` / `get(*_, key, now_ms)` / `peek(key, now_ms)` / `deleteKey(*_, key, writer_id, now_ms)`
  - `compareAndSwap(*_, key, expected_version, new_val, writer_id, ttl_ms, now_ms)`：支持 `expected_version == 0` 原子创建（Create-If-Absent），冲突时返回 `version_conflict` 及当前最新 `version`
  - `acquireLease(*_, key, owner_id, lease_ttl_ms, now_ms)` / `releaseLease(*_, key, owner_id, now_ms)`：排他编辑权与同主续租，租约到期自动释放并写入 `lease_expired` 事件
  - `pollChanges(prefix, since_seq, *out_events): BlackboardPollResult` / `listKeysByPrefix(prefix, now_ms, *out_entries): usize`。

```cp
import {
    BLACKBOARD_NO_TTL,
    BlackboardEvent,
    SharedBlackboard,
} from "cap:a2a";

pub fn demoSharedBlackboard() {
    let bb = SharedBlackboard.init();
    // Agent 101 原子创建 plan/step1 (expected_version = 0 -> version 1)
    const r1 = bb.compareAndSwap("plan/step1", 0, "draft", 101, BLACKBOARD_NO_TTL, 1000);
    // Agent 102 持版本 1 乐观更新 -> version 2
    * = bb.compareAndSwap("plan/step1", r1.version, "approved", 102, BLACKBOARD_NO_TTL, 1100);

    let evs: [8]BlackboardEvent = [BlackboardEvent{}; 8];
    let evs_s: []BlackboardEvent = evs[0..];
    const poll = bb.pollChanges("plan/", 0, *evs_s);
    * = poll.count; // 2
}
```

---

### 零分配 FIPA Contract Net Protocol 多智能体招投标与自适应中标分配引擎 (`contractNet.cp`)

面向多智能体市场化任务竞标与故障自动递补的 100% 纯 CP 零动态堆分配 FIPA Contract Net Protocol (CNP) 经纪器：

- **常量与枚举**：
  - `CNP_MAX_AUCTIONS = 32`、`CNP_MAX_BIDS_PER_CFP = 16`、`CNP_BPS_SCALE = 10000`
  - `CnpAuctionState`（`open | awarded | completed | failed_no_bids | cancelled`）
  - `CnpBidStatus`（`submitted | awarded | rejected | succeeded | failed`）
  - `CnpBidSubmitStatus`（`accepted | cfp_not_found | cfp_not_open | deadline_passed | capability_mismatch | cost_exceeded | latency_exceeded | duplicate_worker | bids_full | invalid_arg`）
- **配置与数据模型**：
  - `CnpScoreWeights`（`cost_weight`, `latency_weight`, `confidence_weight`, `reputation_weight`; 提供 `defaults()` 与 `isValid()`）
  - `CnpBid`、`CnpAwardResult`、`CnpAuctionView`、`CnpReapReport`、`CnpStats`
- **核心引擎 `ContractNetBroker`**：
  - `init()` / `initWithWeights(weights)` / `reset(*_)` / `openCfp(...)` / `cancelCfp(*_, cfp_id)`
  - `submitBid(...)`: 严格拦截能力位图不满足、超预算、超延迟、逾期、重复 Worker 投标、满标（16 份）及非法基点，预计算纯整数多目标效用分（`0..=10000`）。
  - `evaluateAndAward(*_, cfp_id, now_ms)`: 选出最高效用中标者并将落选者标记 `rejected`（保留作为次优递补池）；无有效标则流标 `failed_no_bids`。
  - `reportExecutionOutcome(*_, cfp_id, worker_id, succeeded, now_ms)` / `fallbackToRunnerUp(*_, cfp_id, new_exec_deadline_ms, now_ms)`: 中标者成功即闭环；失败或经 `reapExpired` 判定超时后，无需重新广播招标即可直接按效用顺序递补给次优候选。
  - `reapExpired(*_, now_ms)` / `purgeTerminal(*_)` / `getAuction(cfp_id)` / `stats()`。

```cp
import {
    CnpBidSubmitStatus,
    ContractNetBroker,
} from "cap:a2a";

pub fn demoContractNet() {
    let broker = ContractNetBroker.init();
    const CAP_SQL: u64 = 1 << 0;
    const CAP_GPU: u64 = 1 << 1;

    * = broker.openCfp(100, CAP_SQL | CAP_GPU, 500, 1000, 2000, 5000);
    * = (broker.submitBid(100, 1, CAP_SQL | CAP_GPU, 200, 300, 9000, 8500, 1100) == CnpBidSubmitStatus.accepted);
    * = (broker.submitBid(100, 2, CAP_SQL | CAP_GPU, 300, 400, 8000, 8000, 1200) == CnpBidSubmitStatus.accepted);

    const award = broker.evaluateAndAward(100, 2000); // Worker 1 中标
    * = broker.reportExecutionOutcome(100, award.winner_worker_id, false, 2500); // Worker 1 失败
    const fb = broker.fallbackToRunnerUp(100, 6000, 2600); // 自动递补给 Worker 2
    * = broker.reportExecutionOutcome(100, fb.winner_worker_id, true, 3000); // 递补成功!
}
```

---

### 零分配多智能体层次成本/Token 预算账本与级联熔断控制器 (`swarmBudget.cp`)

面向多智能体蜂群（Multi-Agent Swarm）防无限递归烧钱、成本隔离与截止期硬熔断的 100% 纯 CP 零动态堆分配层次预算账本：

- **常量与枚举**：
  - `SWARM_BUDGET_MAX_NODES = 64`、`SWARM_BUDGET_NO_NODE = 0xFFFF`
  - `SwarmAgentState`（`active | completed | halted_budget | halted_deadline`）
  - `SwarmBudgetViolation`（`none | agent_not_found | agent_not_active | ancestor_halted | token_limit_exceeded | cost_limit_exceeded | tool_limit_exceeded | deadline_exceeded`）
  - `SwarmSpawnStatus`（`ok | parent_not_found | parent_not_active | duplicate_agent_id | invalid_agent_id | capacity_full | exceeds_parent_remaining | deadline_exceeds_parent`）
- **数据模型**：
  - `SwarmBudgetLimits`（`max_tokens`, `max_cost_micros`, `max_tool_calls`, `deadline_ms`, `reserve_from_parent`）
  - `SwarmAgentNode`、`SwarmChargeOutcome`、`SwarmRemainingBudget`、`SwarmBudgetStats`
- **核心引擎 `SwarmBudgetLedger`**：
  - `init(root_id, root_limits): SwarmBudgetLedger?` / `spawnChild(*_, parent_id, child_id, sub_limits)`：支持独立子配额、截止期单调收紧与可选父池预留（`reserve_from_parent = true`）。
  - `charge(*_, agent_id, tokens, cost_micros, tool_calls, now_ms): SwarmChargeOutcome`：两阶段原子级联记账（先沿父链全量校验、再统一提交）；任一节点或祖先超配额/超截止期时拒绝扣减，并通过纯迭代 BFS 将违约节点及其全部活跃子树级联熔断为 `halted_budget` / `halted_deadline`。
  - `completeAgent(*_, agent_id)`：完工并精确归还未使用的保留额度至父节点自由池。
  - `remainingBudget(agent_id, now_ms)` / `getAgent(agent_id)` / `stats()`。

```cp
import {
    SwarmBudgetLimits,
    SwarmBudgetLedger,
} from "cap:a2a";

pub fn demoSwarmBudget() {
    let ledger = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 10000,
        max_cost_micros: 500_000,
        max_tool_calls: 20,
        deadline_ms: 60_000,
    }) ?? return;

    * = ledger.spawnChild(1, 2, SwarmBudgetLimits{
        max_tokens: 4000,
        max_cost_micros: 200_000,
        max_tool_calls: 8,
        reserve_from_parent: true,
    });
    * = ledger.charge(2, 1500, 75_000, 2, 10_000);
    * = ledger.completeAgent(2); // 归还剩余 2500 tokens 预留给根节点
}
```

---

### 零分配 SWIM 多智能体集群成员关系与故障检测 Gossip 状态机 (`gossipHealth.cp`)

面向大规模多智能体去中心化集群发现、心跳搭便车传播与故障收敛的 100% 纯 CP 零动态堆分配 SWIM 风格 Gossip 状态机：

- **常量与枚举**：
  - `GOSSIP_MAX_MEMBERS = 64`、`GOSSIP_QUEUE_CAP = 64`、`GOSSIP_DEFAULT_SUSPECT_TIMEOUT_MS = 5000`、`GOSSIP_DEFAULT_RETRANSMIT_LIMIT = 4`、`GOSSIP_DEFAULT_PRNG_SEED`
  - `GossipMemberState`（`alive | suspect | dead`；配套 `gossipStateRank(state)` 满足 `dead > suspect > alive`）
  - `GossipApplyOutcome`（`ignored_stale | applied_new | applied_updated | refuted_self | capacity_full`）
- **配置与数据模型**：
  - `GossipConfig`（`suspect_timeout_ms`, `retransmit_limit`, `prng_seed`）
  - `GossipMember`（`agent_id`, `state`, `incarnation`, `capabilities`, `state_changed_ms`, `last_seen_ms`）
  - `GossipUpdate`、`GossipDisseminationEntry`、`GossipHealthStats`
- **核心引擎 `GossipHealthMachine`**：
  - `init(local_agent_id, capabilities, now_ms, config): GossipHealthMachine?`：初始化本机节点（`incarnation = 1`）并自动入队初始 `alive` 宣告。
  - `upsertPeer(*_, agent_id, capabilities, now_ms)` / `applyUpdate(*_, update, now_ms)` / `applyBatch(*_, updates, now_ms)`：严格执行 SWIM 化身号优先级规则（高 `incarnation` 覆盖低 `incarnation`；同 `incarnation` 按 `dead > suspect > alive` 收敛）；收到针对本机且 `incarnation >= local.incarnation` 的 `suspect` / `dead` 谣言时，自动递增 `local.incarnation = update.incarnation + 1` 并广播 `alive` 反驳事件。
  - `markSuspect(*_, agent_id, now_ms)` / `recordDirectAck(*_, agent_id, now_ms)` / `updateLocalCapabilities(*_, new_capabilities, now_ms)`。
  - `buildGossipBatch(*_, max_items, *out_updates): usize`：从有界搭便车传播队列中优先选取已传播次数（`tx_count`）最少、序号最新（`seq` 最大）的变更随心跳发出，并递减 `retransmits_left` 直至退役。
  - `reapSuspects(*_, now_ms)` / `purgeDead(*_, now_ms, min_dead_age_ms)` / `selectProbeTarget(*_)`（Fisher-Yates 洗牌轮询均匀探测所有非死亡远端节点）/ `getMember(agent_id)` / `localMember()` / `listCapableAlive(required_caps, include_local, *out_ids)` / `stats()`。

```cp
import {
    GossipConfig,
    GossipMemberState,
    GossipUpdate,
    GossipHealthMachine,
} from "cap:a2a";

pub fn demoGossipHealth() {
    let node = GossipHealthMachine.init(1, 0b011, 1000, GossipConfig{
        suspect_timeout_ms: 5000,
        retransmit_limit: 3,
    }) ?? return;

    * = node.upsertPeer(2, 0b101, 1000);
    // 收到针对本机的 suspect 谣言 -> 自动递增 incarnation 并在队列中广播 alive 反驳
    * = node.applyUpdate(GossipUpdate{
        agent_id: 1,
        state: GossipMemberState.suspect,
        incarnation: 1,
    }, 1500);

    let batch_buf: [4]GossipUpdate = [GossipUpdate{}; 4];
    let batch: []GossipUpdate = batch_buf[0..];
    const sent = node.buildGossipBatch(4, batch);
    * = sent;
}
```
