// Reference-hardened test suite against cessen/ropey (MIT) & crossbeam-rs/crossbeam (MIT OR Apache-2.0)
// Covers Rope strict bounds/empty ops/split/concat/AVL & newline conservation vs flat byte buffer model,
// SkipList strict monotonic ordering/upsert overwrite/non-existent remove/level clamping/range &
// predecessor/successor/floor queries vs sorted reference model, 16-lane @Vector(16, u8) SIMD vs scalar
// 0-diff parity across all 256 byte values & 0..=257 lengths, and 1,600 deterministic PRNG fuzz cases
// (800 Rope edits + 800 SkipList ops).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Rope,
    RopeError,
    simdRopeCountNewlines,
    scalarRopeCountNewlines,
    simdRopeFindNewline,
    scalarRopeFindNewline,
} from "../../lib/ds/rope.cp";
import {
    SKIPLIST_MAX_LEVEL,
    SkipListError,
    SkipList,
} from "../../lib/ds/skiplist.cp";

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
}

fn checkTest1RopeBoundsSplitConcat(): u32 {
    const alloc = std.heap.page_allocator;
    let r = Rope.init(alloc);
    defer r.deinit();

    // Empty rope invariants
    if (r.len() != 0 || r.lineCount() != 1 || !r.verifyInvariants()) return 1;

    // Out-of-bounds guards
    let caught = false;
    * = r.charAt(0) catch |e| {
        caught = (e == RopeError.OutOfBounds);
    };
    if (!caught) return 2;

    caught = false;
    * = r.insert(1, "x") catch |e| {
        caught = (e == @as(anyerror, error.OutOfBounds));
    };
    if (!caught) return 3;

    caught = false;
    * = r.delete(2, 1) catch |e| {
        caught = (e == RopeError.InvalidRange);
    };
    if (!caught) return 4;

    caught = false;
    * = r.delete(0, 1) catch |e| {
        caught = (e == RopeError.OutOfBounds);
    };
    if (!caught) return 5;

    // Empty insert & empty delete are safe no-ops
    r.insert(0, "") catch return 6;
    r.delete(0, 0) catch return 7;
    if (r.len() != 0 || !r.verifyInvariants()) return 8;

    // Multi-leaf build, splitAt, concat, and lineAt verification
    let big: [260]u8 = [0; 260];
    let i: usize = 0;
    while (i < big.len) : (i += 1) {
        big[i] = if ((i % 25) == 24) '\n' else ((0x41 as u8) + @as(u8, @truncate(i % 26)));
    }
    r.insert(0, big[0..]) catch return 9;
    if (r.len() != 260 || r.lineCount() != scalarRopeCountNewlines(big[0..]) + 1 || !r.verifyInvariants()) return 10;

    // Out-of-bounds splitAt
    caught = false;
    * = r.splitAt(261) catch |e| {
        caught = (e == @as(anyerror, error.OutOfBounds));
    };
    if (!caught) return 11;

    // Split at middle (137), check both halves, then concat back
    let right_half = r.splitAt(137) catch return 12;
    defer right_half.deinit();
    if (r.len() != 137 || right_half.len() != 123) return 13;
    if (!r.verifyInvariants() || !right_half.verifyInvariants()) return 14;

    const lhs_s = r.toString(alloc) catch return 15;
    defer alloc.free(lhs_s);
    const rhs_s = right_half.toString(alloc) catch return 16;
    defer alloc.free(rhs_s);
    if (!std.mem.eql(u8, lhs_s, big[0..137])) return 17;
    if (!std.mem.eql(u8, rhs_s, big[137..260])) return 18;

    r.concat(&right_half) catch return 19;
    if (right_half.len() != 0 || r.len() != 260 || !r.verifyInvariants()) return 20;
    const rejoined = r.toString(alloc) catch return 21;
    defer alloc.free(rejoined);
    if (!std.mem.eql(u8, rejoined, big[0..])) return 22;

    // Verify lineAt(0) and lineAt(1)
    const line0 = r.lineAt(alloc, 0) catch return 23;
    defer alloc.free(line0);
    if (!std.mem.eql(u8, line0, big[0..24])) return 24;

    const line1 = r.lineAt(alloc, 1) catch return 25;
    defer alloc.free(line1);
    if (!std.mem.eql(u8, line1, big[25..49])) return 26;

    // Delete spanning multiple leaves down to empty
    r.delete(50, 220) catch return 27;
    if (r.len() != 90 || !r.verifyInvariants()) return 28;
    r.delete(0, 90) catch return 29;
    if (r.len() != 0 || r.lineCount() != 1 || !r.verifyInvariants()) return 30;

    return 0;
}

