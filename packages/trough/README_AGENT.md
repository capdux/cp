# `trough` — AI Agent Quick Reference

- **Package**: `trough` (`2.2.0`, MIT)
- **Origin**: `npm:trough` (plus `zwitch` tag routing)
- **Import**: `import { trough, TroughPipeline, StageOutcome, RunReport, TagRoute, stageOk, stageErr, stageHalt, wrapStage, dispatchTag } from "trough";`

## Types

```cp
class StageOutcome { ok: bool, value: i64, err_code: i32, halted: bool }
class RunReport { outcome: StageOutcome, steps_executed: usize, stopped_index: i32 }
class TagRoute { tag: string, handler_id: i32 }
// StageFn = *const fn (i64) StageOutcome
// ErrorHandlerFn = *const fn (i32, i64) StageOutcome
```

## Core Signatures

```cp
fn trough(): TroughPipeline
fn stageOk(value: i64): StageOutcome
fn stageErr(err_code: i32): StageOutcome
fn stageHalt(value: i64): StageOutcome
fn wrapStage(stage: StageFn, value: i64): StageOutcome
fn dispatchTag(tag: string, routes: &[]TagRoute): i32

class TroughPipeline {
    fn new(): TroughPipeline
    fn use(_: auto, stage: StageFn): bool
    fn run(initial: i64): StageOutcome
    fn runWithReport(initial: i64): RunReport
    fn runWithErrorHandler(initial: i64, on_error: ErrorHandlerFn): StageOutcome
    fn len(): usize
    fn clear(_: auto)
    fn isEmpty(): bool
    fn isFull(): bool
    fn clone(): TroughPipeline
    fn extend(_: auto, other: TroughPipeline): bool
}
```

## Minimal Pattern

```cp
let p = trough();
* = p.use(myStage);
let report = p.runWithReport(10);
if (report.outcome.ok) { /* use report.outcome.value */ }
```

## Pitfalls

1. **Capacity limit (32)**: `use()` and `extend()` return `false` if total stages would exceed 32; always check or discard return with `* =`.
2. **Error vs Halt**: `stageErr(code)` triggers `on_error` in `runWithErrorHandler`; `stageHalt(val)` stops early with `ok: true, halted: true` and bypasses `on_error`.
3. **`stopped_index` sentinel**: `RunReport.stopped_index` is `-1` when all stages complete without error or halt.
4. **Wildcard route**: `dispatchTag` returns `-1` when no route matches and no `"*"` route is present.
