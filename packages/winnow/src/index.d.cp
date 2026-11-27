// Type definitions for CP

export class Cursor {
    data: string,
    pos: usize,
    fn new(data: string): Cursor;
    fn remaining(): usize;
    fn isEmpty(): bool;
    fn peek(): i32;
    fn rest(): string;
    fn sliceFrom(start: usize): string;
}

export class SpanResult {
    ok: bool,
    start: usize,
    len: usize,
}

export class IntResult {
    ok: bool,
    value: i64,
}

export class UintResult {
    ok: bool,
    value: u64,
}

export fn nbToLower(c: u8): u8;
export fn nbIsDigit(c: u8): bool;
export fn nbIsAlpha(c: u8): bool;
export fn nbIsHex(c: u8): bool;
export fn nbFailSpan(start: usize): SpanResult;
export fn tag(cur: &mut Cursor, lit: string): bool;
export fn tagNoCase(cur: &mut Cursor, lit: string): bool;
export fn charLit(cur: &mut Cursor, ch: u8): bool;
export fn take(cur: &mut Cursor, count: usize): SpanResult;
export fn takeUntil(cur: &mut Cursor, needle: string): SpanResult;
export fn takeTillByte(cur: &mut Cursor, stop: u8): SpanResult;
export fn space0(cur: &mut Cursor): SpanResult;
export fn multispace0(cur: &mut Cursor): SpanResult;
export fn digit1(cur: &mut Cursor): SpanResult;
export fn alpha1(cur: &mut Cursor): SpanResult;
export fn alphanumeric1(cur: &mut Cursor): SpanResult;
export fn hexDigit1(cur: &mut Cursor): SpanResult;
export fn parseU64(cur: &mut Cursor): UintResult;
export fn parseI64(cur: &mut Cursor): IntResult;
export fn delimited(cur: &mut Cursor, open: u8, close: u8): SpanResult;
