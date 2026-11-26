// Type definitions for CP

export fn ptoMaxU64(): u64;
export fn ptoSaturatingAdd(a: u64, b: u64): u64;
export class TimeoutOptions {
    milliseconds: u64,
    has_fallback: bool,
    fallback_code: i32,
    error_message: string,
    fn withMs(ms: u64): TimeoutOptions;
}

export class TimeoutGuard {
    start_ms: u64,
    deadline_ms: u64,
    opts: TimeoutOptions,
    state: u8,
    result_code: i32,
    used_fallback: bool,
    fn new(start_ms: u64, opts: TimeoutOptions): TimeoutGuard;
    fn deadlineMs(): u64;
    fn remainingMs(now_ms: u64): u64;
    fn isExpired(now_ms: u64): bool;
    fn poll(_: auto, now_ms: u64): u8;
    fn resolve(_: auto, now_ms: u64, value_code: i32): bool;
    fn reject(_: auto, now_ms: u64, err_code: i32): bool;
    fn cancel(_: auto): bool;
    fn isSettled(): bool;
}

