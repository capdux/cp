// Type definitions for CP

export class MittListener {
    active: bool,
    event_type: string,
    handler_id: u32,
    is_wildcard: bool,
    fn empty(): MittListener;
}

export class MittDispatch {
    handler_id: u32,
    payload: i64,
    is_wildcard: bool,
    fn empty(): MittDispatch;
}

export fn mitStrEq(a: string, b: string): bool;
export fn mitIsWildcard(event_type: string): bool;
export class Emitter {
    listeners: [32]MittListener,
    count: usize,
    last_dispatched: [32]MittDispatch,
    last_dispatch_count: usize,
    emit_count: usize,
    fn new(): Emitter;
    fn on(_: auto, event_type: string, handler_id: u32): bool;
    fn off(_: auto, event_type: string, handler_id: u32): bool;
    fn emit(_: auto, event_type: string, payload: i64): usize;
    fn listenerCount(event_type: string): usize;
    fn totalListeners(): usize;
    fn clear(_: auto);
    fn hasListener(event_type: string, handler_id: u32): bool;
}

export fn mitt(): Emitter;
