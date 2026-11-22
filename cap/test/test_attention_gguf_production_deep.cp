// Origin: Pure-CP Stdlib Test Suite (Zero-Copy GGUF v2/v3 Model Parser) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/gguf.cp
// Covers:
//   1. Full GGUF v3 binary parsing: string/u32/alignment/array KVs + Q4_0 & F32 tensor infos
//   2. Custom general.alignment = 64 produces exact 64-byte aligned data_offset
//   3. ggufFindKv, ggufFindTensor & ggufTensorByteSize exact verification (F32/F16/BF16/Q8_0/Q4_0/Q4_K)
//   4. GGUF v2 compatibility & default 32-byte alignment
//   5. Malformed input defenses: bad magic, unsupported v1, truncated payload, invalid value type,
//      nested recursive array, n_dims > 4, unaligned quantization dims & undersized output slices

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    GGUF_MAGIC,
    GGUF_DEFAULT_ALIGNMENT,
    GgufValueType,
    GgmlType,
    GgufHeader,
    GgufKvEntry,
    GgufTensorInfo,
    parseGgufHeader,
    parseGgufFile,
    ggufFindKv,
    ggufFindTensor,
    ggufTensorByteSize,
} from "../../lib/attention/index.cp";

struct ByteWriter {
    buf: []u8;
    pos: usize = 0;

    fn writeU8(*_, v: u8) {
        _.buf[_.pos] = v;
        _.pos += 1;
    }

    fn writeU32Le(*_, v: u32) {
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
    }

    fn writeU64Le(*_, v: u64) {
        _.writeU32Le((v & 0xFFFFFFFF) as u32);
        _.writeU32Le(((v >> 32) & 0xFFFFFFFF) as u32);
    }

    fn writeBytes(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.buf[_.pos + i] = s[i];
        }
        _.pos += s.len;
    }

    fn writeGgufStr(*_, s: string) {
        _.writeU64Le(s.len as u64);
        _.writeBytes(s);
    }
}

fn buildStandardGgufV3(buf: *[]u8): usize {
    let w = ByteWriter{ buf: buf, pos: 0 };

    // Header: magic="GGUF", version=3, tensor_count=2, kv_count=4
    w.writeU32Le(GGUF_MAGIC);
    w.writeU32Le(3);
    w.writeU64Le(2);
    w.writeU64Le(4);

    // KV 0: "general.architecture" = String("llama")
    w.writeGgufStr("general.architecture");
    w.writeU32Le(8); // String
    w.writeGgufStr("llama");

    // KV 1: "llama.embedding_length" = Uint32(4096)
    w.writeGgufStr("llama.embedding_length");
    w.writeU32Le(4); // Uint32
    w.writeU32Le(4096);

    // KV 2: "general.alignment" = Uint32(64)
    w.writeGgufStr("general.alignment");
    w.writeU32Le(4); // Uint32
    w.writeU32Le(64);

    // KV 3: "tokenizer.ggml.scores" = Array(Float32, len=3)
    w.writeGgufStr("tokenizer.ggml.scores");
    w.writeU32Le(9); // Array
    w.writeU32Le(6); // Float32
    w.writeU64Le(3);
    w.writeU32Le(0x3F800000); // 1.0f
    w.writeU32Le(0x40000000); // 2.0f
    w.writeU32Le(0x40400000); // 3.0f

    // Tensor 0: "blk.0.attn_q.weight", n_dims=2, dims=[64, 32], dtype=Q4_0(2), offset=0
    w.writeGgufStr("blk.0.attn_q.weight");
    w.writeU32Le(2);
    w.writeU64Le(64);
    w.writeU64Le(32);
    w.writeU32Le(2); // Q4_0
    w.writeU64Le(0);

    // Tensor 1: "output_norm.weight", n_dims=1, dims=[64], dtype=F32(0), offset=1216
    w.writeGgufStr("output_norm.weight");
    w.writeU32Le(1);
    w.writeU64Le(64);
    w.writeU32Le(0); // F32
    w.writeU64Le(1216);

    return w.pos;
}