fn expectSkipListInvalidMaxLevel(alloc: std.mem.Allocator, max_lvl: usize): bool {
    let sl = SkipList<u64>.initWithSeed(alloc, max_lvl, 1) catch |e| {
        return e == SkipListError.InvalidMaxLevel;
    };
    sl.deinit();
    return false;
}

fn expectSkipListInvalidRange(sl: *SkipList<u64>, alloc: std.mem.Allocator, lo: []const u8, hi: []const u8): bool {
    const scan = sl.rangeScan(alloc, lo, hi) catch |e| {
        return e == SkipListError.InvalidRange;
    };
    alloc.free(scan);
    return false;
}

fn checkTest2SkipListBoundsAndQueries(): u32 {
    const alloc = std.heap.page_allocator;

    // 2a. Max level rejection in initWithSeed & clamping in initClamped
    if (!expectSkipListInvalidMaxLevel(alloc, 0)) return 1;
    if (!expectSkipListInvalidMaxLevel(alloc, SKIPLIST_MAX_LEVEL + 1)) return 2;

    let clamped_lo = SkipList<u64>.initClamped(alloc, 0, 7) catch return 3;
    defer clamped_lo.deinit();
    let clamped_hi = SkipList<u64>.initClamped(alloc, 999, 7) catch return 4;
    defer clamped_hi.deinit();
    if (!clamped_lo.verifyInvariants() || !clamped_hi.verifyInvariants()) return 5;

    // 2b. Upsert, duplicate overwrite, non-existent delete, predecessor/successor/floor/upperBound/rangeScan
    let sl = SkipList<u64>.initWithSeed(alloc, 8, 42) catch return 6;
    defer sl.deinit();

    if (sl.removeKey("missing") != null) return 7;
    if (sl.predecessor("b") != null || sl.successor("b") != null || sl.floor("b") != null) return 8;

    const ins1 = sl.upsert("b", 20) catch return 9;
    const ins2 = sl.upsert("d", 40) catch return 10;
    const ins3 = sl.upsert("f", 60) catch return 11;
    const ovw = sl.upsert("d", 44) catch return 12;
    if (!ins1 || !ins2 || !ins3 || ovw) return 13;
    if (sl.len() != 3 || !sl.verifyInvariants()) return 14;
    if ((sl.get("d") orelse 0) != 44) return 15;

    // Predecessor / successor / floor / upperBound boundary checks
    if (sl.predecessor("b") != null) return 16;
    const p_d = sl.predecessor("d") orelse return 17;
    if (!std.mem.eql(u8, p_d.key, "b") || p_d.value != 20) return 18;
    const p_e = sl.predecessor("e") orelse return 19;
    if (!std.mem.eql(u8, p_e.key, "d") || p_e.value != 44) return 20;

    const fl_d = sl.floor("d") orelse return 21;
    if (!std.mem.eql(u8, fl_d.key, "d") || fl_d.value != 44) return 22;
    const fl_e = sl.floor("e") orelse return 23;
    if (!std.mem.eql(u8, fl_e.key, "d")) return 24;
    if (sl.floor("a") != null) return 25;

    const s_d = sl.successor("d") orelse return 26;
    if (!std.mem.eql(u8, s_d.key, "f") || s_d.value != 60) return 27;
    if (sl.successor("f") != null) return 28;
    const ub_b = sl.upperBound("b") orelse return 29;
    if (!std.mem.eql(u8, ub_b.key, "d")) return 30;

    // InvalidRange guard
    if (!expectSkipListInvalidRange(&sl, alloc, "z", "a")) return 31;

    // Valid rangeScan [b, f) -> b, d
    const scan = sl.rangeScan(alloc, "b", "f") catch return 32;
    defer alloc.free(scan);
    if (scan.len != 2 || !std.mem.eql(u8, scan[0].key, "b") || !std.mem.eql(u8, scan[1].key, "d")) return 33;

    // Safe remove existing & non-existing
    if ((sl.removeKey("d") orelse 0) != 44) return 34;
    if (sl.removeKey("d") != null) return 35;
    if (sl.len() != 2 || !sl.verifyInvariants()) return 36;

    return 0;
}

