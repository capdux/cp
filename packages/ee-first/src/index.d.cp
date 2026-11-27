// Type definitions for CP

export fn eefStrEq(a: string, b: string): bool;
export class EventWatch {
    emitter_id: u32,
    event_name: string,
    active: bool,
}

export class FirstResult {
    triggered: bool,
    emitter_id: u32,
    event_name: string,
    payload_code: i32,
    cancelled_count: usize,
    fn empty(): FirstResult;
}

export class FirstRace {
    watchers: [16]EventWatch,
    count: usize,
    settled: bool,
    result: FirstResult,
    fn new(): FirstRace;
    fn add(_: auto, emitter_id: u32, event_name: string): bool;
    fn emit(_: auto, emitter_id: u32, event_name: string, payload_code: i32): FirstResult;
    fn cancel(_: auto): usize;
    fn activeCount(): usize;
    fn isSettled(): bool;
    fn winner(): FirstResult;
}

