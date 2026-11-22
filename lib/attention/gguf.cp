// Origin: Pure-CP Stdlib (AI Inference Zero-Copy GGUF v2/v3 Model Header, Metadata & Tensor Info Parser) | Created: 2026-11-22
// Zero-heap-allocation, zero-copy binary parser for llama.cpp GGUF v2/v3 model files.
// Parses header, typed metadata KV entries (including automatic `general.alignment` override),
// tensor descriptors (`GgufTensorInfo`), aligned tensor data offset, and exact GGML tensor byte sizes
// directly over an in-memory/mmap'd `[]const u8` buffer.

const std = @import("std");

/// Little-endian magic bytes `"GGUF"` (`0x46554747`).
pub const GGUF_MAGIC: u32 = 0x46554747;

/// Default GGUF tensor data alignment in bytes when `general.alignment` is not specified.
pub const GGUF_DEFAULT_ALIGNMENT: u64 = 32;

/// GGUF v2/v3 metadata value type tags.
pub enum GgufValueType: u32 {
    Uint8 = 0,
    Int8 = 1,
    Uint16 = 2,
    Int16 = 3,
    Uint32 = 4,
    Int32 = 5,
    Float32 = 6,
    Bool = 7,
    String = 8,
    Array = 9,
    Uint64 = 10,
    Int64 = 11,
    Float64 = 12,
}

/// GGML tensor quantization / floating-point element types.
pub enum GgmlType: u32 {
    F32 = 0,
    F16 = 1,
    Q4_0 = 2,
    Q4_1 = 3,
    Q5_0 = 6,
    Q5_1 = 7,
    Q8_0 = 8,
    Q8_1 = 9,
    Q2_K = 10,
    Q3_K = 11,
    Q4_K = 12,
    Q5_K = 13,
    Q6_K = 14,
    BF16 = 30,
}

/// Parsed GGUF file header plus resolved alignment and aligned tensor-data byte offset.
pub struct GgufHeader {
    version: u32 = 0;
    tensor_count: u64 = 0;
    kv_count: u64 = 0;
    alignment: u64 = GGUF_DEFAULT_ALIGNMENT;
    data_offset: usize = 0;
}

/// Zero-copy GGUF metadata key-value entry (`key` and `str_val` borrow directly from the source buffer).
pub struct GgufKvEntry {
    key: string = "";
    val_type: GgufValueType = GgufValueType.Uint8;
    u64_val: u64 = 0;
    i64_val: i64 = 0;
    f64_val: f64 = 0.0;
    str_val: string = "";
    array_elem_type: GgufValueType = GgufValueType.Uint8;
    array_len: u64 = 0;
}

/// Zero-copy GGUF tensor descriptor (`name` borrows directly from the source buffer).
pub struct GgufTensorInfo {
    name: string = "";
    n_dims: u32 = 0;
    dims: [4]u64 = [0; 4];
    dtype: GgmlType = GgmlType.F32;
    offset: u64 = 0;
}

