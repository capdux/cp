// Origin: Pure-CP Stdlib Test Suite (lib/pack/delta_varint.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc ZigZag Varint, Delta & Delta-of-Delta Codecs:
//   1. ZigZag i64 <-> u64 full-range bijection (0, -1, 1, -2, i64::MAX, i64::MIN) & Svarint round-trip
//   2. ULEB128 boundary values (0, 127, 128, 16383, 16384, u64::MAX) & exact encoded byte lengths
//   3. Malformed varint rejection: empty, truncated, >10 bytes & 10th byte > 0x01 overflow guard
//   4. Monotonic DocID Delta codec high compression ratio (>5x), lossless round-trip & non-monotonic rejection
//   5. Periodic jittered timestamp Delta-of-Delta codec (>5x compression) & lossless round-trip
//   6. Empty, 1-element & 2-element boundary round-trips for both Delta & Delta-of-Delta
//   7. Insufficient encode buffer, insufficient decode output slice & truncated stream guards

import { test, expect_eq } from "cap:test";
import {
    VARINT_MAX_U64_BYTES,
    VarintDecodeResult,
    SignedVarintDecodeResult,
    DeltaBlockStats,
    zigzagEncodeI64,
    zigzagDecodeU64,
    encodeUvarint64,
    decodeUvarint64,
    encodeSvarint64,
    decodeSvarint64,
    encodeDeltaU64,
    decodeDeltaU64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
    computeDeltaCompressionStats,
} from "cap:pack";

// ============================================================================
// Test 1: ZigZag Full-Range Bijection (incl. i64::MIN / i64::MAX) & Svarint
// ============================================================================
test("delta_varint: zigzag i64 bijection including extremes & svarint round-trip", () => {
    expect_eq(VARINT_MAX_U64_BYTES, 10);

    const i64_max: i64 = 9223372036854775807;
    const i64_min: i64 = -9223372036854775807 - 1;

    expect_eq(zigzagEncodeI64(0), 0);
    expect_eq(zigzagEncodeI64(-1), 1);
    expect_eq(zigzagEncodeI64(1), 2);
    expect_eq(zigzagEncodeI64(-2), 3);
    expect_eq(zigzagEncodeI64(2), 4);
    expect_eq(zigzagEncodeI64(i64_max), 0xFFFFFFFFFFFFFFFE);
    expect_eq(zigzagEncodeI64(i64_min), 0xFFFFFFFFFFFFFFFF);

    expect_eq(zigzagDecodeU64(0), 0);
    expect_eq(zigzagDecodeU64(1), -1);
    expect_eq(zigzagDecodeU64(2), 1);
    expect_eq(zigzagDecodeU64(3), -2);
    expect_eq(zigzagDecodeU64(0xFFFFFFFFFFFFFFFE), i64_max);
    expect_eq(zigzagDecodeU64(0xFFFFFFFFFFFFFFFF), i64_min);

    const samples: [7]i64 = [0, -1, 1, -64, 63, i64_max, i64_min];
    let buf: [16]u8 = [0; 16];
    let buf_s: []u8 = buf[0..];

    let i: usize = 0;
    while (i < 7) : (i += 1) {
        const n = encodeSvarint64(samples[i], buf_s) ?? return;
        const res: SignedVarintDecodeResult = decodeSvarint64(buf[0..n]);
        expect_eq(res.ok, true);
        expect_eq(res.value, samples[i]);
        expect_eq(res.bytes_read, n);
    }
});

// ============================================================================
// Test 2: ULEB128 Unsigned Varint Boundary Values & Exact Byte Counts
// ============================================================================
test("delta_varint: uvarint64 boundary values & exact encoded lengths", () => {
    const vals: [6]u64 = [0, 127, 128, 16383, 16384, 0xFFFFFFFFFFFFFFFF];
    const lens: [6]usize = [1, 1, 2, 2, 3, 10];

    let buf: [16]u8 = [0; 16];
    let buf_s: []u8 = buf[0..];

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const n = encodeUvarint64(vals[i], buf_s) ?? return;
        expect_eq(n, lens[i]);

        const res: VarintDecodeResult = decodeUvarint64(buf[0..n]);
        expect_eq(res.ok, true);
        expect_eq(res.value, vals[i]);
        expect_eq(res.bytes_read, n);
    }
});

