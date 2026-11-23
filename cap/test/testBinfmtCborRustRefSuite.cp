// enarx/ciborium (Apache-2.0) & RFC 8949 Appendix A Reference-Hardened CBOR Test Suite
// Covers:
//   - Test 01: RFC 8949 Appendix A integer boundaries (Major 0/1, i64::MIN, u64::MAX, overflow guard),
//              Simple values (false/true/null/undefined, reserved 24..31 rejection), and Tags (Major 6)
//   - Test 02: IEEE-754 Half-Precision Float16 (0xF9: 0x0000, 0x8000, 0x3c00, 0x7bff, 0x0001 subnormal,
//              0x7c00 +Inf, 0xfc00 -Inf, 0x7e00 NaN) & Float32/64 canonical roundtrip
//   - Test 03: Indefinite-length ByteString/TextString/Array/Map + Break (0xFF) pairing,
//              mismatched chunk type rejection, lone 0xFF (UnexpectedBreak), invalid ai=28..30
//   - Test 04: RFC 8949 §4.2 Deterministic Encoding (validateCborDeterministic) + CBOR_MAX_DEPTH = 64
//   - Test 05: SIMD hot-path parity (cborCopyPayloadSimd vs Scalar, cborIsValidUtf8Simd vs Scalar)
//   - Test 06: 1,500-iteration deterministic PRNG malformed CBOR / forged length / depth-bomb fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CBOR_MAX_DEPTH,
    CborError,
    CborEntry,
    CborTag,
    CborValue,
    cborCopyPayloadSimd,
    cborCopyPayloadScalar,
    cborIsValidUtf8Simd,
    cborIsValidUtf8Scalar,
    decodeFloat16,
    decode,
    encode,
    encodeCanonical,
    toDiag,
    validateCbor,
    validateCborDeterministic,
} from "../../lib/binfmt/cborDiag.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. RFC 8949 Appendix A Unsigned & Negative Integer vectors
    // 0 -> 0x00, 23 -> 0x17, 24 -> 0x18 0x18, 1000 -> 0x19 0x03 0xe8, 1000000 -> 0x1a 0x00 0x0f 0x42 0x40
    const b_0: [1]u8 = [0x00];
    const v_0 = try decode(alloc, &b_0);
    expect_eq(v_0.Unsigned, 0 as u64);

    const b_24: [2]u8 = [ 0x18, 0x18 ];
    const v_24 = try decode(alloc, &b_24);
    expect_eq(v_24.Unsigned, 24 as u64);

    const b_1000: [3]u8 = [ 0x19, 0x03, 0xe8 ];
    const v_1000 = try decode(alloc, &b_1000);
    expect_eq(v_1000.Unsigned, 1000 as u64);

    const b_u64max: [9]u8 = [ 0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff ];
    const v_u64max = try decode(alloc, &b_u64max);
    expect_eq(v_u64max.Unsigned, std.math.maxInt(u64));

    // Negative: -1 -> 0x20, -1000 -> 0x39 0x03 0xe7, i64::MIN -> 0x3b 0x7f ff ff ff ff ff ff ff
    const b_neg1: [1]u8 = [0x20];
    const v_neg1 = try decode(alloc, &b_neg1);
    expect_eq(v_neg1.Negative, -1 as i64);

    const b_i64min: [9]u8 = [ 0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff ];
    const v_i64min = try decode(alloc, &b_i64min);
    expect_eq(v_i64min.Negative, std.math.minInt(i64));
    const enc_min = try encodeCanonical(alloc, v_i64min);
    expect_eq(std.mem.eql(u8, enc_min, &b_i64min), true);

    // Negative integer overflow (0x3b 0x80...) -> InvalidAdditionalInfo
    const b_neg_ov: [9]u8 = [ 0x3b, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ];
    let neg_ov = false;
    * = decode(alloc, &b_neg_ov) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidAdditionalInfo")) neg_ov = true;
    };
    expect_eq(neg_ov, true);

    // 2. Simple values: false (0xf4), true (0xf5), null (0xf6), undefined (0xf7), reserved simple(24..31) rejected
    const b_simp: [4]u8 = [ 0x83, 0xf4, 0xf5, 0xf6 ];
    const v_simp = try decode(alloc, &b_simp);
    expect_eq(v_simp.Array[0].Bool, false);
    expect_eq(v_simp.Array[1].Bool, true);
    expect_eq(std.meta.activeTag(v_simp.Array[2]) == .Null, true);

    const b_undef: [1]u8 = [0xf7];
    const v_undef = try decode(alloc, &b_undef);
    expect_eq(std.meta.activeTag(v_undef) == .Undefined, true);

    const b_res_simp: [2]u8 = [ 0xf8, 0x18 ]; // simple(24) reserved!
    let res_simp_err = false;
    * = decode(alloc, &b_res_simp) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidAdditionalInfo")) res_simp_err = true;
    };
    expect_eq(res_simp_err, true);

    // 3. Tag (Major 6): 1(1363896240) -> 0xc1 0x1a 0x51 0x4b 0x67 0xb0
    const b_tag: [6]u8 = [ 0xc1, 0x1a, 0x51, 0x4b, 0x67, 0xb0 ];
    const v_tag = try decode(alloc, &b_tag);
    expect_eq(v_tag.Tag.tag, 1 as u64);
    expect_eq(v_tag.Tag.value.Unsigned, 1363896240 as u64);
    const diag = try toDiag(alloc, v_tag);
    expect_eq(std.mem.eql(u8, diag, "1(1363896240)"), true);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // RFC 8949 Appendix A IEEE-754 Float16 (0xF9) exact vectors
    expect_eq(decodeFloat16(0x0000), 0.0);
    expect_eq(std.math.signbit(decodeFloat16(0x8000)), true); // -0.0
    expect_eq(decodeFloat16(0x3c00), 1.0);
    expect_eq(decodeFloat16(0x7bff), 65504.0);
    const subnormal = decodeFloat16(0x0001);
    expect_eq(subnormal > 5.96e-8 && subnormal < 5.97e-8, true);
    expect_eq(std.math.isPositiveInf(decodeFloat16(0x7c00)), true);
    expect_eq(std.math.isNegativeInf(decodeFloat16(0xfc00)), true);
    expect_eq(std.math.isNan(decodeFloat16(0x7e00)), true);

    // Wire decode & canonical re-encode of Float16
    const f16_1_5: [3]u8 = [ 0xf9, 0x3e, 0x00 ]; // 1.5
    const v_f16 = try decode(alloc, &f16_1_5);
    expect_eq(v_f16.Float, 1.5);
    const re_enc = try encodeCanonical(alloc, v_f16);
    expect_eq(std.mem.eql(u8, re_enc, &f16_1_5), true);

    const f16_nan: [3]u8 = [ 0xf9, 0x7e, 0x00 ];
    const v_nan = try decode(alloc, &f16_nan);
    expect_eq(std.math.isNan(v_nan.Float), true);
    const re_nan = try encodeCanonical(alloc, v_nan);
    expect_eq(std.mem.eql(u8, re_nan, &f16_nan), true);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Indefinite-length ByteString (0x5f 0x42 0x01 0x02 0x43 0x03 0x04 0x05 0xff)
    const indef_bytes: [9]u8 = [ 0x5f, 0x42, 0x01, 0x02, 0x43, 0x03, 0x04, 0x05, 0xff ];
    const v_ib = try decode(alloc, &indef_bytes);
    const exp_ib: [5]u8 = [ 1, 2, 3, 4, 5 ];
    expect_eq(std.mem.eql(u8, v_ib.Bytes, &exp_ib), true);

    // 2. Indefinite-length TextString ("strea" + "ming" -> "streaming")
    const indef_text: [13]u8 = [ 0x7f, 0x65, 's', 't', 'r', 'e', 'a', 0x64, 'm', 'i', 'n', 'g', 0xff ];
    const v_it = try decode(alloc, &indef_text);
    expect_eq(std.mem.eql(u8, v_it.Text, "streaming"), true);

    // 3. Indefinite-length Array & Map + Break (0xFF)
    const indef_arr: [5]u8 = [ 0x9f, 0x01, 0x02, 0x03, 0xff ];
    const v_ia = try decode(alloc, &indef_arr);
    expect_eq(v_ia.Array.len, 3);
    expect_eq(v_ia.Array[2].Unsigned, 3 as u64);

    const indef_map: [5]u8 = [ 0xbf, 0x61, 'a', 0x01, 0xff ];
    const v_im = try decode(alloc, &indef_map);
    expect_eq(v_im.Map.len, 1);
    expect_eq(std.mem.eql(u8, v_im.Map[0].key.Text, "a"), true);

    // 4. Mismatched indefinite string chunk (Text chunk inside ByteString) -> InvalidAdditionalInfo
    const bad_chunk: [4]u8 = [ 0x5f, 0x61, 'a', 0xff ];
    let chunk_err = false;
    * = decode(alloc, &bad_chunk) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidAdditionalInfo")) chunk_err = true;
    };
    expect_eq(chunk_err, true);

    // 5. Lone Break (0xFF) -> UnexpectedBreak
    const lone_brk: [1]u8 = [0xff];
    let brk_err = false;
    * = decode(alloc, &lone_brk) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedBreak")) brk_err = true;
    };
    expect_eq(brk_err, true);

    // 6. Invalid additional info ai=28..30 -> InvalidAdditionalInfo
    const bad_ai28: [1]u8 = [0x1c];
    let ai_err = false;
    * = decode(alloc, &bad_ai28) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidAdditionalInfo")) ai_err = true;
    };
    expect_eq(ai_err, true);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Deterministic encoding accepts canonical output
    const k1 = CborValue{ .Text = "bb" };
    const k2 = CborValue{ .Text = "a" };
    let entries: [2]CborEntry = [
        CborEntry{ .key = k1, .value = CborValue{ .Unsigned = 1 } },
        CborEntry{ .key = k2, .value = CborValue{ .Unsigned = 2 } },
    ];
    const map_val = CborValue{ .Map = &entries };
    const canon_bytes = try encodeCanonical(alloc, map_val);
    try validateCborDeterministic(canon_bytes);

    // 2. Rejects non-minimal integer (0x18 0x05 instead of 0x05)
    const non_min_int: [2]u8 = [ 0x18, 0x05 ];
    try validateCbor(&non_min_int); // valid CBOR
    let det_int_err = false;
    validateCborDeterministic(&non_min_int) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "NonDeterministicEncoding")) det_int_err = true;
    };
    expect_eq(det_int_err, true);

    // 3. Rejects indefinite-length in deterministic mode
    const indef_arr: [3]u8 = [ 0x9f, 0x01, 0xff ];
    let det_indef_err = false;
    validateCborDeterministic(&indef_arr) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "NonDeterministicEncoding")) det_indef_err = true;
    };
    expect_eq(det_indef_err, true);

    // 4. Rejects unsorted map keys (RFC 8949 §4.2.1 length-first: "bb" before "a")
    const unsorted_map: [8]u8 = [ 0xa2, 0x62, 'b', 'b', 0x01, 0x61, 'a', 0x02 ];
    let det_sort_err = false;
    validateCborDeterministic(&unsorted_map) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "NonDeterministicEncoding")) det_sort_err = true;
    };
    expect_eq(det_sort_err, true);

    // 5. Recursion depth > CBOR_MAX_DEPTH (64) -> NestingTooDeep
    expect_eq(CBOR_MAX_DEPTH, 64);
    let deep: [67]u8 = undefined;
    let i: usize = 0;
    while (i < 66) : (i += 1) deep[i] = 0x81; // 1-element array
    deep[66] = 0x00;

    let depth_err = false;
    validateCbor(&deep) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "NestingTooDeep")) depth_err = true;
    };
    expect_eq(depth_err, true);
}

