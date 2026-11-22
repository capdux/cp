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

---

## 子模块：多智能体自洽投票与置信度加权共识仲裁器（`consensus.cp`）

### 核心特性

- **Self-Consistency 多数决与置信度加权仲裁（`resolveConsensus`）**：按 `answer_hash` 聚合多路 Agent 投票权重（支持 `agent_weight` 等权/专家加权或 `agent_weight * confidence` 置信度加权），精确计算胜出占比 `agreement_ratio` 与 Shannon 分歧熵 `disagreement_entropy`（比特）；全一致时熵精确为 `0.0`，平票返回 `TieUnresolved`，未达法定人数 `quorum_ratio` 返回 `NoQuorum`。
- **加权 / 等权 Borda 排名投票汇总（`resolveBordaRanking`）**：支持多智能体对多候选方案排序打分，自动校验候选编号范围与同一投票者重复作弊，输出每位候选人的累计 Borda 总分、胜出者索引及平局标记 `is_tie`。
- **纯 CP 零堆分配**：内置满精度 IEEE-754 纯 CP `log2` 实现，零外部依赖；对 `NaN`/Inf、越界置信度/权重/法定人数及缓冲不足全面防御返回 `null`。

### 导出清单

- **自洽投票与共识仲裁**：`AgentVote`、`ConsensusStatus`（`Reached | NoQuorum | TieUnresolved | Empty`）、`ConsensusResult`、`resolveConsensus`
- **Borda 排名聚合**：`BordaResult`、`resolveBordaRanking`

### 使用示例

```typescript
import {
    AgentVote,
    ConsensusStatus,
    resolveConsensus,
    resolveBordaRanking,
} from "cap:agent";

fn multiAgentConsensusDemo() {
    const votes: [5]AgentVote = [
        AgentVote{ answer_hash: 1001, answer_index: 0, confidence: 0.9, agent_weight: 1.0 },
        AgentVote{ answer_hash: 1001, answer_index: 1, confidence: 0.85, agent_weight: 1.0 },
        AgentVote{ answer_hash: 1001, answer_index: 2, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 2002, answer_index: 3, confidence: 0.7, agent_weight: 1.0 },
        AgentVote{ answer_hash: 2002, answer_index: 4, confidence: 0.6, agent_weight: 1.0 },
    ];
    let h_arr: [5]u64 = undefined;
    let w_arr: [5]f64 = undefined;
    let idx_arr: [5]usize = undefined;
    let h_s: []u64 = h_arr[0..];
    let w_s: []f64 = w_arr[0..];
    let idx_s: []usize = idx_arr[0..];

    if (res := resolveConsensus(votes[0..], 0.5, true, h_s, w_s, idx_s)) {
        * = res; // status == ConsensusStatus.Reached, winner_hash == 1001
    }

    const rankings: [6]usize = [0, 1, 2, 1, 0, 2];
    const equal_w: []const f64 = [];
    let pts_arr: [3]f64 = undefined;
    let pts: []f64 = pts_arr[0..];
    * = resolveBordaRanking(rankings[0..], 2, 3, equal_w, pts);
}
```

---

## 子模块：人机协同审批队列与多签 Quorum 治理（`hitl.cp`）

### 核心特性

- **风险分级自动放行与人工挂起（`ActionRisk` + `HitlPolicy`）**：`risk <= policy.auto_approve_max_risk` 的低风险动作立即记为 `Approved` 自动放行；更高风险动作进入 `Pending`，其中 `Critical` 级别强制要求 `policy.critical_quorum`（默认 `2`）人多签复核。
- **确定性 SLA 过期与终态不可逆守卫**：`approve`/`reject`/`cancel` 执行前优先校验 `now_ms >= deadline_ms` 自动转为 `Expired`；支持 `sweepExpired(now_ms)` 批量清理超时挂起项；一旦进入 `Approved`/`Rejected`/`Expired`/`Cancelled` 终态则永久不可逆，完整保留 `created_ms` / `deadline_ms` / `resolved_ms` 审计时间戳。
- **固定池零堆分配**：由调用方提供 `ApprovalRequest` 切片，拒绝重复 ID、空动作名与池满写入。

### 导出清单

- **类型与策略**：`ApprovalState`、`ActionRisk`、`ApprovalRequest`、`HitlPolicy`（含 `HitlPolicy.default()`）
- **固定池队列**：`HitlQueue`（`init`, `submit`, `approve`, `reject`, `cancel`, `sweepExpired`, `pendingCount`, `findRequest`）

### 使用示例

```typescript
import {
    ActionRisk,
    ApprovalRequest,
    ApprovalState,
    HitlPolicy,
    HitlQueue,
} from "cap:agent";

fn hitlApprovalDemo() {
    let pool_arr: [8]ApprovalRequest = undefined;
    let pool: []ApprovalRequest = pool_arr[0..];
    let q = HitlQueue.init(pool, HitlPolicy.default());

    // Low-risk action auto-approves immediately
    * = q.submit(1, 10, "read_docs", 0x11, ActionRisk.Low, 1000);

    // Critical action requires 2 approvals before deadline
    * = q.submit(2, 10, "deploy_prod", 0x22, ActionRisk.Critical, 1000);
    * = q.approve(2, 2000); // Pending (1/2)
    * = q.approve(2, 3000); // Approved (2/2)
}
```

---

## 子模块：AI Agent 沙箱策略裁决器（`sandbox.cp`）

### 核心特性

- **工作区路径边界裁决（`checkPathSandbox`）**：零堆分配词法路径归一化（消解 `.`/`..`/`//`），按路径段边界精确匹配 `allowed_roots`（杜绝 `/data/app` 与 `/data/app2` 前缀伪匹配）与 `denied_prefixes`（优先拦截工作区内 `.env`/`.git/config` 等受保护文件），严格执行 `Read`/`Write`/`Delete` 只读权限及 NUL 字节拦截。
- **命令与参数白名单/元字符策略（`checkCommandSandbox`）**：黑名单优先、白名单按完整路径与基名匹配，开启 `forbid_shell_metachars` 时拦截 `;`/`|`/`&`/`$`/`` ` ``/`>`/`<`/`\n`/`\x00` 等控制字符。
- **子进程环境变量脱敏过滤（`isSensitiveEnvKey` / `filterEnvIndices`）**：大小写不敏感识别 `_KEY`/`_SECRET`/`_TOKEN`/`_PASSWORD`/`_CREDENTIAL` 等敏感凭据键，支持白名单与敏感键剥离组合过滤。

### 导出清单

- **路径策略**：`PathAccessMode`、`SandboxDecision`、`PathRule`、`checkPathSandbox`
- **命令策略**：`CommandPolicy`、`checkCommandSandbox`
- **环境变量过滤**：`isSensitiveEnvKey`、`filterEnvIndices`

### 使用示例

```typescript
import {
    PathAccessMode,
    PathRule,
    CommandPolicy,
    checkPathSandbox,
    checkCommandSandbox,
    filterEnvIndices,
} from "cap:agent";

fn codingAgentSandboxDemo() {
    const roots: [1]PathRule = [
        PathRule{ root: "/data/app", allow_write: true, allow_delete: false },
    ];
    const denied: [2]string = [".env", ".git/config"];
    let scratch_arr: [256]u8 = undefined;
    let scratch: []u8 = scratch_arr[0..];

    * = checkPathSandbox("/data/app/src/main.cp", PathAccessMode.Write, roots[0..], denied[0..], scratch);

    const allowed_bins: [2]string = ["git", "cap"];
    const denied_bins: [1]string = ["sh"];
    const cmd_pol = CommandPolicy{
        allowed_binaries: allowed_bins[0..],
        denied_binaries: denied_bins[0..],
        forbid_shell_metachars: true,
    };
    const args: [2]string = ["status", "--short"];
    * = checkCommandSandbox("git", args[0..], cmd_pol);

    const env_keys: [3]string = ["PATH", "OPENAI_API_KEY", "HOME"];
    const allow_env: []const string = [];
    let keep_arr: [3]usize = undefined;
    let keep: []usize = keep_arr[0..];
    * = filterEnvIndices(env_keys[0..], allow_env, true, keep);
}
```

---

## 子模块：AI Agent 层级 Span 链路追踪与 GenAI 遥测导出器（`tracer.cp`）

### 核心特性

- **对标 OpenTelemetry GenAI 语义约定**：提供 `AgentRun` / `LlmCall` / `ToolExec` / `Retrieval` / `Guardrail` / `Handoff` 六类语义 Span，通过 `NO_PARENT_SPAN` 与 `parent_id` 构建完整树形调用链。
- **确定性时间与指标汇总（`rollup`）**：`endSpan` 禁止重复结束与回退为 `Running`，自动对时间回拨执行 `end_ms >= start_ms` 钳位；`rollup()` 精确汇总全链路 `prompt_tokens`、`completion_tokens`、微美元成本、错误 Span 数、LLM/Tool 调用次数及端到端耗时。
- **零堆分配 RFC-8259 转义 JSON 导出（`exportJson`）**：直接将 `trace_id`、`rollup` 与全部 `spans` 序列化至调用方缓冲（自动转义 `"`/`\`/`\n`/控制字符），可直接对接可观测性平台。

### 导出清单

- **常量与类型**：`NO_PARENT_SPAN`、`AgentSpanKind`、`AgentSpanStatus`、`AgentSpan`、`TraceRollup`
- **链路追踪器**：`AgentTracer`（`init`, `startSpan`, `endSpan`, `rollup`, `exportJson`）

### 使用示例

```typescript
import {
    NO_PARENT_SPAN,
    AgentSpan,
    AgentSpanKind,
    AgentSpanStatus,
    AgentTracer,
} from "cap:agent";

fn agentTracerDemo() {
    let pool_arr: [8]AgentSpan = undefined;
    let pool: []AgentSpan = pool_arr[0..];
    let tr = AgentTracer.init(9001, pool);

    if (root := tr.startSpan(NO_PARENT_SPAN, AgentSpanKind.AgentRun, "solve", "planner", 1000)) {
        if (llm := tr.startSpan(root, AgentSpanKind.LlmCall, "chat", "gpt-4o", 1010)) {
            * = tr.endSpan(llm, AgentSpanStatus.Ok, 250, 80, 1200, 1450);
        }
        * = tr.endSpan(root, AgentSpanStatus.Ok, 0, 0, 0, 1500);
    }

    const r = tr.rollup();
    * = r;

    let json_arr: [2048]u8 = undefined;
    let json_buf: []u8 = json_arr[0..];
    * = tr.exportJson(json_buf);
}
```

---

## 子模块：AI Agent Skill / Plugin 注册中心与能力匹配引擎（`skill_registry.cp`）

### 核心特性

