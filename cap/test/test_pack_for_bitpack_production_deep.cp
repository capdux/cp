// Origin: Pure-CP Stdlib Test Suite (lib/pack/for_bitpack.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc FOR & Delta-FOR BitPacking Codec:
//   1. Native @clz bitWidthU32 boundary values (0..0xFFFFFFFF) & packedByteLen
//   2. Arbitrary bit widths (0, 1, 3, 5, 7, 13, 17, 24, 31, 32) pack/unpack + O(1) unpackSingleU32
//   3. Frame-of-Reference (FOR) narrow-span compression & 0-bit constant block fast path (14B header only)
//   4. FOR full 32-bit extremes (0 & 0xFFFFFFFF) & empty 0-element block round-trip
//   5. Sorted Delta-FOR inverted DocID block round-trip & O(1) blockMayContain skip-pointer pruning
//   6. Sorted Delta-FOR non-monotonic rejection (not_monotonic) & single/constant sorted blocks
//   7. Malformed & boundary guards (bit_width > 32, value overflow, small buf, truncation, mode mismatch)
//   8. 128-element compression ratio & cap.pack namespace access

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FOR_BLOCK_HEADER_LEN,
    ForBlockMode,
    ForPackStatus,
    bitWidthU32,
    packedByteLen,
    blockMayContain,
    decodeForBlockHeader,
    packBitsU32,
    unpackBitsU32,
    unpackSingleU32,
    encodeForBlockU32,
    decodeForBlockU32,
    encodeDeltaForBlockU32,
    decodeDeltaForBlockU32,
} from "cap:pack";

// ============================================================================
// Test 1: Native @clz bitWidthU32 Boundaries & packedByteLen
// ============================================================================

fn testBitWidthAndPackedByteLen()! {
    expect_eq(bitWidthU32(0), 0);
    expect_eq(bitWidthU32(1), 1);
    expect_eq(bitWidthU32(2), 2);
    expect_eq(bitWidthU32(3), 2);
    expect_eq(bitWidthU32(7), 3);
    expect_eq(bitWidthU32(8), 4);
    expect_eq(bitWidthU32(255), 8);
    expect_eq(bitWidthU32(256), 9);
    expect_eq(bitWidthU32(65535), 16);
    expect_eq(bitWidthU32(0x7FFFFFFF), 31);
    expect_eq(bitWidthU32(0x80000000), 32);
    expect_eq(bitWidthU32(0xFFFFFFFF), 32);

    expect_eq(packedByteLen(0, 13), 0);
    expect_eq(packedByteLen(100, 0), 0);
    expect_eq(packedByteLen(8, 1), 1);
    expect_eq(packedByteLen(9, 1), 2);
    expect_eq(packedByteLen(10, 5), 7);  // 50 bits -> 7 bytes
    expect_eq(packedByteLen(4, 32), 16);
}

// ============================================================================
// Test 2: Arbitrary Bit-Width Pack/Unpack + O(1) Random-Access unpackSingleU32
// ============================================================================

fn testArbitraryBitWidthsAndRandomAccess()! {
    const widths: [11]u8 = [0, 1, 3, 5, 7, 8, 13, 17, 24, 31, 32];
    let packed_buf: [128]u8 = [0; 128];
    let packed_s: []u8 = packed_buf[0..];
    let out_vals: [17]u32 = [0; 17];
    let out_s: []u32 = out_vals[0..];

    let w_idx: usize = 0;
    while (w_idx < widths.len) : (w_idx += 1) {
        const bw: u8 = widths[w_idx];
        const mask: u32 = if (bw == 0) 0 else if (bw == 32) 0xFFFFFFFF else (((1 as u32) << (bw as u5)) - 1);

        let src: [17]u32 = [0; 17];
        let i: usize = 0;
        while (i < 17) : (i += 1) {
            if (bw == 0) {
                src[i] = 0;
            } else {
                const raw: u32 = ((i as u32) *% 2654435761) ^ 0xA5A5A5A5;
                src[i] = raw & mask;
            }
        }
        if (bw > 0) {
            src[0] = 0;
            src[16] = mask; // ensure exact max value for this bit width
        }

        const pres = packBitsU32(src[0..], bw, *packed_s);
        expect_eq(pres.status == ForPackStatus.ok, true);
        expect_eq(pres.bytes_written, packedByteLen(17, bw));

        const ures = unpackBitsU32(packed_buf[0..pres.bytes_written], 17, bw, *out_s);
        expect_eq(ures.status == ForPackStatus.ok, true);
        expect_eq(ures.elements_decoded, 17);
        expect_eq(ures.bytes_consumed, pres.bytes_written);

        // Verify bulk decode and O(1) random-access single-element decode for every index
        let k: usize = 0;
        while (k < 17) : (k += 1) {
            expect_eq(out_vals[k], src[k]);
            const single = unpackSingleU32(packed_buf[0..pres.bytes_written], bw, k);
            expect_eq(single.status == ForPackStatus.ok, true);
            expect_eq(single.value, src[k]);
        }
    }
}

