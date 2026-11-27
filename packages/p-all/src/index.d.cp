// Type definitions for CP

import { PLimit } from "../../p-limit/src/index.cp";
export fn pallFindSlot(ids: [32]u64, count: usize, id: u64): usize?;
export class PAllPlanner {
    limiter: PLimit,
    stop_on_error: bool,
    failed: bool,
    first_err: i32,
    total_count: usize,
    completed_count: usize,
    error_count: usize,
    ids: [32]u64,
    states: [32]u8,
    values: [32]i64,
    errors: [32]i32,
    fn new(max_concurrency: usize, stop_on_error: bool): PAllPlanner?;
    fn addTask(_: auto, id: u64): bool;
    fn dispatchNext(_: auto): u64?;
    fn reportSuccess(_: auto, id: u64, value: i64): bool;
    fn reportError(_: auto, id: u64, err_code: i32): bool;
    fn isDone(): bool;
    fn hasFailed(): bool;
    fn firstErrorCode(): i32;
    fn completedCount(): usize;
    fn resultOf(id: u64): i64?;
    fn totalCount(): usize;
    fn errorCount(): usize;
    fn activeCount(): usize;
    fn pendingCount(): usize;
    fn concurrency(): usize;
    fn capacity(): usize;
}