fn testParseStandardGgufV3AndAlignment()! {
    let raw_buf: [512]u8 = [0; 512];
    let raw: []u8 = raw_buf[0..];
    const header_end = buildStandardGgufV3(raw);

    // Fast header parse
    if (h0 := parseGgufHeader(raw_buf[0..header_end])) {
        expect_eq(h0.version, 3);
        expect_eq(h0.tensor_count, 2);
        expect_eq(h0.kv_count, 4);
        expect_eq(h0.alignment, GGUF_DEFAULT_ALIGNMENT);
    } else {
        expect_eq(true, false);
    }

    // Full zero-copy parse
    let kvs_buf: [8]GgufKvEntry = [GgufKvEntry{}; 8];
    let tensors_buf: [4]GgufTensorInfo = [GgufTensorInfo{}; 4];
    let kvs: []GgufKvEntry = kvs_buf[0..];
    let tensors: []GgufTensorInfo = tensors_buf[0..];

    if (hdr := parseGgufFile(raw_buf[0..header_end], kvs, tensors)) {
        expect_eq(hdr.version, 3);
        expect_eq(hdr.kv_count, 4);
        expect_eq(hdr.tensor_count, 2);
        expect_eq(hdr.alignment, 64);
        // data_offset must be >= header_end and strictly a multiple of 64
        expect_eq(hdr.data_offset >= header_end, true);
        expect_eq(hdr.data_offset % 64, 0);
        expect_eq(hdr.data_offset - header_end < 64, true);

        const kv_slice = kvs_buf[0..4];
        const t_slice = tensors_buf[0..2];

        // Verify KV lookups
        if (idx_arch := ggufFindKv(kv_slice, "general.architecture")) {
            expect_eq(kv_slice[idx_arch].val_type, GgufValueType.String);
            expect_eq(std.mem.eql(u8, kv_slice[idx_arch].str_val, "llama"), true);
        } else {
            expect_eq(true, false);
        }

        if (idx_emb := ggufFindKv(kv_slice, "llama.embedding_length")) {
            expect_eq(kv_slice[idx_emb].val_type, GgufValueType.Uint32);
            expect_eq(kv_slice[idx_emb].u64_val, 4096);
        } else {
            expect_eq(true, false);
        }

        if (idx_arr := ggufFindKv(kv_slice, "tokenizer.ggml.scores")) {
            expect_eq(kv_slice[idx_arr].val_type, GgufValueType.Array);
            expect_eq(kv_slice[idx_arr].array_elem_type, GgufValueType.Float32);
            expect_eq(kv_slice[idx_arr].array_len, 3);
        } else {
            expect_eq(true, false);
        }

        expect_eq(ggufFindKv(kv_slice, "nonexistent.key") == null, true);

        // Verify tensor lookups
        if (t0_i := ggufFindTensor(t_slice, "blk.0.attn_q.weight")) {
            const t0 = t_slice[t0_i];
            expect_eq(t0.n_dims, 2);
            expect_eq(t0.dims[0], 64);
            expect_eq(t0.dims[1], 32);
            expect_eq(t0.dtype, GgmlType.Q4_0);
            expect_eq(t0.offset, 0);
        } else {
            expect_eq(true, false);
        }

        if (t1_i := ggufFindTensor(t_slice, "output_norm.weight")) {
            const t1 = t_slice[t1_i];
            expect_eq(t1.n_dims, 1);
            expect_eq(t1.dims[0], 64);
            expect_eq(t1.dtype, GgmlType.F32);
            expect_eq(t1.offset, 1216);
        } else {
            expect_eq(true, false);
        }

        expect_eq(ggufFindTensor(t_slice, "missing.tensor") == null, true);
    } else {
        expect_eq(true, false);
    }
}

