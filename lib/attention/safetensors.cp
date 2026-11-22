// Origin: Pure-CP Stdlib (AI Inference Zero-Copy HuggingFace Safetensors Header & Tensor View Parser) | Created: 2026-11-22
// Zero-heap-allocation, zero-copy parser for HuggingFace Safetensors binary weight files:
//   [8B LE u64 header_len][UTF-8 JSON header][raw tensor payload bytes]
// Parses top-level tensor descriptors, skips `__metadata__`, validates `dtype`, `shape`,
// `data_offsets`, exact byte length (`end - start == expectedByteLen()`), and payload bounds.

const std = @import("std");

/// Safetensors element data types.
pub enum SafeDtype {
    F64,
    F32,
    F16,
    BF16,
    I64,
    I32,
    I16,
    I8,
    U64,
    U32,
    U16,
    U8,
    Bool,

    /// Returns the element size in bytes.
    fn byteSize(): usize {
        return switch (_) {
            SafeDtype.F64, SafeDtype.I64, SafeDtype.U64 => 8,
            SafeDtype.F32, SafeDtype.I32, SafeDtype.U32 => 4,
            SafeDtype.F16, SafeDtype.BF16, SafeDtype.I16, SafeDtype.U16 => 2,
            SafeDtype.I8, SafeDtype.U8, SafeDtype.Bool => 1,
        };
    }
}

/// Zero-copy Safetensors tensor descriptor (`name` borrows directly from the JSON header).
/// Supports 0D scalars (`n_dims == 0`, `numElements() == 1`) up to 6D tensors.
pub struct SafeTensorDesc {
    name: string = "";
    dtype: SafeDtype = SafeDtype.F32;
    n_dims: usize = 0;
    shape: [6]u64 = [0; 6];
    data_start: u64 = 0;
    data_end: u64 = 0;

    /// Computes total element count (`1` for 0D scalars), returning `null` on `n_dims > 6` or `u64` overflow.
    fn numElements(): u64? {
        if (_.n_dims > 6) return null;
        let prod: u64 = 1;
        let i: usize = 0;
        while (i < _.n_dims) : (i += 1) {
            const d = _.shape[i];
            if (d == 0) return 0;
            if (prod > std.math.maxInt(u64) / d) return null;
            prod *= d;
        }
        return prod;
    }

    /// Computes expected payload length in bytes (`numElements() * dtype.byteSize()`), or `null` on overflow.
    fn expectedByteLen(): u64? {
        const elems = _.numElements() ?? return null;
        const elem_bytes = _.dtype.byteSize() as u64;
        if (elems > std.math.maxInt(u64) / elem_bytes) return null;
        return elems * elem_bytes;
    }
}

/// Summary metadata returned by `parseSafeTensors`.
pub struct SafeTensorsInfo {
    header_len: u64 = 0;
    data_offset: usize = 0;
    tensor_count: usize = 0;
}

fn readU64Le(data: []const u8, pos: usize): u64 {
    const b0 = data[pos] as u64;
    const b1 = data[pos + 1] as u64;
    const b2 = data[pos + 2] as u64;
    const b3 = data[pos + 3] as u64;
    const b4 = data[pos + 4] as u64;
    const b5 = data[pos + 5] as u64;
    const b6 = data[pos + 6] as u64;
    const b7 = data[pos + 7] as u64;
    return b0
        | (b1 << 8)
        | (b2 << 16)
        | (b3 << 24)
        | (b4 << 32)
        | (b5 << 40)
        | (b6 << 48)
        | (b7 << 56);
}

fn parseDtypeStr(s: string): SafeDtype? {
    if (std.mem.eql(u8, s, "F64")) return SafeDtype.F64;
    if (std.mem.eql(u8, s, "F32")) return SafeDtype.F32;
    if (std.mem.eql(u8, s, "F16")) return SafeDtype.F16;
    if (std.mem.eql(u8, s, "BF16")) return SafeDtype.BF16;
    if (std.mem.eql(u8, s, "I64")) return SafeDtype.I64;
    if (std.mem.eql(u8, s, "I32")) return SafeDtype.I32;
    if (std.mem.eql(u8, s, "I16")) return SafeDtype.I16;
    if (std.mem.eql(u8, s, "I8")) return SafeDtype.I8;
    if (std.mem.eql(u8, s, "U64")) return SafeDtype.U64;
    if (std.mem.eql(u8, s, "U32")) return SafeDtype.U32;
    if (std.mem.eql(u8, s, "U16")) return SafeDtype.U16;
    if (std.mem.eql(u8, s, "U8")) return SafeDtype.U8;
    if (std.mem.eql(u8, s, "BOOL")) return SafeDtype.Bool;
    return null;
}

