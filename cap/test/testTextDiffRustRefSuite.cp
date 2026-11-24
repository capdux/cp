// Reference-hardened test suite against mitsuhiko/similar (Apache-2.0) & Unified Diff specification
// Covers Myers SES boundaries & step-limit guards, Unified Diff generate/parse/apply roundtrip,
// malformed patch rejection, 16-lane @Vector(16, u8) SIMD differential checks, and 1,600 deterministic PRNG fuzz cases.
const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_MAX_EDIT_STEPS,
    DiffError,
    DiffKind,
    DiffOptions,
    simdBytesEqual,
    scalarBytesEqual,
    simdCommonPrefixLen,
    scalarCommonPrefixLen,
    simdCommonSuffixLen,
    scalarCommonSuffixLen,
    simdFindNewline,
    scalarFindNewline,
    areLinesEqual,
    splitLines,
    diffLines,
    diffBytes,
    diffBytesWithMaxSteps,
} from "../../lib/text/myersDiff.cp";
import {
    DiffHunkHeader,
    DiffLineKind,
    PatchApplyStatus,
    UnifiedDiffError,
    parseHunkHeader,
    parseDiffOp,
    applyUnifiedPatch,
    applyUnifiedDiff,
    generateUnifiedDiff,
} from "../../lib/text/unifiedDiff.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn verifyRoundtrip(alloc: std.mem.Allocator, old_text: string, new_text: string, ctx: usize): bool {
    const patch = generateUnifiedDiff(alloc, old_text, new_text, "a/file.txt", "b/file.txt", ctx) catch return false;
    defer if (patch.len > 0) alloc.free(patch);

    const applied = applyUnifiedDiff(alloc, old_text, patch) catch return false;
    defer if (applied.len > 0) alloc.free(applied);

    return simdBytesEqual(applied, new_text);
}

fn checkMyersBoundaries(alloc: std.mem.Allocator): u32 {
    // 1. Empty vs empty, empty vs single, single vs empty (bytes & lines)
    {
        let r0 = diffBytes(alloc, "", "") catch return 1;
        if (r0.len() != 0 || !r0.isIdentical()) return 2;

        let r1 = diffBytes(alloc, "a", "") catch return 3;
        if (r1.editDistance() != 1 || r1.items[0].kind != .insert || r1.items[0].value != 'a') return 4;

        let r2 = diffBytes(alloc, "", "a") catch return 5;
        if (r2.editDistance() != 1 || r2.items[0].kind != .delete || r2.items[0].value != 'a') return 6;
    }

    // 2. Identical & completely different
    {
        let same = diffBytes(alloc, "abcdefghijklmnopqrstuvwxyz0123456789", "abcdefghijklmnopqrstuvwxyz0123456789") catch return 7;
        if (!same.isIdentical() || same.len() != 36) return 8;

        let disj = diffBytes(alloc, "AAAA", "BBBB") catch return 9;
        if (disj.editDistance() != 8) return 10;
    }

    // 3. Classic Myers paper vector: ABCABBA vs CBABAC (optimal edit distance = 5)
    {
        let classic = diffBytes(alloc, "ABCABBA", "CBABAC") catch return 11;
        if (classic.editDistance() != 5) return 12;

        // Reconstruct actual & expected from ByteDiff items
        let rec_a: std.ArrayList(u8) = .empty;
        defer rec_a.deinit(alloc);
        let rec_e: std.ArrayList(u8) = .empty;
        defer rec_e.deinit(alloc);
        for (it of classic.items) {
            if (it.kind == .equal) {
                rec_a.append(alloc, it.value) catch return 13;
                rec_e.append(alloc, it.value) catch return 14;
            } else if (it.kind == .insert) {
                rec_a.append(alloc, it.value) catch return 15;
            } else {
                rec_e.append(alloc, it.value) catch return 16;
            }
        }
        if (!simdBytesEqual(rec_a.items, "ABCABBA")) return 17;
        if (!simdBytesEqual(rec_e.items, "CBABAC")) return 18;
    }

    // 4. Common prefix/suffix trimming with large shared bookends + small middle edit
    {
        let a_buf: [128]u8 = [0; 128];
        let b_buf: [128]u8 = [0; 128];
        let i: usize = 0;
        while (i < 128) : (i += 1) {
            a_buf[i] = @as(u8, 'a') + @as(u8, @intCast(i % 26));
            b_buf[i] = a_buf[i];
        }
        b_buf[64] = 'Z';
        b_buf[65] = 'Y';
        let trimmed = diffBytesWithMaxSteps(alloc, a_buf[0..], b_buf[0..], 8) catch return 19;
        if (trimmed.editDistance() != 4) return 20;
    }

    // 5. Max edit steps guard prevents OOM / runaway search on large divergent inputs
    {
        let a_big: [64]u8 = ['A'; 64];
        let b_big: [64]u8 = ['B'; 64];
        let got_too_large = false;
        * = diffBytesWithMaxSteps(alloc, a_big[0..], b_big[0..], 10) catch |e| {
            if (e == DiffError.DiffTooLarge) got_too_large = true;
        };
        if (!got_too_large) return 21;

        let la: [20]string = ["old"; 20];
        let lb: [20]string = ["new"; 20];
        let got_line_too_large = false;
        * = diffLines(alloc, la[0..], lb[0..], DiffOptions{ max_edit_steps: 5 }) catch |e| {
            if (e == DiffError.DiffTooLarge) got_line_too_large = true;
        };
        if (!got_line_too_large) return 22;
    }

    // 6. Comma disparity & splitLines
    {
        if (!areLinesEqual("foo,", "foo", true)) return 23;
        if (areLinesEqual("foo,", "foo", false)) return 24;
        const lines = splitLines(alloc, "first\nsecond\nthird") catch return 25;
        defer alloc.free(lines);
        if (lines.len != 3) return 26;
        if (!simdBytesEqual(lines[0], "first") || !simdBytesEqual(lines[2], "third")) return 27;
    }

    return 0;
}

