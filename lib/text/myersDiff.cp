// Reference-hardened against mitsuhiko/similar (Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/runtime/node/assert/myers_diff.cp | Synced: 2026-11-22T10:36:56Z
// Myers O(ND) shortest-edit-script diff engine for lines and bytes with common prefix/suffix trimming
// and 16-lane @Vector(16, u8) SIMD byte/newline scanning.

const std = @import("std");

type DiffVec16u8 = @Vector(16, u8);

pub const DEFAULT_MAX_EDIT_STEPS: usize = 8192;
const MAX_DIFF_LEN: usize = 0x3FFF_FFFF;

pub err DiffError {
    InputsTooLarge,
    DiffTooLarge,
    OutOfMemory,
}

pub enum DiffKind: u8 {
    equal = 0,
    insert = 1,
    delete = 2,

    fn prefixByte(): u8 {
        return switch (_) {
            .equal => ' ',
            .insert => '+',
            .delete => '-',
        };
    }
}

pub struct LineDiff {
    kind: DiffKind,
    value: string,
}

pub struct ByteDiff {
    kind: DiffKind,
    value: u8,
}

pub struct DiffOptions {
    check_comma_disparity: bool = false,
    max_edit_steps: usize = DEFAULT_MAX_EDIT_STEPS,
}

pub struct LineDiffResult {
    items: []LineDiff,
    allocator: std.mem.Allocator,

    fn len(): usize {
        return _.items.len;
    }

    fn countKind(kind: DiffKind): usize {
        let n: usize = 0;
        for (it of _.items) {
            if (it.kind == kind) {
                n += 1;
            }
        }
        return n;
    }

    fn editDistance(): usize {
        return _.countKind(.insert) + _.countKind(.delete);
    }

    fn isIdentical(): bool {
        return _.editDistance() == 0;
    }

    ~init() {
        if (_.items.len > 0) {
            _.allocator.free(_.items);
            _.items = [];
        }
    }
}

pub struct ByteDiffResult {
    items: []ByteDiff,
    allocator: std.mem.Allocator,

    fn len(): usize {
        return _.items.len;
    }

    fn editDistance(): usize {
        let n: usize = 0;
        for (it of _.items) {
            if (it.kind != .equal) {
                n += 1;
            }
        }
        return n;
    }

    fn isIdentical(): bool {
        return _.editDistance() == 0;
    }

    ~init() {
        if (_.items.len > 0) {
            _.allocator.free(_.items);
            _.items = [];
        }
    }
}

