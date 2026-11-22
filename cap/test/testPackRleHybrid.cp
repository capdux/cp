// Origin: Pure-CP Stdlib Test Suite (lib/pack/rleHybrid.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Parquet/Arrow Hybrid RLE + Bit-Packed Codec:
//   1. Native @clz bit-width computation (0..32) & value byte-width helpers
//   2. Pure RLE run wire-format verification & extreme compression ratio (200 repeated values)
//   3. Automatic Hybrid RLE <-> Bit-Packed switching (RLE + high-entropy + RLE)
//   4. Full bit-width round-trip matrix (0, 1, 3, 5, 7, 8, 13, 16, 24, 31, 32 incl. 0xFFFFFFFF)
//   5. Non-multiple-of-8 tail padding & exact expected_count truncation
//   6. Streaming zero-copy HybridRleDecoder.nextBatch across RLE & Bit-Packed boundaries
//   7. Encode/decode boundary guards (bit_width > 32, value_out_of_range, 0 len, buffer_too_small)
//   8. Malformed stream guards (bad varint, zero run header, truncated RLE/BitPack, excess expected_count)

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HYBRID_RLE_MAX_BIT_WIDTH,
    hybridBitWidthU32,
    hybridMinBitWidthSlice,
    hybridValueByteWidth,
    hybridMaxEncodedSize,
    HybridRleStatus,
    encodeHybridRleU32,
    HybridRleDecoder,
    decodeHybridRleU32,
} from "cap:pack";

test("rleHybrid: native @clz bit-width & value byte-width helpers", () => {
    expect_eq(hybridBitWidthU32(0), 0 as u8);
    expect_eq(hybridBitWidthU32(1), 1 as u8);
    expect_eq(hybridBitWidthU32(7), 3 as u8);
    expect_eq(hybridBitWidthU32(8), 4 as u8);
    expect_eq(hybridBitWidthU32(255), 8 as u8);
    expect_eq(hybridBitWidthU32(256), 9 as u8);
    expect_eq(hybridBitWidthU32(65535), 16 as u8);
    expect_eq(hybridBitWidthU32(0x7FFFFFFF), 31 as u8);
    expect_eq(hybridBitWidthU32(0xFFFFFFFF), 32 as u8);

    const sample: [4]u32 = [2, 19, 5, 31];
    expect_eq(hybridMinBitWidthSlice(sample[0..]), 5 as u8);

    expect_eq(hybridValueByteWidth(0), 0 as usize);
    expect_eq(hybridValueByteWidth(1), 1 as usize);
    expect_eq(hybridValueByteWidth(8), 1 as usize);
    expect_eq(hybridValueByteWidth(9), 2 as usize);
    expect_eq(hybridValueByteWidth(17), 3 as usize);
    expect_eq(hybridValueByteWidth(32), 4 as usize);
});

test("rleHybrid: pure RLE wire format & extreme compression ratio", () => {
    const vals: [200]u32 = [42; 200];
    let buf: [32]u8 = [0; 32];
    let buf_s: []u8 = buf[0..];

    const enc = encodeHybridRleU32(vals[0..], 6, *buf_s);
    expect_eq(enc.status, HybridRleStatus.ok);
    expect_eq(enc.rle_runs, 1 as usize);
    expect_eq(enc.bitpack_groups, 0 as usize);
    // Header = (200 << 1) = 400 -> 2-byte ULEB128 [0x90, 0x03], plus 1-byte value 42 = 3 bytes total!
    expect_eq(enc.bytes_written, 3 as usize);
    expect_eq(buf[0], 0x90 as u8);
    expect_eq(buf[1], 0x03 as u8);
    expect_eq(buf[2], 42 as u8);

    let out: [200]u32 = [0; 200];
    let out_s: []u32 = out[0..];
    const dec_res = decodeHybridRleU32(buf[0..enc.bytes_written], 6, 200, *out_s);
    expect_eq(dec_res.status, HybridRleStatus.ok);
    expect_eq(dec_res.values_decoded, 200 as usize);
    expect_eq(dec_res.bytes_consumed, 3 as usize);
    let i: usize = 0;
    while (i < 200) : (i += 1) {
        expect_eq(out[i], 42 as u32);
    }
});

