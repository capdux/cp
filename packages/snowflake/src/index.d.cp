// Type definitions for CP

import { formatU64 } from "../../itoa/src/index.cp";
export class SnowflakeParts {
    timestamp_ms: u64,
    datacenter_id: u16,
    worker_id: u16,
    sequence: u16,
}

export fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;
export fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;
export fn decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts;
export fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;
export fn timestamp(id: u64, epoch_ms: u64): u64;
export fn datacenterId(id: u64): u16;
export fn workerId(id: u64): u16;
export fn sequence(id: u64): u16;
export fn minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;
export fn maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;
export fn formatSnowflake(id: u64, out: []u8): usize;
export fn parseSnowflake(s: string): u64?;
export fn parse(s: string): u64?;
export fn isValidSnowflakeString(s: string): bool;
export class SnowflakeGenerator {
    epoch_ms: u64,
    datacenter_id: u16,
    worker_id: u16,
    last_ts_ms: u64,
    sequence: u16,
    fn new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn withDiscordEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn nextId(_: auto, now_ms: u64): u64?;
    fn nextBatch(_: auto, now_ms: u64, out: []u64): usize;
    fn compose(timestamp_ms: u64, seq: u16): u64;
    fn decompose(id: u64): SnowflakeParts;
    fn timestamp(id: u64): u64;
    fn reset(_: auto);
}

