// Reference-hardened test suite against RoaringBitmap/roaring-rs (MIT/Apache-2.0) & RoaringFormatSpec
// Covers:
//   1. Boundary values (0, 65535, 65536, u32::MAX), empty & full 65536-elem chunk, 4095<->4096<->4097 oscillation
//   2. Set algebra laws & reference-model differential testing (union/intersect/diff/xor/rank/select/isSubset/isDisjoint)
//   3. Portable Roaring Format (12346 / 12347) roundtrip & malformed bitstream rejection
//   4. 4-lane @Vector(4, u64) SIMD vs scalar 0-diff parity (AND / OR / XOR / ANDNOT / Popcount)
//   5. 1,600 deterministic PRNG fuzz cases (800 random ops vs ref model + 800 malformed binary streams)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ROARING_SERIAL_COOKIE_NO_RUNCONTAINER,
    ROARING_SERIAL_COOKIE_RUNCONTAINER,
    RoaringError,
    RoaringBitmap,
    scalarBitmapPopcount,
    simdBitmapPopcount,
    scalarBitmapAnd,
    simdBitmapAnd,
    scalarBitmapOr,
    simdBitmapOr,
    scalarBitmapXor,
    simdBitmapXor,
    scalarBitmapAndNot,
    simdBitmapAndNot,
} from "../../lib/ds/roaring.cp";

fn nextPrng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn tryDeserializePortable(alloc: std.mem.Allocator, bytes: []const u8): RoaringBitmap? {
    return RoaringBitmap.deserializePortable(alloc, bytes) catch return null;
}

fn tryDeserializeRoar(alloc: std.mem.Allocator, bytes: []const u8): RoaringBitmap? {
    return RoaringBitmap.deserialize(alloc, bytes) catch return null;
}

fn putU16LE(buf: []u8, off: usize, v: u16) {
    buf[off] = @truncate(v & 0xFF);
    buf[off + 1] = @truncate((v >> 8) & 0xFF);
}

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = @truncate(v & 0xFF);
    buf[off + 1] = @truncate((v >> 8) & 0xFF);
    buf[off + 2] = @truncate((v >> 16) & 0xFF);
    buf[off + 3] = @truncate((v >> 24) & 0xFF);
}