- **固定槽位注册与生命周期治理**：支持按 ID/名称去重注册（`register`）、安全注销（正在执行中的技能禁止强行卸载）、启用/禁用/废弃状态切换（`enable`/`disable`/`setStatus`）与快速检索（`findById`/`findByName`）。
- **位掩码能力匹配 + 标签过滤 + Top-K 综合打分（`matchTopK`）**：支持 `required_caps` 强制能力掩码、`preferred_caps` 偏好能力加分（`popcount * 1000`）、`skillTagHash` 标签匹配、`max_cost_units` 成本上限与并发空闲槽位过滤，按 `score` 降序确定性输出 Top-K 技能。
- **确定性依赖拓扑激活解析（`resolveActivationOrder`）**：基于 Kahn 拓扑算法校验依赖图完整性，严格拦截缺失依赖、禁用依赖、自依赖与循环依赖，输出确定性的激活序列。
- **并发配额限流与状态汇总（`tryAcquire` / `release` / `stats`）**：精确执行每技能 `max_concurrency` 并发上限与防下溢释放，并实时输出 `SkillRegistryStats`。

### 导出清单

- **常量与类型**：`SKILL_NO_DEP`、`SkillStatus`、`SkillDescriptor`、`SkillQuery`、`SkillMatch`、`SkillRegistryStats`
- **辅助与注册中心**：`skillTagHash`、`SkillRegistry`（`init`, `register`, `unregister`, `setStatus`, `enable`, `disable`, `findById`, `findByName`, `tryAcquire`, `release`, `matchTopK`, `resolveActivationOrder`, `stats`）

### 使用示例

```typescript
import {
    SkillDescriptor,
    SkillQuery,
    SkillMatch,
    SkillRegistry,
    skillTagHash,
} from "cap:agent";

fn skillRegistryDemo() {
    let slots_arr: [8]SkillDescriptor = undefined;
    let slots: []SkillDescriptor = slots_arr[0..];
    let reg = SkillRegistry.init(slots);

    const tag_code = skillTagHash("coding");
    * = reg.register(SkillDescriptor{
        id: 1,
        name: "code_search",
        capability_mask: 0b0011,
        priority: 20,
        cost_units: 5,
        max_concurrency: 2,
        tag_hash_0: tag_code,
    });

    let matches_arr: [4]SkillMatch = undefined;
    let matches: []SkillMatch = matches_arr[0..];
    const q = SkillQuery{ required_caps: 0b0001, preferred_caps: 0b0010, required_tag_hash: tag_code };
    * = reg.matchTopK(q, matches, 2);
}
```

---

## 子模块：LLM API 自适应重试、限流感知与故障分类引擎（`retry_policy.cp`）

### 核心特性

- **标准化故障分类（`classifyHttpStatus`）**：将常见 LLM 厂商 HTTP 状态码（`401`/`403`、`400`/`413`/`422` + 上下文溢出标记、`429`、`503`/`529`、`408`/`504`/`524`、`5xx` 等）归一化为 `LlmErrorClass`。
- **限流头解析与优先尊重（`parseRetryAfterMs` / `RateLimitSnapshot`）**：零堆分配解析整秒/小数秒及 `ms`/`s`/`m` 后缀（防负数、非法字符与溢出），并在 `decideLlmRetry` 中优先尊重 `retry_after_ms` 及耗尽桶的 `reset_requests_ms`/`reset_tokens_ms`（按 `max_delay_ms` 上限钳位）。
- **确定性抖动指数退避与智能切流（`decideLlmRetry`）**：基于 SplitMix64 实现可复现的 `±jitter_pct%` 指数退避；对认证/参数/致命错误立即 `Abort`；对 `ContextLengthExceeded` 建议 `ShrinkContextAndRetry`；对连续 `429`/`529` 或重试耗尽自动建议 `FallbackProvider`。

### 导出清单

- **错误与决策类型**：`LlmErrorClass`、`RetryDecisionKind`、`RetryDecision`、`RateLimitSnapshot`、`LlmRetryPolicy`
- **核心函数**：`classifyHttpStatus(status_code, is_context_overflow)`、`parseRetryAfterMs(header_val)`、`decideLlmRetry(policy, err_class, attempt, rl_snapshot, seed)`

### 使用示例

```typescript
import {
    LlmRetryPolicy,
    RateLimitSnapshot,
    classifyHttpStatus,
    parseRetryAfterMs,
    decideLlmRetry,
} from "cap:agent";

fn llmRetryDecisionDemo(http_status: u16, retry_after_header: string, attempt: u32) {
    const err_class = classifyHttpStatus(http_status, false);
    const rl = RateLimitSnapshot{
        retry_after_ms: parseRetryAfterMs(retry_after_header) ?? 0,
    };
    const policy = LlmRetryPolicy.default();
    const decision = decideLlmRetry(policy, err_class, attempt, rl, 42);
    * = decision;
}
```

---

## 子模块：幂等工具调用结果记忆化缓存与副作用门控器（`tool_cache.cp`）

### 核心特性

- **作用域隔离复合键（`ToolCacheKey` / `computeToolCacheKey`）**：基于 64 位 FNV-1a 哈希将 `(tool_name, args_canonical, scope_id)` 映射为固定大小键，按会话/任务作用域隔离读写。
- **副作用门控（`ToolSideEffectMode`）**：
  - `ReadOnlyCacheable`：允许写入并命中缓存，支持 TTL 过期与覆盖更新；
  - `MutatingBypass`：写操作拒绝入缓存，累计 `bypassed_mutating`；
  - `MutatingInvalidateScope`：写操作拒绝入缓存并自动失效同一 `scope_id` 下的所有读缓存。
- **零堆分配负载池与 LRU 驱逐（`ToolCallCache`）**：由调用方提供固定 `ToolCacheEntry` 槽位切片与扁平 `[]u8` 负载缓冲；槽位满时按 `(last_hit_ms, last_access_seq)` 执行精确 LRU 驱逐；字节池满时执行原地前向紧缩（`compactPayload`）并按需 LRU 驱逐，同时累计 `hits`/`misses`/`evictions`/`invalidations`/`total_saved_latency_ms`。

### 导出清单

- **键与模式**：`ToolCacheKey`、`computeToolCacheKey(tool_name, args_canonical, scope_id)`、`ToolSideEffectMode`
- **缓存与遥测**：`ToolCacheEntry`、`ToolCacheStats`、`ToolCallCache`（`init`, `lookup`, `put`, `invalidateScope`, `invalidateTool`, `purgeExpired`, `clear`, `stats`）

### 使用示例

```typescript
import {
    ToolCacheEntry,
    ToolSideEffectMode,
    ToolCallCache,
    computeToolCacheKey,
} from "cap:agent";

fn toolCacheDemo() {
    let slots_arr: [8]ToolCacheEntry = [ToolCacheEntry{}; 8];
    let payload_arr: [512]u8 = [0; 512];
    let slots: []ToolCacheEntry = slots_arr[0..];
    let payload: []u8 = payload_arr[0..];
    let cache = ToolCallCache.init(slots, payload);

    const k_read = computeToolCacheKey("read_file", "{\"path\":\"main.cp\"}", 1);
    * = cache.put(k_read, ToolSideEffectMode.ReadOnlyCacheable, "fn main() {}", 60000, 120, 1000);

    if (cached := cache.lookup(k_read, 1500)) {
        * = cached; // 命中并累计 120ms 节省延迟
    }

    // 同作用域写操作自动失效该作用域所有读缓存
    const k_write = computeToolCacheKey("write_file", "{\"path\":\"main.cp\"}", 1);
    * = cache.put(k_write, ToolSideEffectMode.MutatingInvalidateScope, "ok", 0, 30, 1600);
}
```

---

## 子模块：多租户 LLM 双维度 TPM/RPM 滑动窗口配额限流与预留/回退引擎（`quota_limiter.cp`）

### 核心特性

- **双窗口加权滑动计数器（`TenantQuotaSlot`）**：使用全整数 `u128` 精确计算当前窗口与上一窗口的线性衰减加权 RPM/TPM 消耗，杜绝固定窗口边界突刺，并在限流拒绝时给出精确平滑的 `retry_after_ms`。
- **三维度独立门控（`tryAcquire`）**：按 `ConcurrencyExceeded` -> `RpmExceeded` -> `TpmExceeded` 顺序独立校验各租户配额（任一维度设为 `0` 表示不限制），多租户槽位物理隔离。
- **响应后多退少补与失败全量回退（`reconcileTokens` / `abortRequest`）**：`reconcileTokens` 在 LLM 响应后按 `actual_tokens` 对比 `estimated_tokens` 精准退还盈余或补扣差额并释放并发槽位；`abortRequest` 在上游失败/取消时全量回退并发、RPM 与预估 TPM 配额。

### 导出清单

- **枚举与配置**：`QuotaRejectReason`、`TenantQuotaConfig`、`QuotaDecision`、`TenantQuotaStats`、`TenantQuotaSlot`
- **核心限流器**：`LlmQuotaLimiter`（`init`, `tenantCount`, `registerTenant`, `updateTenantConfig`, `tryAcquire`, `reconcileTokens`, `abortRequest`, `getStats`）

### 使用示例

```typescript
import {
    QuotaRejectReason,
    TenantQuotaConfig,
    TenantQuotaSlot,
    LlmQuotaLimiter,
} from "cap:agent";

fn multiTenantQuotaDemo() {
    let slots_arr: [4]TenantQuotaSlot = [TenantQuotaSlot{}; 4];
    let slots: []TenantQuotaSlot = slots_arr[0..];
    let limiter = LlmQuotaLimiter.init(slots);

    * = limiter.registerTenant(1001, TenantQuotaConfig{
        rpm_limit: 60,
        tpm_limit: 50000,
        max_concurrent: 4,
        window_ms: 60000,
    });

    const dec = limiter.tryAcquire(1001, 1200, 1000);
    if (dec.reason == QuotaRejectReason.Allowed) {
        // LLM 实际消耗 850 tokens -> 退还 350 tokens 并释放并发槽位
        * = limiter.reconcileTokens(1001, 1200, 850, 1400);
    }
}
```

---

## 子模块：LLM 流式 Tool-Call JSON 参数增量组装与半完成闭合器（`tool_stream.cp`）

### 核心特性

