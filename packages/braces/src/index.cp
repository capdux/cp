// @cap/braces — Zero-allocation Bash/Glob brace expansion & sequence range generator for CP
// Clean-room MIT implementation inspired by npm `braces` (~500M/wk).
// Delegates numeric/alphabetic sequence range expansion & zero-padded formatting to `@cap/fill-range`.

import {
    countRange,
    expandNumbers,
    expandChars,
    formatRange,
} from "../../fill-range/src/index.cp";

const BR_MAX_ITEMS: usize = 64;
const BR_MAX_DEPTH: usize = 16;
const BR_MAX_WORK_BUF: usize = 512;
const BR_RANGE_BUF: usize = 2048;

pub struct BraceResult {
    pub buf: []u8,
    pub starts: [BR_MAX_ITEMS]usize,
    pub lens: [BR_MAX_ITEMS]usize,
    pub count: usize,
    pub truncated: bool,

    pub fn itemAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        const s = _.starts[idx];
        const e = s + _.lens[idx];
        return _.buf[s..e];
    }
}

struct BrExpander {
    out: []u8,
    pos: usize,
    starts: [BR_MAX_ITEMS]usize,
    lens: [BR_MAX_ITEMS]usize,
    count: usize,
    truncated: bool,
}

fn brInitExpander(out: []u8) -> BrExpander {
    let starts: [BR_MAX_ITEMS]usize = undefined;
    let lens: [BR_MAX_ITEMS]usize = undefined;
    let i: usize = 0;
    while (i < BR_MAX_ITEMS) {
        starts[i] = 0 as usize;
        lens[i] = 0 as usize;
        i += 1;
    }
    return BrExpander{
        out: out,
        pos: 0 as usize,
        starts: starts,
        lens: lens,
        count: 0 as usize,
        truncated: false,
    };
}

fn brToResult(e: *BrExpander) -> BraceResult {
    return BraceResult{
        buf: e.out,
        starts: e.starts,
        lens: e.lens,
        count: e.count,
        truncated: e.truncated,
    };
}

fn brPushItem(e: *BrExpander, s: string) {
    if (e.count >= BR_MAX_ITEMS) {
        e.truncated = true;
        return;
    }
    if (e.pos + s.len > e.out.len) {
        e.truncated = true;
        return;
    }
    const start = e.pos;
    let i: usize = 0;
    while (i < s.len) {
        e.out[e.pos] = s[i];
        e.pos += 1;
        i += 1;
    }
    e.starts[e.count] = start;
    e.lens[e.count] = s.len;
    e.count += 1;
}

fn brIsDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn brIsUpper(c: u8) -> bool {
    return c >= 'A' && c <= 'Z';
}

fn brIsLower(c: u8) -> bool {
    return c >= 'a' && c <= 'z';
}

struct BrInt {
    ok: bool,
    val: i64,
}

fn brParseInt(s: string) -> BrInt {
    if (s.len == 0) {
        return BrInt{ ok: false, val: 0 as i64 };
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) {
        return BrInt{ ok: false, val: 0 as i64 };
    }
    let val: i64 = 0;
    while (i < s.len) {
        if (!brIsDigit(s[i])) {
            return BrInt{ ok: false, val: 0 as i64 };
        }
        val = val * (10 as i64) + ((s[i] - '0') as i64);
        i += 1;
    }
    if (neg) {
        return BrInt{ ok: true, val: -val };
    }
    return BrInt{ ok: true, val: val };
}

fn brFormatSimpleInt(v: i64, buf: *[32]u8) -> string {
    let neg = false;
    let mag: u64 = 0;
    if (v < 0) {
        neg = true;
        mag = (-v) as u64;
    } else {
        mag = v as u64;
    }
    let tmp: [32]u8 = undefined;
    let n: usize = 0;
    if (mag == 0) {
        tmp[0] = '0';
        n = 1 as usize;
    } else {
        while (mag > 0 && n < 30) {
            tmp[n] = ((mag % (10 as u64)) as u8) + '0';
            mag /= 10 as u64;
            n += 1;
        }
    }
    let pos: usize = 0;
    if (neg) {
        buf[pos] = '-';
        pos += 1;
    }
    let k: usize = n;
    while (k > 0 && pos < 32) {
        k -= 1;
        buf[pos] = tmp[k];
        pos += 1;
    }
    return buf[0..pos];
}