fn runBoundariesAndThresholdOscillation(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Empty bitmap invariants
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();
    expect_eq(bm.isEmpty(), true);
    expect_eq(bm.cardinality(), 0 as u64);
    expect_eq(bm.containerCount(), 0 as usize);
    expect_eq(bm.minimum() == null, true);
    expect_eq(bm.maximum() == null, true);
    expect_eq(bm.select(0) == null, true);
    expect_eq(bm.rank(0), 0 as u64);
    expect_eq(bm.rank(0xFFFFFFFF), 0 as u64);

    // 2. Extreme boundary values: 0, 65535, 65536, 0xFFFF0000, u32::MAX (0xFFFFFFFF)
    expect_eq(bm.add(0)?, true);
    expect_eq(bm.add(65535)?, true);
    expect_eq(bm.add(65536)?, true);
    expect_eq(bm.add(0xFFFF0000)?, true);
    expect_eq(bm.add(0xFFFFFFFF)?, true);
    expect_eq(bm.add(0xFFFFFFFF)?, false); // duplicate

    expect_eq(bm.cardinality(), 5 as u64);
    expect_eq(bm.containerCount(), 3 as usize);
    expect_eq(bm.minimum().?, 0 as u32);
    expect_eq(bm.maximum().?, 0xFFFFFFFF as u32);
    expect_eq(bm.select(0).?, 0 as u32);
    expect_eq(bm.select(1).?, 65535 as u32);
    expect_eq(bm.select(2).?, 65536 as u32);
    expect_eq(bm.select(3).?, 0xFFFF0000 as u32);
    expect_eq(bm.select(4).?, 0xFFFFFFFF as u32);
    expect_eq(bm.select(5) == null, true);
    expect_eq(bm.rank(0), 1 as u64);
    expect_eq(bm.rank(65535), 2 as u64);
    expect_eq(bm.rank(65536), 3 as u64);
    expect_eq(bm.rank(0xFFFFFFFF), 5 as u64);

    // 3. Full single chunk (all 65536 elements in key 0) + inclusive range ending at u32::MAX
    let full = RoaringBitmap.init(alloc);
    defer full.deinit();
    full.addRangeInclusive(0, 65535)?;
    expect_eq(full.cardinality(), 65536 as u64);
    expect_eq(full.containerCount(), 1 as usize);
    expect_eq(full.isContainerBitset(0), true);
    expect_eq(full.minimum().?, 0 as u32);
    expect_eq(full.maximum().?, 65535 as u32);
    expect_eq(full.select(0).?, 0 as u32);
    expect_eq(full.select(32768).?, 32768 as u32);
    expect_eq(full.select(65535).?, 65535 as u32);
    expect_eq(full.select(65536) == null, true);

    full.addRangeInclusive(0xFFFFFFFE, 0xFFFFFFFF)?;
    expect_eq(full.cardinality(), 65538 as u64);
    expect_eq(full.maximum().?, 0xFFFFFFFF as u32);
    expect_eq(full.select(65537).?, 0xFFFFFFFF as u32);

    // 4. 4095 <-> 4096 <-> 4097 threshold oscillation (repeated upgrade & downgrade)
    let osc = RoaringBitmap.init(alloc);
    defer osc.deinit();
    let i: u32 = 0;
    while (i < 4095) : (i += 1) {
        expect_eq(osc.add(i * 3)?, true);
    }
    expect_eq(osc.cardinality(), 4095 as u64);
    expect_eq(osc.isContainerBitset(0), false);

    let round: u32 = 0;
    while (round < 8) : (round += 1) {
        // 4095 -> 4096 (still Array)
        expect_eq(osc.add(4095 * 3)?, true);
        expect_eq(osc.cardinality(), 4096 as u64);
        expect_eq(osc.isContainerBitset(0), false);

        // 4096 -> 4097 (upgrades to Bitset)
        expect_eq(osc.add(4096 * 3)?, true);
        expect_eq(osc.cardinality(), 4097 as u64);
        expect_eq(osc.isContainerBitset(0), true);
        expect_eq(osc.contains(4096 * 3), true);
        expect_eq(osc.select(4096).?, 4096 * 3);
        expect_eq(osc.rank(4096 * 3), 4097 as u64);

        // 4097 -> 4096 (downgrades back to Array)
        expect_eq(osc.removeVal(4096 * 3), true);
        expect_eq(osc.cardinality(), 4096 as u64);
        expect_eq(osc.isContainerBitset(0), false);
        expect_eq(osc.contains(4096 * 3), false);
        expect_eq(osc.select(4095).?, 4095 * 3);

        // 4096 -> 4095 (remains Array)
        expect_eq(osc.removeVal(4095 * 3), true);
        expect_eq(osc.cardinality(), 4095 as u64);
        expect_eq(osc.isContainerBitset(0), false);
    }
}