fn checkTest3SimdDifferential(): u32 {
    // 3a. All 256 byte values (0x00..=0xFF) single-byte & multi-lane verification
    let all256: [256]u8 = [0; 256];
    let b: usize = 0;
    while (b < 256) : (b += 1) {
        all256[b] = @as(u8, @truncate(b));
    }
    if (simdRopeCountNewlines(all256[0..]) != 1) return 1;
    if (scalarRopeCountNewlines(all256[0..]) != 1) return 2;
    if ((simdRopeFindNewline(all256[0..]) orelse 999) != 0x0A) return 3;
    if ((scalarRopeFindNewline(all256[0..]) orelse 999) != 0x0A) return 4;

    // 3b. Every length 0..=257 with deterministic PRNG patterns (varying newline density)
    let rng = XorShift64.init(0x524F5045_53494D44);
    let buf: [257]u8 = [0; 257];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const r = rng.nextByte();
            // ~12% newlines, plus arbitrary bytes across full 0..255 range
            buf[i] = if ((r & 7) == 0) '\n' else r;
        }
        const c_simd = simdRopeCountNewlines(buf[0..len]);
        const c_scalar = scalarRopeCountNewlines(buf[0..len]);
        if (c_simd != c_scalar) return 5;

        const f_simd = simdRopeFindNewline(buf[0..len]);
        const f_scalar = scalarRopeFindNewline(buf[0..len]);
        if (fs := f_simd) {
            const fc = f_scalar orelse return 7;
            if (fs != fc) return 8;
        } else {
            if (f_scalar != null) return 6;
        }
    }

    // All-newline buffer of length 257
    let all_nl: [257]u8 = ['\n'; 257];
    if (simdRopeCountNewlines(all_nl[0..]) != 257) return 9;
    if ((simdRopeFindNewline(all_nl[0..]) orelse 999) != 0) return 10;

    // Zero-newline buffer of length 257
    let no_nl: [257]u8 = ['a'; 257];
    if (simdRopeCountNewlines(no_nl[0..]) != 0) return 11;
    if (simdRopeFindNewline(no_nl[0..]) != null) return 12;

    return 0;
}

