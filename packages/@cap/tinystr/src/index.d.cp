// Type definitions for CP

export fn isValidAsciiByte(b: u8): bool;
export fn isAlphaByte(b: u8): bool;
export fn isDigitByte(b: u8): bool;
export fn toLowerByte(b: u8): u8;
export fn toUpperByte(b: u8): u8;
export fn getByte32(raw: u32, idx: usize): u8;
export fn putByte32(raw: u32, idx: usize, b: u8): u32;
export fn getByte64(raw: u64, idx: usize): u8;
export fn putByte64(raw: u64, idx: usize, b: u8): u64;
export class TinyStr4 {
    raw: u32,
    fn fromStr(s: string): TinyStr4?;
    fn len(): usize;
    fn equals(other: TinyStr4): bool;
    fn toAsciiLower(): TinyStr4;
    fn toAsciiUpper(): TinyStr4;
    fn toAsciiTitle(): TinyStr4;
    fn isAsciiAlpha(): bool;
    fn isAsciiNumeric(): bool;
    fn isAsciiAlphanumeric(): bool;
    fn writeTo(out: []u8): string;
}

export class TinyStr8 {
    raw: u64,
    fn fromStr(s: string): TinyStr8?;
    fn len(): usize;
    fn equals(other: TinyStr8): bool;
    fn toAsciiLower(): TinyStr8;
    fn toAsciiUpper(): TinyStr8;
    fn toAsciiTitle(): TinyStr8;
    fn isAsciiAlpha(): bool;
    fn isAsciiNumeric(): bool;
    fn isAsciiAlphanumeric(): bool;
    fn writeTo(out: []u8): string;
}

