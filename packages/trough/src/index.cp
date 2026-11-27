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