fn checkTest4Rope800PrngFuzz(): u32 {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64.init(0x524F5045_00000800);
    let r = Rope.init(alloc);
    defer r.deinit();
    let model = std.ArrayList(u8).empty;
    defer model.deinit(alloc);

    let tmp_ins: [48]u8 = [0; 48];
    let step: usize = 0;
    while (step < 800) : (step += 1) {
        const op = rng.next() % 10;
        const cur_len = model.items.len;

        if (op < 5 || cur_len == 0) {
            // Insert 0..=40 bytes at random pos in 0..=cur_len
            const pos = @as(usize, @truncate(rng.next())) % (cur_len + 1);
            const ins_len = @as(usize, @truncate(rng.next())) % 41;
            let k: usize = 0;
            while (k < ins_len) : (k += 1) {
                const b = rng.nextByte();
                tmp_ins[k] = if ((b & 7) == 0) '\n' else ((0x20 as u8) + (b % (90 as u8)));
            }
            // Cap total length at ~1500 bytes to exercise both growth and shrink cycles
            if (cur_len + ins_len <= 1500) {
                r.insert(pos, tmp_ins[0..ins_len]) catch return 1;
                model.insertSlice(alloc, pos, tmp_ins[0..ins_len]) catch return 2;
            }
        } else if (op < 8) {
            // Delete random range [start..end)
            const a = @as(usize, @truncate(rng.next())) % (cur_len + 1);
            const b = @as(usize, @truncate(rng.next())) % (cur_len + 1);
            const start = if (a <= b) a else b;
            const end = if (a <= b) b else a;
            r.delete(start, end) catch return 3;
            const del_len = end - start;
            if (del_len > 0) {
                std.mem.copyForwards(u8, model.items[start .. cur_len - del_len], model.items[end..cur_len]);
                model.shrinkRetainingCapacity(cur_len - del_len);
            }
        } else {
            // Split at random pos and re-concat (verifies split+concat conservation)
            const pos = @as(usize, @truncate(rng.next())) % (cur_len + 1);
            let rhs = r.splitAt(pos) catch return 4;
            defer rhs.deinit();
            if (r.len() != pos || rhs.len() != cur_len - pos) return 5;
            if (!r.verifyInvariants() || !rhs.verifyInvariants()) return 6;
            r.concat(&rhs) catch return 7;
        }

        // Strict conservation & differential checks every single step
        if (r.len() != model.items.len) return 8;
        if (r.lineCount() != scalarRopeCountNewlines(model.items) + 1) return 9;
        if (!r.verifyInvariants()) return 10;

        if (model.items.len > 0) {
            const probe = @as(usize, @truncate(rng.next())) % model.items.len;
            const ch = r.charAt(probe) catch return 11;
            if (ch != model.items[probe]) return 12;
        }

        // Full byte-for-byte comparison every 16 steps and on final step
        if ((step & 15) == 0 || step == 799) {
            const got = r.toString(alloc) catch return 13;
            defer alloc.free(got);
            if (!std.mem.eql(u8, got, model.items)) return 14;
        }
    }
    return 0;
}

