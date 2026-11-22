# `cap:agent` — AI Agent 核心运行时标准库

`cap:agent` 是 CP 标准库面向生产级 AI Agent 运行时的统一聚合主包，100% 纯 CP 零堆分配实现，当前包含执行预算/护栏（`budget.cp`）、多智能体任务图 DAG 调度器（`dag.cp`）、评测基准与轨迹打分（`eval.cp`）、ReAct 执行状态机与确定性检查点（`react.cp`）、LLM 流式 SSE 解析与遥测（`sse.cp`）、多模型网关路由与限流故障转移（`model_router.cp`）、输入/输出安全护栏与策略引擎（`guardrail.cp`）、连续批处理与前缀缓存亲和调度器（`scheduler.cp`）、工具调用参数 JSON Schema 校验器（`tool_schema.cp`）、词表 Logits 采样器（`sampler.cp`）、推测解码校验与自适应草稿步长控制（`speculative.cp`）以及束搜索解码器（`beam.cp`）十二个子模块。

---

## 子模块：执行预算、循环护栏与上下文窗口顾问（`budget.cp`）

### 核心特性

- **四维硬预算控制器（`AgentBudget`）**：精确追踪 `prompt_tokens` / `completion_tokens` / `cached_tokens`、微美元成本（`u64` `micro_usd`，0 浮点误差 + 防溢出饱和加法）、工具调用步数与墙钟耗时，支持多模型 `ModelPricing` 单价表、软预警阈值（`SoftLimit`）与分维度硬拦截（`ExceededTokens` / `ExceededCost` / `ExceededSteps` / `ExceededTime`）。
- **死循环与乒乓检测器（`LoopGuard`）**：基于调用方提供的固定环形缓冲与 FNV-1a 128-bit `(tool_hash, args_hash)` 签名，O(1) 空间检测连续重复调用（`RepeatLoop`：`A-A-A`）与交替乒乓死循环（`PingPongLoop`：`A-B-A-B`）。
- **工具熔断器（`ToolCircuitBreaker`）**：完整实现 `Closed -> Open -> HalfOpen -> Closed` 三态熔断状态机与冷却期自动探测恢复。
- **上下文窗口顾问（`ContextWindowAdvisor`）**：结合模型上下文上限与输出预留 Token，实时输出剩余 `headroom_tokens`、利用率与 `Ok | CompactSoon | MustCompact | Overflow` 压缩建议。

### 导出清单

- **预算控制**：`BudgetStatus`、`ModelPricing`（`init`, `computeMicroUsd`）、`BudgetLimits`、`AgentBudget`（`init`, `reset`, `totalTokens`, `remainingTokens`, `remainingCostMicroUsd`, `remainingSteps`, `status`, `isExhausted`, `recordStep`）
- **循环护栏与熔断**：`LoopAction`、`CallSignature`（`fromBytes`, `eql`）、`LoopGuard`（`init`, `reset`, `recordAndCheck`）、`CircuitState`、`ToolCircuitBreaker`（`init`, `reset`, `allowCall`, `recordSuccess`, `recordFailure`）
- **上下文顾问**：`ContextStatus`、`ContextReport`、`ContextWindowAdvisor`（`init`, `evaluate`, `canFitAdditional`）

### 使用示例

```typescript
import std from "std";
import {
    ModelPricing,
    BudgetLimits,
    AgentBudget,
    CallSignature,
    LoopGuard,
    ToolCircuitBreaker,
    ContextWindowAdvisor,
} from "cap:agent";

fn agentBudgetDemo() {
    // 1. Multi-dimensional budget ($3/1M prompt, $15/1M completion, $0.30/1M cached)
    const pricing = ModelPricing.init(3000000, 15000000, 300000);
    const limits = BudgetLimits {
        max_total_tokens: 100000,
        max_cost_micro_usd: 500000, // $0.50
        max_steps: 20,
        max_elapsed_ms: 60000,
        soft_limit_pct: 80,
    };
    let budget = AgentBudget.init(limits) ?? return;
    const st = budget.recordStep(4000, 1000, 2000, pricing, 850);
    * = st;

    // 2. LoopGuard (detect 3 consecutive repeats or 2 ping-pong cycles)
    let ring: [8]CallSignature = std.mem.zeroes([8]CallSignature);
    let ring_s: []CallSignature = ring[0..];
    let guard = LoopGuard.init(&ring_s, 3, 2) ?? return;
    const act = guard.recordAndCheck(CallSignature.fromBytes("read_file", "{\"path\":\"a.cp\"}"));
    * = act;

    // 3. Tool circuit breaker & context window advisor
    let cb = ToolCircuitBreaker.init(3, 2, 5000) ?? return;
    * = cb.allowCall(1000);
    const advisor = ContextWindowAdvisor.init(128000, 8000, 75, 90) ?? return;
    const report = advisor.evaluate(95000);
    * = report;
}
```

