// Type definitions for CP

import { countRange, expandNumbers, expandChars, formatRange } from "../../fill-range/src/index.cp";
export class BraceResult {
    buf: []u8,
    starts: [BR_MAX_ITEMS]usize,
    lens: [BR_MAX_ITEMS]usize,
    count: usize,
    truncated: bool,
    fn itemAt(idx: usize): string;
}

export class BrExpander {
    out: []u8,
    pos: usize,
    starts: [BR_MAX_ITEMS]usize,
    lens: [BR_MAX_ITEMS]usize,
    count: usize,
    truncated: bool,
}

export fn brInitExpander(out: []u8): BrExpander;
export fn brToResult(e: &mut BrExpander): BraceResult;
export fn brPushItem(e: &mut BrExpander, s: string);
export fn brIsDigit(c: u8): bool;
export fn brIsUpper(c: u8): bool;
export fn brIsLower(c: u8): bool;
export class BrInt {
    ok: bool,
    val: i64,
}

export fn brParseInt(s: string): BrInt;
export fn brFormatSimpleInt(v: i64, buf: &mut [32]u8): string;
export class BrRangeSpec {
    ok: bool,
    startStr: string,
    endStr: string,
    step: i32,
}

export fn brParseRangeSpec(body: string): BrRangeSpec;
export fn brHasTopLevelComma(body: string): bool;
export class BrSpan {
    found: bool,
    openIdx: usize,
    closeIdx: usize,
    isCommaSet: bool,
}

export fn brFindExpandableBrace(pat: string): BrSpan;
export fn brBuildCombined(prefix: string, mid: string, suffix: string, buf: &mut [BR_MAX_WORK_BUF]u8): usize;
export fn brEmitToken(e: &mut BrExpander, prefix: string, tok: string, suffix: string, depth: usize);
export fn brExpandRangeWithContext(e: &mut BrExpander, prefix: string, startStr: string, endStr: string, stepIn: i32, suffix: string, depth: usize);
export fn brExpandRec(e: &mut BrExpander, pat: string, depth: usize);
export fn expandBraces(pattern: string, out: []u8): BraceResult;
export fn fillRange(start: string, end: string, step: i32, out: []u8): BraceResult;
