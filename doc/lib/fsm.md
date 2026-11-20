# `cap:fsm` — 确定性有限状态机、守卫、拓扑分析与熔断/DAG 引擎

> **模块路径**: `lib/fsm/`  
> **导入协议**: `import { StateMachine, FsmError, TransitionRecord, GuardFn, ActionFn, ... } from "cap:fsm";`  
> **动态挂载**: `import cap from "cap";` -> `cap.fsm.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:fsm` 是专为 **复杂业务工作流、状态驱动编排与容错调度** 打造的纯 CP 零外部依赖核心库，包含：

1. **确定性有限状态机 (`lib/fsm/fsm.cp` -> `StateMachine`)**：
   - 支持显式状态与终态注册（`addState` / `addTerminalState` / `isTerminal`）；
   - 支持带守卫条件（`GuardFn`）的确定性状态转移（`addTransition` / `addGuardedTransition` / `canTrigger` / `trigger`）；
   - 支持生命周期动作钩子（`onExit` -> `onTransition` -> `onEnter`）；
   - 全量审计轨迹 `history()`（`TransitionRecord { from, event, to, accepted }`），守卫拒绝时记录 `accepted: false` 且保持原状态；
   - 基于 BFS 的图拓扑分析（`reachableStates` / `findDeadEndStates`）自动发现可达状态与非终态死胡同；
   - 标准 GraphViz `digraph` 导出（`toDot`，含初始态入口箭头、终态 `doublecircle` 与事件边标签）。
2. **弹性熔断器 (`lib/fsm/circuit_breaker.cp` -> `CircuitBreaker`)**：`Closed` / `Open` / `HalfOpen` 三态自动流转与探针限流。
3. **工作流 DAG 并行调度器 (`lib/fsm/dag.cp` -> `WorkflowDag`)**：Kahn 分层拓扑排序与失败级联跳过传播。

---

## 二、 核心 API 速查表 (`lib/fsm/fsm.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `FsmError` | `UnknownState`, `InvalidTransition`, `GuardRejected`, `DuplicateTransition`, `EmptyMachine`, `OutOfMemory` |
| `GuardFn` | `*const fn ((*anyopaque)?, []const u8, []const u8, []const u8) bool` |
| `ActionFn` | `*const fn ((*anyopaque)?, []const u8, []const u8, []const u8) void` |
| `TransitionRecord` | `{ from: []const u8, event: []const u8, to: []const u8, accepted: bool }` |
| `StateMachine.init(alloc, initial_state)` | 初始化状态机并自动注册 `initial_state` 为非终态 |
| `deinit(*_)` | 释放所有内部字符串池、状态表、规则表与历史记录 |
| `addState(*_, name)` | 注册普通状态（若未注册） |
| `addTerminalState(*_, name)` | 注册或标记终态 |
| `onEnter(*_, state, action)` / `onExit(*_, state, action)` | 注册指定状态的进入/退出生命周期回调（未注册状态返回 `UnknownState`） |
| `onTransition(*_, action)` | 注册全局成功转移回调 |
| `addTransition(*_, from, event, to)` | 注册无条件转移规则（未知状态返回 `UnknownState`，重复 `(from, event)` 返回 `DuplicateTransition`） |
| `addGuardedTransition(*_, from, event, to, guard)` | 注册带守卫谓词 `GuardFn?` 的转移规则 |
| `canTrigger(event, ctx)` | 只读检查当前状态是否允许触发 `event`（含守卫求值） |
| `trigger(*_, event, ctx)` | 执行状态转移并触发 `onExit` -> `onTransition` -> `onEnter`；非法转移返回 `InvalidTransition`，守卫拒绝返回 `GuardRejected` |
| `currentState()` / `initialState()` / `isTerminal()` | 查询当前状态、初始状态及当前是否处于终态 |
| `availableEvents(alloc)` | 返回当前状态下所有已注册的可触发事件列表（调用方负责 `alloc.free`） |
| `history()` / `reset(*_)` | 获取全量转移审计轨迹，或复位到初始状态并清空历史 |
| `reachableStates(alloc)` | 从初始状态执行 BFS，返回所有可达状态列表（调用方负责 `alloc.free`） |
| `findDeadEndStates(alloc)` | 从初始状态执行 BFS，返回所有无出边且非终态的死胡同状态（调用方负责 `alloc.free`） |
| `toDot(alloc)` | 导出标准 GraphViz `digraph fsm` 源码（调用方负责 `alloc.free`） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { StateMachine, FsmError } from "cap:fsm";

fn isPaid(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8): bool {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        const paid_ptr: *const bool = @ptrCast(@alignCast(p));
        return paid_ptr.*;
    }
    return false;
}

fn runOrderFsm(): !void {
    const alloc = std.heap.page_allocator;
    let sm = StateMachine.init(alloc, "created");
    defer sm.deinit();

    sm.addState("paid")?;
    sm.addTerminalState("shipped")?;
    sm.addGuardedTransition("created", "pay", "paid", isPaid)?;
    sm.addTransition("paid", "ship", "shipped")?;

    let paid = true;
    const next = sm.trigger("pay", &paid)?;
    _ = next;
    _ = sm.trigger("ship", null)?;
}
```