fn skipWs(src: []const u8, pos: *usize) {
    let p = pos.*;
    while (p < src.len) {
        const c = src[p];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            p += 1;
        } else {
            break;
        }
    }
    pos.* = p;
}

fn parseJsonString(src: []const u8, pos: *usize): string? {
    skipWs(src, pos);
    let p = pos.*;
    if (p >= src.len || src[p] != '"') return null;
    p += 1;
    const start = p;
    while (p < src.len) {
        const c = src[p];
        if (c == '"') {
            pos.* = p + 1;
            return src[start..p];
        }
        if (c == '\\') {
            p += 2;
            if (p > src.len) return null;
        } else {
            p += 1;
        }
    }
    return null;
}

fn parseJsonU64(src: []const u8, pos: *usize): u64? {
    skipWs(src, pos);
    let p = pos.*;
    if (p >= src.len) return null;
    const first = src[p];
    if (first < '0' || first > '9') return null;

    let val: u64 = 0;
    while (p < src.len) {
        const c = src[p];
        if (c < '0' || c > '9') break;
        const digit = (c - '0') as u64;
        if (val > (std.math.maxInt(u64) - digit) / 10) return null;
        val = val * 10 + digit;
        p += 1;
    }
    pos.* = p;
    return val;
}

/// Skips an arbitrary JSON value iteratively (bounded nesting depth, zero recursion).
fn skipJsonValue(src: []const u8, pos: *usize): bool {
    skipWs(src, pos);
    if (pos.* >= src.len) return false;
    const ch = src[pos.*];

    if (ch == '"') {
        * = parseJsonString(src, pos) ?? return false;
        return true;
    }
    if (ch == '{' || ch == '[') {
        let depth: usize = 0;
        let p = pos.*;
        while (p < src.len) {
            const c = src[p];
            if (c == '"') {
                let sp = p;
                * = parseJsonString(src, &sp) ?? return false;
                p = sp;
                continue;
            }
            if (c == '{' || c == '[') {
                depth += 1;
                if (depth > 32) return false;
                p += 1;
                continue;
            }
            if (c == '}' || c == ']') {
                if (depth == 0) return false;
                depth -= 1;
                p += 1;
                if (depth == 0) {
                    pos.* = p;
                    return true;
                }
                continue;
            }
            p += 1;
        }
        return false;
    }
    // Literal or number token: advance until delimiter
    let p = pos.*;
    while (p < src.len) {
        const c = src[p];
        if (c == ',' || c == '}' || c == ']' || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            break;
        }
        p += 1;
    }
    if (p == pos.*) return false;
    pos.* = p;
    return true;
}

fn parseTensorObject(
    hdr: []const u8,
    pos: *usize,
    name: string,
    payload_len: u64,
): SafeTensorDesc? {
    skipWs(hdr, pos);
    if (pos.* >= hdr.len || hdr[pos.*] != '{') return null;
    pos.* += 1;

    let desc = SafeTensorDesc{ name: name };
    let has_dtype = false;
    let has_shape = false;
    let has_offsets = false;

    skipWs(hdr, pos);
    if (pos.* < hdr.len && hdr[pos.*] == '}') return null; // empty tensor object is invalid

    while (true) {
        const field = parseJsonString(hdr, pos) ?? return null;
        skipWs(hdr, pos);
        if (pos.* >= hdr.len || hdr[pos.*] != ':') return null;
        pos.* += 1;

        if (std.mem.eql(u8, field, "dtype")) {
            if (has_dtype) return null;
            const dt_str = parseJsonString(hdr, pos) ?? return null;
            desc.dtype = parseDtypeStr(dt_str) ?? return null;
            has_dtype = true;
        } else if (std.mem.eql(u8, field, "shape")) {
            if (has_shape) return null;
            skipWs(hdr, pos);
            if (pos.* >= hdr.len || hdr[pos.*] != '[') return null;
            pos.* += 1;
            skipWs(hdr, pos);
            let nd: usize = 0;
            if (pos.* < hdr.len && hdr[pos.*] == ']') {
                pos.* += 1;
            } else {
                while (true) {
                    if (nd >= 6) return null;
                    const dim = parseJsonU64(hdr, pos) ?? return null;
                    desc.shape[nd] = dim;
                    nd += 1;
                    skipWs(hdr, pos);
                    if (pos.* >= hdr.len) return null;
                    if (hdr[pos.*] == ',') {
                        pos.* += 1;
                        continue;
                    }
                    if (hdr[pos.*] == ']') {
                        pos.* += 1;
                        break;
                    }
                    return null;
                }
            }
            desc.n_dims = nd;
            has_shape = true;
        } else if (std.mem.eql(u8, field, "data_offsets")) {
            if (has_offsets) return null;
            skipWs(hdr, pos);
            if (pos.* >= hdr.len || hdr[pos.*] != '[') return null;
            pos.* += 1;
            const start_off = parseJsonU64(hdr, pos) ?? return null;
            skipWs(hdr, pos);
            if (pos.* >= hdr.len || hdr[pos.*] != ',') return null;
            pos.* += 1;
            const end_off = parseJsonU64(hdr, pos) ?? return null;
            skipWs(hdr, pos);
            if (pos.* >= hdr.len || hdr[pos.*] != ']') return null;
            pos.* += 1;
            desc.data_start = start_off;
            desc.data_end = end_off;
            has_offsets = true;
        } else {
            return null;
        }

        skipWs(hdr, pos);
        if (pos.* >= hdr.len) return null;
        if (hdr[pos.*] == ',') {
            pos.* += 1;
            continue;
        }
        if (hdr[pos.*] == '}') {
            pos.* += 1;
            break;
        }
        return null;
    }

    if (!has_dtype || !has_shape || !has_offsets) return null;
    if (desc.data_start > desc.data_end) return null;
    if (desc.data_end > payload_len) return null;

    const expected = desc.expectedByteLen() ?? return null;
    if (desc.data_end - desc.data_start != expected) return null;

    return desc;
}

