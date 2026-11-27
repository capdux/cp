// Type definitions for CP

export enum PgDateKind {
    Invalid,
    Finite,
    PosInfinity,
    NegInfinity
}

export class PgDateTime {
    kind: PgDateKind,
    year: i32,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    second: u8,
    microsecond: u32,
    tz_offset_sec: i32,
    is_bc: bool,
    fn isValid(): bool;
}

export fn pgdStrEq(a: string, b: string): bool;
export fn pgdIsDigit(b: u8): bool;
export fn pgdParseDigits(s: string): i32?;
export fn pgdIsLeap(y: i32): bool;
export fn pgdDaysInMonth(y: i32, m: u8): u8;
export fn parsePgDate(input: string): PgDateTime;
export fn toUnixMs(dt: PgDateTime): i64?;
