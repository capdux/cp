// Type definitions for CP

export fn koIsSpace(c: u8): bool;
export fn koIsDigit(c: u8): bool;
export fn koEq(s: string, start: usize, end: usize, lit: string): bool;
export fn koStartsWith(s: string, start: usize, end: usize, prefix: string): bool;
export fn koIsNumber(s: string, start: usize, end: usize): bool;
export fn koIsBigInt(s: string, start: usize, end: usize): bool;
export fn koIsIsoDate(s: string, start: usize, end: usize): bool;
export fn kindOf(raw: string): u8;
export fn kindName(kind: u8): string;
export fn kindOfName(raw: string): string;
export fn isPrimitive(raw: string): bool;
export fn isPlainObject(raw: string): bool;