fn checkUnifiedDiffRoundtripAndGuards(alloc: std.mem.Allocator): u32 {
    // 1. Roundtrip matrix: empty, identical, insert, delete, no-trailing-newline, CRLF/LF mix, multi-hunk
    if (!verifyRoundtrip(alloc, "", "", 3)) return 1;
    if (!verifyRoundtrip(alloc, "", "hello\n", 3)) return 2;
    if (!verifyRoundtrip(alloc, "", "hello", 3)) return 3;
    if (!verifyRoundtrip(alloc, "hello\n", "", 3)) return 4;
    if (!verifyRoundtrip(alloc, "hello", "", 3)) return 5;
    if (!verifyRoundtrip(alloc, "same\nlines\n", "same\nlines\n", 3)) return 6;
    if (!verifyRoundtrip(alloc, "a\n", "a", 3)) return 7;
    if (!verifyRoundtrip(alloc, "a", "a\n", 3)) return 8;
    if (!verifyRoundtrip(alloc, "alpha\nbeta", "alpha\ngamma", 3)) return 9;
    if (!verifyRoundtrip(alloc, "alpha\r\nbeta\ngamma\r\n", "alpha\r\nBETA\r\ngamma\r\n", 1)) return 10;
    if (!verifyRoundtrip(alloc, "line1\r\nline2\r\n", "line1\nline2\r\n", 3)) return 11;

    const multi_old =
        "L01\nL02\nL03\nL04\nL05\nL06\nL07\nL08\nL09\nL10\n" ++
        "L11\nL12\nL13\nL14\nL15\nL16\nL17\nL18\nL19\nL20\n";
    const multi_new =
        "L01\nL02_mod\nL03\nL04\nL05\nL06\nL07\nL08\nL09\nL10\n" ++
        "L11\nL12\nL13\nL14\nL15\nL16\nL17\nL18_a\nL18_b\nL20\n";
    if (!verifyRoundtrip(alloc, multi_old, multi_new, 0)) return 12;
    if (!verifyRoundtrip(alloc, multi_old, multi_new, 2)) return 13;
    if (!verifyRoundtrip(alloc, multi_old, multi_new, 3)) return 14;

    // 2. Strict hunk header validation
    if (parseHunkHeader("@@ -1,3 +1,4 @@ fn main()") == null) return 15;
    if (parseHunkHeader("@@ -0,0 +1,2 @@") == null) return 16;
    if (parseHunkHeader("@@ -1,2 +0,0 @@") == null) return 17;
    if (parseHunkHeader("@@ -5 +5,2 @@") == null) return 18;

    // Reject 0-line empty hunk, 0 start with count > 0, u32 overflow, bad syntax
    const bad_headers: [8]string = [
        "@@ -1,0 +1,0 @@",
        "@@ -0,1 +1,1 @@",
        "@@ -1,1 +0,1 @@",
        "@@ -4294967295,1 +1,1 @@",
        "@@ -4294967296,1 +1,1 @@",
        "@@ -4294967295,0 +1,1 @@",
        "@@ -1,2 +1,2 @@garbage",
        "@@ -1,2000000 +1,2 @@",
    ];
    for (bh of bad_headers) {
        if (parseHunkHeader(bh) != null) return 19;
    }

    // 3. Reject malformed diff op prefixes
    if (parseDiffOp("") != null) return 20;
    if (parseDiffOp("Xinvalid") != null) return 21;
    if (parseDiffOp(" context") == null) return 22;
    if (parseDiffOp("-removed") == null) return 23;
    if (parseDiffOp("+added") == null) return 24;

    // 4. Reject truncated hunk, context mismatch, bad prefix, and protect out_buf on BufferTooSmall
    const trunc_patch = "@@ -1,2 +1,2 @@\n line1\n";
    let got_trunc = false;
    * = applyUnifiedDiff(alloc, "line1\nline2\n", trunc_patch) catch |e| {
        if (e == UnifiedDiffError.InvalidHunk) got_trunc = true;
    };
    if (!got_trunc) return 25;

    const mismatch_patch = "@@ -1,2 +1,2 @@\n WRONG\n-line2\n+new2\n";
    let got_mismatch = false;
    * = applyUnifiedDiff(alloc, "line1\nline2\n", mismatch_patch) catch |e| {
        if (e == UnifiedDiffError.ContextMismatch) got_mismatch = true;
    };
    if (!got_mismatch) return 26;

    const bad_prefix_patch = "@@ -1,1 +1,1 @@\n!bad_prefix\n";
    let got_bad_prefix = false;
    * = applyUnifiedDiff(alloc, "line1\n", bad_prefix_patch) catch |e| {
        if (e == UnifiedDiffError.InvalidHunk) got_bad_prefix = true;
    };
    if (!got_bad_prefix) return 27;

    let small_buf: [4]u8 = [0xEE; 4];
    let small_slice: []u8 = small_buf[0..];
    const small_res = applyUnifiedPatch("a\n", "@@ -1,1 +1,1 @@\n-a\n+long_replacement_line\n", small_slice);
    if (small_res.status != PatchApplyStatus.BufferTooSmall || small_res.written_bytes != 0) return 28;
    if (small_buf[0] != 0xEE || small_buf[3] != 0xEE) return 29;

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x4449_4646_5349_4d44);
    let a = std.mem.zeroes([260]u8);
    let b = std.mem.zeroes([260]u8);
    prng.fillBytes(&a);

    // Ensure non-newline bytes for controlled newline placement
    let idx: usize = 0;
    while (idx < 260) : (idx += 1) {
        if (a[idx] == '\n') a[idx] = 'x';
    }

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        @memcpy(b[0..len], a[0..len]);

        // Identical slices
        if (simdBytesEqual(a[0..len], b[0..len]) != scalarBytesEqual(a[0..len], b[0..len])) return 1;
        if (simdCommonPrefixLen(a[0..len], b[0..len]) != scalarCommonPrefixLen(a[0..len], b[0..len])) return 2;
        if (simdCommonSuffixLen(a[0..len], b[0..len]) != scalarCommonSuffixLen(a[0..len], b[0..len])) return 3;

        if (len > 0) {
            const positions: [3]usize = [ 0, len / 2, len - 1 ];
            for (pos of positions) {
                b[pos] = b[pos] ^ 0x01;
                if (simdBytesEqual(a[0..len], b[0..len]) != scalarBytesEqual(a[0..len], b[0..len])) return 4;
                if (simdCommonPrefixLen(a[0..len], b[0..len]) != scalarCommonPrefixLen(a[0..len], b[0..len])) return 5;
                if (simdCommonSuffixLen(a[0..len], b[0..len]) != scalarCommonSuffixLen(a[0..len], b[0..len])) return 6;
                b[pos] = b[pos] ^ 0x01;
            }

            // Newline scanning parity across start offsets and injected '\n'
            b[len - 1] = '\n';
            if (len > 17) b[17] = '\n';
            let start: usize = 0;
            while (start <= len) : (start += 1) {
                const s1 = simdFindNewline(b[0..len], start);
                const s2 = scalarFindNewline(b[0..len], start);
                if (s1 != s2) return 7;
            }
        }
    }

    // Mismatched lengths
    if (simdBytesEqual(a[0..32], a[0..31]) != scalarBytesEqual(a[0..32], a[0..31])) return 8;
    if (simdCommonPrefixLen(a[0..64], a[0..33]) != scalarCommonPrefixLen(a[0..64], a[0..33])) return 9;
    if (simdCommonSuffixLen(a[0..64], a[0..33]) != scalarCommonSuffixLen(a[0..64], a[0..33])) return 10;

    return 0;
}