- **零堆分配增量组装器（`ToolStreamAssembler`）**：基于调用方固定字节缓冲与 64 层嵌套栈逐 chunk 喂入流式 Delta；实时追踪字符串引号、`\` 转义序列与 `{}`/`[]` 平衡深度，精准识别 `Idle` / `Streaming` / `Complete` / `Overflow` / `Malformed`。
- **半完成 JSON 自动闭合（`closePartialJson`）**：将截断的流式 JSON（未闭合字符串与悬空反斜杠、未写值的对象键、尾随逗号/冒号、截断的 `true`/`false`/`null`/小数点及任意层未闭合 `{}`/`[]`）零堆分配补全为语法合法 JSON，便于提前渲染 UI 或做参数预检。
- **顶层流式字符串增量提取（`peekStreamingStringField`）**：跳过嵌套子对象内的同名字段，按 JSON 转义规则实时解码并提取指定顶层字符串字段当前已到达的前缀内容。

### 导出清单

- **状态与组装器**：`ToolStreamState`、`ToolStreamAssembler`（`init`, `feedDelta`, `state`, `isComplete`, `payload`, `nestingDepth`, `reset`）
- **半完成处理辅助**：`closePartialJson(partial, out_buf: *[]u8)`、`peekStreamingStringField(partial, key, out_val: *[]u8)`

### 使用示例

```typescript
import {
    ToolStreamAssembler,
    closePartialJson,
    peekStreamingStringField,
} from "cap:agent";

fn streamingToolCallDemo() {
    let raw_arr: [256]u8 = [0; 256];
    let raw_buf: []u8 = raw_arr[0..];
    let asm_inst = ToolStreamAssembler.init(raw_buf);

    * = asm_inst.feedDelta("{\"path\":\"src/main");

    // 1. 提前提取已到达的顶层 "path" 字段前缀
    let path_arr: [64]u8 = [0; 64];
    let path_buf: []u8 = path_arr[0..];
    if (n := peekStreamingStringField(asm_inst.payload(), "path", path_buf)) {
        * = path_buf[0..n]; // "src/main"
    }

    // 2. 自动闭合为合法 JSON 供下游预校验
    let closed_arr: [256]u8 = [0; 256];
    let closed_buf: []u8 = closed_arr[0..];
    if (m := closePartialJson(asm_inst.payload(), closed_buf)) {
        * = closed_buf[0..m]; // "{\"path\":\"src/main\"}"
    }
}
```

---

## 子模块：确定性多步骤 AI Agent 编排有限状态机与审计回滚引擎（`workflow_fsm.cp`）

### 核心特性

- **确定性优先级边路由（`AgentWorkflowFsm`）**：支持 `Start` / `LlmStep` / `ToolStep` / `Decision` / `HumanGate` / `TerminalOk` / `TerminalFail` 七类节点；同一 `(from_id, event_code)` 存在多条边时按 `priority` 最高者确定性流转。
- **循环护栏与人工审批门控**：每个节点可配置 `max_visits`（`0` 为不限），超限时返回 `VisitLimitExceeded` 且不破坏当前状态或审计日志；`HumanGate` 节点配合 `isWaitingHuman()` 实现原生人工介入暂停与事件恢复。
- **零堆分配审计轨迹、单步回滚与静态可达性校验**：每步流转写入固定 `TransitionRecord` 审计缓冲；`rollbackLastStep()` 精确还原上一节点并回退目标节点 `visit_count`；`validateReachability()` 在零堆分配下检测缺起点、悬空边、孤立节点与不可达终态。

### 导出清单

- **图与记录类型**：`WorkflowNodeKind`、`WorkflowNode`、`WorkflowEdge`、`TransitionRecord`、`WorkflowStepResult`、`ReachabilityReport`
- **状态机**：`AgentWorkflowFsm`（`init`, `start`, `trigger`, `rollbackLastStep`, `currentNodeId`, `currentNodeKind`, `isTerminal`, `isWaitingHuman`, `history`, `validateReachability`）

### 使用示例

```typescript
import {
    WorkflowNodeKind,
    WorkflowNode,
    WorkflowEdge,
    TransitionRecord,
    AgentWorkflowFsm,
} from "cap:agent";

fn workflowFsmDemo() {
    let nodes_buf: [3]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start, max_visits: 1 },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.LlmStep, max_visits: 3 },
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.TerminalOk, max_visits: 1 },
    ];
    const edges: [2]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: 1, priority: 0 },
        WorkflowEdge{ from_id: 2, to_id: 3, event_code: 2, priority: 0 },
    ];
    let hist_buf: [8]TransitionRecord = [TransitionRecord{}; 8];
    let nodes: []WorkflowNode = nodes_buf[0..];
    let hist: []TransitionRecord = hist_buf[0..];

    let fsm = AgentWorkflowFsm.init(nodes, edges[0..], hist);
    if (fsm.validateReachability().isValid() && fsm.start()) {
        * = fsm.trigger(1, 1000);
        * = fsm.trigger(2, 1500); // TerminalReached
    }
}
```

---

## 子模块：多智能体层级 Token 与微美元成本归因账本（`cost_ledger.cp`）

### 核心特性

- **定点数微美元精确核算（`u64` / `u128`）**：所有模型单价以「微美元 / 百万 Token」（`1 USD = 1,000,000 micro_usd`）表示，单次调用四类 Token（`input`、`output`、`cache_read`、`cache_write`）统一在 `u128` 空间合并求值后四舍五入，彻底消灭浮点累计误差与大数乘法溢出。
- **Prompt-Cache 缓存读写折扣与节省核算**：`computeCacheSavingsMicroUsd` 精确对比标准输入单价与缓存读单价，自动累计每个 Agent 的缓存节省金额（`cache_savings_micro_usd`）。
- **环免疫层级子树成本归因（`CostLedger`）**：零堆分配固定槽位账本支持父/子/孙任意深度多智能体子树成本归因（`agentSummary` 同时给出 `direct_micro_usd` 与 `subtree_micro_usd`），内置迭代防环保护免疫自环与互环父子关系，并提供 `totalMicroUsd()` 与 `topSpendingAgent()`。

### 导出清单

- **定价与事件模型**：`ModelRateCard`、`UsageEvent`、`AgentCostSummary`、`TopSpender`
- **纯计算与账本**：`computeUsageCostMicroUsd(rate, ev)`、`computeCacheSavingsMicroUsd(rate, cache_read_tokens)`、`CostLedger`（`init`, `rateCount`, `eventCount`, `findRate`, `registerRate`, `recordUsage`, `agentSummary`, `totalMicroUsd`, `topSpendingAgent`）

### 使用示例

```typescript
import {
    ModelRateCard,
    UsageEvent,
    CostLedger,
} from "cap:agent";

fn costLedgerDemo() {
    let rates_buf: [4]ModelRateCard = [ModelRateCard{}; 4];
    let events_buf: [16]UsageEvent = [UsageEvent{}; 16];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);

    * = ledger.registerRate(ModelRateCard{
        model_id: 1,
        input_per_mtok_micro: 3000000,
        output_per_mtok_micro: 15000000,
        cache_read_per_mtok_micro: 300000,
        cache_write_per_mtok_micro: 3750000,
    });

    * = ledger.recordUsage(UsageEvent{ agent_id: 1, parent_agent_id: 0, model_id: 1, input_tokens: 1000, output_tokens: 200 });
    * = ledger.recordUsage(UsageEvent{ agent_id: 2, parent_agent_id: 1, model_id: 1, input_tokens: 500, cache_read_tokens: 10000 });

    const root_summary = ledger.agentSummary(1); // 包含 Agent 1 与子 Agent 2 的子树总微美元成本
    * = root_summary;
}
```

---

## 子模块：AI Agent Reflexion 自我反思迭代与早停收敛控制器（`reflection.cp`）

### 核心特性

- **结构化失败归因与多门控收敛决策（`ReflexionController`）**：支持 `SyntaxError` / `RuntimeError` / `TestAssertion` / `ToolError` / `Timeout` / `FormatViolation` / `Hallucination` 七类失败归因，单次 `recordAttempt` 依优先级输出 `AcceptBest`（达标且无错误立即接受）、`EscalateHuman`（同类失败连续重复达 `repeat_failure_limit` 或耗尽迭代/槽位上限）、`AbortNoProgress`（连续无显著提升达 `plateau_patience` 早停）或 `ContinueRefine`。
- **经验去重与最优尝试回溯**：通过 `hasSeenLesson(lesson_hash)` 避免 Agent 重复生成相同无效反思；即使后续尝试得分退化，`bestAttempt()` 仍能 O(1) 回溯历史最高分尝试，并支持 `reset()` 零分配复用控制器。

### 导出清单

- **类型与配置**：`FailureCategory`、`ReflectionAction`、`AttemptRecord`、`ReflexionConfig`、`ReflexionStats`
- **控制器**：`ReflexionController`（`init`, `recordAttempt`, `bestAttempt`, `hasSeenLesson`, `stats`, `reset`）

### 使用示例

```cp
import {
    FailureCategory,
    ReflectionAction,
    AttemptRecord,
    ReflexionConfig,
    ReflexionController,
} from "cap:agent";

fn reflexionLoopDemo() {
    let slots_buf: [6]AttemptRecord = [AttemptRecord{}; 6];
    let slots: []AttemptRecord = slots_buf[0..];
    const cfg = ReflexionConfig{
        target_score: 0.9,
        max_attempts: 6,
        plateau_patience: 2,
        min_delta: 0.05,
        repeat_failure_limit: 3,
    };

    let ctrl = ReflexionController.init(slots, cfg) ?? return;
    * = ctrl.recordAttempt(0.45, FailureCategory.SyntaxError, 0x101, 500);   // ContinueRefine
    * = ctrl.recordAttempt(0.75, FailureCategory.TestAssertion, 0x202, 600); // ContinueRefine
    const final_act = ctrl.recordAttempt(0.95, FailureCategory.None, 0x303, 550); // AcceptBest
    * = final_act;
}
```

---

## 子模块：LLM 批量 Tool-Call 读写锁冲突分析与最大并发波次调度器（`tool_parallel.cp`）

### 核心特性

- **读写锁与全局屏障冲突模型（`callsConflict`）**：同资源 `SharedRead` + `SharedRead` 无冲突可并发；同资源任意写（WAR / RAW / WAW）自动冲突；`GlobalBarrier`（如 `git commit` / `cargo build`）与一切调用互斥隔离；不同资源读写互不阻塞。
- **保序拓扑波次打包与加速比估算（`planToolWaves`）**：100% 零堆分配，严格保持原始程序顺序语义，将无冲突后续调用打包至最早合法并发波次，受 `max_parallelism` 宽度约束，并输出 `total_waves`、`max_wave_width`、`serial_ms`、`critical_path_ms` 与定点加速比 `speedup_x100`。

### 导出清单

- **类型**：`ResourceLockMode`（`SharedRead | ExclusiveWrite | GlobalBarrier`）、`ToolCallRequest`、`WaveAssignment`、`ParallelPlanStats`
- **分析与调度**：`callsConflict(a, b): bool`、`planToolWaves(calls, max_parallelism, out_assignments): ParallelPlanStats?`

### 使用示例

```cp
import {
    ResourceLockMode,
    ToolCallRequest,
    WaveAssignment,
    planToolWaves,
} from "cap:agent";

