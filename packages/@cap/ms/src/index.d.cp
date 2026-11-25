// Type definitions for CP

export error MsError { EmptyInput, InvalidFormat, UnknownUnit, Overflow, BufferTooSmall }

export fn isSpace(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn isAlpha(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn eqIgnoreCase(a: &[]u8, lower_lit: &[]u8): bool;
export fn checkedAddU64(a: u64, b: u64): u64 ! MsError;
export fn checkedMulU64(a: u64, b: u64): u64 ! MsError;
export fn absI64ToU64(v: i64): u64;
export fn roundDivU64(num: u64, denom: u64): u64;
export fn lookupMsUnit(unit: &[]u8): u64 ! MsError;
export fn lookupNsUnit(unit: &[]u8): u64 ! MsError;
export fn writeBytes(out: []u8, pos: &mut usize, src: &[]u8): void ! MsError;
export fn writeByte(out: []u8, pos: &mut usize, b: u8): void ! MsError;
export fn writeU64(out: []u8, pos: &mut usize, value: u64): void ! MsError;
export fn parseMs(input: &[]u8): i64 ! MsError;
export fn parseDurationNs(input: &[]u8): u64 ! MsError;
export fn formatShort(ms: i64, out: []u8): &[]u8 ! MsError;
export fn formatLong(ms: i64, out: []u8): &[]u8 ! MsError;
export fn appendNsSegment(out: []u8, pos: &mut usize, value: u64, unit: &[]u8): void ! MsError;
export fn formatDurationNs(ns: u64, out: []u8): &[]u8 ! MsError;