fn buildRandomText(alloc: std.mem.Allocator, prng: *XorShift64): []u8 {
    const line_count = @as(usize, @intCast(prng.next() % 14));
    let out: std.ArrayList(u8) = .empty;
    const vocab: [8]string = [ "", "alpha", "beta", "gamma", "delta", "foo bar", "line_with_16_chars!", "end" ];
    let i: usize = 0;
    while (i < line_count) : (i += 1) {
        const pick = @as(usize, @intCast(prng.next() % 8));
        out.appendSlice(alloc, vocab[pick]) catch unreachable;
        const is_last = (i + 1 == line_count);
        const mode = prng.next() % 5;
        if (!is_last || mode != 0) {
            if (mode == 1) {
                out.append(alloc, '\r') catch unreachable;
            }
            out.append(alloc, '\n') catch unreachable;
        }
    }
    return out.toOwnedSlice(alloc) catch unreachable;
}

fn checkDeterministicFuzz(alloc: std.mem.Allocator): u32 {
    let prng = XorShift64.init(0x2026_1123_4449_4646);

    // 1. 800 random text-pair diff -> apply roundtrips (must satisfy apply(old, diff(old, new)) == new)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const old_t = buildRandomText(alloc, *prng);
        defer if (old_t.len > 0) alloc.free(old_t);
        const new_t = buildRandomText(alloc, *prng);
        defer if (new_t.len > 0) alloc.free(new_t);

        const ctx = @as(usize, @intCast(prng.next() % 4));
        if (!verifyRoundtrip(alloc, old_t, new_t, ctx)) return 1;
    }

    // 2. 800 malformed / mutated unified diff inputs (0 panic / 0 OOB / 0 hang)
    const base_orig = "line1\nline2\nline3\nline4\n";
    const base_patch =
        "--- a/f\n+++ b/f\n@@ -1,4 +1,4 @@\n line1\n-line2\n+line2_new\n line3\n line4\n";
    let rand_buf = std.mem.zeroes([160]u8);

    iter = 0;
    while (iter < 800) : (iter += 1) {
        if (iter % 2 == 0) {
            const cut_len = @as(usize, @intCast(prng.next() % (base_patch.len + 1)));
            let mutated = alloc.dupe(u8, base_patch[0..cut_len]) catch return 2;
            defer alloc.free(mutated);
            if (mutated.len > 0) {
                const flip_idx = @as(usize, @intCast(prng.next() % mutated.len));
                mutated[flip_idx] = prng.nextByte();
            }
            * = parseHunkHeader(mutated);
            * = parseDiffOp(mutated);
            const applied = applyUnifiedDiff(alloc, base_orig, mutated) catch {
                continue;
            };
            if (applied.len > 0) alloc.free(applied);
        } else {
            const n = @as(usize, @intCast(prng.next() % 160));
            prng.fillBytes(rand_buf[0..n]);
            const slice = rand_buf[0..n];
            * = parseHunkHeader(slice);
            * = parseDiffOp(slice);
            const applied = applyUnifiedDiff(alloc, base_orig, slice) catch {
                continue;
            };
            if (applied.len > 0) alloc.free(applied);
        }
    }

    return 0;
}

fn main() {
    test("1. Myers SES boundaries, prefix/suffix trimming, and max_edit_steps guard", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkMyersBoundaries(alloc), 0 as u32);
    });

    test("2. Unified Diff generate/parse/apply roundtrip, CRLF/no-NL, and malformed patch guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkUnifiedDiffRoundtripAndGuards(alloc), 0 as u32);
    });

    test("3. 16-lane @Vector(16, u8) SIMD prefix/suffix/newline/equal 0-diff parity", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (1,600 diff->apply roundtrips & malformed unified patches)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkDeterministicFuzz(alloc), 0 as u32);
    });
}