fn runSetAlgebraAndRefModelDiff(): !void {
    const alloc = std.heap.page_allocator;

    // Universe of 512 elements spread across 2 chunks (key 0: 0..255, key 1: 65536..65791)
    // Plus dense Bitset container in key 2 (131072..141072) to test both Array & Bitset ops.
    let ref_a: [512]bool = [false; 512];
    let ref_b: [512]bool = [false; 512];

    let a = RoaringBitmap.init(alloc);
    defer a.deinit();
    let b = RoaringBitmap.init(alloc);
    defer b.deinit();

    let rng: u64 = 0x123456789ABCDEF1;
    let i: usize = 0;
    while (i < 512) : (i += 1) {
        const val: u32 = if (i < 256) @intCast(i) else (65536 + @as(u32, @intCast(i - 256)));
        const r = nextPrng(&rng);
        if ((r & 1) != 0) {
            ref_a[i] = true;
            * = a.add(val)?;
        }
        if ((r & 2) != 0) {
            ref_b[i] = true;
            * = b.add(val)?;
        }
    }

    let u_ab = a.bitOr(alloc, b)?;
    defer u_ab.deinit();
    let i_ab = a.bitAnd(alloc, b)?;
    defer i_ab.deinit();
    let d_ab = a.bitAndNot(alloc, b)?;
    defer d_ab.deinit();
    let x_ab = a.bitXor(alloc, b)?;
    defer x_ab.deinit();

    // Verify against reference boolean array model
    let exp_u: u64 = 0;
    let exp_i: u64 = 0;
    let exp_d: u64 = 0;
    let exp_x: u64 = 0;
    i = 0;
    while (i < 512) : (i += 1) {
        const val: u32 = if (i < 256) @intCast(i) else (65536 + @as(u32, @intCast(i - 256)));
        const in_u = ref_a[i] || ref_b[i];
        const in_i = ref_a[i] && ref_b[i];
        const in_d = ref_a[i] && !ref_b[i];
        const in_x = ref_a[i] != ref_b[i];
        if (in_u) exp_u += 1;
        if (in_i) exp_i += 1;
        if (in_d) exp_d += 1;
        if (in_x) exp_x += 1;

        expect_eq(u_ab.contains(val), in_u);
        expect_eq(i_ab.contains(val), in_i);
        expect_eq(d_ab.contains(val), in_d);
        expect_eq(x_ab.contains(val), in_x);
        expect_eq(u_ab.rank(val), exp_u);
    }
    expect_eq(u_ab.cardinality(), exp_u);
    expect_eq(i_ab.cardinality(), exp_i);
    expect_eq(d_ab.cardinality(), exp_d);
    expect_eq(x_ab.cardinality(), exp_x);
    expect_eq(a.andCardinality(b), exp_i);

    // Verify select() matches toSlice() 1-to-1
    const u_slice = u_ab.toSlice(alloc)?;
    defer alloc.free(u_slice);
    expect_eq(u_slice.len, @as(usize, @intCast(exp_u)));
    let si: usize = 0;
    while (si < u_slice.len) : (si += 1) {
        expect_eq(u_ab.select(@intCast(si)).?, u_slice[si]);
    }

    // Algebraic laws:
    // 1. (A & B) ⊆ A ⊆ (A | B)
    expect_eq(i_ab.isSubset(a), true);
    expect_eq(i_ab.isSubset(b), true);
    expect_eq(a.isSubset(u_ab), true);
    expect_eq(b.isSubset(u_ab), true);
    // 2. (A \ B) and B are disjoint
    expect_eq(d_ab.isDisjoint(b), true);
    // 3. A ^ B == (A | B) \ (A & B)
    let x_check = u_ab.bitAndNot(alloc, i_ab)?;
    defer x_check.deinit();
    expect_eq(x_ab.equals(x_check), true);
    // 4. Commutativity: A | B == B | A, A & B == B & A, A ^ B == B ^ A
    let u_ba = b.bitOr(alloc, a)?;
    defer u_ba.deinit();
    let i_ba = b.bitAnd(alloc, a)?;
    defer i_ba.deinit();
    let x_ba = b.bitXor(alloc, a)?;
    defer x_ba.deinit();
    expect_eq(u_ab.equals(u_ba), true);
    expect_eq(i_ab.equals(i_ba), true);
    expect_eq(x_ab.equals(x_ba), true);

    // Dense Bitset vs Bitset subset & algebra
    let dense1 = RoaringBitmap.init(alloc);
    defer dense1.deinit();
    let dense2 = RoaringBitmap.init(alloc);
    defer dense2.deinit();
    dense1.addRange(131072, 138000)?;
    dense2.addRange(131072, 141072)?;
    expect_eq(dense1.isSubset(dense2), true);
    expect_eq(dense2.isSubset(dense1), false);
    expect_eq(dense1.isDisjoint(dense2), false);
}

