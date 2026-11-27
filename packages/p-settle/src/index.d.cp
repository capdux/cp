// Type definitions for CP

import { PLimit } from "../../p-limit/src/index.cp";
export enum SettleStatus {
    Pending,
    Fulfilled,
    Rejected
}

export class SettledEntry {
    id: u64,
    status: SettleStatus,
    value: i64,
    err_code: i32,
}

export fn psetFindSlot(ids: [32]u64, count: usize, id: u64): usize?;
export class PSettlePlanner {
    limiter: PLimit,
    total_count: usize,
    fulfilled_count: usize,
    rejected_count: usize,
    ids: [32]u64,
    running: [32]bool,
    statuses: [32]SettleStatus,
    values: [32]i64,
    errors: [32]i32,
    fn new(max_concurrency: usize): PSettlePlanner?;
    fn addTask(_: auto, id: u64): bool;
    fn dispatchNext(_: auto): u64?;
    fn reportFulfilled(_: auto, id: u64, value: i64): bool;
    fn reportRejected(_: auto, id: u64, err_code: i32): bool;
    fn isAllSettled(): bool;
    fn fulfilledCount(): usize;
    fn rejectedCount(): usize;
    fn entryAt(idx: usize): SettledEntry?;
    fn entryOf(id: u64): SettledEntry?;
    fn totalCount(): usize;
    fn activeCount(): usize;
    fn pendingCount(): usize;
    fn concurrency(): usize;
    fn capacity(): usize;
}

