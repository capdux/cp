// Type definitions for CP

import { formatU64 } from "../../itoa/src/index.cp";
export class SnowflakeParts {
    timestamp_ms: u64,
    datacenter_id: u16,
    worker_id: u16,
    sequence: u16,
}

export fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16): u64;
export fn decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts;
export fn formatSnowflake(id: u64, out: []u8): usize;
export fn parseSnowflake(s: string): u64?;
export class SnowflakeGenerator {
    epoch_ms: u64,
    datacenter_id: u16,
    worker_id: u16,
    last_ts_ms: u64,
    sequence: u16,
    fn new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn nextId(_: auto, now_ms: u64): u64?;
}

