// Origin: Pure-CP Stdlib Test Suite (Zero-Copy HuggingFace Safetensors Parser) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/safetensors.cp
// Covers:
//   1. Full Safetensors parsing with __metadata__ + F32, BF16, I8 & 0D scalar (shape: []) tensors
//   2. safeTensorFind & safeTensorBytes zero-copy exact payload byte verification
//   3. Byte-length mismatch vs shape * dtype.byteSize() strictly rejected
//   4. Out-of-bounds data_offsets, start > end & undersized output buffer defenses
//   5. Malformed JSON, unknown dtype, > 6 dims, empty/truncated header defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SafeDtype,
    SafeTensorDesc,
    SafeTensorsInfo,
    parseSafeTensors,
    safeTensorFind,
    safeTensorBytes,
} from "../../lib/attention/index.cp";

fn writeU64Le(buf: *[]u8, pos: usize, v: u64) {
    buf[pos] = (v & 0xFF) as u8;
    buf[pos + 1] = ((v >> 8) & 0xFF) as u8;
    buf[pos + 2] = ((v >> 16) & 0xFF) as u8;
    buf[pos + 3] = ((v >> 24) & 0xFF) as u8;
    buf[pos + 4] = ((v >> 32) & 0xFF) as u8;
    buf[pos + 5] = ((v >> 40) & 0xFF) as u8;
    buf[pos + 6] = ((v >> 48) & 0xFF) as u8;
    buf[pos + 7] = ((v >> 56) & 0xFF) as u8;
}

fn buildSafeTensorsBuf(buf: *[]u8, json_hdr: string, payload_len: usize): usize {
    writeU64Le(buf, 0, json_hdr.len as u64);
    let i: usize = 0;
    while (i < json_hdr.len) : (i += 1) {
        buf[8 + i] = json_hdr[i];
    }
    const data_off = 8 + json_hdr.len;
    let p: usize = 0;
    while (p < payload_len) : (p += 1) {
        buf[data_off + p] = ((p * 7 + 13) & 0xFF) as u8;
    }
    return data_off + payload_len;
}

