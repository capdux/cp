// Type definitions for CP

import { PLimit } from "../../p-limit-core/src/index.cp";
export class PLocateOptions {
    concurrency: usize,
    preserve_order: bool,
    fn defaults(): PLocateOptions;
}

export class LocateCandidate {
    id: u32,
    matched: bool,
    finished: bool,
}

export fn plocEmptyCandidate(): LocateCandidate;
export class PLocatePlanner {
    opts: PLocateOptions,
    limiter: PLimit,
    items: [32]LocateCandidate,
    count: usize,
    settled: bool,
    has_winner: bool,
    winner: u32,
    fn new(opts: PLocateOptions): PLocatePlanner?;
    fn defaultPlanner(): PLocatePlanner;
    fn addCandidate(_: auto, id: u32): bool;
    fn dispatchNext(_: auto): u32?;
    fn reportResult(_: auto, id: u32, matched: bool): u32?;
    fn isSettled(): bool;
    fn winnerId(): u32?;
}