struct BrRangeSpec {
    ok: bool,
    startStr: string,
    endStr: string,
    step: i32,
}

fn brParseRangeSpec(body: string) -> BrRangeSpec {
    let idx: usize = 0;
    while (idx < body.len) {
        const c = body[idx];
        if (c == '{' || c == '}' || c == ',') {
            return BrRangeSpec{ ok: false, startStr: "", endStr: "", step: 1 as i32 };
        }
        idx += 1;
    }

    let firstDot: usize = 0;
    let foundFirst = false;
    let i: usize = 0;
    while (i + 1 < body.len) {
        if (body[i] == '.' && body[i + 1] == '.') {
            firstDot = i;
            foundFirst = true;
            break;
        }
        i += 1;
    }
    if (!foundFirst || firstDot == 0 || firstDot + 2 >= body.len) {
        return BrRangeSpec{ ok: false, startStr: "", endStr: "", step: 1 as i32 };
    }

    const startStr = body[0..firstDot];
    const rest = body[(firstDot + 2)..body.len];

    let secondDot: usize = 0;
    let foundSecond = false;
    let j: usize = 0;
    while (j + 1 < rest.len) {
        if (rest[j] == '.' && rest[j + 1] == '.') {
            secondDot = j;
            foundSecond = true;
            break;
        }
        j += 1;
    }

    let endStr = rest;
    let step: i32 = 1;
    if (foundSecond) {
        if (secondDot == 0 || secondDot + 2 >= rest.len) {
            return BrRangeSpec{ ok: false, startStr: "", endStr: "", step: 1 as i32 };
        }
        endStr = rest[0..secondDot];
        const stepStr = rest[(secondDot + 2)..rest.len];
        const ps = brParseInt(stepStr);
        if (!ps.ok || ps.val == 0) {
            return BrRangeSpec{ ok: false, startStr: "", endStr: "", step: 1 as i32 };
        }
        step = ps.val as i32;
    }

    const pi1 = brParseInt(startStr);
    const pi2 = brParseInt(endStr);
    if (pi1.ok && pi2.ok) {
        return BrRangeSpec{ ok: true, startStr: startStr, endStr: endStr, step: step };
    }
    if (startStr.len == 1 && endStr.len == 1) {
        const c1 = startStr[0];
        const c2 = endStr[0];
        if ((brIsLower(c1) && brIsLower(c2)) || (brIsUpper(c1) && brIsUpper(c2))) {
            return BrRangeSpec{ ok: true, startStr: startStr, endStr: endStr, step: step };
        }
    }
    return BrRangeSpec{ ok: false, startStr: "", endStr: "", step: 1 as i32 };
}