// ============================================================================
// Test 3: Malformed Varint Rejection (Empty, Truncated, >10B, 10th Byte >0x01)
// ============================================================================
test("delta_varint: reject empty, truncated, >10-byte & 10th-byte overflow streams", () => {
    const empty: [0]u8 = [];
    expect_eq(decodeUvarint64(empty[0..]).ok, false);
    expect_eq(decodeSvarint64(empty[0..]).ok, false);

    // Truncated stream (MSB set on final byte)
    const trunc1: [1]u8 = [0x80];
    const trunc2: [2]u8 = [0xFF, 0x80];
    expect_eq(decodeUvarint64(trunc1[0..]).ok, false);
    expect_eq(decodeUvarint64(trunc2[0..]).ok, false);

    // 10th byte == 0x02 overflows u64 (bit 64 set)
    const ovf_10th: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
    expect_eq(decodeUvarint64(ovf_10th[0..]).ok, false);

    // 10th byte == 0x81 has continuation bit set (>10 bytes)
    const ovf_11: [11]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x00];
    expect_eq(decodeUvarint64(ovf_11[0..]).ok, false);

    // Valid max 10th byte == 0x01 decodes to 1 << 63
    const valid_10th: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01];
    const ok_res = decodeUvarint64(valid_10th[0..]);
    expect_eq(ok_res.ok, true);
    expect_eq(ok_res.bytes_read, 10);
    expect_eq(ok_res.value, 0x8000000000000000);
});

// ============================================================================
// Test 4: Monotonic DocID Delta Codec Compression & Non-Monotonic Rejection
// ============================================================================
test("delta_varint: monotonic DocID delta round-trip, >5x compression & non-monotonic guard", () => {
    let docs: [64]u64 = [0; 64];
    let cur: u64 = 10_000;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        // Gaps in [0..5] (includes duplicate equal values to verify non-decreasing)
        cur += (i % 6) as u64;
        docs[i] = cur;
    }

    let enc: [256]u8 = [0; 256];
    let enc_s: []u8 = enc[0..];
    const n_bytes = encodeDeltaU64(docs[0..], enc_s) ?? return;

    const stats: DeltaBlockStats = computeDeltaCompressionStats(64, n_bytes);
    expect_eq(stats.count, 64);
    expect_eq(stats.raw_bytes, 512);
    expect_eq(stats.encoded_bytes, n_bytes);
    expect_eq(stats.compression_ratio_x100 >= 500, true); // > 5.0x compression

    let decoded: [64]u64 = [0; 64];
    let dec_s: []u64 = decoded[0..];
    const dec_count = decodeDeltaU64(enc[0..n_bytes], dec_s) ?? return;
    expect_eq(dec_count, 64);

    let j: usize = 0;
    while (j < 64) : (j += 1) {
        expect_eq(decoded[j], docs[j]);
    }

    // Non-monotonic sequence MUST be rejected with null
    const bad_docs: [4]u64 = [10, 20, 19, 30];
    expect_eq(encodeDeltaU64(bad_docs[0..], enc_s) == null, true);
});

// ============================================================================
// Test 5: Periodic Jittered Timestamps Delta-of-Delta (>5x Compression)
// ============================================================================
test("delta_varint: periodic jittered timestamps delta-of-delta >5x compression", () => {
    let ts: [100]i64 = [0; 100];
    let base: i64 = 1_700_000_000_000;
    let i: usize = 0;
    while (i < 100) : (i += 1) {
        // 1000ms cadence with small deterministic jitter in {-2, -1, 0, +1, +2}
        const jitter: i64 = ((i % 5) as i64) - 2;
        ts[i] = base + ((i as i64) * 1000) + jitter;
    }

    let enc: [512]u8 = [0; 512];
    let enc_s: []u8 = enc[0..];
    const n_bytes = encodeDeltaOfDeltaI64(ts[0..], enc_s) ?? return;

    const stats = computeDeltaCompressionStats(100, n_bytes);
    expect_eq(stats.raw_bytes, 800);
    // ~108 bytes vs 800 raw bytes => ~740 (> 5.0x)
    expect_eq(stats.compression_ratio_x100 >= 500, true);

    let decoded: [100]i64 = [0; 100];
    let dec_s: []i64 = decoded[0..];
    const dec_count = decodeDeltaOfDeltaI64(enc[0..n_bytes], dec_s) ?? return;
    expect_eq(dec_count, 100);

    let j: usize = 0;
    while (j < 100) : (j += 1) {
        expect_eq(decoded[j], ts[j]);
    }
});

