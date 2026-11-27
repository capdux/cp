// Type definitions for CP

export class StageOutcome {
    ok: bool,
    value: i64,
    err_code: i32,
    halted: bool,
}

export class TagRoute {
    tag: string,
    handler_id: i32,
}

export fn trghStrEq(a: string, b: string): bool;
export fn trghIdentity(v: i64): StageOutcome;
export fn stageOk(value: i64): StageOutcome;
export fn stageErr(err_code: i32): StageOutcome;
export fn stageHalt(value: i64): StageOutcome;
export class TroughPipeline {
    stages: [32]StageFn,
    count: usize,
    fn new(): TroughPipeline;
    fn use(_: auto, stage: StageFn): bool;
    fn run(initial: i64): StageOutcome;
    fn len(): usize;
    fn clear(_: auto);
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clone(): TroughPipeline;
    fn extend(_: auto, other: TroughPipeline): bool;
    fn runWithReport(initial: i64): RunReport;
    fn runWithErrorHandler(initial: i64, on_error: ErrorHandlerFn): StageOutcome;
}

export class RunReport {
    outcome: StageOutcome,
    steps_executed: usize,
    stopped_index: i32,
}

export fn trough(): TroughPipeline;
export fn wrapStage(stage: StageFn, value: i64): StageOutcome;
export fn dispatchTag(tag: string, routes: &[]TagRoute): i32;
