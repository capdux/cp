// Type definitions for CP

export enum JpSegKind {
    Root,
    Property,
    Index,
    Wildcard,
    RecursiveProp,
    RecursiveWildcard,
    Slice
}

export class JpSegment {
    kind: JpSegKind,
    name: string,
    index: i64,
    slice_start: i64,
    slice_end: i64,
    has_start: bool,
    has_end: bool,
}

export fn jprIsWs(c: u8): bool;
export fn jprIsDigit(c: u8): bool;
export fn jprIsIdentStart(c: u8): bool;
export fn jprIsIdentCont(c: u8): bool;
export fn jprIsSimpleIdent(s: string): bool;
export fn jprMakeSeg(kind: JpSegKind, name: string): JpSegment;
export class JprIntRes {
    val: i64,
    next_pos: usize,
    ok: bool,
}

export fn jprParseInt(s: string, start_pos: usize): JprIntRes;
export fn parseJsonPath(expr: string, out: []JpSegment): i64;
export fn jprPushByte(out: []u8, pos: usize, b: u8): usize;
export fn jprPushSlice(out: []u8, pos: usize, s: string): usize;
export fn jprPushI64(out: []u8, pos: usize, val: i64): usize;
export fn formatJsonPath(segs: &[]JpSegment, out: []u8): usize;