fn testParseStandardSafeTensorsAndZeroCopyBytes()! {
    // 4 tensors:
    //   w_f32:  F32,  [2, 2] -> 16B  [0, 16]
    //   w_bf16: BF16, [3]    ->  6B  [16, 22]
    //   w_i8:   I8,   [4]    ->  4B  [22, 26]
    //   s_bool: BOOL, []     ->  1B  [26, 27] (0D scalar)
    // Total payload = 27 bytes
    const hdr_json = "{\"__metadata__\":{\"format\":\"pt\",\"arch\":\"qwen3\"},\"w_f32\":{\"dtype\":\"F32\",\"shape\":[2,2],\"data_offsets\":[0,16]},\"w_bf16\":{\"shape\":[3],\"dtype\":\"BF16\",\"data_offsets\":[16,22]},\"w_i8\":{\"data_offsets\":[22,26],\"dtype\":\"I8\",\"shape\":[4]},\"s_bool\":{\"dtype\":\"BOOL\",\"shape\":[],\"data_offsets\":[26,27]}}";

    let raw_buf: [512]u8 = [0; 512];
    let raw: []u8 = raw_buf[0..];
    const total_len = buildSafeTensorsBuf(raw, hdr_json, 27);

    let descs_buf: [8]SafeTensorDesc = [SafeTensorDesc{}; 8];
    let descs: []SafeTensorDesc = descs_buf[0..];

    if (info := parseSafeTensors(raw_buf[0..total_len], descs)) {
        expect_eq(info.header_len, hdr_json.len as u64);
        expect_eq(info.data_offset, 8 + hdr_json.len);
        expect_eq(info.tensor_count, 4);

        const active = descs_buf[0..4];

        // 1. Verify F32 2x2 tensor
        if (idx0 := safeTensorFind(active, "w_f32")) {
            const d0 = active[idx0];
            expect_eq(d0.dtype, SafeDtype.F32);
            expect_eq(d0.dtype.byteSize(), 4);
            expect_eq(d0.n_dims, 2);
            expect_eq(d0.shape[0], 2);
            expect_eq(d0.shape[1], 2);
            expect_eq(d0.numElements() ?? 0, 4);
            expect_eq(d0.expectedByteLen() ?? 0, 16);
            if (bytes0 := safeTensorBytes(raw_buf[0..total_len], info, d0)) {
                expect_eq(bytes0.len, 16);
                expect_eq(bytes0[0], 13);
                expect_eq(bytes0[15], ((15 * 7 + 13) & 0xFF) as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // 2. Verify BF16 tensor
        if (idx1 := safeTensorFind(active, "w_bf16")) {
            const d1 = active[idx1];
            expect_eq(d1.dtype, SafeDtype.BF16);
            expect_eq(d1.dtype.byteSize(), 2);
            expect_eq(d1.expectedByteLen() ?? 0, 6);
            if (bytes1 := safeTensorBytes(raw_buf[0..total_len], info, d1)) {
                expect_eq(bytes1.len, 6);
                expect_eq(bytes1[0], ((16 * 7 + 13) & 0xFF) as u8);
                expect_eq(bytes1[5], ((21 * 7 + 13) & 0xFF) as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // 3. Verify I8 tensor
        if (idx2 := safeTensorFind(active, "w_i8")) {
            const d2 = active[idx2];
            expect_eq(d2.dtype, SafeDtype.I8);
            expect_eq(d2.expectedByteLen() ?? 0, 4);
            if (bytes2 := safeTensorBytes(raw_buf[0..total_len], info, d2)) {
                expect_eq(bytes2.len, 4);
                expect_eq(bytes2[0], ((22 * 7 + 13) & 0xFF) as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // 4. Verify 0D scalar BOOL tensor (shape: [])
        if (idx3 := safeTensorFind(active, "s_bool")) {
            const d3 = active[idx3];
            expect_eq(d3.dtype, SafeDtype.Bool);
            expect_eq(d3.n_dims, 0);
            expect_eq(d3.numElements() ?? 0, 1);
            expect_eq(d3.expectedByteLen() ?? 0, 1);
            if (bytes3 := safeTensorBytes(raw_buf[0..total_len], info, d3)) {
                expect_eq(bytes3.len, 1);
                expect_eq(bytes3[0], ((26 * 7 + 13) & 0xFF) as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        expect_eq(safeTensorFind(active, "__metadata__") == null, true);
        expect_eq(safeTensorFind(active, "missing") == null, true);
    } else {
        expect_eq(true, false);
    }
}

fn testByteLengthMismatchRejected()! {
    let raw_buf: [256]u8 = [0; 256];
    let raw: []u8 = raw_buf[0..];
    let descs_buf: [4]SafeTensorDesc = [SafeTensorDesc{}; 4];
    let descs: []SafeTensorDesc = descs_buf[0..];

    // F32 [2, 2] expects 16 bytes, but data_offsets claims [0, 12]
    const bad_len_json = "{\"w\":{\"dtype\":\"F32\",\"shape\":[2,2],\"data_offsets\":[0,12]}}";
    const total = buildSafeTensorsBuf(raw, bad_len_json, 16);
    expect_eq(parseSafeTensors(raw_buf[0..total], descs) == null, true);
}

fn testOutOfBoundsAndReversedOffsetsRejected()! {
    let raw_buf: [256]u8 = [0; 256];
    let raw: []u8 = raw_buf[0..];
    let descs_buf: [4]SafeTensorDesc = [SafeTensorDesc{}; 4];
    let descs: []SafeTensorDesc = descs_buf[0..];

    // Case A: data_end (16) exceeds actual payload_len (12)
    const oob_json = "{\"w\":{\"dtype\":\"F32\",\"shape\":[4],\"data_offsets\":[0,16]}}";
    const total_oob = buildSafeTensorsBuf(raw, oob_json, 12);
    expect_eq(parseSafeTensors(raw_buf[0..total_oob], descs) == null, true);

    // Case B: start > end ([16, 0])
    const rev_json = "{\"w\":{\"dtype\":\"F32\",\"shape\":[4],\"data_offsets\":[16,0]}}";
    const total_rev = buildSafeTensorsBuf(raw, rev_json, 16);
    expect_eq(parseSafeTensors(raw_buf[0..total_rev], descs) == null, true);

    // Case C: undersized tensors_out slice
    const ok_json = "{\"a\":{\"dtype\":\"U8\",\"shape\":[2],\"data_offsets\":[0,2]},\"b\":{\"dtype\":\"U8\",\"shape\":[2],\"data_offsets\":[2,4]}}";
    const total_ok = buildSafeTensorsBuf(raw, ok_json, 4);
    let short_descs: []SafeTensorDesc = descs_buf[0..1];
    expect_eq(parseSafeTensors(raw_buf[0..total_ok], short_descs) == null, true);
}

fn testMalformedJsonUnknownDtypeAndDimsDefenses()! {
    let raw_buf: [256]u8 = [0; 256];
    let raw: []u8 = raw_buf[0..];
    let descs_buf: [4]SafeTensorDesc = [SafeTensorDesc{}; 4];
    let descs: []SafeTensorDesc = descs_buf[0..];

    // 1. Truncated < 8 bytes & zero header_len
    expect_eq(parseSafeTensors(raw_buf[0..7], descs) == null, true);
    writeU64Le(raw, 0, 0);
    expect_eq(parseSafeTensors(raw_buf[0..16], descs) == null, true);

    // 2. header_len exceeds file size
    writeU64Le(raw, 0, 1000);
    expect_eq(parseSafeTensors(raw_buf[0..32], descs) == null, true);

    // 3. Unknown dtype "FP8_E4M3"
    const bad_dt = "{\"w\":{\"dtype\":\"FP8_E4M3\",\"shape\":[4],\"data_offsets\":[0,4]}}";
    const t1 = buildSafeTensorsBuf(raw, bad_dt, 4);
    expect_eq(parseSafeTensors(raw_buf[0..t1], descs) == null, true);

    // 4. Malformed JSON (missing closing brace)
    const bad_json = "{\"w\":{\"dtype\":\"U8\",\"shape\":[2],\"data_offsets\":[0,2]}";
    const t2 = buildSafeTensorsBuf(raw, bad_json, 2);
    expect_eq(parseSafeTensors(raw_buf[0..t2], descs) == null, true);

    // 5. > 6 dims rejected
    const bad_dims = "{\"w\":{\"dtype\":\"U8\",\"shape\":[1,1,1,1,1,1,1],\"data_offsets\":[0,1]}}";
    const t3 = buildSafeTensorsBuf(raw, bad_dims, 1);
    expect_eq(parseSafeTensors(raw_buf[0..t3], descs) == null, true);

    // 6. Trailing garbage after JSON object rejected
    const bad_trail = "{\"w\":{\"dtype\":\"U8\",\"shape\":[1],\"data_offsets\":[0,1]}}garbage";
    const t4 = buildSafeTensorsBuf(raw, bad_trail, 1);
    expect_eq(parseSafeTensors(raw_buf[0..t4], descs) == null, true);
}

fn testEmptyAndPaddedWhitespaceHeader()! {
    let raw_buf: [256]u8 = [0; 256];
    let raw: []u8 = raw_buf[0..];
    let descs_buf: [4]SafeTensorDesc = [SafeTensorDesc{}; 4];
    let descs: []SafeTensorDesc = descs_buf[0..];

    // Standard Safetensors headers often pad trailing spaces for alignment
    const padded = "  { \"t\" : { \"dtype\" : \"I16\" , \"shape\" : [ 2 ] , \"data_offsets\" : [ 0 , 4 ] } }   ";
    const total = buildSafeTensorsBuf(raw, padded, 4);
    if (info := parseSafeTensors(raw_buf[0..total], descs)) {
        expect_eq(info.tensor_count, 1);
        expect_eq(descs_buf[0].dtype, SafeDtype.I16);
        expect_eq(descs_buf[0].expectedByteLen() ?? 0, 4);
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. safetensors: __metadata__ + F32/BF16/I8/0D-scalar parse & zero-copy bytes", () => {
        testParseStandardSafeTensorsAndZeroCopyBytes()?;
    });
    test("2. safetensors: byte length mismatch vs shape * dtype.byteSize() rejected", () => {
        testByteLengthMismatchRejected()?;
    });
    test("3. safetensors: OOB offsets, start > end & undersized buffer defenses", () => {
        testOutOfBoundsAndReversedOffsetsRejected()?;
    });
    test("4. safetensors: malformed JSON, unknown dtype, >6 dims & trailing garbage defenses", () => {
        testMalformedJsonUnknownDtypeAndDimsDefenses()?;
    });
    test("5. safetensors: padded whitespace alignment header parsing", () => {
        testEmptyAndPaddedWhitespaceHeader()?;
    });
}