test("rleHybrid: automatic Hybrid RLE <-> Bit-Packed switching & lossless round-trip", () => {
    // 12 x (5) [RLE] + 8 distinct values [Bit-Packed] + 16 x (2) [RLE] = 36 values, bit_width=3
    let src: [36]u32 = [0; 36];
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        src[i] = 5;
    }
    src[12] = 0;
    src[13] = 1;
    src[14] = 2;
    src[15] = 3;
    src[16] = 4;
    src[17] = 5;
    src[18] = 6;
    src[19] = 7;
    i = 20;
    while (i < 36) : (i += 1) {
        src[i] = 2;
    }

    let enc_buf: [64]u8 = [0; 64];
    let enc_s: []u8 = enc_buf[0..];
    const enc = encodeHybridRleU32(src[0..], 3, *enc_s);
    expect_eq(enc.status, HybridRleStatus.ok);
    expect_eq(enc.rle_runs, 2 as usize);
    expect_eq(enc.bitpack_groups, 1 as usize);
    // 2B (RLE1) + 1B+3B (BP1) + 2B (RLE2) = 8 bytes for 36 u32s (144B raw -> 18x compression)
    expect_eq(enc.bytes_written, 8 as usize);

    let out: [36]u32 = [0; 36];
    let out_s: []u32 = out[0..];
    const res = decodeHybridRleU32(enc_buf[0..enc.bytes_written], 3, 36, *out_s);
    expect_eq(res.status, HybridRleStatus.ok);
    expect_eq(res.values_decoded, 36 as usize);
    expect_eq(res.bytes_consumed, enc.bytes_written);
    i = 0;
    while (i < 36) : (i += 1) {
        expect_eq(out[i], src[i]);
    }
});

test("rleHybrid: full bit-width matrix (0, 1, 3, 5, 7, 8, 13, 16, 24, 31, 32)", () => {
    // 1. bit_width == 0: 100 zeros encoded into 2 bytes
    const zeros: [100]u32 = [0; 100];
    let z_buf: [16]u8 = [0; 16];
    let z_s: []u8 = z_buf[0..];
    const z_enc = encodeHybridRleU32(zeros[0..], 0, *z_s);
    expect_eq(z_enc.status, HybridRleStatus.ok);
    expect_eq(z_enc.bytes_written, 2 as usize);
    let z_out: [100]u32 = [7; 100];
    let z_out_s: []u32 = z_out[0..];
    const z_dec = decodeHybridRleU32(z_buf[0..z_enc.bytes_written], 0, 100, *z_out_s);
    expect_eq(z_dec.status, HybridRleStatus.ok);
    expect_eq(z_out[0], 0 as u32);
    expect_eq(z_out[99], 0 as u32);

    // 2. Test widths 1..32 with deterministic pseudo-random + extreme boundary values
    const widths: [10]u8 = [1, 3, 5, 7, 8, 13, 16, 24, 31, 32];
    let enc_buf: [512]u8 = [0; 512];
    let src: [24]u32 = [0; 24];
    let dst: [24]u32 = [0; 24];

    let wi: usize = 0;
    while (wi < 10) : (wi += 1) {
        const bw: u8 = widths[wi];
        const max_v: u32 = if (bw == 32) 0xFFFFFFFF else (((1 as u32) << (bw as u5)) - 1);
        let k: usize = 0;
        while (k < 24) : (k += 1) {
            if (k == 0) {
                src[k] = 0;
            } else if (k == 1) {
                src[k] = max_v;
            } else {
                src[k] = (((k as u32) *% 2654435761 +% 19) & max_v);
            }
        }

        let enc_s: []u8 = enc_buf[0..];
        const enc = encodeHybridRleU32(src[0..], bw, *enc_s);
        expect_eq(enc.status, HybridRleStatus.ok);
        expect_eq(enc.bytes_written <= hybridMaxEncodedSize(24, bw), true);

        let dst_s: []u32 = dst[0..];
        const dec_r = decodeHybridRleU32(enc_buf[0..enc.bytes_written], bw, 24, *dst_s);
        expect_eq(dec_r.status, HybridRleStatus.ok);
        expect_eq(dec_r.values_decoded, 24 as usize);
        k = 0;
        while (k < 24) : (k += 1) {
            expect_eq(dst[k], src[k]);
        }
    }
});

