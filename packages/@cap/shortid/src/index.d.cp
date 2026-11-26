// Type definitions for CP

export fn sidU64(hi: u32, lo: u32): u64;
export fn sidCharIndex(c: u8, alphabet: string): i32;
export class ShortIdGenerator {
    alphabet: [64]u8,
    worker_id: u8,
    fmt_version: u8,
    last_tick: u64,
    counter: u32,
    rng_state: u64,
    fn new(seed: u64, worker_id: u8): ShortIdGenerator;
    fn workerId(): u8;
    fn version(): u8;
    fn nextRand(_: auto): u64;
    fn encodeValue(_: auto, val: u64, out: []u8, pos: usize): usize;
    fn generate(_: auto, tick_ms: u64, out: []u8): usize;
    fn isValid(id: string): bool;
    fn decodeVersion(id: string): i32;
    fn decodeWorker(id: string): i32;
}

export fn isValidShortId(id: string): bool;