fn runPortableFormatAndMalformedDefense(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Empty Portable roundtrip (COOKIE_NO_RUNCONTAINER = 12346)
    let empty = RoaringBitmap.init(alloc);
    defer empty.deinit();
    const empty_p = empty.serializePortable(alloc)?;
    defer alloc.free(empty_p);
    expect_eq(empty_p.len, 8 as usize);
    let empty_dec = RoaringBitmap.deserializePortable(alloc, empty_p)?;
    defer empty_dec.deinit();
    expect_eq(empty_dec.isEmpty(), true);

    // 2. Mixed Array + Bitset + u32::MAX Portable roundtrip
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();
    * = bm.add(0)?;
    * = bm.add(42)?;
    bm.addRange(65536, 71000)?; // Bitset container (5464 elems)
    * = bm.add(0xFFFFFFFF)?;

    const pbytes = bm.serializePortable(alloc)?;
    defer alloc.free(pbytes);
    let restored = RoaringBitmap.deserializePortable(alloc, pbytes)?;
    defer restored.deinit();
    expect_eq(restored.equals(bm), true);
    expect_eq(restored.cardinality(), bm.cardinality());
    expect_eq(restored.isContainerBitset(1), true);
    expect_eq(restored.contains(0xFFFFFFFF), true);

    // 3. Official COOKIE_RUNCONTAINER (12347) decoding (< 4 containers -> no offset header)
    // 1 container: key=2, card=100 (stored as 99), run_bitset=0x01, num_runs=2: [10..=59] (50) & [100..=149] (50)
    let run_stream: [19]u8 = [0; 19];
    // cookie_raw = 12347 | ((size - 1) << 16) = 12347 (size = 1)
    putU32LE(&run_stream, 0, ROARING_SERIAL_COOKIE_RUNCONTAINER);
    run_stream[4] = 0x01; // container 0 is a RunContainer
    putU16LE(&run_stream, 5, 2); // key = 2
    putU16LE(&run_stream, 7, 99); // card - 1 = 99 -> card = 100
    putU16LE(&run_stream, 9, 2); // num_runs = 2
    putU16LE(&run_stream, 11, 10); // run0 start = 10
    putU16LE(&run_stream, 13, 49); // run0 len = 49 -> [10..=59]
    putU16LE(&run_stream, 15, 100); // run1 start = 100
    putU16LE(&run_stream, 17, 49); // run1 len = 49 -> [100..=149]

    let run_bm = RoaringBitmap.deserializePortable(alloc, &run_stream)?;
    defer run_bm.deinit();
    expect_eq(run_bm.cardinality(), 100 as u64);
    expect_eq(run_bm.contains((2 << 16) | 10), true);
    expect_eq(run_bm.contains((2 << 16) | 59), true);
    expect_eq(run_bm.contains((2 << 16) | 60), false);
    expect_eq(run_bm.contains((2 << 16) | 100), true);
    expect_eq(run_bm.contains((2 << 16) | 149), true);

    // 4. Malformed Portable bitstreams must return InvalidSerializedData (no panic / no OOB)
    // 4a. Truncated header (< 4 bytes, < 8 bytes)
    expect_eq(tryDeserializePortable(alloc, pbytes[0..3]) == null, true);
    expect_eq(tryDeserializePortable(alloc, pbytes[0..6]) == null, true);
    // 4b. Truncated payload
    expect_eq(tryDeserializePortable(alloc, pbytes[0..pbytes.len - 1]) == null, true);
    // 4c. Bad cookie
    let bad_cookie = alloc.dupe(u8, pbytes) catch unreachable;
    defer alloc.free(bad_cookie);
    putU32LE(bad_cookie, 0, 99999);
    expect_eq(tryDeserializePortable(alloc, bad_cookie) == null, true);

    // 4d. Duplicate / out-of-order keys in descriptive header
    let bad_key = alloc.dupe(u8, pbytes) catch unreachable;
    defer alloc.free(bad_key);
    putU16LE(bad_key, 12, 0); // container 1 key set to 0 == container 0 key
    expect_eq(tryDeserializePortable(alloc, bad_key) == null, true);

    // 4e. Corrupted offset header (out of bounds / mismatched)
    let bad_off = alloc.dupe(u8, pbytes) catch unreachable;
    defer alloc.free(bad_off);
    putU32LE(bad_off, 8 + 3 * 4, 0x7FFFFFFF);
    expect_eq(tryDeserializePortable(alloc, bad_off) == null, true);

    // 4f. Non-monotonic array container elements (swap elements 0 and 42 in container 0)
    let bad_mono = alloc.dupe(u8, pbytes) catch unreachable;
    defer alloc.free(bad_mono);
    const data_start: usize = 8 + 3 * 4 + 3 * 4;
    putU16LE(bad_mono, data_start, 42);
    putU16LE(bad_mono, data_start + 2, 0);
    expect_eq(tryDeserializePortable(alloc, bad_mono) == null, true);

    // 4g. Bitmap cardinality mismatch (flip 1 bit in bitset container 1)
    let bad_card = alloc.dupe(u8, pbytes) catch unreachable;
    defer alloc.free(bad_card);
    bad_card[data_start + 4] ^= 0x80;
    expect_eq(tryDeserializePortable(alloc, bad_card) == null, true);

    // 4h. Trailing garbage bytes
    let trailing = alloc.alloc(u8, pbytes.len + 2) catch unreachable;
    defer alloc.free(trailing);
    @memcpy(trailing[0..pbytes.len], pbytes);
    trailing[pbytes.len] = 0xAA;
    trailing[pbytes.len + 1] = 0xBB;
    expect_eq(tryDeserializePortable(alloc, trailing) == null, true);

    // 4i. Malformed run container: overlapping runs
    let bad_run: [19]u8 = run_stream;
    putU16LE(&bad_run, 15, 55); // overlaps [10..=59]
    expect_eq(tryDeserializePortable(alloc, &bad_run) == null, true);
}

