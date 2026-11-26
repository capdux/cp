// Type definitions for CP

export class Ulid {
    bytes: [16]u8,
}

export fn ulEncodeChar(v: u8): u8;
export fn ulDecodeChar(c: u8): u8;
export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): Ulid;
export fn timestampMs(u: Ulid): u64;
export fn ulEncodeBlock40(b0: u8, b1: u8, b2: u8, b3: u8, b4: u8, out: []u8, offset: usize);
export fn encode(u: Ulid, out: []u8): usize;
export fn decode(s: string, out: &mut Ulid): bool;
export fn isValidUlid(s: string): bool;
export fn increment(u: Ulid, out: &mut Ulid): bool;
export fn compare(a: Ulid, b: Ulid): i32;
