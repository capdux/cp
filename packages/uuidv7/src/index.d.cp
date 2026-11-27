// Type definitions for CP

export class UuidV7 {
    bytes: [16]u8,
}

export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7;
export fn timestampMs(u: UuidV7): u64;
export fn version(u: UuidV7): u8;
export fn isRfcVariant(u: UuidV7): bool;
export fn hexLower(nibble: u8): u8;
export fn hexVal(b: u8): i32;
export fn formatHyphenated(u: UuidV7, out: []u8): usize;
export fn parse(s: string, out: &mut UuidV7): bool;
export fn isValidUuidV7(s: string): bool;
export fn compare(a: UuidV7, b: UuidV7): i32;