fn parallelToolWavesDemo() {
    const calls: [4]ToolCallRequest = [
        ToolCallRequest{ call_id: 1, resource_hash: 0xAA, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 2, resource_hash: 0xBB, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 120 },
        ToolCallRequest{ call_id: 3, resource_hash: 0xAA, lock_mode: ResourceLockMode.ExclusiveWrite, estimated_ms: 80 },
        ToolCallRequest{ call_id: 4, resource_hash: 0x0, lock_mode: ResourceLockMode.GlobalBarrier, estimated_ms: 200 },
    ];
    let waves_buf: [4]WaveAssignment = [WaveAssignment{}; 4];
    let waves: []WaveAssignment = waves_buf[0..];

    if (stats := planToolWaves(calls[0..], 4, waves)) {
        // Wave 0: [1, 2] (120ms), Wave 1: [3] (80ms), Wave 2: [4] (200ms)
        * = stats;
    }
}
```

---

## 子模块：多轮对话 Prompt-Cache 缓存断点规划与前缀稳定性优化器（`prompt_cache_plan.cp`）

### 核心特性

- **前缀稳定性截断与语义断点放置（`planPromptCacheBreakpoints`）**：对标 Anthropic / OpenAI Prefix Caching，自动扫描最长非易变（`!volatile_flag` 且非 `CurrentUser`）稳定前缀，按 `min_cacheable_tokens` 门槛与语义块切换边界放置至多 `max_breakpoints` 个断点（始终保留末尾断点覆盖完整稳定前缀）。
- **多轮成本收益防倒亏核算**：依据 `expected_future_turns`、`write_overhead_pct`（如 25%）与 `read_discount_pct`（如 90%）精确核算 `estimated_net_savings_tokens`，短对话无复用时自动标记 `worth_caching = false` 避免写缓存反而增加费用。
- **跨轮前缀命中检测（`computePrefixMatchTokens`）**：按顺序比对连续两轮 Prompt 块，遇首处 `content_hash` / `kind` / `tokens` 变异立即停止，精确返回可命中的缓存前缀 Token 数。

### 导出清单

- **类型与配置**：`PromptBlockKind`、`PromptBlockDesc`、`CachePlanConfig`、`CachePlanResult`
- **规划与分析**：`planPromptCacheBreakpoints(blocks, cfg, out_breakpoint_indices): CachePlanResult?`、`computePrefixMatchTokens(prev_blocks, curr_blocks): u32`

### 使用示例

```cp
import {
    PromptBlockKind,
    PromptBlockDesc,
    CachePlanConfig,
    planPromptCacheBreakpoints,
    computePrefixMatchTokens,
} from "cap:agent";

fn promptCachePlanDemo() {
    const blocks: [4]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 600, content_hash: 0x11, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.ToolDefs, tokens: 700, content_hash: 0x22, volatile_flag: false },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.DocContext, tokens: 1500, content_hash: 0x33, volatile_flag: false },
        PromptBlockDesc{ index: 3, kind: PromptBlockKind.CurrentUser, tokens: 120, content_hash: 0x44, volatile_flag: false },
    ];
    let bp_buf: [4]u16 = [0; 4];
    let bps: []u16 = bp_buf[0..];
    const cfg = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 3,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };

    if (plan := planPromptCacheBreakpoints(blocks[0..], cfg, bps)) {
        // plan.worth_caching == true, cached_prefix_tokens == 2800, breakpoints == [1, 2]
        * = plan;
    }
    * = computePrefixMatchTokens(blocks[0..], blocks[0..]);
}
```

---

## 子模块：多来源上下文窗口 Token 预算水填充分配器（`context_budget.cp`）

### 核心特性

- **两阶段零堆分配预算分配（`allocateContextBudget`）**：先扣除 `reserved_output` 得到可用输入预算 `usable_input_budget`：
  1. **保底阶段**：严格按 `priority` 降序满足各来源（System、ToolDefs、RAG 文档、历史对话、Memory）的 `min_tokens` 保底配额；若保底之和超预算则优先保高优先级段并置 `min_floors_met = false`；
  2. **加权水填充阶段**：剩余预算按 `priority` 分层，同层内按 `weight` 比例多轮水填充至 `desired_tokens`，触顶段多余配额自动回流同层未满段，余数逐 Token 补齐实现零浪费。

### 导出清单

- **类型**：`ContextSectionRequest`、`ContextSectionAlloc`、`ContextBudgetSummary`
- **分配器**：`allocateContextBudget(total_window, reserved_output, requests, out_allocs): ContextBudgetSummary?`

### 使用示例

```cp
import {
    ContextSectionRequest,
    ContextSectionAlloc,
    allocateContextBudget,
} from "cap:agent";

fn contextBudgetDemo() {
    const reqs: [3]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 1, priority: 100, min_tokens: 500, desired_tokens: 500, weight: 1 },  // System
        ContextSectionRequest{ section_id: 2, priority: 50, min_tokens: 400, desired_tokens: 2000, weight: 3 },  // RAG Docs
        ContextSectionRequest{ section_id: 3, priority: 50, min_tokens: 200, desired_tokens: 1000, weight: 1 },  // History
    ];
    let alloc_buf: [3]ContextSectionAlloc = [ContextSectionAlloc{}; 3];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    if (summary := allocateContextBudget(4096, 1096, reqs[0..], allocs)) {
        // usable_input_budget = 3000 -> System=500, RAG & History 按 3:1 水填充零浪费
        * = summary;
    }
}
```

---

## 子模块：高并发 Agent 集群心跳存活监视与重启风暴隔离熔断器（`liveness.cp`）

### 核心特性

- **两阶段心跳租约超时探测（`sweepTimeouts`）**：`Healthy` 超过 `heartbeat_timeout_ms` 自动进入 `Suspect` 宽限态，再超过 `suspect_grace_ms` 进入 `Unreachable` 并批量输出失联 Agent ID（输出截断安全）；收到新心跳立即恢复 `Healthy`，内置时钟回拨防护。
- **BEAM 风格重启风暴滑动窗口隔离（`recordRestart`）**：统计 `restart_window_ms` 窗口内重启次数，超过 `max_restarts_in_window` 立即熔断进入 `Quarantined` 隔离态（隔离期内拒绝心跳续期），隔离满 `quarantine_duration_ms` 后由 `sweepTimeouts` 或心跳自动解除并重置窗口计数。

### 导出清单

- **状态与配置**：`AgentHealthState`（`Healthy | Suspect | Unreachable | Quarantined | Stopped`）、`LivenessConfig`、`AgentLivenessSlot`、`LivenessSummary`
- **监视器**：`AgentLivenessMonitor`（`init`, `registerAgent`, `unregisterAgent`, `stopAgent`, `heartbeat`, `recordRestart`, `sweepTimeouts`, `getState`, `getSlot`, `summary`）

### 使用示例

```cp
import {
    LivenessConfig,
    AgentLivenessSlot,
    AgentLivenessMonitor,
} from "cap:agent";

fn livenessMonitorDemo() {
    let slots_buf: [16]AgentLivenessSlot = [AgentLivenessSlot{}; 16];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 3000,
        max_restarts_in_window: 3,
        restart_window_ms: 60000,
        quarantine_duration_ms: 30000,
    };

    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;
    * = mon.registerAgent(101, 1000);
    * = mon.heartbeat(101, 4000);

    let unreach_buf: [8]u32 = [0; 8];
    let unreach: []u32 = unreach_buf[0..];
    * = mon.sweepTimeouts(15000, unreach); // Agent 101 -> Unreachable
    * = mon.summary();
}
```

---

## 子模块：多优先级有界 Agent 信箱与背压丢弃/按键合并引擎（`mailbox.cp`）

### 核心特性

- **4 级优先级 + 同级 FIFO 确定性出队（`AgentMailbox`）**：支持 `System > High > Normal > Low` 四级优先级调度，同优先级严格按 `enqueue_seq` FIFO 升序出队，支持 `peekNext()` 只读预览与 `recvNext()` 出队。
- **三种生产级背压策略（`BackpressurePolicy`）**：
  - `RejectNew`：满箱直接返回 `RejectedFull` 并累计 `rejected_total`；
  - `DropLowestOldest`：满箱时驱逐优先级最低且最旧的消息并返回 `EvictedAndEnqueued`，但**严格保护更高优先级消息（如 `System`）绝不被低优消息驱逐**；
  - `CoalesceByKey`：对相同非零 `coalesce_key` 原地覆盖最新载荷（满箱亦可合并、优先级只升不降并保留原队位防饥饿）。

### 导出清单

- **类型与策略**：`MailPriority`（`Low | Normal | High | System`）、`BackpressurePolicy`（`RejectNew | DropLowestOldest | CoalesceByKey`）、`MailEnvelope`、`SendOutcome`、`MailboxStats`
- **有界信箱**：`AgentMailbox`（`init`, `send`, `recvNext`, `peekNext`, `len`, `isEmpty`, `stats`, `clear`）

### 使用示例

```cp
import {
    MailPriority,
    BackpressurePolicy,
    MailEnvelope,
    AgentMailbox,
} from "cap:agent";

fn agentMailboxDemo() {
    let slots_buf: [8]MailEnvelope = [MailEnvelope{}; 8];
    let slots: []MailEnvelope = slots_buf[0..];
    let mbox = AgentMailbox.init(slots, BackpressurePolicy.DropLowestOldest);

    * = mbox.send(1, MailPriority.Low, 0, 10, 100);
    * = mbox.send(2, MailPriority.System, 0, 99, 999);
    if (msg := mbox.recvNext()) {
        // 优先取出 System 消息 (payload_u64 == 999)
        * = msg;
    }
}
```

---

## 子模块：多核 Agent 调度器 Work-Stealing 双端队列池与负载均衡引擎（`work_steal.cp`）

### 核心特性

- **本地 LIFO + 远端 FIFO 双端队列（`WorkerQueue`）**：固定环形槽位缓冲，`pushLocal`/`popLocal` 从队尾操作保障 CPU L1/L2 缓存局部性，`stealFromHead` 从队头窃取最老任务以降低与本地核心的竞争。
- **多核自动负载均衡池（`WorkStealPool`）**：支持最多 16 个核心（`MAX_WORK_STEAL_WORKERS`），单块扁平缓冲零堆分配；`nextTask(worker_idx)` 优先本地 LIFO 弹出，本地为空时自动选择当前队列最长的最繁忙核心进行头部 FIFO 窃取，并提供 `WorkStealStats` 负载倾斜监控。

### 导出清单

- **常量与类型**：`MAX_WORK_STEAL_WORKERS` (`16`)、`WorkTask`、`WorkStealStats`
- **双端队列与多核池**：`WorkerQueue`（`init`, `pushLocal`, `popLocal`, `stealFromHead`, `count`, `capacity`, `isEmpty`）、`WorkStealPool`（`init`, `submit`, `nextTask`, `queueLen`, `stats`）

### 使用示例

```cp
import { WorkTask, WorkStealPool } from "cap:agent";