---

## 子模块：多智能体任务图 / 工作流 DAG 调度器（`dag.cp`）

### 核心特性

- **100% 零堆分配调度内核（`AgentDag`）**：任务节点缓冲（`[]DagTask`）、依赖边缓冲（`[]DagEdge`）以及 Kahn 检环临时缓冲（`[]u16` / `[]u32`）全部由调用方提供，天然适合高并发 Agent Worker 池与嵌入式/受限内存场景。
- **Kahn 拓扑检环与安全启动**：`validateAcyclic` 执行线性 Kahn 拓扑遍历，精准拒收自环与任意多跳环路依赖，验证通过后自动将所有入度为 `0` 的根节点置为 `Ready`。
- **确定性优先级 + 并行 Fan-Out/Fan-In 调度**：`claimNextReady` 按最高 `priority`、同优先级最小 `id` 确定性领取任务；`claimReadyBatch` 支持单次批量领取多路可并行 Fan-Out 的子任务，上游全部完成后自动递减入度并解锁下游 Fan-In 汇聚节点。
- **内置瞬态重试与级联跳过**：`failTask` 在 `attempts < max_retries` 时自动将任务退回 `Ready` 重试；重试耗尽转为 `Failed` 时，按 `allow_failure` 决定放行下游或零分配不动点级联将所有可达未完成下游标记为 `Skipped`。

### 导出清单

- **状态与节点类型**：`TaskState`（`Pending | Ready | Running | Completed | Failed | Skipped | Cancelled`）、`DagTask`、`DagEdge`、`DagSummary`
- **调度器（`AgentDag`）**：`init`、`addTask`、`addDependency`、`validateAcyclic`、`claimNextReady`、`claimReadyBatch`、`completeTask`、`failTask`、`cancelAllPending`、`taskState`、`taskAt`、`summary`

### 使用示例（多智能体菱形 Fan-Out / Fan-In 工作流）

```typescript
import std from "std";
import { DagTask, DagEdge, AgentDag } from "cap:agent";

fn multiAgentWorkflowDemo() {
    let tasks_arr: [8]DagTask = std.mem.zeroes([8]DagTask);
    let edges_arr: [16]DagEdge = std.mem.zeroes([16]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);

    // 规划节点 -> 并行 Fan-Out (检索 Agent / 代码分析 Agent) -> 汇聚总结 Agent
    const plan = dag.addTask(100, 1, false) ?? return;
    const search = dag.addTask(50, 2, false) ?? return;
    const analyze = dag.addTask(80, 1, false) ?? return;
    const synthesize = dag.addTask(10, 0, false) ?? return;

    * = dag.addDependency(plan, search);
    * = dag.addDependency(plan, analyze);
    * = dag.addDependency(search, synthesize);
    * = dag.addDependency(analyze, synthesize);

    // 1. Kahn 拓扑检环并解锁根节点
    let indeg_arr: [8]u16 = std.mem.zeroes([8]u16);
    let queue_arr: [8]u32 = std.mem.zeroes([8]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    if (!dag.validateAcyclic(&indeg_s, &queue_s)) {
        return;
    }

    // 2. 领取并完成规划任务，解锁并行 Fan-Out 批次
    if (root_id := dag.claimNextReady()) {
        * = dag.completeTask(root_id);
    }

    // 3. 批量领取并行任务（按优先级：analyze(80) -> search(50)）
    let batch_arr: [4]u32 = std.mem.zeroes([4]u32);
    let batch_s: []u32 = batch_arr[0..];
    const n = dag.claimReadyBatch(&batch_s);
    let i: usize = 0;
    while (i < n) {
        * = dag.completeTask(batch_s[i]);
        i += 1;
    }

    // 4. 汇聚节点自动就绪并完成
    if (final_id := dag.claimNextReady()) {
        * = dag.completeTask(final_id);
    }
    const snap = dag.summary();
    * = snap;
}
```

---

## 子模块：Agent 评测基准与轨迹打分（`eval.cp`）

### 核心特性

