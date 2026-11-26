// Type definitions for CP

export enum ContentRangeKind {
    Invalid,
    Bytes,
    BytesUnknownLength,
    Unsatisfied
}

export class ContentRange {
    kind: ContentRangeKind,
    first_byte: u64,
    last_byte: u64,
    complete_length: u64,
    fn isValid(): bool;
    fn rangeLen(): u64;
}

export fn hcrParseU64(s: string): u64?;
export fn hcrPush(out: []u8, pos: usize, s: string): usize;
export fn hcrPushU64(out: []u8, pos: usize, val: u64): usize;
export fn parseContentRange(header: string): ContentRange;
export fn formatContentRangeHeader(cr: ContentRange, out: []u8): usize;
