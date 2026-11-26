// Type definitions for CP

export class OnceCallResult {
    status: i32,
    executed_now: bool,
    value: i64,
    call_count: u64,
}

export class OnceGuard {
    called: bool,
    strict: bool,
    call_count: u64,
    cached_value: i64,
    fn new(): OnceGuard;
    fn newStrict(): OnceGuard;
    fn isCalled(): bool;
    fn callCount(): u64;
    fn cachedValue(): i64;
    fn runWithValue(_: auto, val: i64): OnceCallResult;
    fn shouldRun(_: auto): bool;
    fn reset(_: auto);
}

export class OnceRegistry {
    keys: [64]u64,
    values: [64]i64,
    counts: [64]u64,
    len: usize,
    fn new(): OnceRegistry;
    fn size(): usize;
    fn has(key: u64): bool;
    fn callCountOf(key: u64): u64;
    fn runForKey(_: auto, key: u64, val: i64): OnceCallResult;
}

