// @cap/range-parser — Zero-heap RFC 7233 HTTP `Range` header parser & interval combiner.
// Clean-room MIT port of npm `range-parser` v1.2 (170M+/week).

pub const RANGE_OK: i32 = 0;
pub const RANGE_UNSATISFIABLE: i32 = -1;
pub const RANGE_MALFORMED: i32 = -2;
pub const RANGE_MAX_SPECS: usize = 32;

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

fn rpIsWs(b: u8): bool {
    return b == 32 || b == 9;
}

fn rpTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && rpIsWs(s[start])) { start = start + 1; }
    let end: usize = s.len;
    while (end > start && rpIsWs(s[end - 1])) { end = end - 1; }
    return s[start..end];
}

fn rpParseU64(s: string, ok_out: []bool): u64 {
    if (s.len == 0) { ok_out[0] = false; return 0; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let b = s[i];
        if (b < 48 || b > 57) { ok_out[0] = false; return 0; }
        let d: u64 = (b - 48) as u64;
        if (acc > 1844674407370955161) { ok_out[0] = false; return 0; }
        acc = acc * 10 + d;
        i = i + 1;
    }
    ok_out[0] = true;
    return acc;
}

/// Parses an RFC 7233 `Range` header (`"bytes=0-499, -100, 900-"`) against `size` bytes.
/// If `combine` is true, sorts and merges overlapping or adjacent intervals.
pub fn parseRange(size: u64, header: string, combine: bool): RangeResult {
    let empty_r = ByteRange { start: 0, end: 0 };
    let arr: [32]ByteRange = undefined;
    let init_i: usize = 0;
    while (init_i < RANGE_MAX_SPECS) { arr[init_i] = empty_r; init_i = init_i + 1; }

    let eq: usize = 0;
    while (eq < header.len && header[eq] != 61) { eq = eq + 1; }
    if (eq == 0 || eq >= header.len - 1) {
        return RangeResult { status: RANGE_MALFORMED, unit: "", ranges: arr, count: 0 };
    }

    let unit = rpTrim(header[0..eq]);
    if (unit.len == 0) {
        return RangeResult { status: RANGE_MALFORMED, unit: "", ranges: arr, count: 0 };
    }
    let list = header[eq + 1..header.len];
    let count: usize = 0;
    let pos: usize = 0;

    while (pos <= list.len) {
        let sep = pos;
        while (sep < list.len && list[sep] != 44) { sep = sep + 1; }
        let spec = rpTrim(list[pos..sep]);
        let dash: usize = 0;
        while (dash < spec.len && spec[dash] != 45) { dash = dash + 1; }
        if (dash >= spec.len) {
            return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
        }
        let left = rpTrim(spec[0..dash]);
        let right = rpTrim(spec[dash + 1..spec.len]);
        if (left.len == 0 && right.len == 0) {
            return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
        }

        let ok_l: [1]bool = [false];
        let ok_r: [1]bool = [false];
        let start_val: u64 = 0;
        let end_val: u64 = 0;
        let valid_interval = true;

        if (left.len == 0) {
            // Suffix range: "-N" -> last N bytes
            let suffix = rpParseU64(right, ok_r[0..1]);
            if (!ok_r[0]) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
            if (suffix == 0 || size == 0) {
                valid_interval = false;
            } else {
                start_val = if (suffix >= size) { 0 } else { size - suffix };
                end_val = size - 1;
            }
        } else if (right.len == 0) {
            // Open-ended range: "N-" -> from N to EOF
            let s = rpParseU64(left, ok_l[0..1]);
            if (!ok_l[0]) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
            if (size == 0 || s >= size) {
                valid_interval = false;
            } else {
                start_val = s;
                end_val = size - 1;
            }
        } else {
            // Explicit "start-end"
            let s = rpParseU64(left, ok_l[0..1]);
            let e = rpParseU64(right, ok_r[0..1]);
            if (!ok_l[0] || !ok_r[0]) {
                return RangeResult { status: RANGE_MALFORMED, unit: unit, ranges: arr, count: 0 };
            }
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
            let cur = arr[i];
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