- **无偏 Pass@k 估计器（`passAtK`）**：实现 Chen et al. HumanEval 精确无溢出连乘公式 $1 - \prod_{i=0}^{k-1} \frac{n-c-i}{n-i}$，即使在大样本 `n=200, k=10` 场景下仍零溢出且精确处理 `c == 0 -> 0.0` 与 `n - c < k -> 1.0`。
- **零分配文本问答评测（`exactMatchNormalized` / `tokenOverlapF1`）**：ASCII 大小写无关 + 标点/空白自动分词；`tokenOverlapF1` 基于调用方缓冲与原地堆排序实现 SQuAD 语义多重集 Token 交集 Precision/Recall/F1。
- **工具调用轨迹匹配评估（`scoreToolTrajectory`）**：基于两行滚动 DP 与零分配去重集合扫描，输出严格顺序匹配（`exact_order_match`）、LCS 有序比率（`order_lcs_ratio`）以及无序集合 `set_precision` / `set_recall` / `set_f1`。
- **批量评测汇总与延迟分位数（`summarizeEvalRun`）**：支持显式 `passed` 标记或按 `pass_threshold` 阈值判定，线性插值计算精确 P50 / P95 / P99 延迟并饱和累加微美元成本，内置 `NaN`、负延迟与缓冲越界防御。

### 导出清单

- **Pass@k 与文本评测**：`passAtK`、`exactMatchNormalized`、`TextF1Result`、`tokenOverlapF1`
- **工具轨迹评测**：`TrajectoryScore`、`scoreToolTrajectory`
- **评测运行汇总**：`EvalSummary`、`summarizeEvalRun`

### 使用示例

```typescript
import std from "std";
import {
    passAtK,
    exactMatchNormalized,
    tokenOverlapF1,
    scoreToolTrajectory,
    summarizeEvalRun,
} from "cap:agent";

fn agentEvalDemo() {
    // 1. HumanEval Pass@k
    const p10 = passAtK(200, 20, 10) ?? return;
    * = p10;

    // 2. QA Exact Match & Multiset Token-F1
    const em = exactMatchNormalized("The Quick, Brown Fox!", "the quick brown fox");
    * = em;
    let ph_arr: [32]u64 = std.mem.zeroes([32]u64);
    let gh_arr: [32]u64 = std.mem.zeroes([32]u64);
    let ph_s: []u64 = ph_arr[0..];
    let gh_s: []u64 = gh_arr[0..];
    const f1_res = tokenOverlapF1("cat cat dog", "cat dog bird", &ph_s, &gh_s) ?? return;
    * = f1_res;

    // 3. Tool Trajectory Eval
    const actual: [3]u64 = [101, 202, 303];
    const expected: [3]u64 = [101, 303, 202];
    let dp_prev: [8]usize = std.mem.zeroes([8]usize);
    let dp_curr: [8]usize = std.mem.zeroes([8]usize);
    let prev_s: []usize = dp_prev[0..];
    let curr_s: []usize = dp_curr[0..];
    const traj = scoreToolTrajectory(actual[0..], expected[0..], &prev_s, &curr_s) ?? return;
    * = traj;

    // 4. Summarize Eval Run with P50/P95/P99 latencies
    const scores: [4]f64 = [1.0, 0.8, 0.4, 0.9];
    const passed: [4]u8 = [1, 1, 0, 1];
    const lats: [4]f64 = [120.0, 45.0, 310.0, 80.0];
    const costs: [4]u64 = [1500, 1200, 2300, 1800];
    let scratch: [4]f64 = std.mem.zeroes([4]f64);
    let scratch_s: []f64 = scratch[0..];
    const summary = summarizeEvalRun(scores[0..], passed[0..], lats[0..], costs[0..], 0.7, &scratch_s) ?? return;
    * = summary;
}
```

---

## 子模块：ReAct 执行状态机与确定性检查点（`react.cp`）

### 核心特性

- **严格 ReAct 生命周期状态机（`ReactSession`）**：完整建模 `Idle -> Thinking -> AwaitingTool -> Observing -> Thinking -> Completed | Failed | Halted` 跃迁约束，精准拒收非法状态跃迁，并在工具步数达到 `max_steps` 硬上限时自动转入 `Halted`。
- **固定环缓冲轨迹记录（`TraceEntry`）**：所有推理思考、工具调用、工具返回、最终答案与异常记录写入调用方提供的固定环形缓冲，容量溢出时自动覆盖最旧记录并保留最新轨迹，`traceAt(idx)` 按时间正序访问。
- **确定性二进制检查点（`saveCheckpoint` / `loadCheckpoint`）**：小端紧凑编码（`CHECKPOINT_MAGIC = 0x43504147` `"CPAG"` + 版本号 + 会话状态 + ZigZag 编码时间戳轨迹 + 64-bit FNV-1a 校验和），专为 BEAM Actor 崩溃重启与长任务断点续跑设计，可精准拦截任意字节篡改或截断。