/// Zero-heap-allocation, single-pass Safetensors parser.
///
/// Validates the 8-byte LE `header_len`, parses the UTF-8 JSON header, skips `"__metadata__"`,
/// populates `tensors_out[0..info.tensor_count]`, and strictly verifies every tensor's
/// `data_start <= data_end`, `data_end - data_start == expectedByteLen()`, and payload bounds.
///
/// Returns `null` on truncated input, malformed JSON, unknown dtype, inconsistent byte length,
/// out-of-bounds offsets, or undersized `tensors_out`.
pub fn parseSafeTensors(
    data: []const u8,
    tensors_out: *[]SafeTensorDesc,
): SafeTensorsInfo? {
    if (data.len < 8) return null;
    const header_len = readU64Le(data, 0);
    if (header_len == 0) return null;
    const rem_after_len = (data.len - 8) as u64;
    if (header_len > rem_after_len) return null;

    const data_offset = 8 + (header_len as usize);
    const payload_len = (data.len - data_offset) as u64;
    const hdr = data[8..data_offset];

    let pos: usize = 0;
    skipWs(hdr, &pos);
    if (pos >= hdr.len || hdr[pos] != '{') return null;
    pos += 1;

    let tensor_count: usize = 0;
    skipWs(hdr, &pos);
    if (pos < hdr.len && hdr[pos] == '}') {
        pos += 1;
    } else {
        while (true) {
            const key = parseJsonString(hdr, &pos) ?? return null;
            skipWs(hdr, &pos);
            if (pos >= hdr.len || hdr[pos] != ':') return null;
            pos += 1;

            if (std.mem.eql(u8, key, "__metadata__")) {
                if (!skipJsonValue(hdr, &pos)) return null;
            } else {
                const desc = parseTensorObject(hdr, &pos, key, payload_len) ?? return null;
                if (tensor_count >= tensors_out.len) return null;
                tensors_out[tensor_count] = desc;
                tensor_count += 1;
            }

            skipWs(hdr, &pos);
            if (pos >= hdr.len) return null;
            if (hdr[pos] == ',') {
                pos += 1;
                continue;
            }
            if (hdr[pos] == '}') {
                pos += 1;
                break;
            }
            return null;
        }
    }

    skipWs(hdr, &pos);
    if (pos != hdr.len) return null;

    return SafeTensorsInfo{
        header_len: header_len,
        data_offset: data_offset,
        tensor_count: tensor_count,
    };
}

/// Finds the first tensor descriptor whose `name` matches `name`, returning its index or `null`.
pub fn safeTensorFind(tensors: []const SafeTensorDesc, name: string): usize? {
    let i: usize = 0;
    while (i < tensors.len) : (i += 1) {
        if (std.mem.eql(u8, tensors[i].name, name)) return i;
    }
    return null;
}

/// Returns a zero-copy read-only byte slice pointing directly to `desc`'s raw payload inside `data`.
/// Validates payload bounds and exact byte-length consistency before slicing.
pub fn safeTensorBytes(
    data: []const u8,
    info: SafeTensorsInfo,
    desc: SafeTensorDesc,
): ([]const u8)? {
    if (info.data_offset > data.len) return null;
    if (desc.data_start > desc.data_end) return null;
    const payload_len = (data.len - info.data_offset) as u64;
    if (desc.data_end > payload_len) return null;

    const expected = desc.expectedByteLen() ?? return null;
    if (desc.data_end - desc.data_start != expected) return null;

    const start = info.data_offset + (desc.data_start as usize);
    const end = info.data_offset + (desc.data_end as usize);
    return data[start..end];
}
