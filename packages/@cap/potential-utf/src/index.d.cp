// Type definitions for CP

export fn putfIsCont(b: u8): bool;
export fn putfValidSeqLen(bytes: &[]u8, pos: usize): usize;
export fn putfMaximalSubpart(bytes: &[]u8, pos: usize): usize;
export fn putfEncodeScalar(scalar: u32, out: []u8, w: usize): usize;
export fn putfWriteReplacement(out: []u8, w: usize): usize;
export class PotentialUtf8 {
    bytes: &[]u8,
    fn fromBytes(b: &[]u8): PotentialUtf8;
    fn fromStr(s: string): PotentialUtf8;
    fn asBytes(): &[]u8;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isValidUtf8(): bool;
    fn toValidPrefix(): &[]u8;
    fn decodeLossy(out: []u8): usize;
}

export class PotentialUtf16 {
    units: &[]u16,
    fn fromSlice(u: &[]u16): PotentialUtf16;
    fn asSlice(): &[]u16;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isValidUtf16(): bool;
    fn toUtf8Lossy(out: []u8): usize;
}

export class PotentialCodePoint {
    raw: u32,
    fn fromU32(v: u32): PotentialCodePoint;
    fn toU32(): u32;
    fn isValidScalar(): bool;
    fn toCharOrReplacement(): u32;
    fn encodeUtf8Lossy(out: []u8): usize;
}