### 导出清单

- **常量与枚举**：`CHECKPOINT_MAGIC`、`ReactPhase`、`StepKind`
- **会话与轨迹**：`TraceEntry`、`ReactSession`（`init`, `phase`, `stepCount`, `isTerminal`, `traceLen`, `traceAt`, `startTurn`, `recordThought`, `requestToolCall`, `recordToolResult`, `finishWithAnswer`, `failSession`, `haltSession`）
- **检查点序列化**：`saveCheckpoint`、`loadCheckpoint`

### 使用示例（`AwaitingTool` 挂起保存检查点并断点续跑）

```typescript
import std from "std";
import {
    TraceEntry,
    ReactSession,
    saveCheckpoint,
    loadCheckpoint,
} from "cap:agent";

fn reactCheckpointResumeDemo() {
    let trace_buf: [16]TraceEntry = std.mem.zeroes([16]TraceEntry);
    let trace_s: []TraceEntry = trace_buf[0..];

    let sess = ReactSession.init(42, 8, &trace_s);
    * = sess.startTurn(1000);
    * = sess.recordThought(320, 45, 1045);
    * = sess.requestToolCall(0xAABBCCDD, 1050);

    // 挂起等待外部工具响应：将状态序列化为二进制检查点
    let ckpt: [1024]u8 = std.mem.zeroes([1024]u8);
    let ckpt_s: []u8 = ckpt[0..];
    const written = saveCheckpoint(sess, &ckpt_s) ?? return;

    // 重启后从检查点无损恢复并继续完成会话
    let restored_trace: [16]TraceEntry = std.mem.zeroes([16]TraceEntry);
    let restored_s: []TraceEntry = restored_trace[0..];
    let resumed = loadCheckpoint(ckpt_s[0..written], &restored_s) ?? return;
    * = resumed.recordToolResult(0xAABBCCDD, true, 120, 1170);
    * = resumed.recordThought(180, 30, 1200);
    * = resumed.finishWithAnswer(210, 1250);
}
```

---

## 子模块：LLM 流式 SSE 解析与 Token 吞吐遥测（`sse.cp`）

### 核心特性

- **零分配增量 SSE 解析器（`SseStreamParser`）**：完整兼容 WHATWG EventSource 规范与 OpenAI / Anthropic 流式响应协议；支持任意单字节或半行分块投喂、`\n`/`\r\n`/`\r` 三种换行符、UTF-8 BOM 自动剥离、`:comment` 心跳注释跳过、多行 `data:` 以 `\n` 自动拼接、首字节可选空格剥离与 `data: [DONE]` 终止哨兵自动识别。
- **TTFT / TPS 流式遥测（`StreamTelemetry`）**：精确记录首字延迟（`ttft_ms`）、总耗时（`total_ms`）、吞吐（`tokens_per_sec`）与平均帧间隔（`inter_token_avg_ms`）。
- **轻量 JSON Delta 字符串提取（`extractJsonStringField`）**：零分配扫描 OpenAI `"content"` 或 Anthropic `"text"` 字段并解码 `\n`/`\r`/`\t`/`\"`/`\\`/`\/`/`\uXXXX` 转义序列。

### 导出清单

- **SSE 解析**：`SseEvent`、`SseFeedResult`、`SseStreamParser`（`init`, `reset`, `nextEvent`）
- **流式遥测**：`StreamMetrics`、`StreamTelemetry`（`init`, `onTokenChunk`, `finish`）
- **JSON Delta 辅助**：`extractJsonStringField`

### 使用示例（OpenAI / Anthropic 流式分块消费 + 遥测）

