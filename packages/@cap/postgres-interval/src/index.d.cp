// Type definitions for CP

export class PgInterval {
    valid: bool,
    years: i32,
    months: i32,
    days: i32,
    hours: i32,
    minutes: i32,
    seconds: i32,
    milliseconds: i32,
}

export class PgivWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn writeByte(_: auto, b: u8);
    fn writeStr(_: auto, s: string);
    fn writeU64(_: auto, val: u64);
    fn writeI32(_: auto, val: i32);
    fn writeU64Pad2(_: auto, val: u64);
    fn writeMsTrimmed(_: auto, ms_abs: u64);
}

export class PgivIntRes {
    ok: bool,
    val: i64,
    next: usize,
}

export class PgivFracRes {
    ms: i32,
    next: usize,
}

export fn pgivInvalid(): PgInterval;
export fn pgivZero(): PgInterval;
export fn pgivIsSpace(c: u8): bool;
export fn pgivIsDigit(c: u8): bool;
export fn pgivIsAlpha(c: u8): bool;
export fn pgivToLower(c: u8): u8;
export fn pgivEqIgnoreCase(a: string, b: string): bool;
export fn pgivSkipSpace(s: string, start: usize): usize;
export fn pgivParseUint(s: string, start: usize): PgivIntRes;
export fn pgivParseFracMs(s: string, start: usize): PgivFracRes;
export fn pgivAbsI32ToU64(v: i32): u64;
export fn pgivParseIso(s: string, start: usize, end: usize): PgInterval;
export fn parsePgInterval(input: string): PgInterval;
export fn toIsoDuration(iv: PgInterval, out: []u8): usize;
export fn toPostgresString(iv: PgInterval, out: []u8): usize;
export fn totalSecondsApprox(iv: PgInterval): i64;
