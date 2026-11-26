// Type definitions for CP

export fn uldDecodeChar(ch: u8): u8;
export class Ulid {
    hi: u64,
    lo: u64,
    fn fromParts(timestamp_ms: u64, rand_hi16: u16, rand_lo64: u64): Ulid;
    fn fromBytes(src: &[]u8): Ulid?;
    fn timestampMs(): u64;
    fn cmp(other: Ulid): i32;
    fn eq(other: Ulid): bool;
    fn increment(): Ulid?;
    fn toBytes(out: []u8): usize;
}

export fn encodeUlid(u: Ulid, out: []u8): usize;
export fn decodeUlid(s: string): Ulid?;
export fn isValidUlid(s: string): bool;