```typescript
import std from "std";
import {
    SseEvent,
    SseStreamParser,
    StreamTelemetry,
    extractJsonStringField,
} from "cap:agent";

fn llmStreamingDemo() {
    let line_b: [256]u8 = std.mem.zeroes([256]u8);
    let data_b: [512]u8 = std.mem.zeroes([512]u8);
    let ev_b: [64]u8 = std.mem.zeroes([64]u8);
    let id_b: [64]u8 = std.mem.zeroes([64]u8);
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];

    let parser = SseStreamParser.init(&line_s, &data_s, &ev_s, &id_s);
    let tel = StreamTelemetry.init(1000);

    const payload: string = "data: {\"delta\":{\"content\":\"Hello\\n\"}}\n\ndata: [DONE]\n\n";
    let rest: []const u8 = payload;
    let ev = SseEvent { event_type: "", data: "", id: "", is_done: false };
    let out_b: [128]u8 = std.mem.zeroes([128]u8);
    let out_s: []u8 = out_b[0..];

    while (rest.len > 0) {
        const res = parser.nextEvent(rest, &ev);
        rest = rest[res.consumed..];
        if (res.overflow) break;
        if (res.has_event && !ev.is_done) {
            if (n := extractJsonStringField(ev.data, "content", &out_s)) {
                tel.onTokenChunk(1120, 2);
                * = n;
            }
        }
    }

    const metrics = tel.finish(1250);
    * = metrics;
}
```

---

## 子模块：多模型 LLM 网关路由、RPM/TPM 限流与故障转移（`model_router.cp`）

### 核心特性

- **三种确定性选路策略（`ModelRoutePolicy`）**：支持 `PriorityOrder`（高优先级优先）、`LowestCost`（每百万 Token 微美元单价最低优先）与 `LowestLatency`（EWMA 延迟最低优先），同指标按优先级与最小 ID 打破平局。
- **RPM + TPM 双速率 60s 滑动窗口限流**：`selectEndpoint` 自动刷新跨 60 秒窗口的请求与 Token 计数，并原子预扣 1 次请求与 `est_tokens`；请求完成后 `recordSuccess` 按真实消耗差值 `actual_tokens_delta` 精确回补/补扣 `window_tokens`。
- **429 Retry-After 与 5xx 指数退避冷却**：`recordRateLimitOrFailure` 在 `retry_after_ms > 0` 时严格遵守上游冷却时间，否则执行 `1s -> 2s -> 4s -> ... -> 60s` 封顶指数退避，冷却期内自动无缝故障转移至备选模型，冷却到期自动回切。

### 导出清单

- **策略与描述符**：`ModelRoutePolicy`（`PriorityOrder | LowestCost | LowestLatency`）、`ModelEndpoint`
- **网关路由器**：`ModelRouter`（`init`, `addEndpoint`, `setEnabled`, `endpointAt`, `selectEndpoint`, `recordSuccess`, `recordRateLimitOrFailure`, `availableCount`）

### 使用示例（多模型 429 自动故障转移与回切）

```typescript
import std from "std";
import { ModelRoutePolicy, ModelEndpoint, ModelRouter } from "cap:agent";

fn multiModelFailoverDemo() {
    let pool: [4]ModelEndpoint = std.mem.zeroes([4]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    const primary = router.addEndpoint(1, "claude-sonnet", 100, 3000000, 60, 100000) ?? return;
    const backup = router.addEndpoint(2, "gpt-4o-mini", 50, 150000, 120, 200000) ?? return;
    * = backup;

    // 1. 优先命中主模型
    const slot1 = router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 2000, 10000) ?? return;
    if (slot1 == primary) {
        // 模拟主模型返回 429 Retry-After: 5000ms
        * = router.recordRateLimitOrFailure(slot1, 10000, 5000);
    }

    // 2. 冷却期内自动故障转移至备选模型
    const slot2 = router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 2000, 11000) ?? return;
    * = router.recordSuccess(slot2, 240.0, -300);

    // 3. 冷却期过后（15000ms）自动回切主模型
    const slot3 = router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 2000, 15000) ?? return;
    * = slot3;
}
```

---

## 子模块：输入/输出安全护栏与策略引擎（`guardrail.cp`）

### 核心特性

- **零分配规则策略引擎（`evaluateGuardrails`）**：在进入 LLM 推理或返回最终响应前，单次完成载荷长度超限校验（`MaxLengthExceeded`）、NUL/控制字符异常检测（`ControlCharAnomaly`）、JSON 嵌套深度预检（`NestingTooDeep`）、内置凭证密钥泄漏扫描（`SecretLeak`：`sk-` / `ghp_` / `AKIA` / PEM 私钥头）以及自定义规则表匹配与风险分累加升级（`Allow | Warn | Block`）。
- **工具参数载荷结构深度预检（`inspectJsonPayloadDepth`）**：零分配跳过字符串内嵌的 `{}`、`[]` 与 `\"`/`\\` 转义序列，精确统计最大括号嵌套深度与字符串数量，超过 `max_allowed_depth` 时立即提前截断以防范深层嵌套载荷耗尽解析栈。