test("rleHybrid: non-multiple-of-8 tail lengths (1, 5, 11, 19)", () => {
    const lens: [4]usize = [1, 5, 11, 19];
    let enc_buf: [128]u8 = [0; 128];
    let src: [19]u32 = [0; 19];
    let dst: [19]u32 = [0; 19];

    let i: usize = 0;
    while (i < 19) : (i += 1) {
        src[i] = ((i * 7 + 3) % 1000) as u32;
    }

    let li: usize = 0;
    while (li < 4) : (li += 1) {
        const n: usize = lens[li];
        let enc_s: []u8 = enc_buf[0..];
        const enc = encodeHybridRleU32(src[0..n], 10, *enc_s);
        expect_eq(enc.status, HybridRleStatus.ok);

        let dst_s: []u32 = dst[0..n];
        const dec_r = decodeHybridRleU32(enc_buf[0..enc.bytes_written], 10, n, *dst_s);
        expect_eq(dec_r.status, HybridRleStatus.ok);
        expect_eq(dec_r.values_decoded, n);
        let k: usize = 0;
        while (k < n) : (k += 1) {
            expect_eq(dst[k], src[k]);
        }
    }
});

test("rleHybrid: streaming HybridRleDecoder.nextBatch across mixed RLE & Bit-Packed runs", () => {
    // 10 x (11) [RLE] + 13 distinct values [2 Bit-Packed groups] + 9 x (4) [RLE] = 32 values
    let src: [32]u32 = [0; 32];
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        src[i] = 11;
    }
    while (i < 23) : (i += 1) {
        src[i] = (i % 15) as u32;
    }
    while (i < 32) : (i += 1) {
        src[i] = 4;
    }

    let enc_buf: [128]u8 = [0; 128];
    let enc_s: []u8 = enc_buf[0..];
    const enc = encodeHybridRleU32(src[0..], 4, *enc_s);
    expect_eq(enc.status, HybridRleStatus.ok);

    // Decode in prime-sized batches of 5 to cross every run & group boundary
    let decoder = HybridRleDecoder.init(enc_buf[0..enc.bytes_written], 4, 32) ?? unreachable;
    let out_all: [32]u32 = [0; 32];
    let total_got: usize = 0;
    let batch: [5]u32 = [0; 5];

    while (!decoder.isDone()) {
        let b_s: []u32 = batch[0..];
        const r = decoder.nextBatch(*b_s);
        expect_eq(r.status, HybridRleStatus.ok);
        expect_eq(r.values_decoded > 0, true);
        let k: usize = 0;
        while (k < r.values_decoded) : (k += 1) {
            out_all[total_got + k] = batch[k];
        }
        total_got += r.values_decoded;
    }

    expect_eq(total_got, 32 as usize);
    expect_eq(decoder.remaining(), 0 as usize);
    expect_eq(decoder.bytesConsumed(), enc.bytes_written);
    i = 0;
    while (i < 32) : (i += 1) {
        expect_eq(out_all[i], src[i]);
    }
});

