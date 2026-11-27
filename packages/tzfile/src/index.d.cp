// Type definitions for CP

export class TzifHeader {
    version: u8,
    isutcnt: u32,
    isstdcnt: u32,
    leapcnt: u32,
    timecnt: u32,
    typecnt: u32,
    charcnt: u32,
}

export class PosixTz {
    std_abbr: string,
    std_offset_secs: i32,
    has_dst: bool,
    dst_abbr: string,
    dst_offset_secs: i32,
}

export class TzTransition {
    at_unix: i64,
    offset_secs: i32,
    is_dst: bool,
}

export fn tzReadU32Be(buf: []u8, off: usize): u32;
export fn isTzifMagic(buf: []u8): bool;
export fn parseTzifHeader(buf: []u8): TzifHeader?;
export fn tzIsAlpha(c: u8): bool;
export fn tzIsDigit(c: u8): bool;
export class TzAbbrRes {
    abbr: string,
    next_pos: usize,
    ok: bool,
}

export class TzOffRes {
    secs: i32,
    next_pos: usize,
    ok: bool,
}

export fn tzParseAbbr(s: string, pos_in: usize): TzAbbrRes;
export fn tzParsePosixOffset(s: string, pos_in: usize): TzOffRes;
export fn parsePosixTz(spec: string): PosixTz?;
export fn lookupTransitionOffset(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32;
export fn utcToLocalSeconds(unix_secs: i64, offset_secs: i32): i64;