### 导出清单

- **裁决与配置**：`GuardAction`（`Allow | Warn | Block`）、`GuardReason`、`GuardRule`、`GuardVerdict`、`GuardConfig`（`default`）
- **评估与预检**：`evaluateGuardrails`、`JsonDepthReport`、`inspectJsonPayloadDepth`

### 使用示例

```typescript
import {
    GuardAction,
    GuardRule,
    GuardConfig,
    evaluateGuardrails,
    inspectJsonPayloadDepth,
} from "cap:agent";

fn guardrailDemo() {
    const rules: [2]GuardRule = [
        GuardRule { id: 101, pattern: "ignore previous instructions", case_insensitive: true, action: GuardAction.Warn, weight: 50 },
        GuardRule { id: 102, pattern: "reveal system prompt", case_insensitive: true, action: GuardAction.Warn, weight: 50 },
    ];
    const cfg = GuardConfig.default();

    const v = evaluateGuardrails("Please summarize the release notes.", rules[0..], cfg);
    * = v;

    const depth_report = inspectJsonPayloadDepth("{\"args\":{\"query\":\"{nested}\"}}", 16);
    * = depth_report;
}
```

---

## 子模块：LLM 连续批处理与前缀缓存亲和调度器（`scheduler.cp`）

### 核心特性

- **vLLM / SGLang 风格连续批处理核心（`BatchScheduler`）**：每轮 `stepSchedule` 严格遵守 `max_batch_reqs`、`max_batch_tokens` 与 `total_kv_blocks` 硬约束，优先保障进行中 `Decode` 请求的持续推进，并将剩余预算分配给等待队列中的 `Prefill` 请求。
- **前缀缓存亲和（Prefix-Cache Affinity）与防饥饿老化**：与当前活跃 `active_prefix_hash` 匹配的等待请求自动获得前缀亲和加分以复用 KV Cache；同时支持按等待时长 `aging_ms_per_priority` 线性老化提升低优先级请求的有效优先级，杜绝饥饿。
- **动态 KV-Block 抢占与自动回收**：当高优先级请求到达或 `Decode` 序列增长导致 KV-Block 不足时，自动按最低有效优先级抢占（`Preempted`）并回收其持有的 KV-Block；请求生成达 `max_new_tokens` 或调用 `cancel` 时立即 `Finished`/`Cancelled` 并归还全部块。

### 导出清单

- **请求与配置**：`ReqState`（`Queued | Prefill | Decode | Finished | Preempted | Cancelled`）、`InferRequest`、`BatchLimits`、`ScheduleStep`
- **调度器**：`BatchScheduler`（`init`, `enqueue`, `stepSchedule`, `advanceDecode`, `cancel`, `freeKvBlocks`, `requestAt`）

### 使用示例

```typescript
import std from "std";
import { InferRequest, BatchLimits, BatchScheduler } from "cap:agent";

fn continuousBatchingDemo() {
    let slots: [8]InferRequest = std.mem.zeroes([8]InferRequest);
    let slots_s: []InferRequest = slots[0..];
    const limits = BatchLimits {
        max_batch_reqs: 4,
        max_batch_tokens: 512,
        total_kv_blocks: 32,
        tokens_per_kv_block: 16,
        aging_ms_per_priority: 1000,
    };

    let sched = BatchScheduler.init(&slots_s, limits) ?? return;
    * = sched.enqueue(1, 0xCAFE, 64, 10, 50, 1000);
    * = sched.enqueue(2, 0xCAFE, 64, 10, 50, 1005);

    let prefill_ids: [4]u64 = std.mem.zeroes([4]u64);
    let decode_ids: [4]u64 = std.mem.zeroes([4]u64);
    let pf_s: []u64 = prefill_ids[0..];
    let dc_s: []u64 = decode_ids[0..];

    const step = sched.stepSchedule(1010, 0xCAFE, &pf_s, &dc_s);
    * = step;
}
```

---

## 子模块：LLM 工具调用参数 JSON Schema 零分配校验器（`tool_schema.cp`）

### 核心特性