test("rleHybrid: encode/decode boundary guards (bit_width > 32, value OOB, empty, small buffer)", () => {
    const vals: [4]u32 = [0, 1, 2, 8]; // 8 does not fit in 3 bits
    let buf: [32]u8 = [0; 32];
    let buf_s: []u8 = buf[0..];

    // bit_width > 32
    expect_eq(encodeHybridRleU32(vals[0..], 33, *buf_s).status, HybridRleStatus.invalid_bit_width);
    expect_eq(HybridRleDecoder.init(buf[0..], 33, 4) == null, true);

    // value_out_of_range (8 >= 2^3)
    expect_eq(encodeHybridRleU32(vals[0..], 3, *buf_s).status, HybridRleStatus.value_out_of_range);
    // non-zero value with bit_width == 0
    expect_eq(encodeHybridRleU32(vals[0..], 0, *buf_s).status, HybridRleStatus.value_out_of_range);

    // Empty input / expected_count == 0
    const empty_v: [0]u32 = [];
    const e_empty = encodeHybridRleU32(empty_v[0..], 4, *buf_s);
    expect_eq(e_empty.status, HybridRleStatus.ok);
    expect_eq(e_empty.bytes_written, 0 as usize);

    let out_v: [4]u32 = [0; 4];
    let out_s: []u32 = out_v[0..];
    expect_eq(decodeHybridRleU32(buf[0..0], 4, 0, *out_s).status, HybridRleStatus.ok);

    // Undersized encode buffer & decode buffer
    let tiny_enc: [1]u8 = [0; 1];
    let tiny_enc_s: []u8 = tiny_enc[0..];
    expect_eq(encodeHybridRleU32(vals[0..], 4, *tiny_enc_s).status, HybridRleStatus.buffer_too_small);

    const enc_ok = encodeHybridRleU32(vals[0..], 4, *buf_s);
    expect_eq(enc_ok.status, HybridRleStatus.ok);
    let tiny_out: [2]u32 = [0; 2];
    let tiny_out_s: []u32 = tiny_out[0..];
    expect_eq(decodeHybridRleU32(buf[0..enc_ok.bytes_written], 4, 4, *tiny_out_s).status, HybridRleStatus.buffer_too_small);
});

test("rleHybrid: malformed stream guards (bad varint, zero header, truncated payload, excess count)", () => {
    let out: [16]u32 = [0; 16];
    let out_s: []u32 = out[0..];

    // 1. Unterminated varint (continuation bit set on final byte)
    const bad_varint1: [2]u8 = [0x80, 0x80];
    expect_eq(decodeHybridRleU32(bad_varint1[0..], 4, 4, *out_s).status, HybridRleStatus.malformed_varint);

    // 2. Overflowing 5-byte varint (high nibble non-zero in 5th byte)
    const bad_varint2: [5]u8 = [0x81, 0x80, 0x80, 0x80, 0x10];
    expect_eq(decodeHybridRleU32(bad_varint2[0..], 4, 4, *out_s).status, HybridRleStatus.malformed_varint);

    // 3. Zero-length RLE header (0x00) and zero-group Bit-Packed header (0x01)
    const zero_rle: [2]u8 = [0x00, 0x05];
    const zero_bp: [2]u8 = [0x01, 0x05];
    expect_eq(decodeHybridRleU32(zero_rle[0..], 4, 4, *out_s).status, HybridRleStatus.zero_run_header);
    expect_eq(decodeHybridRleU32(zero_bp[0..], 4, 4, *out_s).status, HybridRleStatus.zero_run_header);

    // 4. Truncated RLE value payload (header says 8 values, bit_width=16 needs 2B value, only 1B present)
    const trunc_rle: [2]u8 = [0x10, 0x01];
    expect_eq(decodeHybridRleU32(trunc_rle[0..], 16, 8, *out_s).status, HybridRleStatus.truncated_stream);

    // 5. RLE value out of bit_width range (header says 8 values, bit_width=3, value=0xFF)
    const oob_rle: [2]u8 = [0x10, 0xFF];
    expect_eq(decodeHybridRleU32(oob_rle[0..], 3, 8, *out_s).status, HybridRleStatus.value_out_of_range);

    // 6. Truncated Bit-Packed payload (header 0x03 = 1 group, bit_width=4 needs 4B, only 2B present)
    const trunc_bp: [3]u8 = [0x03, 0x12, 0x34];
    expect_eq(decodeHybridRleU32(trunc_bp[0..], 4, 8, *out_s).status, HybridRleStatus.truncated_stream);

    // 7. Excess expected_count exceeding encoded stream count
    const valid_rle_4: [2]u8 = [0x08, 0x03]; // RLE run of 4 values == 3
    expect_eq(decodeHybridRleU32(valid_rle_4[0..], 4, 10, *out_s).status, HybridRleStatus.truncated_stream);
});