fn brHasTopLevelComma(body: string) -> bool {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < body.len) {
        const c = body[i];
        if (c == '\\' && i + 1 < body.len) {
            i += 2;
            continue;
        }
        if (c == '{') {
            depth += 1;
        } else if (c == '}') {
            if (depth > 0) {
                depth -= 1;
            }
        } else if (c == ',' && depth == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

struct BrSpan {
    found: bool,
    openIdx: usize,
    closeIdx: usize,
    isCommaSet: bool,
}

fn brFindExpandableBrace(pat: string) -> BrSpan {
    let i: usize = 0;
    while (i < pat.len) {
        if (pat[i] == '\\' && i + 1 < pat.len) {
            i += 2;
            continue;
        }
        if (pat[i] == '{') {
            let depth: usize = 1;
            let j: usize = i + 1;
            let matched = false;
            while (j < pat.len) {
                if (pat[j] == '\\' && j + 1 < pat.len) {
                    j += 2;
                    continue;
                }
                if (pat[j] == '{') {
                    depth += 1;
                } else if (pat[j] == '}') {
                    depth -= 1;
                    if (depth == 0) {
                        matched = true;
                        break;
                    }
                }
                j += 1;
            }
            if (matched) {
                const body = pat[(i + 1)..j];
                if (brHasTopLevelComma(body)) {
                    return BrSpan{ found: true, openIdx: i, closeIdx: j, isCommaSet: true };
                }
                const rs = brParseRangeSpec(body);
                if (rs.ok) {
                    return BrSpan{ found: true, openIdx: i, closeIdx: j, isCommaSet: false };
                }
            }
        }
        i += 1;
    }
    return BrSpan{ found: false, openIdx: 0 as usize, closeIdx: 0 as usize, isCommaSet: false };
}

fn brBuildCombined(prefix: string, mid: string, suffix: string, buf: *[BR_MAX_WORK_BUF]u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < BR_MAX_WORK_BUF) {
        buf[pos] = prefix[i];
        pos += 1;
        i += 1;
    }
    let j: usize = 0;
    while (j < mid.len && pos < BR_MAX_WORK_BUF) {
        buf[pos] = mid[j];
        pos += 1;
        j += 1;
    }
    let k: usize = 0;
    while (k < suffix.len && pos < BR_MAX_WORK_BUF) {
        buf[pos] = suffix[k];
        pos += 1;
        k += 1;
    }
    return pos;
}

fn brEmitToken(e: *BrExpander, prefix: string, tok: string, suffix: string, depth: usize) {
    if (prefix.len == 0 && suffix.len == 0) {
        brPushItem(e, tok);
    } else {
        let work: [BR_MAX_WORK_BUF]u8 = undefined;
        const wlen = brBuildCombined(prefix, tok, suffix, &work);
        brExpandRec(e, work[0..wlen], depth + 1);
    }
}

fn brExpandRangeWithContext(
    e: *BrExpander,
    prefix: string,
    startStr: string,
    endStr: string,
    stepIn: i32,
    suffix: string,
    depth: usize
) {
    // Reject mixed-case alpha ranges before delegating to @cap/fill-range
    if (startStr.len == 1 && endStr.len == 1) {
        const c1 = startStr[0];
        const c2 = endStr[0];
        const alpha1 = brIsLower(c1) || brIsUpper(c1);
        const alpha2 = brIsLower(c2) || brIsUpper(c2);
        if (alpha1 || alpha2) {
            if (!((brIsLower(c1) && brIsLower(c2)) || (brIsUpper(c1) && brIsUpper(c2)))) {
                return;
            }
            let stepU: usize = 1;
            if (stepIn < 0) {
                stepU = (-stepIn) as usize;
            } else if (stepIn > 0) {
                stepU = stepIn as usize;
            }
            let chOut: [32]u8 = undefined;
            const n = expandChars(c1, c2, stepU, chOut[0..32]);
            if (n > 0) {
                let idx: usize = 0;
                const total = n as usize;
                while (idx < total && !e.truncated) {
                    brEmitToken(e, prefix, chOut[idx..(idx + 1)], suffix, depth);
                    idx += 1;
                }
            }
            return;
        }
    }

    // Primary path: delegate numeric (with zero-padding & negatives) to @cap/fill-range formatRange
    let sepBuf: [1]u8 = undefined;
    sepBuf[0] = 0 as u8;
    let rawBuf: [BR_RANGE_BUF]u8 = undefined;
    const written = formatRange(startStr, endStr, stepIn as i64, sepBuf[0..1], rawBuf[0..BR_RANGE_BUF]);
    if (written > 0) {
        const totalBytes = written as usize;
        let segStart: usize = 0;
        let i: usize = 0;
        while (i <= totalBytes && !e.truncated) {
            if (i == totalBytes || rawBuf[i] == (0 as u8)) {
                brEmitToken(e, prefix, rawBuf[segStart..i], suffix, depth);
                segStart = i + 1;
            }
            i += 1;
        }
        return;
    }

    // Fallback for huge numeric ranges exceeding BR_RANGE_BUF: cap to BR_MAX_ITEMS via countRange + expandNumbers
    const pi1 = brParseInt(startStr);
    const pi2 = brParseInt(endStr);
    if (pi1.ok && pi2.ok) {
        const total = countRange(pi1.val, pi2.val, stepIn as i64);
        let stepAbs: i64 = 1;
        if (stepIn < 0) {
            stepAbs = (-stepIn) as i64;
        } else if (stepIn > 0) {
            stepAbs = stepIn as i64;
        }
        let cappedEnd = pi2.val;
        const willTrunc = total > BR_MAX_ITEMS;
        if (willTrunc) {
            const span = ((BR_MAX_ITEMS - 1) as i64) * stepAbs;
            if (pi2.val >= pi1.val) {
                cappedEnd = pi1.val + span;
            } else {
                cappedEnd = pi1.val - span;
            }
        }
        let nums: [BR_MAX_ITEMS]i64 = undefined;
        const cnt = expandNumbers(pi1.val, cappedEnd, stepIn as i64, nums[0..BR_MAX_ITEMS]);
        if (cnt > 0) {
            let idx: usize = 0;
            const n = cnt as usize;
            while (idx < n && !e.truncated) {
                let numBuf: [32]u8 = undefined;
                const numStr = brFormatSimpleInt(nums[idx], &numBuf);
                brEmitToken(e, prefix, numStr, suffix, depth);
                idx += 1;
            }
            if (willTrunc) {
                e.truncated = true;
            }
        }
    }
}

fn brExpandRec(e: *BrExpander, pat: string, depth: usize) {
    if (e.truncated) {
        return;
    }
    if (depth >= BR_MAX_DEPTH) {
        brPushItem(e, pat);
        return;
    }

    const span = brFindExpandableBrace(pat);
    if (!span.found) {
        brPushItem(e, pat);
        return;
    }

    const prefix = pat[0..span.openIdx];
    const body = pat[(span.openIdx + 1)..span.closeIdx];
    const suffix = pat[(span.closeIdx + 1)..pat.len];

    if (span.isCommaSet) {
        let segStart: usize = 0;
        let bDepth: usize = 0;
        let i: usize = 0;
        while (i <= body.len && !e.truncated) {
            const atEnd = i == body.len;
            let split = false;
            if (!atEnd) {
                const c = body[i];
                if (c == '\\' && i + 1 < body.len) {
                    i += 2;
                    continue;
                }
                if (c == '{') {
                    bDepth += 1;
                } else if (c == '}') {
                    if (bDepth > 0) {
                        bDepth -= 1;
                    }
                } else if (c == ',' && bDepth == 0) {
                    split = true;
                }
            }
            if (atEnd || split) {
                const part = body[segStart..i];
                let work: [BR_MAX_WORK_BUF]u8 = undefined;
                const wlen = brBuildCombined(prefix, part, suffix, &work);
                brExpandRec(e, work[0..wlen], depth + 1);
                segStart = i + 1;
            }
            i += 1;
        }
        return;
    }

    const rs = brParseRangeSpec(body);
    if (rs.ok) {
        brExpandRangeWithContext(e, prefix, rs.startStr, rs.endStr, rs.step, suffix, depth);
        return;
    }

    brPushItem(e, pat);
}

pub fn expandBraces(pattern: string, out: []u8) -> BraceResult {
    let e = brInitExpander(out);
    brExpandRec(&e, pattern, 0 as usize);
    return brToResult(&e);
}

pub fn fillRange(start: string, end: string, step: i32, out: []u8) -> BraceResult {
    let e = brInitExpander(out);
    brExpandRangeWithContext(&e, "", start, end, step, "", 0 as usize);
    return brToResult(&e);
}

export {
    BraceResult,
    expandBraces,
    fillRange,
};