fn workStealDemo() {
    let flat_buf: [16]WorkTask = [WorkTask{}; 16];
    let flat: []WorkTask = flat_buf[0..];
    let pool = WorkStealPool.init(4, flat, 4) ?? return;

    // 所有任务集中在核 0，核 1 空闲拉取时自动从核 0 头部 FIFO 窃取最老任务
    * = pool.submit(0, WorkTask{ task_id: 1, agent_id: 10, reductions_budget: 2000 });
    * = pool.submit(0, WorkTask{ task_id: 2, agent_id: 11, reductions_budget: 2000 });
    if (stolen := pool.nextTask(1)) {
        * = stolen; // task_id == 1
    }
    * = pool.stats();
}
```

---

## 子模块：多智能体 Handoff 移交路由、版本化共享黑板与扇出汇聚 Quorum（`handoff.cp`）

### 核心特性

- **Handoff 移交路由与防环保护（`HandoffRouter` / `HandoffRecord`）**：支持显式 Agent 所有权移交、`max_hops` 最大跳数硬限制、自移交拦截（`SelfHandoff`）、`A -> B -> A` 环路检测（`CycleDetected`）、移交上下文摘要与完整轨迹回溯（`history()` / `recordAt()`）。
- **版本化共享黑板（`AgentBlackboard` / `BlackboardEntry`）**：固定槽位零堆分配共享键值板，支持无条件写入（`put`）、乐观锁版本 CAS 写入（`putIfVersion`：键不存在要求 `expected_version == 0`，存在要求精确匹配版本号，否则返回 `VersionConflict`）、写入者 `writer_agent_id` 审计追踪，以及按前缀过滤（`listByPrefix`）与全局单调序列号增量拉取（`listSinceSeq`）。
- **扇出/扇入法定人数汇聚器（`FanOutAggregator` / `FanOutSlot`）**：支持多路并行子 Agent 任务提交、成功/失败状态机转移、达到 `quorum` 立即提前就绪（`isQuorumReady` / `FanOutStatus.QuorumReached`）、不可达提前失败判定（`FanOutStatus.QuorumFailed`）与按得分降序提取已成功结果（`collectSucceeded`）。

### 导出清单

- **移交路由**：`HandoffStatus`（`Accepted | SelfHandoff | CycleDetected | MaxHopsExceeded | HistoryFull`）、`HandoffRecord`、`HandoffRouter`（`init`, `hasVisited`, `handoff`, `currentAgent`, `hopCount`, `remainingHops`, `recordAt`, `history`, `reset`）
- **共享黑板**：`BlackboardWriteStatus`（`Ok | VersionConflict | Full | InvalidKey`）、`BlackboardEntry`、`AgentBlackboard`（`init`, `get`, `put`, `putIfVersion`, `remove`, `listByPrefix`, `listSinceSeq`, `count`, `capacity`, `currentSeq`）
- **扇出汇聚**：`FanOutTaskState`（`Pending | Succeeded | Failed`）、`FanOutStatus`（`InProgress | QuorumReached | QuorumFailed`）、`FanOutSlot`、`FanOutSummary`、`FanOutAggregator`（`init`, `submit`, `recordSuccess`, `recordFailure`, `isQuorumReady`, `status`, `summary`, `collectSucceeded`）

### 使用示例

```cp
import {
    HandoffRecord,
    HandoffRouter,
    BlackboardEntry,
    AgentBlackboard,
    FanOutSlot,
    FanOutAggregator,
} from "cap:agent";

fn multiAgentHandoffDemo() {
    // 1. Handoff 移交路由（Triage Agent 1 -> Coder Agent 2 -> Reviewer Agent 3，禁止回环）
    let trace_buf: [4]HandoffRecord = [HandoffRecord{}; 4];
    let trace_s: []HandoffRecord = trace_buf[0..];
    let router = HandoffRouter.init(trace_s, 1, 3, true) ?? return;
    * = router.handoff(2, "implement feature", 1000);
    * = router.handoff(3, "review patch", 2000);
    // 试图移交回 Agent 1 将触发 HandoffStatus.CycleDetected
    * = router.handoff(1, "loop back", 3000);

    // 2. 版本化共享黑板（乐观锁 CAS 防并发覆盖）
    let bb_buf: [8]BlackboardEntry = [BlackboardEntry{}; 8];
    let bb_s: []BlackboardEntry = bb_buf[0..];
    let board = AgentBlackboard.init(bb_s);
    * = board.putIfVersion("plan.status", "draft", 0, 1, 1000);       // Ok (v1)
    * = board.putIfVersion("plan.status", "approved", 1, 3, 2000);    // Ok (v2)
    * = board.putIfVersion("plan.status", "stale", 1, 2, 2100);       // VersionConflict

    // 3. 扇出/扇入汇聚器（3 路并行子 Agent，Quorum = 2 提前就绪）
    let fan_buf: [4]FanOutSlot = [FanOutSlot{}; 4];
    let fan_s: []FanOutSlot = fan_buf[0..];
    let agg = FanOutAggregator.init(fan_s, 2) ?? return;
    * = agg.submit(10, 101);
    * = agg.submit(11, 102);
    * = agg.submit(12, 103);
    * = agg.recordSuccess(10, 0.91, "answer-A", 1200);
    * = agg.recordSuccess(12, 0.97, "answer-C", 1300);
    if (agg.isQuorumReady()) {
        let winners: [2]FanOutSlot = [FanOutSlot{}; 2];
        let winners_s: []FanOutSlot = winners[0..];
        * = agg.collectSucceeded(winners_s); // winners[0].task_id == 12 (0.97)
    }
}
```

---

## 子模块：端到端 Agent 回合编排状态机（`turn_runner.cp`）

### 核心特性

- **极简端到端闭环（`AgentTurnRunner`）**：开发者仅需 ~25 行代码即可完整驱动“回合预算守卫 -> 模型响应记账 -> 工具调用分发与连续失败熔断 -> 时序步骤审计轨迹 -> 确定性终止汇总”全流程，100% 纯 CP 零堆分配。
- **多维硬限制与熔断（`TurnRunnerConfig` / `TurnStopReason`）**：统一支持 `max_turns`、`max_tool_calls`、`max_input_tokens`、`max_output_tokens`、`max_cost_micro_usd`（微美元防溢出饱和累加）与 `max_consecutive_errors` 连续工具失败熔断（单次工具成功自动清零连续失败计数）。
- **可审计步骤轨迹与缓冲满防护（`TurnStepEntry` / `TurnOutcome`）**：在调用方提供的固定栈缓冲 `[]TurnStepEntry` 中按 `seq` 单调递增记录 `TurnStarted` / `LlmResponse` / `ToolSucceeded` / `ToolFailed` / `Stopped` 全量轨迹；缓冲满时确定性返回 `TurnStopReason.StepBufferFull` 并计数 `steps_dropped`，绝不越界。
- **零分配复用（`reset`）**：支持在不重新分配缓冲的前提下重置状态机，适用于高并发 Agent Worker 池循环执行多轮任务。

### 导出清单

- **配置与枚举**：`TurnRunnerConfig`、`TurnStopReason`（`Completed | MaxTurnsReached | MaxToolCallsReached | InputTokenBudgetExceeded | OutputTokenBudgetExceeded | CostBudgetExceeded | ConsecutiveErrorsExceeded | StepBufferFull | Cancelled | LlmError`）、`TurnStepKind`（`TurnStarted | LlmResponse | ToolSucceeded | ToolFailed | Stopped`）
- **轨迹与汇总**：`TurnStepEntry`、`TurnOutcome`（`isSuccess`, `isBudgetExceeded`）
- **状态机（`AgentTurnRunner`）**：`init`、`beginTurn`、`recordLlmResponse`、`recordToolCall`、`finish`、`currentTurn`、`toolCallCount`、`consecutiveErrors`、`totalTokens`、`totalCostMicroUsd`、`isStopped`、`stopReason`、`stepCount`、`steps`、`reset`

### 25 行 QuickStart 示例

```cp
import { TurnRunnerConfig, TurnStepEntry, TurnStopReason, AgentTurnRunner } from "cap:agent";

fn runAgentQuickStart(): TurnStopReason {
    let steps_buf: [32]TurnStepEntry = [TurnStepEntry{}; 32];
    let steps_s: []TurnStepEntry = steps_buf[0..];
    let runner = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 6,
        max_tool_calls: 12,
        max_input_tokens: 16000,
        max_output_tokens: 4000,
        max_cost_micro_usd: 50000,
        max_consecutive_errors: 2,
    }) ?? return TurnStopReason.Cancelled;

    let now_ms: i64 = 1000;
    let stop: TurnStopReason = TurnStopReason.Completed;
    while (true) {
        if (reason := runner.beginTurn(now_ms)) { stop = reason; break; }
        const need_tool = runner.currentTurn() == 1;
        if (reason := runner.recordLlmResponse(800, 160, 1200, need_tool, "plan", now_ms + 50)) {
            stop = reason;
            break;
        }
        if (reason := runner.recordToolCall("search_docs", true, "3 hits", now_ms + 120)) {
            stop = reason;
            break;
        }
        now_ms += 200;
    }
    const outcome = runner.finish(stop, now_ms + 200);
    return outcome.stop_reason;
}
```

---

## 子模块：自动化评测门禁与基线回归对比器（`eval_gate.cp`）

### 核心特性

- **单遍聚合 + 精确 P50/P95 延迟（`evaluateAgentGate`）**：在调用方提供的栈排序工作缓冲 `[]u64` 上执行原地堆排序计算精确 Nearest-Rank P50/P95/Max 延迟，同步汇总通过率、平均/总微美元成本与工具错误率，100% 零堆分配。
- **多维 SLA 门禁判定（`EvalGateThresholds` / `EvalGateVerdict`）**：按确定性优先级拦截 `PassRateBelowMin`、`LatencySlaViolated`、`CostSlaViolated`、`ToolErrorRateExceeded`。
- **基线回归拦截（`compareWithBaselineGate`）**：对比候选集与基线集通过率，超过 `max_pass_rate_drop` 自动输出 `EvalGateVerdict.BaselineRegression` 并记录 `baseline_pass_rate` 与 `pass_rate_delta`。
- **零堆 CI Markdown 汇总渲染（`renderEvalGateSummary`）**：将 `EvalGateReport` 渲染为可直接嵌入 PR 评论 / CI 日志的简洁 Markdown 报告，内置严格缓冲边界防护（不足时返回 `null`，绝不越界）。

### 导出清单

- **配置与数据结构**：`EvalGateThresholds`、`EvalCaseResult`、`EvalGateVerdict`（`Passed | PassRateBelowMin | LatencySlaViolated | CostSlaViolated | ToolErrorRateExceeded | BaselineRegression`）、`EvalGateReport`（`isPassed`）
- **门禁函数**：`evaluateAgentGate`、`compareWithBaselineGate`、`renderEvalGateSummary`

### QuickStart 示例

```cp
import {
    EvalGateThresholds,
    EvalCaseResult,
    evaluateAgentGate,
    compareWithBaselineGate,
    renderEvalGateSummary,
} from "cap:agent";