fn readU16Le(data: []const u8, pos: usize): u16 {
    return (data[pos] as u16) | ((data[pos + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, pos: usize): u32 {
    return (data[pos] as u32)
        | ((data[pos + 1] as u32) << 8)
        | ((data[pos + 2] as u32) << 16)
        | ((data[pos + 3] as u32) << 24);
}

fn readU64Le(data: []const u8, pos: usize): u64 {
    const lo = readU32Le(data, pos) as u64;
    const hi = readU32Le(data, pos + 4) as u64;
    return lo | (hi << 32);
}

fn parseValueType(raw: u32): GgufValueType? {
    return switch (raw) {
        0 => GgufValueType.Uint8,
        1 => GgufValueType.Int8,
        2 => GgufValueType.Uint16,
        3 => GgufValueType.Int16,
        4 => GgufValueType.Uint32,
        5 => GgufValueType.Int32,
        6 => GgufValueType.Float32,
        7 => GgufValueType.Bool,
        8 => GgufValueType.String,
        9 => GgufValueType.Array,
        10 => GgufValueType.Uint64,
        11 => GgufValueType.Int64,
        12 => GgufValueType.Float64,
        else => null,
    };
}

fn parseGgmlType(raw: u32): GgmlType? {
    return switch (raw) {
        0 => GgmlType.F32,
        1 => GgmlType.F16,
        2 => GgmlType.Q4_0,
        3 => GgmlType.Q4_1,
        6 => GgmlType.Q5_0,
        7 => GgmlType.Q5_1,
        8 => GgmlType.Q8_0,
        9 => GgmlType.Q8_1,
        10 => GgmlType.Q2_K,
        11 => GgmlType.Q3_K,
        12 => GgmlType.Q4_K,
        13 => GgmlType.Q5_K,
        14 => GgmlType.Q6_K,
        30 => GgmlType.BF16,
        else => null,
    };
}

fn scalarByteSize(vt: GgufValueType): usize {
    return switch (vt) {
        GgufValueType.Uint8, GgufValueType.Int8, GgufValueType.Bool => 1,
        GgufValueType.Uint16, GgufValueType.Int16 => 2,
        GgufValueType.Uint32, GgufValueType.Int32, GgufValueType.Float32 => 4,
        GgufValueType.Uint64, GgufValueType.Int64, GgufValueType.Float64 => 8,
        else => 0,
    };
}

fn readGgufString(data: []const u8, pos_inout: *usize): string? {
    const pos = pos_inout.*;
    if (pos > data.len || data.len - pos < 8) return null;
    const str_len_u64 = readU64Le(data, pos);
    const str_start = pos + 8;
    const rem = data.len - str_start;
    if (str_len_u64 > (rem as u64)) return null;
    const str_len = str_len_u64 as usize;
    pos_inout.* = str_start + str_len;
    return data[str_start .. str_start + str_len];
}

/// Fast-path GGUF v2/v3 header parser.
/// Validates the 24-byte header, little-endian `"GGUF"` magic, and `version in {2, 3}`.
/// Note: `alignment` defaults to `GGUF_DEFAULT_ALIGNMENT` and `data_offset` is `24`
/// until full metadata/tensor parsing via `parseGgufFile`.
pub fn parseGgufHeader(data: []const u8): GgufHeader? {
    if (data.len < 24) return null;
    const magic = readU32Le(data, 0);
    if (magic != GGUF_MAGIC) return null;

    const version = readU32Le(data, 4);
    if (version != 2 && version != 3) return null;

    const tensor_count = readU64Le(data, 8);
    const kv_count = readU64Le(data, 16);

    return GgufHeader{
        version: version,
        tensor_count: tensor_count,
        kv_count: kv_count,
        alignment: GGUF_DEFAULT_ALIGNMENT,
        data_offset: 24,
    };
}

/// Single-pass zero-copy parser for GGUF v2/v3 metadata KV pairs and tensor info entries.
///
/// - Populates `kvs_out[0..header.kv_count]` and `tensors_out[0..header.tensor_count]`.
/// - Automatically applies `general.alignment` (must be a non-zero power of two) to compute
///   the aligned `data_offset` where raw tensor payloads begin.
/// - Defends against invalid magic/version, truncated payloads, unknown value/GGML types,
///   nested recursive arrays, `n_dims == 0` or `n_dims > 4`, integer overflow, and undersized output slices.
pub fn parseGgufFile(
    data: []const u8,
    kvs_out: *[]GgufKvEntry,
    tensors_out: *[]GgufTensorInfo,
): GgufHeader? {
    const base_hdr = parseGgufHeader(data) ?? return null;

    if (base_hdr.kv_count > (kvs_out.len as u64)) return null;
    if (base_hdr.tensor_count > (tensors_out.len as u64)) return null;

    let pos: usize = 24;
    let alignment: u64 = GGUF_DEFAULT_ALIGNMENT;

    // 1. Parse metadata KV entries
    const kv_n = base_hdr.kv_count as usize;
    let ki: usize = 0;
    while (ki < kv_n) : (ki += 1) {
        const key = readGgufString(data, &pos) ?? return null;
        if (data.len - pos < 4) return null;
        const vt = parseValueType(readU32Le(data, pos)) ?? return null;
        pos += 4;

        let entry = GgufKvEntry{
            key: key,
            val_type: vt,
        };

        switch (vt) {
            GgufValueType.Uint8 => {
                if (data.len - pos < 1) return null;
                entry.u64_val = data[pos] as u64;
                pos += 1;
            },
            GgufValueType.Int8 => {
                if (data.len - pos < 1) return null;
                const s8: i8 = @bitCast(data[pos]);
                entry.i64_val = s8 as i64;
                pos += 1;
            },
            GgufValueType.Uint16 => {
                if (data.len - pos < 2) return null;
                entry.u64_val = readU16Le(data, pos) as u64;
                pos += 2;
            },
            GgufValueType.Int16 => {
                if (data.len - pos < 2) return null;
                const s16: i16 = @bitCast(readU16Le(data, pos));
                entry.i64_val = s16 as i64;
                pos += 2;
            },
            GgufValueType.Uint32 => {
                if (data.len - pos < 4) return null;
                entry.u64_val = readU32Le(data, pos) as u64;
                pos += 4;
            },
            GgufValueType.Int32 => {
                if (data.len - pos < 4) return null;
                const s32: i32 = @bitCast(readU32Le(data, pos));
                entry.i64_val = s32 as i64;
                pos += 4;
            },
            GgufValueType.Float32 => {
                if (data.len - pos < 4) return null;
                const f32_v: f32 = @bitCast(readU32Le(data, pos));
                entry.f64_val = f32_v as f64;
                pos += 4;
            },
            GgufValueType.Bool => {
                if (data.len - pos < 1) return null;
                const b = data[pos];
                if (b > 1) return null;
                entry.u64_val = b as u64;
                pos += 1;
            },
            GgufValueType.String => {
                const sv = readGgufString(data, &pos) ?? return null;
                entry.str_val = sv;
            },
            GgufValueType.Array => {
                if (data.len - pos < 12) return null;
                const elem_vt = parseValueType(readU32Le(data, pos)) ?? return null;
                pos += 4;
                // Reject nested recursive arrays for bounded O(1)-stack parsing
                if (elem_vt == GgufValueType.Array) return null;

                const arr_len = readU64Le(data, pos);
                pos += 8;

                if (elem_vt == GgufValueType.String) {
                    let ai: u64 = 0;
                    while (ai < arr_len) : (ai += 1) {
                        * = readGgufString(data, &pos) ?? return null;
                    }
                } else {
                    const esize = scalarByteSize(elem_vt);
                    if (esize == 0) return null;
                    const rem = data.len - pos;
                    if (arr_len > ((rem / esize) as u64)) return null;
                    pos += (arr_len as usize) * esize;
                }

                entry.array_elem_type = elem_vt;
                entry.array_len = arr_len;
            },
            GgufValueType.Uint64 => {
                if (data.len - pos < 8) return null;
                entry.u64_val = readU64Le(data, pos);
                pos += 8;
            },
            GgufValueType.Int64 => {
                if (data.len - pos < 8) return null;
                const s64: i64 = @bitCast(readU64Le(data, pos));
                entry.i64_val = s64;
                pos += 8;
            },
            GgufValueType.Float64 => {
                if (data.len - pos < 8) return null;
                const f64_v: f64 = @bitCast(readU64Le(data, pos));
                entry.f64_val = f64_v;
                pos += 8;
            },
        }

        if (std.mem.eql(u8, key, "general.alignment")) {
            if (vt == GgufValueType.Uint8
                || vt == GgufValueType.Uint16
                || vt == GgufValueType.Uint32
                || vt == GgufValueType.Uint64)
            {
                const cand = entry.u64_val;
                if (cand == 0 || (cand & (cand - 1)) != 0) return null;
                alignment = cand;
            }
        }

        kvs_out[ki] = entry;
    }

    // 2. Parse tensor info entries
    const tensor_n = base_hdr.tensor_count as usize;
    let ti: usize = 0;
    while (ti < tensor_n) : (ti += 1) {
        const name = readGgufString(data, &pos) ?? return null;
        if (data.len - pos < 4) return null;
        const n_dims = readU32Le(data, pos);
        pos += 4;

        if (n_dims == 0 || n_dims > 4) return null;
        const dims_bytes = (n_dims as usize) * 8;
        if (data.len - pos < dims_bytes + 12) return null;

        let dims: [4]u64 = [0; 4];
        let di: usize = 0;
        while (di < (n_dims as usize)) : (di += 1) {
            const d = readU64Le(data, pos);
            if (d == 0) return null;
            dims[di] = d;
            pos += 8;
        }

        const dtype = parseGgmlType(readU32Le(data, pos)) ?? return null;
        pos += 4;
        const offset = readU64Le(data, pos);
        pos += 8;

        tensors_out[ti] = GgufTensorInfo{
            name: name,
            n_dims: n_dims,
            dims: dims,
            dtype: dtype,
            offset: offset,
        };
    }

    // 3. Compute aligned tensor data offset
    if (alignment > (std.math.maxInt(usize) as u64)) return null;
    const align_usize = alignment as usize;
    const rem = pos % align_usize;
    const pad = if (rem == 0) 0 else align_usize - rem;
    if (pad > std.math.maxInt(usize) - pos) return null;
    const data_offset = pos + pad;

    return GgufHeader{
        version: base_hdr.version,
        tensor_count: base_hdr.tensor_count,
        kv_count: base_hdr.kv_count,
        alignment: alignment,
        data_offset: data_offset,
    };
}

/// Finds the first metadata KV entry whose key equals `key`, returning its index or `null`.
pub fn ggufFindKv(kvs: []const GgufKvEntry, key: string): usize? {
    let i: usize = 0;
    while (i < kvs.len) : (i += 1) {
        if (std.mem.eql(u8, kvs[i].key, key)) return i;
    }
    return null;
}

/// Finds the first tensor info entry whose name equals `name`, returning its index or `null`.
pub fn ggufFindTensor(tensors: []const GgufTensorInfo, name: string): usize? {
    let i: usize = 0;
    while (i < tensors.len) : (i += 1) {
        if (std.mem.eql(u8, tensors[i].name, name)) return i;
    }
    return null;
}

/// Computes the exact physical byte size of a GGML tensor with `(dtype, dims, n_dims)`.
/// Validates `1 <= n_dims <= 4`, non-zero dimensions, inner-dimension block alignment
/// (`dims[0] % block_elems == 0`), and guards against `u64` overflow.
pub fn ggufTensorByteSize(dtype: GgmlType, dims: [4]u64, n_dims: u32): u64? {
    if (n_dims == 0 || n_dims > 4) return null;

    let block_elems: u64 = 1;
    let block_bytes: u64 = 4;
    switch (dtype) {
        GgmlType.F32 => {
            block_elems = 1;
            block_bytes = 4;
        },
        GgmlType.F16, GgmlType.BF16 => {
            block_elems = 1;
            block_bytes = 2;
        },
        GgmlType.Q4_0 => {
            block_elems = 32;
            block_bytes = 18;
        },
        GgmlType.Q4_1 => {
            block_elems = 32;
            block_bytes = 20;
        },
        GgmlType.Q5_0 => {
            block_elems = 32;
            block_bytes = 22;
        },
        GgmlType.Q5_1 => {
            block_elems = 32;
            block_bytes = 24;
        },
        GgmlType.Q8_0 => {
            block_elems = 32;
            block_bytes = 34;
        },
        GgmlType.Q8_1 => {
            block_elems = 32;
            block_bytes = 36;
        },
        GgmlType.Q2_K => {
            block_elems = 256;
            block_bytes = 84;
        },
        GgmlType.Q3_K => {
            block_elems = 256;
            block_bytes = 110;
        },
        GgmlType.Q4_K => {
            block_elems = 256;
            block_bytes = 144;
        },
        GgmlType.Q5_K => {
            block_elems = 256;
            block_bytes = 176;
        },
        GgmlType.Q6_K => {
            block_elems = 256;
            block_bytes = 210;
        },
    }

    if (dims[0] == 0 || (dims[0] % block_elems) != 0) return null;

    let n_elements: u64 = 1;
    let d_idx: usize = 0;
    while (d_idx < (n_dims as usize)) : (d_idx += 1) {
        const dim = dims[d_idx];
        if (dim == 0) return null;
        if (n_elements > std.math.maxInt(u64) / dim) return null;
        n_elements *= dim;
    }

    const n_blocks = n_elements / block_elems;
    if (n_blocks > std.math.maxInt(u64) / block_bytes) return null;
    return n_blocks * block_bytes;
}
