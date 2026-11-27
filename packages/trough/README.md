[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `trough`

> **Source Attribution**: `npm:trough` `2.2.0` ([wooorm/trough](https://github.com/wooorm/trough)) — License: MIT

Zero-heap middleware pipeline engine (`TroughPipeline`, `trough`, `wrapStage`) with execution reporting, error handlers, pipeline cloning/composition, and `zwitch`-style tag routing (`dispatchTag`) in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`trough`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: trough)
import { trough } from "trough";
const pipeline = trough().use((n) => n + 1).use((n) => n * 2);
pipeline.run(5, (err, value) => console.log(value)); // 12
```

```cp
// CP (zero-heap fixed-capacity pipeline)
import { trough, stageOk, StageOutcome } from "trough";

fn addOne(v: i64): StageOutcome { return stageOk(v + 1); }
fn doubleVal(v: i64): StageOutcome { return stageOk(v * 2); }

let p = trough();
* = p.use(addOne);
* = p.use(doubleVal);
let res = p.run(5); // res.ok == true, res.value == 12
```

## Installation

Add `trough` to your `cap.json`:

```json
{
  "dependencies": {
    "trough": "2.2.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `StageOutcome` | `class { ok: bool, value: i64, err_code: i32, halted: bool }` | Result of executing a middleware stage |
| `RunReport` | `class { outcome: StageOutcome, steps_executed: usize, stopped_index: i32 }` | Execution trace (`stopped_index == -1` if all stages ran) |
| `TagRoute` | `class { tag: string, handler_id: i32 }` | Route entry for `dispatchTag` (supports `"*"` wildcard fallback) |
| `TroughPipeline` | `class` (capacity 32 stages) | Methods: `new()`, `use(stage)`, `run(initial)`, `runWithReport(initial)`, `runWithErrorHandler(initial, on_error)`, `len()`, `clear()`, `isEmpty()`, `isFull()`, `clone()`, `extend(other)` |
| `trough` | `(): TroughPipeline` | Factory function creating an empty `TroughPipeline` |
| `stageOk` / `stageErr` / `stageHalt` | `(value: i64)` / `(err_code: i32)` / `(value: i64): StageOutcome` | Outcome constructors (continue, fail, or short-circuit halt) |
| `wrapStage` | `(stage: StageFn, value: i64): StageOutcome` | Invoke a single stage directly (`trough.wrap` parity) |
| `dispatchTag` | `(tag: string, routes: &[]TagRoute): i32` | Match `tag` against `routes` with `"*"` wildcard fallback (`-1` if unmatched) |

## Practical Examples

### 1. Pipeline Composition, Reporting & Error Handling

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

### 2. Tag Routing (`zwitch` Parity)

```cp
import { TagRoute, dispatchTag } from "trough";

let routes: [3]TagRoute = [
    TagRoute { tag: "heading", handler_id: 1 },
    TagRoute { tag: "paragraph", handler_id: 2 },
    TagRoute { tag: "*", handler_id: 99 },
];

let h1 = dispatchTag("heading", &routes[0..3]); // 1
let h2 = dispatchTag("code", &routes[0..3]);    // 99 (wildcard fallback)
```

## Notes & Edge Cases

- **Fixed capacity**: `TroughPipeline` holds up to 32 stages (`use` and `extend` return `false` when capacity would be exceeded).
- **Short-circuiting**: Both `stageErr(code)` (`ok: false`) and `stageHalt(val)` (`halted: true`) stop remaining stages immediately.
- **Zero heap allocations**: Pipelines are plain value structs (`clone()` copies stages by value).