fn testTensorByteSizeCalculations()! {
    // F32: 64 elements * 4B = 256B
    if (sz := ggufTensorByteSize(GgmlType.F32, [64, 0, 0, 0], 1)) {
        expect_eq(sz, 256);
    } else {
        expect_eq(true, false);
    }

    // F16 & BF16: 64 x 32 = 2048 elements * 2B = 4096B
    if (sz := ggufTensorByteSize(GgmlType.F16, [64, 32, 0, 0], 2)) {
        expect_eq(sz, 4096);
    } else {
        expect_eq(true, false);
    }
    if (sz := ggufTensorByteSize(GgmlType.BF16, [64, 32, 0, 0], 2)) {
        expect_eq(sz, 4096);
    } else {
        expect_eq(true, false);
    }

    // Q4_0: 64 x 32 = 2048 elements = 64 blocks * 18B = 1152B
    if (sz := ggufTensorByteSize(GgmlType.Q4_0, [64, 32, 0, 0], 2)) {
        expect_eq(sz, 1152);
    } else {
        expect_eq(true, false);
    }

    // Q8_0: 64 x 32 = 2048 elements = 64 blocks * 34B = 2176B
    if (sz := ggufTensorByteSize(GgmlType.Q8_0, [64, 32, 0, 0], 2)) {
        expect_eq(sz, 2176);
    } else {
        expect_eq(true, false);
    }

    // Q4_K: 256 x 4 = 1024 elements = 4 super-blocks * 144B = 576B
    if (sz := ggufTensorByteSize(GgmlType.Q4_K, [256, 4, 0, 0], 2)) {
        expect_eq(sz, 576);
    } else {
        expect_eq(true, false);
    }

    // Unaligned inner dimension for Q4_0 (30 % 32 != 0) -> null
    expect_eq(ggufTensorByteSize(GgmlType.Q4_0, [30, 32, 0, 0], 2) == null, true);
    // Zero dims / > 4 dims -> null
    expect_eq(ggufTensorByteSize(GgmlType.F32, [64, 0, 0, 0], 0) == null, true);
    expect_eq(ggufTensorByteSize(GgmlType.F32, [64, 1, 1, 1], 5) == null, true);
    expect_eq(ggufTensorByteSize(GgmlType.F32, [64, 0, 0, 0], 2) == null, true);
}

fn testGgufV2DefaultAlignmentAndScalarTypes()! {
    let raw_buf: [256]u8 = [0; 256];
    let w = ByteWriter{ buf: raw_buf[0..], pos: 0 };

    // GGUF v2, 0 tensors, 3 KVs (Bool, Int32, Float64)
    w.writeU32Le(GGUF_MAGIC);
    w.writeU32Le(2);
    w.writeU64Le(0);
    w.writeU64Le(3);

    w.writeGgufStr("flag");
    w.writeU32Le(7); // Bool
    w.writeU8(1);

    w.writeGgufStr("neg");
    w.writeU32Le(5); // Int32 (-7 = 0xFFFFFFF9)
    w.writeU32Le(0xFFFFFFF9);

    w.writeGgufStr("pi_approx");
    w.writeU32Le(12); // Float64 (3.5 = 0x400C000000000000)
    w.writeU64Le(0x400C000000000000);

    let kvs_buf: [4]GgufKvEntry = [GgufKvEntry{}; 4];
    let tensors_buf: [1]GgufTensorInfo = [GgufTensorInfo{}; 1];
    let kvs: []GgufKvEntry = kvs_buf[0..];
    let tensors: []GgufTensorInfo = tensors_buf[0..];

    if (hdr := parseGgufFile(raw_buf[0..w.pos], kvs, tensors)) {
        expect_eq(hdr.version, 2);
        expect_eq(hdr.alignment, GGUF_DEFAULT_ALIGNMENT);
        expect_eq(hdr.data_offset % 32, 0);
        expect_eq(kvs_buf[0].u64_val, 1);
        expect_eq(kvs_buf[1].i64_val, -7);
        expect_eq(@abs(kvs_buf[2].f64_val - 3.5) <= 0.000000000001, true);
    } else {
        expect_eq(true, false);
    }
}

fn testMalformedHeaderAndTruncationDefenses()! {
    let raw_buf: [512]u8 = [0; 512];
    let raw: []u8 = raw_buf[0..];
    const valid_len = buildStandardGgufV3(raw);

    let kvs_buf: [8]GgufKvEntry = [GgufKvEntry{}; 8];
    let tensors_buf: [4]GgufTensorInfo = [GgufTensorInfo{}; 4];
    let kvs: []GgufKvEntry = kvs_buf[0..];
    let tensors: []GgufTensorInfo = tensors_buf[0..];

    // 1. Undersized output slices
    let short_kvs: []GgufKvEntry = kvs_buf[0..2];
    let short_tensors: []GgufTensorInfo = tensors_buf[0..1];
    expect_eq(parseGgufFile(raw_buf[0..valid_len], short_kvs, tensors) == null, true);
    expect_eq(parseGgufFile(raw_buf[0..valid_len], kvs, short_tensors) == null, true);

    // 2. Truncated payloads (every prefix shorter than valid_len must fail)
    expect_eq(parseGgufHeader(raw_buf[0..23]) == null, true);
    expect_eq(parseGgufFile(raw_buf[0 .. valid_len - 1], kvs, tensors) == null, true);
    expect_eq(parseGgufFile(raw_buf[0..40], kvs, tensors) == null, true);

    // 3. Wrong magic
    let bad_magic: [512]u8 = raw_buf;
    bad_magic[0] = 'X';
    expect_eq(parseGgufHeader(bad_magic[0..valid_len]) == null, true);
    expect_eq(parseGgufFile(bad_magic[0..valid_len], kvs, tensors) == null, true);

    // 4. Unsupported version v1 and v4
    let bad_v1: [512]u8 = raw_buf;
    bad_v1[4] = 1;
    expect_eq(parseGgufHeader(bad_v1[0..valid_len]) == null, true);
    let bad_v4: [512]u8 = raw_buf;
    bad_v4[4] = 4;
    expect_eq(parseGgufHeader(bad_v4[0..valid_len]) == null, true);
}