fn runSimdScalarZeroDiffParity(): !void {
    const lens: [10]usize = [0, 1, 2, 3, 4, 5, 7, 8, 15, 1024];
    let a: [1024]u64 = [0; 1024];
    let b: [1024]u64 = [0; 1024];
    let out_sc: [1024]u64 = [0; 1024];
    let out_sm: [1024]u64 = [0; 1024];

    let rng: u64 = 0x243F6A8885A308D3;
    let i: usize = 0;
    while (i < 1024) : (i += 1) {
        a[i] = nextPrng(&rng);
        b[i] = nextPrng(&rng);
    }

    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        expect_eq(simdBitmapPopcount(a[0..n]), scalarBitmapPopcount(a[0..n]));
        expect_eq(simdBitmapPopcount(b[0..n]), scalarBitmapPopcount(b[0..n]));

        const c_and_sc = scalarBitmapAnd(out_sc[0..n], a[0..n], b[0..n]);
        const c_and_sm = simdBitmapAnd(out_sm[0..n], a[0..n], b[0..n]);
        expect_eq(c_and_sm, c_and_sc);
        let k: usize = 0;
        while (k < n) : (k += 1) {
            expect_eq(out_sm[k], out_sc[k]);
        }

        const c_or_sc = scalarBitmapOr(out_sc[0..n], a[0..n], b[0..n]);
        const c_or_sm = simdBitmapOr(out_sm[0..n], a[0..n], b[0..n]);
        expect_eq(c_or_sm, c_or_sc);
        k = 0;
        while (k < n) : (k += 1) {
            expect_eq(out_sm[k], out_sc[k]);
        }

        const c_xor_sc = scalarBitmapXor(out_sc[0..n], a[0..n], b[0..n]);
        const c_xor_sm = simdBitmapXor(out_sm[0..n], a[0..n], b[0..n]);
        expect_eq(c_xor_sm, c_xor_sc);
        k = 0;
        while (k < n) : (k += 1) {
            expect_eq(out_sm[k], out_sc[k]);
        }

        const c_sub_sc = scalarBitmapAndNot(out_sc[0..n], a[0..n], b[0..n]);
        const c_sub_sm = simdBitmapAndNot(out_sm[0..n], a[0..n], b[0..n]);
        expect_eq(c_sub_sm, c_sub_sc);
        k = 0;
        while (k < n) : (k += 1) {
            expect_eq(out_sm[k], out_sc[k]);
        }
    }
}

