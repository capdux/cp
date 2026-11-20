# `cap:fsm` — 状态机、熔断器与 AI Agent 工作流 DAG 调度引擎

> **模块路径**: `lib/fsm/`  
> **导入协议**: `import { StateMachine, CircuitBreaker, WorkflowDag, ... } from "cap:fsm";`  
> **动态挂载**: `import cap from "cap";` -> `cap.fsm.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:fsm` 是专为 **多 Agent 编排、容错调用与有向无环图（DAG）并行波次调度** 打造的纯 CP 零外部依赖核心库，由三大正交组件构成：

1. **确定性有限状态机 (`StateMachine`)**：支持声明式状态转移表、前置合法性探测（`canTrigger`）、非法转移精确报错（`FsmError.InvalidTransition`）、全量转移审计历史（`history()`）及标准 Mermaid `stateDiagram-v2` 可视化导出（`toMermaid()`）。
2. **弹性熔断器 (`CircuitBreaker`)**：完整实现 `Closed` / `Open` / `HalfOpen` 三态自动流转，支持外部注入 `now_ms` 实现 100% 确定性单测、半开探针并发限流（`half_open_max_probes`）、探针失败立即重开、连续成功阈值恢复（`success_threshold_half_open`）及全量统计快照（`CircuitStats`）。
3. **工作流 DAG 并行调度器 (`WorkflowDag`)**：基于 Kahn 分层拓扑排序生成可并行执行的波次列表（`executionWaves`），自动拦截环路（`DagError.CycleDetected` / `hasCycle`），并提供动态就绪节点查询（`readyNodes`）与上游失败下游自动级联跳过（`Skipped`）传播。

---

## 二、 核心 API 速查表

### 1. 有限状态机 (`lib/fsm/state_machine.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `FsmError` | `InvalidTransition`, `DuplicateTransition`, `EmptyState`, `EmptyEvent`, `OutOfMemory` |
| `TransitionRule` | `{ from_state, event, to_state }` |
| `TransitionRecord` | `{ from_state, event, to_state, step }` |
| `StateMachine.init(alloc, initial_state)` | 初始化状态机（默认持有初始状态副本） |
| `addTransition(*_, from_state, event, to_state)` | 注册确定性转移规则（重复 `(from_state, event)` 返回 `DuplicateTransition`） |
| `canTrigger(event)` | 只读检查当前状态是否允许触发 `event` |
| `trigger(*_, event)` | 执行转移并记录审计日志；非法转移返回 `FsmError.InvalidTransition` |
| `currentState()` / `history()` / `reset(*_)` | 查询当前状态、审计历史，或重置回初始状态并清空历史 |
| `toMermaid(alloc)` | 导出标准 `stateDiagram-v2` 文本（调用方负责 `alloc.free`） |
| `deinit(*_)` | 释放规则表、审计历史与内部字符串池 |

### 2. 弹性熔断器 (`lib/fsm/circuit_breaker.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `CircuitState` | `Closed`, `Open`, `HalfOpen` |
| `CircuitBreakerConfig` | `failure_threshold`, `success_threshold_half_open`, `open_timeout_ms`, `half_open_max_probes` |
| `CircuitStats` | 运行态快照（当前状态、连续成功/失败计数、总放行/拒绝/熔断次数等） |
| `CircuitBreaker.init(config)` / `initDefault()` | 初始化熔断器实例 |
| `allowRequest(*_, now_ms)` | 判断是否允许放行请求（自动评估 `Open` -> `HalfOpen` 冷却超时并控制探针配额） |
| `recordSuccess(*_, now_ms)` / `recordFailure(*_, now_ms)` | 记录调用结果并驱动 `Closed <-> Open <-> HalfOpen` 状态流转 |
| `state(*_, now_ms)` / `currentState()` | 评估或只读获取当前熔断器状态 |
| `forceOpen(*_, now_ms)` / `reset(*_)` / `stats()` | 手动强开熔断、复位及获取统计快照 |

### 3. 工作流 DAG 调度器 (`lib/fsm/dag.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `DagError` | `CycleDetected`, `DuplicateNode`, `NodeNotFound`, `EmptyNodeId`, `InvalidStateTransition`, `OutOfMemory` |
| `TaskStatus` | `Pending`, `Running`, `Succeeded`, `Failed`, `Skipped` |
| `WorkflowDag.init(alloc)` / `deinit(*_)` | 创建与销毁 DAG 调度器 |
| `addNode(*_, id)` / `addEdge(*_, dep_id, task_id)` | 注册任务节点及依赖边（`dep_id` 成功后方可执行 `task_id`） |
| `hasCycle()` | O(V+E) Kahn 算法检测是否存在环路 |
| `executionWaves(alloc)` / `freeWaves(alloc, waves)` | 分层拓扑排序返回并行波次；若存在环路返回 `DagError.CycleDetected` |
| `readyNodes(alloc)` | 返回所有前置依赖均已 `Succeeded` 的 `Pending` 任务 ID 列表 |
| `markRunning(*_, id)` / `markSucceeded(*_, id)` | 推进任务状态并校验依赖前置条件 |
| `markFailed(*_, id)` | 标记节点为 `Failed`，并沿下游依赖 BFS 自动将 `Pending` 节点级联标为 `Skipped` |
| `isComplete()` / `nodeStatus(id)` / `reset(*_)` | 检查是否全部进入终态、查询节点状态或重置为全 `Pending` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";

fn runAgentPipeline(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 工作流 DAG 分层波次规划
    let dag = cap.fsm.WorkflowDag.init(alloc);
    defer dag.deinit();
    dag.addNode("fetch")?;
    dag.addNode("analyze")?;
    dag.addEdge("fetch", "analyze")?;

    const waves = dag.executionWaves(alloc)?;
    defer cap.fsm.WorkflowDag.freeWaves(alloc, waves);

    // 2. 熔断保护 Tool 调用
    let cb = cap.fsm.CircuitBreaker.init(cap.fsm.CircuitBreakerConfig{
        failure_threshold: 3,
        success_threshold_half_open: 1,
        open_timeout_ms: 5000,
        half_open_max_probes: 1,
    });
    if (cb.allowRequest(1000)) {
        cb.recordSuccess(1010);
    }
}
```