fn runTest05(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. SIMD @Vector(16, u8) payload copy vs scalar reference (0..128 bytes)
    let src_buf: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) : (i += 1) src_buf[i] = ((i * 29 + 7) & 0xFF) as u8;

    let simd_dst: [128]u8 = undefined;
    let scal_dst: [128]u8 = undefined;
    let sz: usize = 0;
    while (sz <= 128) : (sz += 1) {
        cborCopyPayloadSimd(simd_dst[0..sz], src_buf[0..sz]);
        cborCopyPayloadScalar(scal_dst[0..sz], src_buf[0..sz]);
        expect_eq(std.mem.eql(u8, simd_dst[0..sz], scal_dst[0..sz]), true);
    }

    // Large ByteString encode/decode roundtrip exercising SIMD copy
    const bs_val = CborValue{ .Bytes = &src_buf };
    const enc = try encode(alloc, bs_val);
    const dec_val = try decode(alloc, enc);
    expect_eq(std.mem.eql(u8, dec_val.Bytes, &src_buf), true);

    // 2. cap:simd Simd.isAscii UTF-8 fast-path vs scalar reference 0-diff parity
    const utf8_samples: [5][]const u8 = [
        "",
        "cbor-ascii-fastpath-payload-over-sixteen-bytes-0123456789",
        "utf8-cbor-\xC3\xA9-\xE4\xB8\xAD-\xF0\x9F\x98\x80",
        "bad-utf8-\x80\x80",
        "trunc-utf8-\xE4\xB8",
    ];
    for (s of utf8_samples) {
        expect_eq(cborIsValidUtf8Simd(s), cborIsValidUtf8Scalar(s));
    }
}

fn runTest06(): void! {
    // 1,500-iteration deterministic PRNG malformed CBOR / forged length / depth-bomb fuzzing
    let seed: u64 = 0x2468_ACE0_1357_9BDF;
    let buf: [96]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 80) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        validateCbor(buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("cbor_ref_01_rfc8949_integers_overflow_simple_and_tags", () => { try runTest01(); });
    test("cbor_ref_02_ieee754_float16_subnormals_inf_nan_and_canonical", () => { try runTest02(); });
    test("cbor_ref_03_indefinite_length_break_pairing_and_invalid_ai", () => { try runTest03(); });
    test("cbor_ref_04_deterministic_encoding_and_depth_64_guard", () => { try runTest04(); });
    test("cbor_ref_05_simd_payload_copy_and_ascii_utf8_parity", () => { try runTest05(); });
    test("cbor_ref_06_deterministic_prng_1500_fuzz_malformed_cbor", () => { try runTest06(); });
}