fn runCiEvalGate() {
    const baseline: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 120, cost_micro_usd: 800, tool_calls: 2, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 180, cost_micro_usd: 900, tool_calls: 3, tool_errors: 0 },
    ];
    const candidate: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 110, cost_micro_usd: 820, tool_calls: 2, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 190, cost_micro_usd: 880, tool_calls: 2, tool_errors: 0 },
    ];
    let scratch: [4]u64 = [0; 4];
    let scratch_s: []u64 = scratch[0..];
    const thresholds = EvalGateThresholds{
        min_pass_rate: 0.90,
        max_p95_latency_ms: 300,
        max_avg_cost_micro_usd: 1500,
        max_tool_error_rate: 0.10,
        max_pass_rate_drop: 0.02,
    };

    if (report := compareWithBaselineGate(candidate[0..], baseline[0..], thresholds, scratch_s)) {
        let md_buf: [512]u8 = [0; 512];
        let md_s: []u8 = md_buf[0..];
        if (written := renderEvalGateSummary(report, md_s)) {
            * = md_s[0..written];
        }
    }
}
```

---

## 子模块：统一 LLM 流式增量解析器（OpenAI / Anthropic SSE 兼容）（`llm_stream.cp`）

### 核心特性

- **双协议统一抽象（`LlmProviderWire` / `LlmDeltaKind` / `LlmDeltaEvent`）**：将 OpenAI Chat Completions SSE 与 Anthropic Messages SSE 归一化为统一的 `TextDelta | ReasoningDelta | ToolCallStart | ToolCallArgsDelta | Usage | Done | Ignore` 事件模型，一次对接覆盖主流大模型流式输出。
- **100% 零堆分配栈缓冲解码**：所有字符串切片（`text`、`tool_call_id`、`tool_name`、`finish_reason`）全部写入调用方提供的单一 `[]u8` 缓冲，自动解码 `\"` / `\n` / `\r` / `\t` / `\\` / `\/` / `\b` / `\f` / `\uXXXX`（UTF-8）转义序列，缓冲不足时安全截断并置位 `truncated = true`，绝不越界。
- **全特性协议兼容**：
  - **OpenAI（`parseOpenAiSseData`）**：支持 `choices[0].delta.content`、`reasoning_content`、`tool_calls`（`index`/`id`/`function.name`/`function.arguments`）、`finish_reason`、顶层 `usage`（`prompt_tokens`/`completion_tokens`）与 `data: [DONE]` 哨兵。
  - **Anthropic（`parseAnthropicSseData`）**：支持 `message_start`（`input_tokens`）、`content_block_start`（`tool_use`）、`content_block_delta`（`text_delta` / `thinking_delta` / `input_json_delta`）、`message_delta`（`stop_reason` / `usage`）与 `message_stop`；支持显式 `event_type` 或自动回退 JSON 顶层 `"type"` 字段。
- **统计状态机（`LlmDeltaParser` / `LlmStreamStats`）**：内置流式事件计数、字节量、Token 用量、截断与畸形 JSON 错误统计，支持 `reset()` 复用，可与 `SseStreamParser` 无缝串联。

### 导出清单

- **协议与事件模型**：`LlmProviderWire`（`OpenAi | Anthropic`）、`LlmDeltaKind`（`TextDelta | ReasoningDelta | ToolCallStart | ToolCallArgsDelta | Usage | Done | Ignore`）、`LlmDeltaEvent`、`LlmStreamStats`
- **解析函数与状态机**：`parseOpenAiSseData`、`parseAnthropicSseData`、`LlmDeltaParser`（`init`, `reset`, `stats`, `isDone`, `parseEvent`, `parseData`）

### QuickStart 示例

```cp
import {
    LlmProviderWire,
    LlmDeltaKind,
    LlmDeltaParser,
} from "cap:agent";

fn streamLlmQuickStart() {
    let parser = LlmDeltaParser.init(LlmProviderWire.OpenAi);
    let scratch: [512]u8 = [0; 512];
    let out_buf: []u8 = scratch[0..];

    const frames: [4]string = [
        "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"Plan first\\n\"}}]}",
        "data: {\"choices\":[{\"delta\":{\"content\":\"Hello, \\\"world\\\"!\"}}]}",
        "data: {\"choices\":[],\"usage\":{\"prompt_tokens\":42,\"completion_tokens\":18}}",
        "data: [DONE]",
    ];

    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (ev := parser.parseData(frames[i], out_buf)) {
            if (ev.kind == LlmDeltaKind.TextDelta) {
                * = ev.text; // "Hello, \"world\"!"
            }
        }
    }
    const stats = parser.stats();
    * = stats; // done == true, input_tokens == 42, output_tokens == 18
}
```

---

## 子模块：离线确定性回放录像与轨迹 Diff 调试器（`replay.cp`）

### 核心特性

- **零堆分配录像带（`ReplayCassette`）**：基于调用方提供的固定 `[]ReplayFrame` 切片录制 `LlmCall` / `ToolCall` 调用序列（`name`、`args_hash`、`response`、`latency_ms`、`succeeded`），内置 `computeReplayArgsHash`（64-bit FNV-1a）与 `recordRawArgs` 便捷录制接口，满槽安全拦截绝不越界。
- **双模式确定性回放器（`ReplayPlayer`）**：
  - `StrictOrder`：严格校验执行顺序、`kind`、`name` 与 `args_hash`，发生入参漂移或调用失序时精准区分 `arg_mismatches` 与 `misses` 并保留 `cursor` 现场。
  - `KeyedLoose`：允许乱序/并行调度的 Agent 按 `(kind, name, args_hash)` 跨槽命中已录制响应，同时准确识别同名工具的入参漂移。
- **步进轨迹 Diff 对比器（`diffReplayTrajectories`）**：逐帧对比基线与候选轨迹，精准定位首个漂移步号 `first_divergence_step`、分类记录 `KindMismatch | NameMismatch | ArgsMismatch | MissingStep | ExtraStep` 并计算保真度分数 `fidelity_score`（`0.0 ~ 1.0`），`out_diffs` 缓冲不足时安全置位 `diffs_truncated`。

### 导出清单

- **录制与哈希**：`computeReplayArgsHash`、`ReplayEntryKind`（`LlmCall | ToolCall`）、`ReplayMatchMode`（`StrictOrder | KeyedLoose`）、`ReplayFrame`、`ReplayCassette`（`init`, `record`, `recordRawArgs`, `count`, `capacity`, `isFull`, `frameAt`, `frames`, `clear`）
- **离线回放**：`ReplayStats`、`ReplayPlayer`（`init`, `reset`, `stats`, `remaining`, `isExhausted`, `replay`, `replayRawArgs`）
- **轨迹 Diff**：`ReplayDiffKind`、`ReplayStepDiff`、`ReplayDiffSummary`（`isExactMatch`）、`diffReplayTrajectories`

### QuickStart 示例

```cp
import {
    ReplayEntryKind,
    ReplayMatchMode,
    ReplayFrame,
    ReplayCassette,
    ReplayPlayer,
    ReplayStepDiff,
    diffReplayTrajectories,
} from "cap:agent";

fn offlineReplayQuickStart() {
    let rec_buf: [8]ReplayFrame = [ReplayFrame{}; 8];
    let rec_s: []ReplayFrame = rec_buf[0..];
    let cassette = ReplayCassette.init(rec_s);

    * = cassette.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "{\"msg\":\"find bug\"}", "call search", 120, true);
    * = cassette.recordRawArgs(ReplayEntryKind.ToolCall, "search_code", "{\"q\":\"panic\"}", "main.cp:42", 15, true);

    // 1. 脱离线上 LLM 毫秒级严格离线回放
    let player = ReplayPlayer.init(cassette.frames(), ReplayMatchMode.StrictOrder);
    if (f1 := player.replayRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "{\"msg\":\"find bug\"}")) {
        * = f1.response;
    }
    if (f2 := player.replayRawArgs(ReplayEntryKind.ToolCall, "search_code", "{\"q\":\"panic\"}")) {
        * = f2.response;
    }

    // 2. 步进对比新旧轨迹保真度
    let diffs_buf: [8]ReplayStepDiff = [ReplayStepDiff{}; 8];
    let diffs_s: []ReplayStepDiff = diffs_buf[0..];
    const summary = diffReplayTrajectories(cassette.frames(), cassette.frames(), diffs_s);
    * = summary.fidelity_score; // 1.0
}
```

---

## 子模块：SIMD 语义 + 标签混合工具路由器（`tool_router.cp`）

### 核心特性

- **CP 原生 8-lane SIMD `@dot` 语义路由（`SemanticToolRouter`）**：面向大规模 MCP / 函数工具集（50~200+ 工具），注册时自动使用 `@dot` + `@sqrt` 对工具 Embedding 做 L2 归一化并紧凑存入调用方提供的扁平 `vecs_flat` 缓冲，查询时零堆分配计算余弦相似度并降序贪心选出最相关 Top-K 工具，大幅压缩送入 LLM 的 Prompt Schema Token 成本。
- **`pinned` 保底机制 + 64 位标签过滤 + 动态启停**：
  - `pinned = true` 的关键工具无视 `min_score` 阈值优先保底入选（并按语义分降序）；
  - 通过 64 位 `required_tag_mask` 实现 O(1) 权限/领域/环境标签硬过滤；
  - 支持 `setEnabled(tool_id, bool)` 运行时动态启停工具。
- **双重预算精确裁剪**：同时强制执行 `max_tools`（最大工具数）与 `max_schema_tokens`（最大 Schema Token 预算），返回精确的 `selected_count`、`pinned_count`、`used_schema_tokens`、`filtered_out_count` 与 `truncated_by_budget`。

### 导出清单

