// @cap/range-parser — Zero-heap RFC 7233 HTTP `Range` header parser & interval combiner.
// Clean-room MIT port of npm `range-parser` v1.2 (170M+/week).
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry whitespace LUT, and undefined stack buffers.

import { parseU64 } from "cap:fmt";

pub const RANGE_OK: i32 = 0;
pub const RANGE_UNSATISFIABLE: i32 = -1;
pub const RANGE_MALFORMED: i32 = -2;
pub const RANGE_MAX_SPECS: usize = 32;

type RpVec16 = @Vector(16, u8);

fn rpBuildWsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[32] = 1; // ' '
    lut[9] = 1;  // '\t'
    lut[10] = 1; // '\n'
    lut[11] = 1; // '\v'
    lut[12] = 1; // '\f'
    lut[13] = 1; // '\r'
    return lut;
}

const RP_WS_LUT: [256]u8 = rpBuildWsLut();

fn rpTrim(s: string): string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && RP_WS_LUT[s[a] as usize] != 0) {
        a += 1;
    }
    while (b > a && RP_WS_LUT[s[b - 1] as usize] != 0) {
        b -= 1;
    }
    return s[a..b];
}

fn rpFindByte(s: string, start: usize, target: u8): usize {
    let i = start;
    const len = s.len;
    const v_t: RpVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: RpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_t)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

pub struct ByteRange {
    pub start: u64,
    pub end: u64,
}

pub struct RangeResult {
    pub status: i32,
    pub unit: string,
    pub ranges: [32]ByteRange,
    pub count: usize,
}

/// Parses an RFC 7233 `Range` header (`"bytes=0-499, -100, 900-"`) against `size` bytes.
/// If `combine` is true, sorts and merges overlapping or adjacent intervals.
pub fn parseRange(size: u64, header: string, combine: bool): RangeResult {
    let arr: [32]ByteRange = undefined;

    const eq: usize = rpFindByte(header, 0, 61); // '='
    if (eq == 0 || eq >= header.len - 1) {
        return RangeResult { status: RANGE_MALFORMED, unit: "", ranges: arr, count: 0 };
    }

    const unit = rpTrim(header[0..eq]);
    if (unit.len == 0) {
        return RangeResult { status: RANGE_MALFORMED, unit: "", ranges: arr, count: 0 };
    }
    const list = header[eq + 1..header.len];
    let count: usize = 0;
    let pos: usize = 0;

    while (pos <= list.len) {
        const sep = rpFindByte(list, pos, 44); // ','
        const spec = rpTrim(list[pos..sep]);
        const dash: usize = rpFindByte(spec, 0, 45); // '-'
        if (dash >= spec.len) {
            return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
        }
        const left = rpTrim(spec[0..dash]);
        const right = rpTrim(spec[dash + 1..spec.len]);
        if (left.len == 0 && right.len == 0) {
            return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
        }

        let start_val: u64 = 0;
        let end_val: u64 = 0;
        let valid_interval = true;

        if (left.len == 0) {
            // Suffix range: "-N" -> last N bytes
            const suffix_opt = parseU64(right);
            if (suffix_opt == null) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
            const suffix = suffix_opt.?;
            if (suffix == 0 || size == 0) {
                valid_interval = false;
            } else {
                start_val = if (suffix >= size) { 0 } else { size - suffix };
                end_val = size - 1;
            }
        } else if (right.len == 0) {
            // Open-ended range: "N-" -> from N to EOF
            const s_opt = parseU64(left);
            if (s_opt == null) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
            const s = s_opt.?;
            if (size == 0 || s >= size) {
                valid_interval = false;
            } else {
                start_val = s;
                end_val = size - 1;
            }
        } else {
            // Explicit "start-end"
            const s_opt = parseU64(left);
            const e_opt = parseU64(right);
            if (s_opt == null || e_opt == null) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
            const s = s_opt.?;
            const e = e_opt.?;
            if (size == 0 || s > e || s >= size) {
                valid_interval = false;
            } else {
                start_val = s;
                end_val = if (e >= size) { size - 1 } else { e };
            }
        }

        if (valid_interval && count < RANGE_MAX_SPECS) {
            arr[count] = ByteRange { start: start_val, end: end_val };
            count = count + 1;
        }

        if (sep >= list.len) { break; }
        pos = sep + 1;
    }

    if (count == 0) {
        return RangeResult { status: RANGE_UNSATISFIABLE, unit: unit, ranges: arr, count: 0 };
    }

    if (combine && count > 1) {
        // Insertion sort by start
        let i: usize = 1;
        while (i < count) {
            const cur = arr[i];
            let j = i;
            while (j > 0 && arr[j - 1].start > cur.start) {
                arr[j] = arr[j - 1];
                j = j - 1;
            }
            arr[j] = cur;
            i = i + 1;
        }
        // Merge overlapping or adjacent intervals
        let merged: usize = 0;
        let k: usize = 1;
        while (k < count) {
            if (arr[k].start <= arr[merged].end + 1) {
                if (arr[k].end > arr[merged].end) {
                    arr[merged].end = arr[k].end;
                }
            } else {
                merged = merged + 1;
                arr[merged] = arr[k];
            }
            k = k + 1;
        }
        count = merged + 1;
    }

    return RangeResult { status: RANGE_OK, unit: unit, ranges: arr, count: count };
}

/// Computes the inclusive byte length (`end - start + 1`) of a `ByteRange`.
pub fn rangeByteLength(r: ByteRange): u64 {
    if (r.end < r.start) { return 0; }
    return (r.end - r.start) + 1;
}