fn diffLoadVec16(data: []const u8, off: usize): DiffVec16u8 {
    const v: DiffVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD byte slice equality check.
pub fn simdBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    const n = a.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const va = diffLoadVec16(a, i);
        const vb = diffLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdBytesEqual` (differential verification).
pub fn scalarBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD common prefix length scanner.
pub fn simdCommonPrefixLen(a: []const u8, b: []const u8): usize {
    const limit = if (a.len <= b.len) a.len else b.len;
    let i: usize = 0;
    while (i + 16 <= limit) {
        const va = diffLoadVec16(a, i);
        const vb = diffLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) {
            break;
        }
        i += 16;
    }
    while (i < limit && a[i] == b[i]) : (i += 1) {}
    return i;
}

/// Scalar reference for `simdCommonPrefixLen` (differential verification).
pub fn scalarCommonPrefixLen(a: []const u8, b: []const u8): usize {
    const limit = if (a.len <= b.len) a.len else b.len;
    let i: usize = 0;
    while (i < limit && a[i] == b[i]) : (i += 1) {}
    return i;
}

/// 16-lane `@Vector(16, u8)` SIMD common suffix length scanner.
pub fn simdCommonSuffixLen(a: []const u8, b: []const u8): usize {
    const limit = if (a.len <= b.len) a.len else b.len;
    let k: usize = 0;
    while (k + 16 <= limit) {
        const va = diffLoadVec16(a, a.len - k - 16);
        const vb = diffLoadVec16(b, b.len - k - 16);
        if (!@reduce(.And, va == vb)) {
            break;
        }
        k += 16;
    }
    while (k < limit && a[a.len - 1 - k] == b[b.len - 1 - k]) : (k += 1) {}
    return k;
}

/// Scalar reference for `simdCommonSuffixLen` (differential verification).
pub fn scalarCommonSuffixLen(a: []const u8, b: []const u8): usize {
    const limit = if (a.len <= b.len) a.len else b.len;
    let k: usize = 0;
    while (k < limit && a[a.len - 1 - k] == b[b.len - 1 - k]) : (k += 1) {}
    return k;
}

/// 16-lane `@Vector(16, u8)` SIMD scanner for the next `'\n'` at or after `start`.
pub fn simdFindNewline(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    const nl_v: DiffVec16u8 = @splat('\n');
    let i: usize = start;
    while (i + 16 <= data.len) {
        const v = diffLoadVec16(data, i);
        if (@reduce(.Or, v == nl_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (data[i + j] == '\n') {
                    return i + j;
                }
            }
        }
        i += 16;
    }
    while (i < data.len) : (i += 1) {
        if (data[i] == '\n') {
            return i;
        }
    }
    return null;
}

/// Scalar reference for `simdFindNewline` (differential verification).
pub fn scalarFindNewline(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        if (data[i] == '\n') {
            return i;
        }
    }
    return null;
}

pub fn areLinesEqual(a: string, b: string, check_comma_disparity: bool): bool {
    if (!check_comma_disparity) {
        return simdBytesEqual(a, b);
    }
    if (a.len == b.len) {
        return simdBytesEqual(a, b);
    }
    if (a.len == b.len + 1) {
        return a[a.len - 1] == ',' && simdBytesEqual(a[0..b.len], b);
    }
    if (b.len == a.len + 1) {
        return b[b.len - 1] == ',' && simdBytesEqual(b[0..a.len], a);
    }
    return false;
}

pub fn splitLines(allocator: std.mem.Allocator, text: string): ([]string) ! DiffError {
    if (text.len == 0) {
        let empty: []string = [];
        return empty;
    }
    let count: usize = 1;
    let scan: usize = 0;
    while (nl_pos := simdFindNewline(text, scan)) {
        count += 1;
        scan = nl_pos + 1;
    }
    let out = allocator.alloc(string, count) catch return error.OutOfMemory;
    let idx: usize = 0;
    let start: usize = 0;
    while (nl_pos := simdFindNewline(text, start)) {
        out[idx] = text[start..nl_pos];
        idx += 1;
        start = nl_pos + 1;
    }
    out[idx] = text[start..text.len];
    return out;
}

fn selectEqualLineValue(a: string, b: string, check_comma_disparity: bool): string {
    if (check_comma_disparity && a.len > 0 && a[a.len - 1] != ',') {
        return b;
    }
    return a;
}

/// Computes the Myers O(ND) shortest edit script between `actual` and `expected` line slices.
/// Applies common prefix/suffix trimming before Myers search and enforces `opts.max_edit_steps`.
/// Results are returned in forward chronological order.
pub fn diffLines(
    allocator: std.mem.Allocator,
    actual: []const string,
    expected: []const string,
    opts: DiffOptions,
): LineDiffResult ! DiffError {
    if (actual.len > MAX_DIFF_LEN || expected.len > MAX_DIFF_LEN) return error.InputsTooLarge;
    const total_full = actual.len + expected.len;
    if (total_full > MAX_DIFF_LEN) return error.InputsTooLarge;

    if (total_full == 0) {
        let empty: []LineDiff = [];
        return LineDiffResult{ items: empty, allocator: allocator };
    }

    // 1. Common prefix & suffix trimming (similar-style search matrix reduction)
    let prefix: usize = 0;
    while (prefix < actual.len && prefix < expected.len && areLinesEqual(actual[prefix], expected[prefix], opts.check_comma_disparity)) {
        prefix += 1;
    }
    const rem_a_after_prefix = actual.len - prefix;
    const rem_e_after_prefix = expected.len - prefix;
    let suffix: usize = 0;
    while (suffix < rem_a_after_prefix && suffix < rem_e_after_prefix &&
        areLinesEqual(actual[actual.len - 1 - suffix], expected[expected.len - 1 - suffix], opts.check_comma_disparity))
    {
        suffix += 1;
    }

    const mid_actual = actual[prefix .. actual.len - suffix];
    const mid_expected = expected[prefix .. expected.len - suffix];

    // Fast path: identical, pure insertion, or pure deletion after trimming
    if (mid_actual.len == 0 || mid_expected.len == 0) {
        const out_len = prefix + mid_actual.len + mid_expected.len + suffix;
        let out = allocator.alloc(LineDiff, out_len) catch return error.OutOfMemory;
        let w: usize = 0;
        let i: usize = 0;
        while (i < prefix) : (i += 1) {
            out[w] = LineDiff{
                kind: .equal,
                value: selectEqualLineValue(actual[i], expected[i], opts.check_comma_disparity),
            };
            w += 1;
        }
        i = 0;
        while (i < mid_actual.len) : (i += 1) {
            out[w] = LineDiff{ kind: .insert, value: mid_actual[i] };
            w += 1;
        }
        i = 0;
        while (i < mid_expected.len) : (i += 1) {
            out[w] = LineDiff{ kind: .delete, value: mid_expected[i] };
            w += 1;
        }
        i = 0;
        while (i < suffix) : (i += 1) {
            const ai = actual.len - suffix + i;
            const ei = expected.len - suffix + i;
            out[w] = LineDiff{
                kind: .equal,
                value: selectEqualLineValue(actual[ai], expected[ei], opts.check_comma_disparity),
            };
            w += 1;
        }
        return LineDiffResult{ items: out, allocator: allocator };
    }

    // 2. Myers O(ND) on trimmed middle slices with edit-step guard
    const total = mid_actual.len + mid_expected.len;
    const max_steps = if (opts.max_edit_steps == 0) DEFAULT_MAX_EDIT_STEPS else opts.max_edit_steps;
    const step_limit = if (total < max_steps) total else max_steps;
    const graph_size = (2 * step_limit) + 1;
    if (graph_size > MAX_DIFF_LEN) return error.DiffTooLarge;

    let graph = allocator.alloc(u32, graph_size) catch return error.OutOfMemory;
    defer allocator.free(graph);
    @memset(graph, 0);

    let trace_frames = allocator.alloc([]u32, step_limit + 1) catch return error.OutOfMemory;
    let trace_len: usize = 0;
    defer {
        let ti: usize = 0;
        while (ti < trace_len) : (ti += 1) {
            allocator.free(trace_frames[ti]);
        }
        allocator.free(trace_frames);
    }

    const max_i: i64 = i64(step_limit);
    let d_usize: usize = 0;
    let found = false;
    while (d_usize <= step_limit) : (d_usize += 1) {
        const frame = allocator.dupe(u32, graph) catch return error.OutOfMemory;
        trace_frames[trace_len] = frame;
        trace_len += 1;

        const d: i64 = i64(d_usize);
        let k: i64 = 0 - d;
        while (k <= d) : (k += 2) {
            const offset: usize = usize(k + max_i);
            let x: usize = 0;
            if (k == (0 - d) || (k != d && graph[offset - 1] < graph[offset + 1])) {
                x = usize(graph[offset + 1]);
            } else {
                x = usize(graph[offset - 1]) + 1;
            }
            let y: usize = usize(i64(x) - k);
            while (x < mid_actual.len && y < mid_expected.len && areLinesEqual(mid_actual[x], mid_expected[y], opts.check_comma_disparity)) {
                x += 1;
                y += 1;
            }
            graph[offset] = u32(x);
            if (x >= mid_actual.len && y >= mid_expected.len) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        return error.DiffTooLarge;
    }

    let rev_list: std.ArrayList(LineDiff) = .empty;
    defer rev_list.deinit(allocator);

    let x_i: i64 = i64(mid_actual.len);
    let y_i: i64 = i64(mid_expected.len);
    let level: usize = trace_len;
    while (level > 0) {
        level -= 1;
        const g = trace_frames[level];
        const k: i64 = x_i - y_i;
        const off: usize = usize(k + max_i);
        const lv_i: i64 = i64(level);
        const prev_k: i64 = if (k == (0 - lv_i) || (k != lv_i && g[off - 1] < g[off + 1]))
            k + 1
        else
            k - 1;
        const prev_x: i64 = i64(g[usize(prev_k + max_i)]);
        const prev_y: i64 = prev_x - prev_k;

        while (x_i > prev_x && y_i > prev_y) {
            const ax = mid_actual[usize(x_i - 1)];
            const ex = mid_expected[usize(y_i - 1)];
            rev_list.append(allocator, LineDiff{
                kind: .equal,
                value: selectEqualLineValue(ax, ex, opts.check_comma_disparity),
            }) catch return error.OutOfMemory;
            x_i -= 1;
            y_i -= 1;
        }
        if (level > 0) {
            if (x_i > prev_x) {
                rev_list.append(allocator, LineDiff{ kind: .insert, value: mid_actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
                x_i -= 1;
            } else {
                rev_list.append(allocator, LineDiff{ kind: .delete, value: mid_expected[usize(y_i - 1)] }) catch return error.OutOfMemory;
                y_i -= 1;
            }
        }
    }

    const mid_n = rev_list.items.len;
    const out_len = prefix + mid_n + suffix;
    let out = allocator.alloc(LineDiff, out_len) catch return error.OutOfMemory;
    let w: usize = 0;
    let i: usize = 0;
    while (i < prefix) : (i += 1) {
        out[w] = LineDiff{
            kind: .equal,
            value: selectEqualLineValue(actual[i], expected[i], opts.check_comma_disparity),
        };
        w += 1;
    }
    let ri: usize = 0;
    while (ri < mid_n) : (ri += 1) {
        out[w] = rev_list.items[mid_n - 1 - ri];
        w += 1;
    }
    i = 0;
    while (i < suffix) : (i += 1) {
        const ai = actual.len - suffix + i;
        const ei = expected.len - suffix + i;
        out[w] = LineDiff{
            kind: .equal,
            value: selectEqualLineValue(actual[ai], expected[ei], opts.check_comma_disparity),
        };
        w += 1;
    }
    return LineDiffResult{ items: out, allocator: allocator };
}

/// Computes the Myers O(ND) shortest edit script between two byte slices with an explicit
/// `max_edit_steps` upper bound and 16-lane SIMD common prefix/suffix trimming.
pub fn diffBytesWithMaxSteps(
    allocator: std.mem.Allocator,
    actual: []const u8,
    expected: []const u8,
    max_edit_steps: usize,
): ByteDiffResult ! DiffError {
    if (actual.len > MAX_DIFF_LEN || expected.len > MAX_DIFF_LEN) return error.InputsTooLarge;
    const total_full = actual.len + expected.len;
    if (total_full > MAX_DIFF_LEN) return error.InputsTooLarge;

    if (total_full == 0) {
        let empty: []ByteDiff = [];
        return ByteDiffResult{ items: empty, allocator: allocator };
    }

    // 1. 16-lane SIMD common prefix & suffix trimming
    const prefix = simdCommonPrefixLen(actual, expected);
    const suffix = simdCommonSuffixLen(actual[prefix..], expected[prefix..]);
    const mid_actual = actual[prefix .. actual.len - suffix];
    const mid_expected = expected[prefix .. expected.len - suffix];

    if (mid_actual.len == 0 || mid_expected.len == 0) {
        const out_len = prefix + mid_actual.len + mid_expected.len + suffix;
        let out = allocator.alloc(ByteDiff, out_len) catch return error.OutOfMemory;
        let w: usize = 0;
        let i: usize = 0;
        while (i < prefix) : (i += 1) {
            out[w] = ByteDiff{ kind: .equal, value: actual[i] };
            w += 1;
        }
        i = 0;
        while (i < mid_actual.len) : (i += 1) {
            out[w] = ByteDiff{ kind: .insert, value: mid_actual[i] };
            w += 1;
        }
        i = 0;
        while (i < mid_expected.len) : (i += 1) {
            out[w] = ByteDiff{ kind: .delete, value: mid_expected[i] };
            w += 1;
        }
        i = 0;
        while (i < suffix) : (i += 1) {
            out[w] = ByteDiff{ kind: .equal, value: actual[actual.len - suffix + i] };
            w += 1;
        }
        return ByteDiffResult{ items: out, allocator: allocator };
    }

    // 2. Myers O(ND) on trimmed middle slices with edit-step guard
    const total = mid_actual.len + mid_expected.len;
    const effective_max = if (max_edit_steps == 0) DEFAULT_MAX_EDIT_STEPS else max_edit_steps;
    const step_limit = if (total < effective_max) total else effective_max;
    const graph_size = (2 * step_limit) + 1;
    if (graph_size > MAX_DIFF_LEN) return error.DiffTooLarge;

    let graph = allocator.alloc(u32, graph_size) catch return error.OutOfMemory;
    defer allocator.free(graph);
    @memset(graph, 0);

    let trace_frames = allocator.alloc([]u32, step_limit + 1) catch return error.OutOfMemory;
    let trace_len: usize = 0;
    defer {
        let ti: usize = 0;
        while (ti < trace_len) : (ti += 1) {
            allocator.free(trace_frames[ti]);
        }
        allocator.free(trace_frames);
    }

    const max_i: i64 = i64(step_limit);
    let d_usize: usize = 0;
    let found = false;
    while (d_usize <= step_limit) : (d_usize += 1) {
        const frame = allocator.dupe(u32, graph) catch return error.OutOfMemory;
        trace_frames[trace_len] = frame;
        trace_len += 1;

        const d: i64 = i64(d_usize);
        let k: i64 = 0 - d;
        while (k <= d) : (k += 2) {
            const offset: usize = usize(k + max_i);
            let x: usize = 0;
            if (k == (0 - d) || (k != d && graph[offset - 1] < graph[offset + 1])) {
                x = usize(graph[offset + 1]);
            } else {
                x = usize(graph[offset - 1]) + 1;
            }
            let y: usize = usize(i64(x) - k);
            while (x < mid_actual.len && y < mid_expected.len && mid_actual[x] == mid_expected[y]) {
                x += 1;
                y += 1;
            }
            graph[offset] = u32(x);
            if (x >= mid_actual.len && y >= mid_expected.len) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        return error.DiffTooLarge;
    }

    let rev_list: std.ArrayList(ByteDiff) = .empty;
    defer rev_list.deinit(allocator);

    let x_i: i64 = i64(mid_actual.len);
    let y_i: i64 = i64(mid_expected.len);
    let level: usize = trace_len;
    while (level > 0) {
        level -= 1;
        const g = trace_frames[level];
        const k: i64 = x_i - y_i;
        const off: usize = usize(k + max_i);
        const lv_i: i64 = i64(level);
        const prev_k: i64 = if (k == (0 - lv_i) || (k != lv_i && g[off - 1] < g[off + 1]))
            k + 1
        else
            k - 1;
        const prev_x: i64 = i64(g[usize(prev_k + max_i)]);
        const prev_y: i64 = prev_x - prev_k;

        while (x_i > prev_x && y_i > prev_y) {
            rev_list.append(allocator, ByteDiff{ kind: .equal, value: mid_actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
            x_i -= 1;
            y_i -= 1;
        }
        if (level > 0) {
            if (x_i > prev_x) {
                rev_list.append(allocator, ByteDiff{ kind: .insert, value: mid_actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
                x_i -= 1;
            } else {
                rev_list.append(allocator, ByteDiff{ kind: .delete, value: mid_expected[usize(y_i - 1)] }) catch return error.OutOfMemory;
                y_i -= 1;
            }
        }
    }

    const mid_n = rev_list.items.len;
    const out_len = prefix + mid_n + suffix;
    let out = allocator.alloc(ByteDiff, out_len) catch return error.OutOfMemory;
    let w: usize = 0;
    let i: usize = 0;
    while (i < prefix) : (i += 1) {
        out[w] = ByteDiff{ kind: .equal, value: actual[i] };
        w += 1;
    }
    let ri: usize = 0;
    while (ri < mid_n) : (ri += 1) {
        out[w] = rev_list.items[mid_n - 1 - ri];
        w += 1;
    }
    i = 0;
    while (i < suffix) : (i += 1) {
        out[w] = ByteDiff{ kind: .equal, value: actual[actual.len - suffix + i] };
        w += 1;
    }
    return ByteDiffResult{ items: out, allocator: allocator };
}

/// Computes the Myers O(ND) shortest edit script between two byte slices.
pub fn diffBytes(
    allocator: std.mem.Allocator,
    actual: []const u8,
    expected: []const u8,
): ByteDiffResult ! DiffError {
    return try diffBytesWithMaxSteps(allocator, actual, expected, DEFAULT_MAX_EDIT_STEPS);
}

/// Formats line diff items into a readable `+`/`-`/` ` prefixed unified block.
pub fn formatLineDiff(allocator: std.mem.Allocator, diffs: []const LineDiff): ([]u8) ! DiffError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(allocator);
    for (d of diffs) {
        buf.append(allocator, d.kind.prefixByte()) catch return error.OutOfMemory;
        buf.append(allocator, ' ') catch return error.OutOfMemory;
        buf.appendSlice(allocator, d.value) catch return error.OutOfMemory;
        buf.append(allocator, '\n') catch return error.OutOfMemory;
    }
    return buf.toOwnedSlice(allocator) catch return error.OutOfMemory;
}