- **描述符与报告**：`RoutableToolEntry`、`ToolRouteHit`、`ToolRouteReport`
- **语义路由器**：`SemanticToolRouter`（`init`, `registerTool`, `setEnabled`, `routeTools`, `toolCount`, `capacity`, `dimension`, `entryAt`, `clear`）

### QuickStart 示例

```cp
import {
    RoutableToolEntry,
    ToolRouteHit,
    SemanticToolRouter,
} from "cap:agent";

fn semanticToolRouterQuickStart() {
    let entries: [8]RoutableToolEntry = [RoutableToolEntry{}; 8];
    let vecs: [64]f32 = [0.0; 64]; // 8 tools * dim 8
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return;

    const TAG_CODE: u64 = 1;
    const v_search: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const v_deploy: [8]f32 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    * = router.registerTool(1, "search_code", TAG_CODE, 120, false, v_search[0..]);
    * = router.registerTool(2, "deploy_prod", TAG_CODE, 180, false, v_deploy[0..]);

    const query: [8]f32 = [0.95, 0.05, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let hits: [4]ToolRouteHit = [ToolRouteHit{}; 4];
    if (report := router.routeTools(query[0..], TAG_CODE, 4, 250, 0.5, hits[0..])) {
        * = report.selected_count; // 1 ("search_code")
    }
}
```

---

## 子模块：LLM 模型能力目录与成本择优器（`model_catalog.cp`）

### 核心特性

- **五维能力位图与模型规格（`ModelSpec`）**：以 64 位掩码精确建模 `MODEL_CAP_TOOLS`、`MODEL_CAP_VISION`、`MODEL_CAP_THINKING`、`MODEL_CAP_STRUCTURED_JSON`、`MODEL_CAP_PROMPT_CACHE`，同时记录 `context_window`、`max_output_tokens`、每百万 Token 微美元单价（常规输入 / 缓存命中输入 / 输出）及 Batch 折扣基点 `batch_discount_bps`（如 `5000` = 50% 折扣）。
- **防溢出固定点计费（`estimateCallCost`）**：内部全程使用 `u128` 算术并在极值处安全饱和到 `u64` 上限，精准拆分 `regular_input_micro_usd`、`cached_input_micro_usd`、`output_micro_usd`、`cache_savings_micro_usd`、`batch_savings_micro_usd` 与 `total_micro_usd`。
- **零堆最低成本择优（`ModelCatalog.selectCheapestCapable`）**：根据所需能力位图、最小上下文窗口、预估输入/输出 Token 与是否走 Batch 模式，在毫秒内确定性选出单次调用总成本最低的合规模型（同成本优先更大上下文窗口）。

### 导出清单

- **能力常量**：`MODEL_CAP_TOOLS`、`MODEL_CAP_VISION`、`MODEL_CAP_THINKING`、`MODEL_CAP_STRUCTURED_JSON`、`MODEL_CAP_PROMPT_CACHE`
- **规格与目录**：`ModelSpec`（`supports`）、`ModelCostEstimate`、`estimateCallCost`、`ModelCatalog`（`init`, `registerModel`, `findById`, `estimateCallCost`, `selectCheapestCapable`, `count`, `capacity`, `clear`）

### QuickStart 示例

```cp
import {
    MODEL_CAP_TOOLS,
    MODEL_CAP_THINKING,
    MODEL_CAP_PROMPT_CACHE,
    ModelSpec,
    ModelCatalog,
    estimateCallCost,
} from "cap:agent";

fn modelCatalogQuickStart() {
    let slots: [4]ModelSpec = [ModelSpec{}; 4];
    let catalog = ModelCatalog.init(slots[0..]);

    * = catalog.registerModel(ModelSpec{
        model_id: "claude-sonnet",
        provider: "anthropic",
        context_window: 200000,
        max_output_tokens: 16384,
        input_micro_usd_per_mtok: 3000000,
        output_micro_usd_per_mtok: 15000000,
        cached_input_micro_usd_per_mtok: 300000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_THINKING | MODEL_CAP_PROMPT_CACHE,
    });

    if (best := catalog.selectCheapestCapable(MODEL_CAP_TOOLS | MODEL_CAP_THINKING, 128000, 10000, 2000, true)) {
        const est = estimateCallCost(best, 2000, 8000, 2000, true);
        * = est.total_micro_usd;
    }
}
```

---

## 子模块：长程 Agent 会话快照二进制编解码器（`session_codec.cp`）

### 核心特性

- **确定性小端序二进制格式（`AGENT_SNAPSHOT_MAGIC = 0x41475353` / `AGENT_SNAPSHOT_VERSION = 1`）**：64 字节固定小端头部（含 ZigZag 编码有符号 `updated_ms` 时间戳、Token/微美元成本、步数与暂停标志）+ 4 段长度前缀 UTF-8 切片（`session_id` / `agent_name` / `goal_summary` / `last_checkpoint_note`）+ 4 字节 IEEE 802.3 CRC32 尾部校验和。
- **零堆分配零拷贝解码（`decodeSessionSnapshot`）**：解码出的所有字符串字段直接切片自原始输入缓冲，无任何堆分配；严格校验 Magic、版本号、保留标志位、CRC32 完整性、长度边界与尾随垃圾字节，任一异常返回精准 `SnapshotDecodeError`。
- **原子安全编码（`encodeSessionSnapshot`）**：支持 `encodedSnapshotSize` 预计算精确字节数，缓冲不足时直接返回 `null`，绝不半写。

### 导出清单

- **常量与类型**：`AGENT_SNAPSHOT_MAGIC`、`AGENT_SNAPSHOT_VERSION`、`SnapshotDecodeError`（`TruncatedInput | InvalidMagic | UnsupportedVersion | InvalidFlags | ChecksumMismatch | TrailingBytes`）、`AgentSessionSnapshot`
- **编解码函数**：`encodedSnapshotSize`、`encodeSessionSnapshot`、`decodeSessionSnapshot`

### QuickStart 示例

```cp
import {
    AgentSessionSnapshot,
    encodeSessionSnapshot,
    decodeSessionSnapshot,
} from "cap:agent";

fn sessionSnapshotQuickStart()! {
    const snap = AgentSessionSnapshot{
        session_id: "sess-2026-001",
        agent_name: "code-reviewer",
        turn_count: 5,
        tool_call_count: 12,
        input_tokens: 14200,
        output_tokens: 3100,
        cost_micro_usd: 89100,
        updated_ms: 1732250000000,
        is_paused: true,
        goal_summary: "Audit PR #42",
        last_checkpoint_note: "awaiting HITL approval",
    };

    let buf: [256]u8 = [0; 256];
    const written = encodeSessionSnapshot(snap, buf[0..]) ?? return;
    const restored = decodeSessionSnapshot(buf[0..written])?;
    * = restored.turn_count; // 5
}
```

---

## 子模块：层级 Plan-Execute-Replan 任务树执行引擎（`plan_executor.cp`）

### 核心特性

- **100% 零堆分配层级任务树（`PlanExecutor`）**：基于调用方固定 `[]PlanStepNode` 缓冲，支持多级父子层级（`parent_id`）与显式前置依赖（`depends_on_step_id`），子步骤全部完成后自动自底向上将父阶段汇总为 `Succeeded` 并解锁后续阶段。
- **确定性叶子调度与自动重试**：`nextReadyStep()` 仅筛选前置依赖与祖先链均已就绪的可执行叶子步骤，并按升序 `step_id` 确定性返回；`completeStep()` 在失败且 `attempts < max_attempts` 时自动回退 `Pending`/`Ready` 支持瞬态重试。
- **级联跳过与在线重规划（Replan）**：步骤重试耗尽彻底 `Failed` 后，不动点级联将下游依赖节点标记为 `Skipped`；支持通过 `replanAppendStep()` 动态追加补救子步骤，并通过带环检测的 `rebindDependency()` 将被跳过的下游重新挂接到补救路径恢复执行。
- **实时进度遥测（`PlanProgress`）**：精确统计总步数、各状态计数、`remaining_steps`、`completion_ratio`、`replan_count` 及终态判定 `isDone()` / `isAllSucceeded()`。

### 导出清单

- **常量与类型**：`PLAN_NO_PARENT`（`0xFFFF_FFFF`）、`PlanStepState`（`Pending | Ready | Running | Succeeded | Failed | Skipped`）、`PlanStepNode`、`PlanProgress`（`isDone`, `isAllSucceeded`）
- **状态机（`PlanExecutor`）**：`init`、`addStep`、`replanAppendStep`、`rebindDependency`、`nextReadyStep`、`startStep`、`completeStep`、`getStep`、`progress`

### QuickStart 示例

```cp
import {
    PLAN_NO_PARENT,
    PlanStepNode,
    PlanExecutor,
} from "cap:agent";

fn planExecuteReplanQuickStart() {
    let buf: [8]PlanStepNode = [PlanStepNode{}; 8];
    let exec = PlanExecutor.init(buf[0..]) ?? return;

    // 阶段 1（父容器）及其两个叶子子步骤，阶段 2 依赖阶段 1 整体完成
    * = exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Phase 1: Prepare", 1);
    * = exec.addStep(2, 1, PLAN_NO_PARENT, "Fetch schema", 2);
    * = exec.addStep(3, 1, 2, "Validate payload", 1);
    * = exec.addStep(4, PLAN_NO_PARENT, 1, "Phase 2: Deploy", 1);

    // 确定性按叶子依赖推进，失败自动重试（step 2 max_attempts=2）
    if (s := exec.nextReadyStep()) {
        * = exec.startStep(s); // s == 2
        * = exec.completeStep(s, false, "transient timeout"); // attempts=1 < 2 -> 可重试
    }
    if (s := exec.nextReadyStep()) {
        * = exec.startStep(s); // s == 2 (重试)
        * = exec.completeStep(s, true, "ok");
    }
    if (s := exec.nextReadyStep()) {
        * = exec.startStep(s); // s == 3
        * = exec.completeStep(s, true, "validated"); // 自动汇总 step 1 -> Succeeded，解锁 step 4
    }
    if (s := exec.nextReadyStep()) {
        * = exec.startStep(s); // s == 4
        * = exec.completeStep(s, true, "deployed");
    }

    const prog = exec.progress();
    * = prog.isAllSucceeded(); // true
}
```

---

## 子模块：思维树（Tree-of-Thoughts）与多分支搜索管理器（`thought_tree.cp`）

### 核心特性