// ============================================================================
// Test 3: Frame-of-Reference (FOR) Narrow-Span & 0-Bit Constant Block
// ============================================================================

fn testForBlockAndConstantFastPath()! {
    // 1. High-base narrow-span values: min=1_000_000, max=1_000_031 -> span=31 -> 5 bits/val!
    let vals: [16]u32 = [
        1000005, 1000000, 1000031, 1000012, 1000020, 1000001, 1000015, 1000029,
        1000008, 1000003, 1000019, 1000024, 1000011, 1000007, 1000030, 1000002,
    ];
    let enc_buf: [64]u8 = [0; 64];
    let enc_s: []u8 = enc_buf[0..];
    const enc = encodeForBlockU32(vals[0..], *enc_s);
    expect_eq(enc.status == ForPackStatus.ok, true);
    expect_eq(enc.bit_width, 5);
    expect_eq(enc.header.base_val, 1000000);
    expect_eq(enc.header.max_val, 1000031);
    // 14B header + (16 * 5 / 8 = 10B) = 24B vs 64B raw!
    expect_eq(enc.bytes_written, FOR_BLOCK_HEADER_LEN + 10);

    let dec_vals: [16]u32 = [0; 16];
    let dec_s: []u32 = dec_vals[0..];
    const decoded = decodeForBlockU32(enc_buf[0..enc.bytes_written], *dec_s);
    expect_eq(decoded.status == ForPackStatus.ok, true);
    expect_eq(decoded.elements_decoded, 16);
    expect_eq(decoded.bytes_consumed, enc.bytes_written);
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        expect_eq(dec_vals[i], vals[i]);
    }

    // 2. Constant block (all identical): bit_width == 0, only 14B header emitted!
    const const_vals: [32]u32 = [424242; 32];
    const c_enc = encodeForBlockU32(const_vals[0..], *enc_s);
    expect_eq(c_enc.status == ForPackStatus.ok, true);
    expect_eq(c_enc.bit_width, 0);
    expect_eq(c_enc.bytes_written, FOR_BLOCK_HEADER_LEN);

    let c_out: [32]u32 = [0; 32];
    let c_out_s: []u32 = c_out[0..];
    const c_dec = decodeForBlockU32(enc_buf[0..c_enc.bytes_written], *c_out_s);
    expect_eq(c_dec.status == ForPackStatus.ok, true);
    expect_eq(c_dec.elements_decoded, 32);
    expect_eq(c_out[0], 424242);
    expect_eq(c_out[31], 424242);
}

// ============================================================================
// Test 4: FOR Full 32-Bit Extremes (0 & 0xFFFFFFFF) & Empty Block
// ============================================================================

fn testForExtremesAndEmptyBlock()! {
    let enc_buf: [64]u8 = [0; 64];
    let enc_s: []u8 = enc_buf[0..];

    // Empty block
    const empty: [1]u32 = [0];
    const e_enc = encodeForBlockU32(empty[0..0], *enc_s);
    expect_eq(e_enc.status == ForPackStatus.ok, true);
    expect_eq(e_enc.bytes_written, FOR_BLOCK_HEADER_LEN);
    expect_eq(blockMayContain(e_enc.header, 0), false);

    let e_out: [4]u32 = [0; 4];
    let e_out_s: []u32 = e_out[0..];
    const e_dec = decodeForBlockU32(enc_buf[0..e_enc.bytes_written], *e_out_s);
    expect_eq(e_dec.status == ForPackStatus.ok, true);
    expect_eq(e_dec.elements_decoded, 0);

    // Full 32-bit extremes
    const ext: [5]u32 = [0, 0xFFFFFFFF, 0x80000000, 1, 0x7FFFFFFF];
    const x_enc = encodeForBlockU32(ext[0..], *enc_s);
    expect_eq(x_enc.status == ForPackStatus.ok, true);
    expect_eq(x_enc.bit_width, 32);
    expect_eq(x_enc.bytes_written, FOR_BLOCK_HEADER_LEN + 20);

    let x_out: [5]u32 = [0; 5];
    let x_out_s: []u32 = x_out[0..];
    const x_dec = decodeForBlockU32(enc_buf[0..x_enc.bytes_written], *x_out_s);
    expect_eq(x_dec.status == ForPackStatus.ok, true);
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_eq(x_out[i], ext[i]);
    }
}

