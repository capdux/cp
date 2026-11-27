// @cap/trough — Zero-heap middleware pipeline & zwitch tag router (clean-room CP)

pub const TROUGH_CAPACITY: usize = 32;

pub struct StageOutcome {
    ok: bool,
    value: i64,
    err_code: i32,
    halted: bool,
}

pub type StageFn = *const fn (i64) StageOutcome;

pub struct TagRoute {
    tag: string,
    handler_id: i32,
}

fn trghStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trghIdentity(v: i64): StageOutcome {
    return StageOutcome{
        ok: true,
        value: v,
        err_code: 0,
        halted: false,
    };
}

pub fn stageOk(value: i64): StageOutcome {
    return StageOutcome{
        ok: true,
        value: value,
        err_code: 0,
        halted: false,
    };
}

pub fn stageErr(err_code: i32): StageOutcome {
    return StageOutcome{
        ok: false,
        value: 0,
        err_code: err_code,
        halted: false,
    };
}

pub fn stageHalt(value: i64): StageOutcome {
    return StageOutcome{
        ok: true,
        value: value,
        err_code: 0,
        halted: true,
    };
}

pub struct TroughPipeline {
    stages: [32]StageFn,
    count: usize,

    static fn new(): TroughPipeline {
        let arr: [32]StageFn = [trghIdentity; 32];
        return TroughPipeline{
            stages: arr,
            count: 0,
        };
    }

    fn use(*_, stage: StageFn): bool {
        if (_.count >= TROUGH_CAPACITY) {
            return false;
        }
        _.stages[_.count] = stage;
        _.count += 1;
        return true;
    }

    fn run(initial: i64): StageOutcome {
        let cur = stageOk(initial);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const f = _.stages[i];
            cur = f(cur.value);
            if (!cur.ok || cur.halted) {
                return cur;
            }
        }
        return cur;
    }

    fn len(): usize {
        return _.count;
    }

    fn clear(*_) {
        _.count = 0;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn isFull(): bool {
        return _.count >= TROUGH_CAPACITY;
    }

    fn clone(): TroughPipeline {
        let copy = TroughPipeline.new();
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            copy.stages[i] = _.stages[i];
        }
        copy.count = _.count;
        return copy;
    }

    fn extend(*_, other: TroughPipeline): bool {
        if (_.count + other.count > TROUGH_CAPACITY) {
            return false;
        }
        let i: usize = 0;
        while (i < other.count) : (i += 1) {
            _.stages[_.count] = other.stages[i];
            _.count += 1;
        }
        return true;
    }

    fn runWithReport(initial: i64): RunReport {
        let cur = stageOk(initial);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const f = _.stages[i];
            cur = f(cur.value);
            if (!cur.ok || cur.halted) {
                return RunReport {
                    outcome: cur,
                    steps_executed: i + 1,
                    stopped_index: i as i32,
                };
            }
        }
        return RunReport {
            outcome: cur,
            steps_executed: _.count,
            stopped_index: -1,
        };
    }

    fn runWithErrorHandler(initial: i64, on_error: ErrorHandlerFn): StageOutcome {
        let cur = stageOk(initial);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const f = _.stages[i];
            const prev_val = cur.value;
            cur = f(prev_val);
            if (!cur.ok) {
                return on_error(cur.err_code, prev_val);
            }
            if (cur.halted) {
                return cur;
            }
        }
        return cur;
    }
}

pub type ErrorHandlerFn = *const fn (i32, i64) StageOutcome;

pub struct RunReport {
    pub outcome: StageOutcome,
    pub steps_executed: usize,
    pub stopped_index: i32,
}

/// Factory function matching npm `trough()`: creates a new empty `TroughPipeline`.
pub fn trough(): TroughPipeline {
    return TroughPipeline.new();
}

/// Invokes a single middleware stage with `value`, matching npm `trough.wrap` semantics.
pub fn wrapStage(stage: StageFn, value: i64): StageOutcome {
    return stage(value);
}

pub fn dispatchTag(tag: string, routes: []const TagRoute): i32 {
    let fallback: i32 = -1;
    let i: usize = 0;
    while (i < routes.len) : (i += 1) {
        if (trghStrEq(routes[i].tag, tag)) {
            return routes[i].handler_id;
        }
        if (trghStrEq(routes[i].tag, "*")) {
            fallback = routes[i].handler_id;
        }
    }
    return fallback;
}
