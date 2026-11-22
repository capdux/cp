# `cap:agent` — AI Agent 核心运行时标准库

`cap:agent` 是 CP 标准库面向生产级 AI Agent 运行时的统一聚合主包，100% 纯 CP 零堆分配实现，当前包含执行预算、死循环检测、工具熔断与上下文窗口压缩策略子模块（`budget.cp`）。

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