// ============================================================================
// Test 5: Sorted Delta-FOR Posting List & O(1) blockMayContain Skip Pruning
// ============================================================================

fn testSortedDeltaForAndSkipPruning()! {
    // Sorted DocIDs with max delta = 7 (3 bits/delta!), including a 0-delta duplicate
    const docs: [10]u32 = [50000, 50002, 50002, 50005, 50012, 50013, 50018, 50020, 50027, 50030];
    let enc_buf: [64]u8 = [0; 64];
    let enc_s: []u8 = enc_buf[0..];

    const enc = encodeDeltaForBlockU32(docs[0..], *enc_s);
    expect_eq(enc.status == ForPackStatus.ok, true);
    expect_eq(enc.bit_width, 3);
    expect_eq(enc.header.mode == ForBlockMode.delta_for_sorted, true);
    expect_eq(enc.header.base_val, 50000);
    expect_eq(enc.header.max_val, 50030);
    // 14B header + ceil(9 * 3 / 8) = 14 + 4 = 18B (vs 40B raw)
    expect_eq(enc.bytes_written, FOR_BLOCK_HEADER_LEN + 4);

    // Header-only decode & O(1) range pruning
    const hres = decodeForBlockHeader(enc_buf[0..enc.bytes_written]);
    expect_eq(hres.status == ForPackStatus.ok, true);
    expect_eq(blockMayContain(hres.header, 49999), false);
    expect_eq(blockMayContain(hres.header, 50000), true);
    expect_eq(blockMayContain(hres.header, 50015), true);
    expect_eq(blockMayContain(hres.header, 50030), true);
    expect_eq(blockMayContain(hres.header, 50031), false);

    let out: [10]u32 = [0; 10];
    let out_s: []u32 = out[0..];
    const decoded = decodeDeltaForBlockU32(enc_buf[0..enc.bytes_written], *out_s);
    expect_eq(decoded.status == ForPackStatus.ok, true);
    expect_eq(decoded.elements_decoded, 10);
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        expect_eq(out[i], docs[i]);
    }
}

// ============================================================================
// Test 6: Sorted Delta-FOR Non-Monotonic Guard & Single/Constant Blocks
// ============================================================================

fn testDeltaForNonMonotonicAndTrivialBlocks()! {
    let enc_buf: [64]u8 = [0; 64];
    let enc_s: []u8 = enc_buf[0..];

    // Non-monotonic input must fail with not_monotonic
    const bad: [4]u32 = [10, 20, 19, 30];
    expect_eq(encodeDeltaForBlockU32(bad[0..], *enc_s).status == ForPackStatus.not_monotonic, true);

    // Single-element block -> 14B header only
    const single: [1]u32 = [777];
    const s_enc = encodeDeltaForBlockU32(single[0..], *enc_s);
    expect_eq(s_enc.status == ForPackStatus.ok, true);
    expect_eq(s_enc.bytes_written, FOR_BLOCK_HEADER_LEN);
    let out: [4]u32 = [0; 4];
    let out_s: []u32 = out[0..];
    const s_dec = decodeDeltaForBlockU32(enc_buf[0..s_enc.bytes_written], *out_s);
    expect_eq(s_dec.status == ForPackStatus.ok, true);
    expect_eq(s_dec.elements_decoded, 1);
    expect_eq(out[0], 777);

    // Constant sorted block (all deltas == 0) -> 14B header only
    const eq_vals: [4]u32 = [99, 99, 99, 99];
    const eq_enc = encodeDeltaForBlockU32(eq_vals[0..], *enc_s);
    expect_eq(eq_enc.status == ForPackStatus.ok, true);
    expect_eq(eq_enc.bytes_written, FOR_BLOCK_HEADER_LEN);
    const eq_dec = decodeDeltaForBlockU32(enc_buf[0..eq_enc.bytes_written], *out_s);
    expect_eq(eq_dec.status == ForPackStatus.ok, true);
    expect_eq(eq_dec.elements_decoded, 4);
    expect_eq(out[3], 99);
}

// ============================================================================
// Test 7: Malformed & Boundary Guards
// ============================================================================

