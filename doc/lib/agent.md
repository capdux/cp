# `cap:agent` — AI Agent 核心运行时标准库

`cap:agent` 是 CP 标准库面向生产级 AI Agent 运行时的统一聚合主包，100% 纯 CP 零堆分配实现，当前包含执行预算/护栏子模块（`budget.cp`）与多智能体任务图 DAG 调度器子模块（`dag.cp`）。

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