fn checkTest5SkipList800PrngFuzz(): u32 {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64.init(0x534B4950_00000800);
    let sl = SkipList<u64>.initClamped(alloc, 12, 0x12345678) catch return 1;
    defer sl.deinit();

    // Sorted reference model over 64 fixed 4-byte decimal keys ("k00" .. "k63")
    let present: [64]bool = [false; 64];
    let vals: [64]u64 = [0; 64];
    let key_buf: [3]u8 = ['k', '0', '0'];
    let key_buf2: [3]u8 = ['k', '0', '0'];

    let step: usize = 0;
    while (step < 800) : (step += 1) {
        const k_idx = @as(usize, @truncate(rng.next())) % 64;
        key_buf[1] = (0x30 as u8) + @as(u8, @truncate(k_idx / 10));
        key_buf[2] = (0x30 as u8) + @as(u8, @truncate(k_idx % 10));
        const op = rng.next() % 10;

        if (op < 5) {
            // Upsert
            const v = rng.next();
            const is_new = sl.upsert(key_buf[0..], v) catch return 2;
            if (is_new != !present[k_idx]) return 3;
            present[k_idx] = true;
            vals[k_idx] = v;
        } else if (op < 8) {
            // Remove
            const removed = sl.removeKey(key_buf[0..]);
            if (present[k_idx]) {
                if ((removed orelse 0) != vals[k_idx]) return 4;
                present[k_idx] = false;
                vals[k_idx] = 0;
            } else {
                if (removed != null) return 5;
            }
        } else {
            // Point + predecessor + successor + lowerBound + rangeScan differential check
            const got = sl.get(key_buf[0..]);
            if (present[k_idx]) {
                if ((got orelse 0) != vals[k_idx]) return 6;
            } else {
                if (got != null) return 7;
            }

            // Expected predecessor (greatest j < k_idx with present[j])
            let exp_pred: usize? = null;
            let j: usize = k_idx;
            while (j > 0) {
                j -= 1;
                if (present[j]) {
                    exp_pred = j;
                    break;
                }
            }
            const got_pred = sl.predecessor(key_buf[0..]);
            if (ep := exp_pred) {
                const gp = got_pred orelse return 8;
                if (gp.value != vals[ep]) return 9;
            } else {
                if (got_pred != null) return 10;
            }

            // Expected successor (least j > k_idx with present[j])
            let exp_succ: usize? = null;
            let s: usize = k_idx + 1;
            while (s < 64) : (s += 1) {
                if (present[s]) {
                    exp_succ = s;
                    break;
                }
            }
            const got_succ = sl.successor(key_buf[0..]);
            if (es := exp_succ) {
                const gs = got_succ orelse return 11;
                if (gs.value != vals[es]) return 12;
            } else {
                if (got_succ != null) return 13;
            }

            // Range scan [lo, hi)
            const k2 = @as(usize, @truncate(rng.next())) % 64;
            const lo = if (k_idx <= k2) k_idx else k2;
            const hi = if (k_idx <= k2) k2 else k_idx;
            key_buf[1] = (0x30 as u8) + @as(u8, @truncate(lo / 10));
            key_buf[2] = (0x30 as u8) + @as(u8, @truncate(lo % 10));
            key_buf2[1] = (0x30 as u8) + @as(u8, @truncate(hi / 10));
            key_buf2[2] = (0x30 as u8) + @as(u8, @truncate(hi % 10));

            const scan = sl.rangeScan(alloc, key_buf[0..], key_buf2[0..]) catch return 14;
            defer alloc.free(scan);
            let exp_cnt: usize = 0;
            let r_i: usize = lo;
            while (r_i < hi) : (r_i += 1) {
                if (present[r_i]) {
                    if (exp_cnt >= scan.len || scan[exp_cnt].value != vals[r_i]) return 15;
                    exp_cnt += 1;
                }
            }
            if (scan.len != exp_cnt) return 16;
        }

        // Verify strict monotonic ordering & structural invariants every 16 steps and at end
        if ((step & 15) == 0 || step == 799) {
            let expected_len: usize = 0;
            let c: usize = 0;
            while (c < 64) : (c += 1) {
                if (present[c]) expected_len += 1;
            }
            if (sl.len() != expected_len) return 17;
            if (!sl.verifyInvariants()) return 18;
        }
    }
    return 0;
}

fn main() {
    test("1. Rope Bounds, Empty Ops, SplitAt/Concat & Line/Byte Conservation", () => {
        expect_eq(checkTest1RopeBoundsSplitConcat(), 0 as u32);
    });

    test("2. SkipList Level Clamping, Overwrite, Safe Remove & Boundary Queries", () => {
        expect_eq(checkTest2SkipListBoundsAndQueries(), 0 as u32);
    });

    test("3. 16-Lane @Vector(16, u8) SIMD vs Scalar 0-Diff Parity (256 Bytes & 0..=257 Lengths)", () => {
        expect_eq(checkTest3SimdDifferential(), 0 as u32);
    });

    test("4. Rope 800-Step PRNG Fuzzing vs Flat Byte Buffer Model", () => {
        expect_eq(checkTest4Rope800PrngFuzz(), 0 as u32);
    });

    test("5. SkipList 800-Step PRNG Fuzzing vs Sorted Reference Model", () => {
        expect_eq(checkTest5SkipList800PrngFuzz(), 0 as u32);
    });
}