fn testMalformedAndBoundaryGuards()! {
    let buf: [32]u8 = [0; 32];
    let buf_s: []u8 = buf[0..];
    let out: [8]u32 = [0; 8];
    let out_s: []u32 = out[0..];
    const vals: [4]u32 = [1, 2, 3, 16];

    // 1. Invalid bit_width (> 32)
    expect_eq(packBitsU32(vals[0..], 33, *buf_s).status == ForPackStatus.invalid_bit_width, true);
    expect_eq(unpackBitsU32(buf[0..], 4, 33, *out_s).status == ForPackStatus.invalid_bit_width, true);
    expect_eq(unpackSingleU32(buf[0..], 33, 0).status == ForPackStatus.invalid_bit_width, true);

    // 2. Value exceeds specified bit_width (16 needs 5 bits, fails at 4 bits)
    expect_eq(packBitsU32(vals[0..], 4, *buf_s).status == ForPackStatus.value_exceeds_bit_width, true);
    expect_eq(packBitsU32(vals[0..], 0, *buf_s).status == ForPackStatus.value_exceeds_bit_width, true);

    // 3. Output buffer too small
    let tiny: [4]u8 = [0; 4];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(encodeForBlockU32(vals[0..], *tiny_s).status == ForPackStatus.buffer_too_small, true);

    // 4. Truncated encoded block & cross-mode mismatch
    const enc = encodeForBlockU32(vals[0..], *buf_s);
    expect_eq(enc.status == ForPackStatus.ok, true);
    expect_eq(decodeForBlockU32(buf[0..enc.bytes_written - 1], *out_s).status == ForPackStatus.truncated_input, true);
    expect_eq(decodeDeltaForBlockU32(buf[0..enc.bytes_written], *out_s).status == ForPackStatus.invalid_header, true);

    // 5. Corrupted header (mode = 9)
    let corrupt: [32]u8 = buf;
    corrupt[0] = 9;
    expect_eq(decodeForBlockHeader(corrupt[0..enc.bytes_written]).status == ForPackStatus.invalid_header, true);

    // 6. Random access OOB truncation
    expect_eq(unpackSingleU32(buf[0..2], 8, 5).status == ForPackStatus.truncated_input, true);
}

// ============================================================================
// Test 8: 128-Element Block Compression Ratio & cap.pack Namespace Access
// ============================================================================

fn testLargeBlockCompressionAndNamespace()! {
    let ids: [128]u32 = [0; 128];
    let cur: u32 = 1000000;
    let i: usize = 0;
    while (i < 128) : (i += 1) {
        ids[i] = cur;
        cur += ((i % 7) as u32) + 1; // deltas in 1..7 -> 3 bits/delta
    }

    let enc_buf: [128]u8 = [0; 128];
    let enc_s: []u8 = enc_buf[0..];
    const enc = cap.pack.encodeDeltaForBlockU32(ids[0..], *enc_s);
    expect_eq(enc.status == ForPackStatus.ok, true);
    expect_eq(enc.bit_width, 3);
    // 128 * 4 = 512 bytes raw -> 14 + ceil(127 * 3 / 8) = 14 + 48 = 62 bytes (> 8x compression!)
    expect_eq(enc.bytes_written, 62);

    let out: [128]u32 = [0; 128];
    let out_s: []u32 = out[0..];
    const decoded = cap.pack.decodeDeltaForBlockU32(enc_buf[0..enc.bytes_written], *out_s);
    expect_eq(decoded.status == ForPackStatus.ok, true);
    expect_eq(decoded.elements_decoded, 128);
    expect_eq(out[0], ids[0]);
    expect_eq(out[63], ids[63]);
    expect_eq(out[127], ids[127]);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("for_bitpack: native @clz bitWidthU32 boundaries & packedByteLen", testBitWidthAndPackedByteLen);
test("for_bitpack: arbitrary bit widths (0..32) pack/unpack & O(1) unpackSingleU32", testArbitraryBitWidthsAndRandomAccess);
test("for_bitpack: FOR narrow-span compression & 0-bit constant block fast path", testForBlockAndConstantFastPath);
test("for_bitpack: FOR full 32-bit extremes (0..0xFFFFFFFF) & empty block", testForExtremesAndEmptyBlock);
test("for_bitpack: Sorted Delta-FOR posting list & O(1) blockMayContain pruning", testSortedDeltaForAndSkipPruning);
test("for_bitpack: Sorted Delta-FOR non-monotonic guard & trivial blocks", testDeltaForNonMonotonicAndTrivialBlocks);
test("for_bitpack: malformed & boundary guards (bit_width, overflow, truncation, mode)", testMalformedAndBoundaryGuards);
test("for_bitpack: 128-element >8x compression & cap.pack namespace access", testLargeBlockCompressionAndNamespace);