- **100% 零堆分配固定节点池（`ThoughtTree`）**：基于调用方固定 `[]ThoughtNode` 缓冲实现树状推理扩展、最大深度守卫（`max_depth`）及终态/剪枝节点防误扩展保护。
- **Best-First 前沿选择与非递归 O(N) 子树剪枝**：`selectBestOpenNode()` 在 `Open` 前沿中按最高 `agg_score`（平局按升序 `node_id`）确定性择优；`pruneSubtree()` 利用拓扑创建序在单遍 O(N) 无递归扫描中将整棵子树标记为 `Pruned`。
- **沿父链得分回传与根到叶路径提取**：`backpropagateScore()` 沿父链更新每个祖先节点的 `visit_count` 与滑动平均 `agg_score`；`extractPathToRoot()` 按从根到目标的正序零堆输出完整推理路径。

### 导出清单

- **常量与类型**：`THOUGHT_NO_PARENT`（`0xFFFF_FFFF`）、`ThoughtNodeState`（`Open | Expanded | Pruned | Terminal`）、`ThoughtNode`、`ThoughtTreeStats`
- **树管理器（`ThoughtTree`）**：`init`、`setRoot`、`addChild`、`selectBestOpenNode`、`pruneSubtree`、`backpropagateScore`、`extractPathToRoot`、`getNode`、`stats`

### QuickStart 示例

```cp
import {
    ThoughtNode,
    ThoughtTree,
} from "cap:agent";

fn thoughtTreeQuickStart() {
    let pool: [16]ThoughtNode = [ThoughtNode{}; 16];
    let tree = ThoughtTree.init(pool[0..], 4) ?? return;

    const root = tree.setRoot("Solve 24-game: [4, 7, 8, 8]", 0.5) ?? return;
    const b1 = tree.addChild(root, "Try (7 - 8/8) * 4", 0.9, false) ?? return;
    const b2 = tree.addChild(root, "Try 8 + 8 + 4 + 7", 0.2, false) ?? return;

    // 剪除低分死胡同分支
    * = tree.pruneSubtree(b2);

    // Best-First 选中 b1 并展开出终态解答节点
    if (best := tree.selectBestOpenNode()) {
        if (ans := tree.addChild(best, "6 * 4 = 24 (verified)", 1.0, true)) {
            * = tree.backpropagateScore(ans, 1.0);
            let path: [8]u32 = [0; 8];
            if (plen := tree.extractPathToRoot(ans, path[0..])) {
                * = path[0..plen]; // [root, b1, ans]
            }
        }
    }
    * = b1;
}
```

---

## 子模块：零分配工具权限沙箱与能力策略引擎（`sandbox_policy.cp`）

### 核心特性

- **零堆分配确定性能力策略内核（`ToolSandboxPolicy`）**：内联固定 32 条策略槽位与定长字节缓冲，覆盖 `fs_read` / `fs_write` / `net_egress` / `exec_cmd` / `env_read` / `secret_access` / `mcp_tool` 七大能力域。
- **严格四层安全求值优先级**：
  1. **路径边界硬拦截**：对 `fs_read` / `fs_write` 零分配规范化路径，硬拦截任何 `..` 目录穿越、NUL (`0x00`) 字节与 `allowed_root` 根前缀逃逸（防 `/workspace_evil` 兄弟目录绕过）；
  2. **显式 `deny` 无条件压倒 `allow` / `require_approval` / `audit_only`**；
  3. **高优先级规则胜出**（支持 `*`、`*.domain.com`、`prefix/*`、`cmd *` 通配及域名大小写无关匹配）；
  4. **默认闭拒绝（default-deny）**。
- **人工审批门禁与单会话配额**：`require_approval` 在未获 `human_approved` 前拒绝且不扣配额；支持每条规则独立 `max_calls` 配额扣减、`reset_session_quotas()` 与 `SandboxAuditStats` 零分配审计统计。

### 导出清单

- **常量与枚举**：`MAX_SANDBOX_RULES`（`32`）、`CapabilityKind`、`RiskTier`、`PolicyAction`
- **数据模型**：`SandboxRule`、`SandboxRequest`、`SandboxPolicyDecision`、`SandboxAuditStats`
- **策略引擎（`ToolSandboxPolicy`）**：`init`、`set_allowed_root`、`normalize_path_raw`、`normalize_and_check_path`、`find_rule`、`add_rule`、`evaluate`、`reset_session_quotas`、`stats`

### QuickStart 示例

```cp
import {
    CapabilityKind,
    RiskTier,
    PolicyAction,
    SandboxRequest,
    ToolSandboxPolicy,
} from "cap:agent";

fn sandboxPolicyQuickStart() {
    let policy = ToolSandboxPolicy.init();
    * = policy.set_allowed_root("/workspace");

    * = policy.add_rule("allow-ws-read", CapabilityKind.fs_read, "/workspace/*", RiskTier.low, PolicyAction.allow, 100, 10);
    * = policy.add_rule("deny-secrets", CapabilityKind.fs_read, "/workspace/.env*", RiskTier.critical, PolicyAction.deny, 0, 5);
    * = policy.add_rule("approve-shell", CapabilityKind.exec_cmd, "rm *", RiskTier.critical, PolicyAction.require_approval, 3, 20);

    // 显式 deny 压倒高优先级 allow -> deny
    const d1 = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace/.env.prod",
    });
    // 路径穿越硬拦截 -> path_violation = true
    const d2 = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace/../etc/passwd",
    });
    * = d1;
    * = d2;
}
```

---

## 子模块：OTP 风格零分配 AI Agent 容错监督树内核（`supervisor_tree.cp`）

### 核心特性

- **OTP 三大监督策略（`AgentSupervisorStrategy`）**：
  - `one_for_one`：仅重启发生退出的单个子 Agent；
  - `one_for_all`：按 `start_order` 升序重启全部非 `temporary` 存活子 Agent；
  - `rest_for_one`：按 `start_order` 升序重启自身及所有后继依赖子 Agent（`start_order > target`）。
- **三种子 Agent 重启策略（`AgentRestartPolicy`）**：`permanent`（任何退出均重启）、`transient`（仅非 `normal`/`shutdown` 异常退出重启）、`temporary`（永不重启，退出即置 `terminated`）。
- **滑动窗口重启强度熔断与指数退避**：基于内联 64 槽位时间戳数组维护 `[now_ms - intensity_window_ms, now_ms]` 滑动重启计数，超过 `max_restarts` 立即触发 `escalate_shutdown` 熔断防重启风暴；退避按 `min(base_backoff_ms << min(consecutive-1, 16), max_backoff_ms)` 防溢出计算，健康运行满窗后自动重置连续崩溃计数。

### 导出清单

- **常量与枚举**：`SUPERVISOR_MAX_CHILDREN` (`32`)、`SUPERVISOR_MAX_RESTART_HISTORY` (`64`)、`AgentSupervisorStrategy`、`AgentRestartPolicy`、`AgentExitReason`、`AgentChildState`、`SupervisorActionKind`
- **数据模型**：`AgentSupervisorConfig`（`isValid`）、`SupervisedChildSlot`、`SupervisorDecision`、`SupervisorStats`
- **监督树内核（`AgentSupervisorTree`）**：`init`、`register_child`、`mark_started`、`report_exit`、`get_child`、`stats`

### QuickStart 示例

```cp
import {
    AgentSupervisorStrategy,
    AgentRestartPolicy,
    AgentExitReason,
    AgentSupervisorConfig,
    AgentSupervisorTree,
} from "cap:agent";

fn supervisorTreeQuickStart() {
    let sup = AgentSupervisorTree.init(AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.rest_for_one,
        max_restarts: 3,
        intensity_window_ms: 5000,
        base_backoff_ms: 100,
        max_backoff_ms: 4000,
    }) ?? return;

    * = sup.register_child(10, AgentRestartPolicy.permanent, 1); // Router Agent
    * = sup.register_child(20, AgentRestartPolicy.permanent, 2); // Planner Agent
    * = sup.register_child(30, AgentRestartPolicy.transient, 3); // Worker Agent
    * = sup.mark_started(10, 1000);
    * = sup.mark_started(20, 1000);
    * = sup.mark_started(30, 1000);

    // Planner (order=2) 崩溃 -> rest_for_one 按 [20, 30] 顺序级联重启，Router (10) 不受影响
    const decision = sup.report_exit(20, AgentExitReason.crash_error, 1500);
    * = decision.restart_count; // 2 (20, 30)
}
```

---

## 子模块：零分配 AI Agent 工具调用批合并、去重与读写冒险波次调度器（`tool_batcher.cp`）

### 核心特性

- **同波次只读自动合并去重（Read Coalescing）**：相同 `tool_hash` + `args_hash` + `resource_hash` 的 `pure_read` 调用（且中间未穿插对该资源的写或 `global_barrier`）自动合并至同波次主调用（`is_coalesced = true`, `coalesced_into_idx = primary_idx`），**零并发槽占用、零冗余执行**。
- **程序序 RAW / WAR / WAW 读写冒险防护**：同资源只读-只读可全并行；同资源写-读（RAW）、读-写（WAR）、写-写（WAW）严格推入后继波次，且写后读绝不与写前读错误合并。
- **全局屏障独占波次（`global_barrier`）**：Shell 命令或全局状态变更自动独占独立波次（前后均设屏障）。
- **并发限流拆波与关键路径加速比**：支持 `max_parallel_per_wave`（`0` 为不限）按波次限制最大并发执行数，自动计算串行耗时、关键路径并行耗时与 `speedup_x100`。

### 导出清单

- **常量与枚举**：`TOOL_BATCH_MAX_CALLS` (`64`)、`ToolAccessKind`（`pure_read | resource_write | global_barrier`）
- **数据模型**：`ToolBatchItem`、`ToolBatchScheduleEntry`、`ToolBatchPlanSummary`
- **规划器**：`planToolCallBatch(items: []const ToolBatchItem, max_parallel_per_wave: usize, out_schedule: *[]ToolBatchScheduleEntry): ToolBatchPlanSummary`

### QuickStart 示例

```cp
import {
    ToolAccessKind,
    ToolBatchItem,
    ToolBatchScheduleEntry,
    planToolCallBatch,
} from "cap:agent";

fn toolBatcherQuickStart() {
    const calls: [4]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 10, args_hash: 100, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 50 },
        ToolBatchItem{ call_id: 2, tool_hash: 10, args_hash: 100, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 50 }, // 合并到 call 0
        ToolBatchItem{ call_id: 3, tool_hash: 20, args_hash: 200, resource_hash: 1, access: ToolAccessKind.resource_write, estimated_ms: 80 }, // WAR -> wave 1
        ToolBatchItem{ call_id: 4, tool_hash: 10, args_hash: 100, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 50 }, // RAW -> wave 2 (不与 call 0 合并)
    ];
    let sched_buf: [4]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 4];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const summary = planToolCallBatch(calls[0..], 4, sched);
    // unique_executions == 3, coalesced_duplicates == 1, wave_count == 3
    * = summary;
}
```