fn testMalformedKvAndTensorDefenses()! {
    let kvs_buf: [4]GgufKvEntry = [GgufKvEntry{}; 4];
    let tensors_buf: [4]GgufTensorInfo = [GgufTensorInfo{}; 4];
    let kvs: []GgufKvEntry = kvs_buf[0..];
    let tensors: []GgufTensorInfo = tensors_buf[0..];

    // Case A: invalid GgufValueType (99)
    let buf_a: [64]u8 = [0; 64];
    let wa = ByteWriter{ buf: buf_a[0..], pos: 0 };
    wa.writeU32Le(GGUF_MAGIC);
    wa.writeU32Le(3);
    wa.writeU64Le(0);
    wa.writeU64Le(1);
    wa.writeGgufStr("bad");
    wa.writeU32Le(99);
    expect_eq(parseGgufFile(buf_a[0..wa.pos], kvs, tensors) == null, true);

    // Case B: nested recursive Array(Array)
    let buf_b: [64]u8 = [0; 64];
    let wb = ByteWriter{ buf: buf_b[0..], pos: 0 };
    wb.writeU32Le(GGUF_MAGIC);
    wb.writeU32Le(3);
    wb.writeU64Le(0);
    wb.writeU64Le(1);
    wb.writeGgufStr("nested");
    wb.writeU32Le(9); // Array
    wb.writeU32Le(9); // elem_type = Array (forbidden)
    wb.writeU64Le(0);
    expect_eq(parseGgufFile(buf_b[0..wb.pos], kvs, tensors) == null, true);

    // Case C: tensor with n_dims == 5 (> 4)
    let buf_c: [128]u8 = [0; 128];
    let wc = ByteWriter{ buf: buf_c[0..], pos: 0 };
    wc.writeU32Le(GGUF_MAGIC);
    wc.writeU32Le(3);
    wc.writeU64Le(1);
    wc.writeU64Le(0);
    wc.writeGgufStr("t5d");
    wc.writeU32Le(5); // > 4 dims
    wc.writeU64Le(2);
    wc.writeU64Le(2);
    wc.writeU64Le(2);
    wc.writeU64Le(2);
    wc.writeU64Le(2);
    wc.writeU32Le(0);
    wc.writeU64Le(0);
    expect_eq(parseGgufFile(buf_c[0..wc.pos], kvs, tensors) == null, true);

    // Case D: non-power-of-two general.alignment = 48
    let buf_d: [64]u8 = [0; 64];
    let wd = ByteWriter{ buf: buf_d[0..], pos: 0 };
    wd.writeU32Le(GGUF_MAGIC);
    wd.writeU32Le(3);
    wd.writeU64Le(0);
    wd.writeU64Le(1);
    wd.writeGgufStr("general.alignment");
    wd.writeU32Le(4);
    wd.writeU32Le(48);
    expect_eq(parseGgufFile(buf_d[0..wd.pos], kvs, tensors) == null, true);
}

fn main() {
    test("1. gguf: full v3 binary parse, alignment=64 data_offset, KV & tensor lookups", () => {
        testParseStandardGgufV3AndAlignment()?;
    });
    test("2. gguf: ggufTensorByteSize exact sizes for F32/F16/BF16/Q4_0/Q8_0/Q4_K", () => {
        testTensorByteSizeCalculations()?;
    });
    test("3. gguf: v2 default 32B alignment & Bool/Int32/Float64 scalar decoding", () => {
        testGgufV2DefaultAlignmentAndScalarTypes()?;
    });
    test("4. gguf: bad magic, unsupported v1/v4, truncation & undersized buffer defenses", () => {
        testMalformedHeaderAndTruncationDefenses()?;
    });
    test("5. gguf: invalid value type, recursive array, n_dims>4 & bad alignment defenses", () => {
        testMalformedKvAndTensorDefenses()?;
    });
}