// ============================================================================
// Test 6: Empty, Single-Element & Two-Element Sequence Boundaries
// ============================================================================
test("delta_varint: empty, 1-element & 2-element boundary round-trips", () => {
    let enc: [64]u8 = [0; 64];
    let enc_s: []u8 = enc[0..];

    // Empty u64 & i64
    const empty_u: [0]u64 = [];
    const empty_i: [0]i64 = [];
    let out_u: [4]u64 = [0; 4];
    let out_i: [4]i64 = [0; 4];
    let out_u_s: []u64 = out_u[0..];
    let out_i_s: []i64 = out_i[0..];

    const nu0 = encodeDeltaU64(empty_u[0..], enc_s) ?? return;
    expect_eq(decodeDeltaU64(enc[0..nu0], out_u_s) ?? 99, 0);

    const ni0 = encodeDeltaOfDeltaI64(empty_i[0..], enc_s) ?? return;
    expect_eq(decodeDeltaOfDeltaI64(enc[0..ni0], out_i_s) ?? 99, 0);

    // 1-element u64 & negative i64
    const one_u: [1]u64 = [987654321];
    const nu1 = encodeDeltaU64(one_u[0..], enc_s) ?? return;
    expect_eq(decodeDeltaU64(enc[0..nu1], out_u_s) ?? 0, 1);
    expect_eq(out_u[0], 987654321);

    const one_i: [1]i64 = [-42000];
    const ni1 = encodeDeltaOfDeltaI64(one_i[0..], enc_s) ?? return;
    expect_eq(decodeDeltaOfDeltaI64(enc[0..ni1], out_i_s) ?? 0, 1);
    expect_eq(out_i[0], -42000);

    // 2-element i64 with negative delta
    const two_i: [2]i64 = [500, -300];
    const ni2 = encodeDeltaOfDeltaI64(two_i[0..], enc_s) ?? return;
    expect_eq(decodeDeltaOfDeltaI64(enc[0..ni2], out_i_s) ?? 0, 2);
    expect_eq(out_i[0], 500);
    expect_eq(out_i[1], -300);
});

// ============================================================================
// Test 7: Insufficient Encode Buffer, Small Decode Slice & Truncated Stream
// ============================================================================
test("delta_varint: buffer too small & truncated stream guards", () => {
    // 1-byte buffer cannot encode 128 (needs 2 bytes)
    let tiny_enc: [1]u8 = [0; 1];
    let tiny_s: []u8 = tiny_enc[0..];
    expect_eq(encodeUvarint64(128, tiny_s) == null, true);

    const docs: [4]u64 = [1000, 1005, 1010, 1015];
    let enc_small: [3]u8 = [0; 3];
    let enc_small_s: []u8 = enc_small[0..];
    expect_eq(encodeDeltaU64(docs[0..], enc_small_s) == null, true);

    let enc: [32]u8 = [0; 32];
    let enc_s: []u8 = enc[0..];
    const n_bytes = encodeDeltaU64(docs[0..], enc_s) ?? return;

    // Decode target slice too small (len 2 < count 4) -> null
    let small_out: [2]u64 = [0; 2];
    let small_out_s: []u64 = small_out[0..];
    expect_eq(decodeDeltaU64(enc[0..n_bytes], small_out_s) == null, true);

    // Truncated encoded stream -> null
    let full_out: [4]u64 = [0; 4];
    let full_out_s: []u64 = full_out[0..];
    expect_eq(decodeDeltaU64(enc[0..(n_bytes - 1)], full_out_s) == null, true);

    // Same for Delta-of-Delta
    const ts: [4]i64 = [1000, 2000, 3000, 4000];
    const n_ts = encodeDeltaOfDeltaI64(ts[0..], enc_s) ?? return;
    let small_i: [2]i64 = [0; 2];
    let small_i_s: []i64 = small_i[0..];
    expect_eq(decodeDeltaOfDeltaI64(enc[0..n_ts], small_i_s) == null, true);

    let full_i: [4]i64 = [0; 4];
    let full_i_s: []i64 = full_i[0..];
    expect_eq(decodeDeltaOfDeltaI64(enc[0..(n_ts - 1)], full_i_s) == null, true);
});
