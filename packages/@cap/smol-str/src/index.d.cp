// Type definitions for CP

export fn smolGetByte(s: SmolStr, idx: usize): u8;
export fn smolMinUsize(a: usize, b: usize): usize;
export class SmolStr {
    inline_buf: [24]u8,
    inline_len: u8,
    is_inline: bool,
    ext: string,
    fn empty(): SmolStr;
    fn new(s: string): SmolStr;
    fn fromInt(v: i64): SmolStr;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isInline(): bool;
    fn byteAt(idx: usize): u8?;
    fn eq(other: SmolStr): bool;
    fn eqStr(s: string): bool;
    fn cmp(other: SmolStr): i32;
    fn startsWith(prefix: string): bool;
    fn endsWith(suffix: string): bool;
    fn writeTo(out: []u8): usize;
}