- **零分配直接扫描原始 JSON 对象（`validateToolArgs`）**：无需先反序列化 DOM/AST，直接在词法层遍历顶层 `"key": value` 键值对，精准区分 `Integer`（纯整数）与 `Number`（含小数/科学计数法），执行 `[min_num, max_num]` 数值区间、解码后字符串长度 `[min_len, max_len]`、`enum_values` 枚举白名单、必填字段缺失、重复键与未知字段校验。
- **LLM 自愈错误提示格式化（`formatValidationHint`）**：将 `ValidationReport` 直接格式化为标准自然语言诊断字符串（如 `"Tool argument validation failed: missing required field 'path'"`），可直接回灌给模型进行下一轮自动参数修正。

### 导出清单

- **Schema 与报告类型**：`FieldType`（`Any | String | Number | Integer | Boolean | Array | Object | Null`）、`FieldSpec`、`SchemaErrorKind`、`ValidationReport`
- **校验与提示生成**：`validateToolArgs`、`formatValidationHint`

### 使用示例

```typescript
import std from "std";
import {
    FieldType,
    FieldSpec,
    validateToolArgs,
    formatValidationHint,
} from "cap:agent";

fn toolSchemaDemo() {
    const modes: [2]string = ["read", "write"];
    const empty_enum: [0]string = [];
    const specs: [3]FieldSpec = [
        FieldSpec { name: "path", kind: FieldType.String, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 1, max_len: 256, enum_values: empty_enum[0..] },
        FieldSpec { name: "mode", kind: FieldType.String, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: modes[0..] },
        FieldSpec { name: "limit", kind: FieldType.Integer, required: false, has_min_num: true, min_num: 1.0, has_max_num: true, max_num: 100.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
    ];

    const rep = validateToolArgs("{\"path\":\"src/main.cp\",\"mode\":\"read\",\"limit\":20}", specs[0..], false);
    let hint_buf: [128]u8 = std.mem.zeroes([128]u8);
    let hint_s: []u8 = hint_buf[0..];
    * = formatValidationHint(rep, &hint_s);
}
```

---

## 子模块：LLM 词表 Logits 采样器（`sampler.cp`）

### 核心特性

- **完整零堆分配采样流水线（`sampleToken`）**：基于调用方工作区切片（`work_logits` / `work_indices` / `work_probs`），依次执行：重复/频率/存在惩罚 -> `temperature == 0` 贪心快捷路径 -> Temperature 缩放与数值稳定 Softmax -> Top-K 截断 -> Top-P 核采样截断 -> Min-P 相对峰值截断 -> 重归一化与确定性 SplitMix64 采样。
- **独立可组合原语**：`applyRepetitionPenalties` 严格遵循 Keskar（正 logit 除以 `repeat_penalty`、负 logit 乘以 `repeat_penalty`）与 OpenAI（`frequency_penalty * count + presence_penalty`）规范；`greedyArgmax` 确定性返回最大非 `NaN` logit 索引。

### 导出清单

- **配置与结果**：`SamplerConfig`（`greedy`, `default`）、`SampledToken`
- **采样原语**：`applyRepetitionPenalties`、`greedyArgmax`、`sampleToken`

### 使用示例

```typescript
import std from "std";
import { SamplerConfig, sampleToken } from "cap:agent";

fn logitsSamplerDemo() {
    const logits: [5]f64 = [4.2, 3.8, 1.5, -0.5, -4.0];
    const recent: [2]usize = [0, 0];
    let w_logits: [5]f64 = std.mem.zeroes([5]f64);
    let w_indices: [5]usize = std.mem.zeroes([5]usize);
    let w_probs: [5]f64 = std.mem.zeroes([5]f64);
    let wl_s: []f64 = w_logits[0..];
    let wi_s: []usize = w_indices[0..];
    let wp_s: []f64 = w_probs[0..];

    let cfg = SamplerConfig.default(2026);
    cfg.repeat_penalty = 1.2;
    cfg.top_k = 3;
    cfg.top_p = 0.9;
    cfg.min_p = 0.05;

    const tok = sampleToken(logits[0..], recent[0..], cfg, &wl_s, &wi_s, &wp_s) ?? return;
    * = tok;
}
```

---

## 子模块：推测解码校验与自适应草稿步长控制（`speculative.cp`）

### 核心特性

