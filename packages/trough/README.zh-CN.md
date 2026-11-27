[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `trough`

> **源包溯源**：`npm:trough` `2.2.0` ([wooorm/trough](https://github.com/wooorm/trough)) — 协议：MIT

纯 CP 实现的零堆分配中间件流水线引擎（`TroughPipeline`、`trough`、`wrapStage`），支持执行报告、错误恢复回调、流水线克隆/拼接，以及 `zwitch` 风格的标签路由（`dispatchTag`）。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `trough`，无需改名：

```typescript
// TypeScript / Node.js (npm: trough)
import { trough } from "trough";
const pipeline = trough().use((n) => n + 1).use((n) => n * 2);
pipeline.run(5, (err, value) => console.log(value)); // 12
```

```cp
// CP（零堆分配，固定容量流水线）
import { trough, stageOk, StageOutcome } from "trough";

fn addOne(v: i64): StageOutcome { return stageOk(v + 1); }
fn doubleVal(v: i64): StageOutcome { return stageOk(v * 2); }

let p = trough();
* = p.use(addOne);
* = p.use(doubleVal);
let res = p.run(5); // res.ok == true, res.value == 12
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "trough": "2.2.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `StageOutcome` | `class { ok: bool, value: i64, err_code: i32, halted: bool }` | 单阶段执行结果 |
| `RunReport` | `class { outcome: StageOutcome, steps_executed: usize, stopped_index: i32 }` | 执行追踪报告（全部跑完时 `stopped_index == -1`） |
| `TagRoute` | `class { tag: string, handler_id: i32 }` | 标签路由项（支持 `"*"` 通配兜底） |
| `TroughPipeline` | `class`（容量 32 阶段） | 方法：`new()`、`use(stage)`、`run(initial)`、`runWithReport(initial)`、`runWithErrorHandler(initial, on_error)`、`len()`、`clear()`、`isEmpty()`、`isFull()`、`clone()`、`extend(other)` |
| `trough` | `(): TroughPipeline` | 工厂函数，创建空的 `TroughPipeline` |
| `stageOk` / `stageErr` / `stageHalt` | `(value: i64)` / `(err_code: i32)` / `(value: i64): StageOutcome` | 构造继续、报错或提前中止的结果 |
| `wrapStage` | `(stage: StageFn, value: i64): StageOutcome` | 单阶段直接调用（对齐 `trough.wrap`） |
| `dispatchTag` | `(tag: string, routes: &[]TagRoute): i32` | 按标签分发处理 ID，支持 `"*"` 兜底，未命中返回 `-1` |

## 实战示例

### 1. 流水线组合、执行报告与错误恢复

```cp
import { trough, stageOk, stageErr, stageHalt, StageOutcome } from "trough";

fn validatePositive(v: i64): StageOutcome {
    if (v < 0) { return stageErr(400); }
    return stageOk(v);
}

fn shortCircuitZero(v: i64): StageOutcome {
    if (v == 0) { return stageHalt(0); }
    return stageOk(v * 10);
}

fn recover(err_code: i32, prev_val: i64): StageOutcome {
    return stageOk(0);
}

let p1 = trough();
* = p1.use(validatePositive);

let p2 = trough();
* = p2.use(shortCircuitZero);

let combined = p1.clone();
* = combined.extend(p2);

let report = combined.runWithReport(0);
// report.outcome.halted == true, report.steps_executed == 2, report.stopped_index == 1

let recovered = combined.runWithErrorHandler(-5, recover);
// recovered.ok == true, recovered.value == 0
```

### 2. 标签路由分发（兼容 `zwitch`）

```cp
import { TagRoute, dispatchTag } from "trough";

let routes: [3]TagRoute = [
    TagRoute { tag: "heading", handler_id: 1 },
    TagRoute { tag: "paragraph", handler_id: 2 },
    TagRoute { tag: "*", handler_id: 99 },
];

let h1 = dispatchTag("heading", &routes[0..3]); // 1
let h2 = dispatchTag("code", &routes[0..3]);    // 99（命中通配兜底）
```

## 注意事项与边界说明

- **固定容量上限**：`TroughPipeline` 最多注册 32 个阶段；超出容量时 `use` 与 `extend` 返回 `false` 且不修改原流水线。
- **短路机制**：遇到 `stageErr(code)` 或 `stageHalt(val)` 时立即终止后续阶段执行。
- **零堆分配**：流水线为栈上值对象，`clone()` 直接按值拷贝，无堆内存开销。
