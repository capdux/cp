// Type definitions for CP

export fn elapsedSince(now_ms: u64, past_ms: u64): u64;
export class Debouncer {
    wait_ms: u64,
    leading: bool,
    trailing: bool,
    max_wait_ms: u64,
    active: bool,
    pending: bool,
    last_call_ms: u64,
    last_exec_ms: u64,
    exec_count: u64,
    fn init(wait_ms: u64, leading: bool, trailing: bool, max_wait_ms: u64): Debouncer;
    fn trigger(_: auto, now_ms: u64): bool;
    fn poll(_: auto, now_ms: u64): bool;
    fn flush(_: auto, now_ms: u64): bool;
    fn cancel(_: auto);
    fn isPending(): bool;
    fn execCount(): u64;
    fn nextDueMs(): u64;
}

export class Throttler {
    interval_ms: u64,
    leading: bool,
    trailing: bool,
    has_window: bool,
    window_start_ms: u64,
    pending: bool,
    exec_count: u64,
    fn init(interval_ms: u64, leading: bool, trailing: bool): Throttler;
    fn trigger(_: auto, now_ms: u64): bool;
    fn poll(_: auto, now_ms: u64): bool;
    fn cancel(_: auto);
    fn isPending(): bool;
    fn execCount(): u64;
}