- **无损拒绝采样校验器（`verifySpeculativeStep`）**：严格实现 Leviathan et al. (2023) 推测解码拒绝采样算法：逐位以 $\min(1, p_i(x_i)/q_i(x_i))$ 接受草稿词；首次拒绝时在残差分布 $\text{norm}(\max(0, p_i - q_i))$ 采样修正词；若 `K` 个草稿词全部通过，则从 `target_probs[K]` 采样额外奖励词 `bonus_token`，单步无损产出 `K + 1` 个 token，数学上严格等价于直接从目标大模型分布 $p$ 采样。
- **确定性贪心校验（`verifySpeculativeGreedy`）**：零随机逐位比对 `draft_tokens[i] == argmax(target_probs[i])`，首个不匹配位置截断并输出目标模型 `argmax`。
- **自适应草稿步长控制器（`AdaptiveDraftController`）**：基于移动平均接受率（EMA）在 `[min_k, max_k]` 区间内自动拉长或缩减草稿步长 `draft_k`，并实时统计全局 `acceptance_rate` 与 `avg_tokens_per_step`。

### 导出清单

- **校验原语**：`SpecVerifyResult`、`verifySpeculativeStep`、`verifySpeculativeGreedy`
- **自适应步长控制**：`SpecStats`、`AdaptiveDraftController`（`init`, `currentK`, `recordStep`, `stats`）

### 使用示例

```typescript
import std from "std";
import {
    verifySpeculativeStep,
    AdaptiveDraftController,
} from "cap:agent";

fn speculativeDemo() {
    const draft_tokens: [2]usize = [0, 1];
    const draft_probs: [6]f64 = [0.7, 0.2, 0.1, 0.1, 0.8, 0.1];
    const target_probs: [9]f64 = [0.7, 0.2, 0.1, 0.1, 0.8, 0.1, 0.2, 0.3, 0.5];
    let emitted: [3]usize = std.mem.zeroes([3]usize);
    let residual: [3]f64 = std.mem.zeroes([3]f64);
    let em_s: []usize = emitted[0..];
    let res_s: []f64 = residual[0..];

    const res = verifySpeculativeStep(
        draft_tokens[0..], draft_probs[0..], target_probs[0..], 3, 42, &em_s, &res_s,
    ) ?? return;

    let ctrl = AdaptiveDraftController.init(1, 8, 2, 0.3) ?? return;
    const next_k = ctrl.recordStep(2, res.accepted_count, res.emitted_count);
    * = next_k;
}
```

---

## 子模块：束搜索解码器（`beam.cp`）

### 核心特性

- **双缓冲零堆分配束搜索引擎（`BeamSearch`）**：基于调用方提供的乒乓双缓冲（`tokens_a`/`tokens_b`、`hyps_a`/`hyps_b`）与完成序列池（`finished_tokens`/`finished_hyps`），支持有界 Top-K 扩展、遇 `eos_token` 自动归档与早停（`early_stopping`）。
- **GNMT 长度惩罚与 N-gram 防重复阻塞**：内置 Wu et al. (2016) 长度惩罚公式 `((5 + L) / 6)^alpha`（`gnmtLengthPenalty` / `normalizedBeamScore`），以及零分配 `wouldRepeatNgram` 严格阻止生成循环重复 N-gram。

### 导出清单

- **长度惩罚与 N-gram 原语**：`gnmtLengthPenalty`、`normalizedBeamScore`、`wouldRepeatNgram`
- **束搜索状态机**：`BeamConfig`、`BeamHyp`、`BeamCandidate`、`BeamSearch`（`init`, `stepLogProbs`, `bestSequence`, `activeBeamTokens`, `activeCount`, `finishedCount`）

### 使用示例

```typescript
import std from "std";
import { BeamConfig, BeamHyp, BeamCandidate, BeamSearch } from "cap:agent";

fn beamSearchDemo() {
    const cfg = BeamConfig {
        beam_width: 2,
        max_len: 4,
        eos_token: 0,
        length_penalty_alpha: 0.6,
        no_repeat_ngram: 2,
    };
    let ta: [8]u32 = std.mem.zeroes([8]u32);
    let tb: [8]u32 = std.mem.zeroes([8]u32);
    let ha: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let hb: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ft: [8]u32 = std.mem.zeroes([8]u32);
    let fh: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ta_s: []u32 = ta[0..];
    let tb_s: []u32 = tb[0..];
    let ha_s: []BeamHyp = ha[0..];
    let hb_s: []BeamHyp = hb[0..];
    let ft_s: []u32 = ft[0..];
    let fh_s: []BeamHyp = fh[0..];

    let bs = BeamSearch.init(cfg, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) ?? return;
    let cands: [4]BeamCandidate = std.mem.zeroes([4]BeamCandidate);
    let cand_s: []BeamCandidate = cands[0..];

    const step1: [3]f64 = [-10.0, -0.2, -0.5];
    * = bs.stepLogProbs(step1[0..], 3, &cand_s);
}
```
