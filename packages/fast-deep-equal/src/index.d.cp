// Type definitions for CP

export class ValueSpan {
    start: usize,
    end: usize,
    kind: u8,
    valid: bool,
}

export class DecChar {
    code: u32,
    next_pos: usize,
    done: bool,
}

export fn isWs(ch: u8): bool;
export fn skipWs(s: string, pos: usize): usize;
export fn matchLit(s: string, pos: usize, lit: string): bool;
export fn hexDigit(ch: u8): i32;
export fn invalidSpan(): ValueSpan;
export fn scanString(s: string, pos: usize): ValueSpan;
export fn scanNumber(s: string, pos: usize): ValueSpan;
export fn parseValueSpan(s: string, pos: usize, depth: u32): ValueSpan;
export fn kindOfJson(s: string): u8;
export fn isPlainObjectJson(s: string): bool;
export fn parseJsonNumberF64(s: string): f64;
export fn nextDecodedChar(s: string, pos: usize): DecChar;
export fn jsonStringsEqual(a: string, b: string): bool;
export fn countObjectMembers(obj: string): usize;
export fn arraysDeepEqual(a: string, b: string, depth: u32): bool;
export fn objectsDeepEqual(a: string, b: string, depth: u32): bool;
export fn valuesDeepEqual(a: string, b: string, kind: u8, depth: u32): bool;
export fn jsonDeepEqual(a: string, b: string): bool;