fn runDeterministicFuzz1600(): !void {
    const alloc = std.heap.page_allocator;
    let rng: u64 = 0x1E3779B97F4A7C15;

    // 1. 800 deterministic random operations against boolean reference model (universe 1024 across 2 chunks)
    let ref_set: [1024]bool = [false; 1024];
    let ref_card: u64 = 0;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    let step: usize = 0;
    while (step < 800) : (step += 1) {
        const r = nextPrng(&rng);
        const op = r % 4;
        const idx: usize = @intCast((r >> 8) % 1024);
        const val: u32 = if (idx < 512) @intCast(idx) else (65536 + @as(u32, @intCast(idx - 512)));

        if (op == 0 || op == 1) {
            const added = bm.add(val)?;
            expect_eq(added, !ref_set[idx]);
            if (!ref_set[idx]) {
                ref_set[idx] = true;
                ref_card += 1;
            }
        } else if (op == 2) {
            const removed = bm.removeVal(val);
            expect_eq(removed, ref_set[idx]);
            if (ref_set[idx]) {
                ref_set[idx] = false;
                ref_card -= 1;
            }
        } else {
            expect_eq(bm.contains(val), ref_set[idx]);
        }
        expect_eq(bm.cardinality(), ref_card);

        if ((step % 100) == 99) {
            const enc = bm.serializePortable(alloc)?;
            defer alloc.free(enc);
            let dec_bm = RoaringBitmap.deserializePortable(alloc, enc)?;
            defer dec_bm.deinit();
            expect_eq(dec_bm.equals(bm), true);
        }
    }

    // 2. 800 deterministic malformed binary streams fed to deserializePortable & deserialize (0 panic / 0 OOB)
    let fbuf: [192]u8 = [0; 192];
    let f: usize = 0;
    while (f < 800) : (f += 1) {
        const len: usize = @intCast(nextPrng(&rng) % 192);
        let k: usize = 0;
        while (k < len) : (k += 1) {
            fbuf[k] = @intCast(nextPrng(&rng) & 0xFF);
        }
        // Inject valid magic/cookies in 50% of cases to probe deeper parser paths
        if (len >= 8) {
            const mode = f % 4;
            if (mode == 0) {
                putU32LE(fbuf[0..len], 0, ROARING_SERIAL_COOKIE_NO_RUNCONTAINER);
                putU32LE(fbuf[0..len], 4, @intCast(nextPrng(&rng) % 12));
            } else if (mode == 1) {
                const sz: u32 = @intCast(nextPrng(&rng) % 8);
                putU32LE(fbuf[0..len], 0, ROARING_SERIAL_COOKIE_RUNCONTAINER | (sz << 16));
            } else if (mode == 2) {
                fbuf[0] = 'R';
                fbuf[1] = 'O';
                fbuf[2] = 'A';
                fbuf[3] = 'R';
                putU16LE(fbuf[0..len], 4, 1);
            }
        }
        if (parsed_p := tryDeserializePortable(alloc, fbuf[0..len])) {
            let mut_p = parsed_p;
            mut_p.deinit();
        }
        if (parsed_r := tryDeserializeRoar(alloc, fbuf[0..len])) {
            let mut_r = parsed_r;
            mut_r.deinit();
        }
    }
}

fn main() {
    test("1. Roaring boundaries (0/65535/65536/u32::MAX), full chunk & 4095<->4096<->4097 oscillation", () => {
        runBoundariesAndThresholdOscillation()?;
    });

    test("2. Set algebra laws & reference-model differential testing (union/intersect/diff/xor/rank/select)", () => {
        runSetAlgebraAndRefModelDiff()?;
    });

    test("3. Portable Roaring Format (12346/12347) roundtrip & malformed bitstream rejection", () => {
        runPortableFormatAndMalformedDefense()?;
    });

    test("4. 4-lane @Vector(4, u64) SIMD vs scalar 0-diff parity (AND/OR/XOR/ANDNOT/Popcount)", () => {
        runSimdScalarZeroDiffParity()?;
    });

    test("5. 1,600 deterministic PRNG fuzz cases (800 ops vs ref model + 800 malformed bitstreams)", () => {
        runDeterministicFuzz1600()?;
    });
}
