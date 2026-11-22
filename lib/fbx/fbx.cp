// Origin: Pure-CP Stdlib (Zero-Alloc Autodesk FBX Binary 7100-7700 Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation Autodesk FBX Binary parser supporting both 32-bit (<7500)
// and 64-bit (>=7500) node headers, NULL sentinels, recursive child traversal, and all
// scalar, string/raw, and raw/zlib-encoded array properties.

const std = @import("std");

/// 21-byte FBX Binary signature prefix `"Kaydara FBX Binary  \x00"`.
pub const FBX_MAGIC: []const u8 = "Kaydara FBX Binary  \x00";

/// Total 27-byte FBX Binary file header size (21B magic + 2B [0x1A, 0x00] + 4B LE version).
pub const FBX_HEADER_SIZE: usize = 27;

/// FBX version threshold switching from 32-bit node headers (13B) to 64-bit node headers (25B).
pub const FBX_V7500_THRESHOLD: u32 = 7500;

/// Parsed Autodesk FBX Binary file header.
pub struct FbxHeader {
    version: u32 = 0,
    is_64bit: bool = false,
    nodes_offset: usize = FBX_HEADER_SIZE,

    /// Returns a zero-allocation iterator over top-level scene nodes in `data`.
    fn rootNodes(data: []const u8): FbxNodeIter {
        if (_.nodes_offset > data.len) {
            return FbxNodeIter {
                src: "",
                base_offset: _.nodes_offset,
                pos: 0,
                is_64bit: _.is_64bit,
                done: true,
            };
        }
        return FbxNodeIter.initWithBase(data[_.nodes_offset..], _.nodes_offset, _.is_64bit);
    }
}

/// Zero-copy FBX scene tree node.
pub struct FbxNode {
    name: []const u8 = "",
    num_properties: u64 = 0,
    props_slice: []const u8 = "",
    children_slice: []const u8 = "",
    children_base_offset: usize = 0,

    /// Returns a zero-allocation typed property iterator for this node.
    fn properties(): FbxPropIter {
        return FbxPropIter.init(_.props_slice, _.num_properties);
    }

    /// Returns a zero-allocation iterator over child nodes of this node.
    fn children(is_64bit: bool): FbxNodeIter {
        return FbxNodeIter.initWithBase(_.children_slice, _.children_base_offset, is_64bit);
    }
}

/// Decoded typed FBX node property.
pub struct FbxProp {
    type_code: u8 = 0,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    bool_val: bool = false,
    raw_bytes: []const u8 = "",
    array_len: u32 = 0,
    array_encoding: u32 = 0,
    array_compressed_len: u32 = 0,
}

// ---------------------------------------------------------------------------
// Internal Little-Endian Readers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = readU32Le(data, off + 4) as u64;
    return lo | (hi << 32);
}

fn readI16Le(data: []const u8, off: usize): i16 {
    const bits = readU16Le(data, off);
    return @bitCast(bits);
}

fn readI32Le(data: []const u8, off: usize): i32 {
    const bits = readU32Le(data, off);
    return @bitCast(bits);
}

fn readI64Le(data: []const u8, off: usize): i64 {
    const bits = readU64Le(data, off);
    return @bitCast(bits);
}

fn readF32Le(data: []const u8, off: usize): f32 {
    const bits = readU32Le(data, off);
    return @bitCast(bits);
}

fn readF64Le(data: []const u8, off: usize): f64 {
    const bits = readU64Le(data, off);
    return @bitCast(bits);
}

fn arrayElemSize(type_code: u8): usize {
    if (type_code == 'b') return 1;
    if (type_code == 'f' || type_code == 'i') return 4;
    if (type_code == 'd' || type_code == 'l') return 8;
    return 0;
}

// ---------------------------------------------------------------------------
// Public API: Magic Detection & Header Parsing
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with the 23-byte FBX Binary magic
/// (`"Kaydara FBX Binary  \x00\x1A\x00"`) and has at least 27 bytes.
pub fn isFbxBinary(data: []const u8): bool {
    if (data.len < FBX_HEADER_SIZE) {
        return false;
    }
    if (!std.mem.eql(u8, data[0..21], FBX_MAGIC)) {
        return false;
    }
    return data[21] == 0x1A && data[22] == 0x00;
}

/// Parses the 27-byte FBX Binary file header.
pub fn parseFbxHeader(data: []const u8): FbxHeader? {
    if (!isFbxBinary(data)) {
        return null;
    }
    const version = readU32Le(data, 23);
    return FbxHeader {
        version: version,
        is_64bit: version >= FBX_V7500_THRESHOLD,
        nodes_offset: FBX_HEADER_SIZE,
    };
}

// ---------------------------------------------------------------------------
// Public API: Zero-Copy Scene Node Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over sibling FBX nodes (supports both 32-bit and 64-bit formats).
pub struct FbxNodeIter {
    src: []const u8 = "",
    base_offset: usize = 0,
    pos: usize = 0,
    is_64bit: bool = false,
    done: bool = true,

    /// Initializes an iterator over top-level nodes starting at `FBX_HEADER_SIZE` (27) in `file_data`.
    fn init(file_data: []const u8, is_64bit: bool): FbxNodeIter {
        if (file_data.len <= FBX_HEADER_SIZE) {
            return FbxNodeIter {
                src: "",
                base_offset: FBX_HEADER_SIZE,
                pos: 0,
                is_64bit: is_64bit,
                done: true,
            };
        }
        return FbxNodeIter {
            src: file_data[FBX_HEADER_SIZE..],
            base_offset: FBX_HEADER_SIZE,
            pos: 0,
            is_64bit: is_64bit,
            done: false,
        };
    }

    /// Initializes an iterator over a slice whose first byte resides at absolute file offset `base_offset`.
    fn initWithBase(slice: []const u8, base_offset: usize, is_64bit: bool): FbxNodeIter {
        return FbxNodeIter {
            src: slice,
            base_offset: base_offset,
            pos: 0,
            is_64bit: is_64bit,
            done: slice.len == 0,
        };
    }

    /// Decodes and returns the next sibling `FbxNode`, or `null` at NULL sentinel / end / malformed bounds.
    fn next(*_): FbxNode? {
        if (_.done) {
            return null;
        }
        const hdr_size: usize = if (_.is_64bit) 25 else 13;
        if (_.pos + hdr_size > _.src.len) {
            _.done = true;
            return null;
        }

        let end_offset: u64 = 0;
        let num_props: u64 = 0;
        let prop_list_len: u64 = 0;
        let name_len: u8 = 0;

        if (_.is_64bit) {
            end_offset = readU64Le(_.src, _.pos);
            num_props = readU64Le(_.src, _.pos + 8);
            prop_list_len = readU64Le(_.src, _.pos + 16);
            name_len = _.src[_.pos + 24];
        } else {
            end_offset = readU32Le(_.src, _.pos) as u64;
            num_props = readU32Le(_.src, _.pos + 4) as u64;
            prop_list_len = readU32Le(_.src, _.pos + 8) as u64;
            name_len = _.src[_.pos + 12];
        }

        // All-zero NULL sentinel terminates this sibling list cleanly
        if (end_offset == 0 && num_props == 0 && prop_list_len == 0 && name_len == 0) {
            _.done = true;
            return null;
        }

        const abs_cur: u64 = (_.base_offset + _.pos) as u64;
        const abs_limit: u64 = (_.base_offset + _.src.len) as u64;

        // Strictly validate end_offset > cur_pos && end_offset <= limit
        if (end_offset <= abs_cur || end_offset > abs_limit) {
            _.done = true;
            return null;
        }

        const rel_end: usize = (end_offset - (_.base_offset as u64)) as usize;
        const name_start: usize = _.pos + hdr_size;
        const name_end: usize = name_start + (name_len as usize);
        if (name_end > rel_end) {
            _.done = true;
            return null;
        }

        const remaining_after_name: u64 = (rel_end - name_end) as u64;
        if (prop_list_len > remaining_after_name) {
            _.done = true;
            return null;
        }
        const props_end: usize = name_end + (prop_list_len as usize);

        const node = FbxNode {
            name: _.src[name_start..name_end],
            num_properties: num_props,
            props_slice: _.src[name_end..props_end],
            children_slice: _.src[props_end..rel_end],
            children_base_offset: _.base_offset + props_end,
        };

        _.pos = rel_end;
        if (_.pos >= _.src.len) {
            _.done = true;
        }
        return node;
    }
}

// ---------------------------------------------------------------------------
// Public API: Zero-Alloc Typed Property Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over an FBX node's typed property list.
pub struct FbxPropIter {
    src: []const u8 = "",
    pos: usize = 0,
    remaining: u64 = 0,
    done: bool = true,

    fn init(props_slice: []const u8, num_properties: u64): FbxPropIter {
        return FbxPropIter {
            src: props_slice,
            pos: 0,
            remaining: num_properties,
            done: num_properties == 0 || props_slice.len == 0,
        };
    }

    fn next(*_): FbxProp? {
        if (_.done || _.remaining == 0 || _.pos >= _.src.len) {
            _.done = true;
            return null;
        }

        const code = _.src[_.pos];
        _.pos += 1;

        let prop = FbxProp { type_code: code };

        if (code == 'Y') {
            if (_.pos + 2 > _.src.len) {
                _.done = true;
                return null;
            }
            prop.int_val = readI16Le(_.src, _.pos) as i64;
            _.pos += 2;
        } else if (code == 'C') {
            if (_.pos + 1 > _.src.len) {
                _.done = true;
                return null;
            }
            const b = _.src[_.pos];
            prop.bool_val = b != 0;
            prop.int_val = b as i64;
            _.pos += 1;
        } else if (code == 'I') {
            if (_.pos + 4 > _.src.len) {
                _.done = true;
                return null;
            }
            prop.int_val = readI32Le(_.src, _.pos) as i64;
            _.pos += 4;
        } else if (code == 'F') {
            if (_.pos + 4 > _.src.len) {
                _.done = true;
                return null;
            }
            prop.float_val = readF32Le(_.src, _.pos) as f64;
            _.pos += 4;
        } else if (code == 'D') {
            if (_.pos + 8 > _.src.len) {
                _.done = true;
                return null;
            }
            prop.float_val = readF64Le(_.src, _.pos);
            _.pos += 8;
        } else if (code == 'L') {
            if (_.pos + 8 > _.src.len) {
                _.done = true;
                return null;
            }
            prop.int_val = readI64Le(_.src, _.pos);
            _.pos += 8;
        } else if (code == 'S' || code == 'R') {
            if (_.pos + 4 > _.src.len) {
                _.done = true;
                return null;
            }
            const slen = readU32Le(_.src, _.pos) as usize;
            _.pos += 4;
            if (slen > _.src.len - _.pos) {
                _.done = true;
                return null;
            }
            prop.raw_bytes = _.src[_.pos .. _.pos + slen];
            _.pos += slen;
        } else {
            const elem_size = arrayElemSize(code);
            if (elem_size == 0) {
                _.done = true;
                return null;
            }
            if (_.pos + 12 > _.src.len) {
                _.done = true;
                return null;
            }
            const arr_len = readU32Le(_.src, _.pos);
            const enc = readU32Le(_.src, _.pos + 4);
            const comp_len = readU32Le(_.src, _.pos + 8);
            _.pos += 12;

            if (enc > 1) {
                _.done = true;
                return null;
            }
            if (enc == 0) {
                const expected_bytes: u64 = (arr_len as u64) * (elem_size as u64);
                if ((comp_len as u64) != expected_bytes) {
                    _.done = true;
                    return null;
                }
            }
            const payload_len = comp_len as usize;
            if (payload_len > _.src.len - _.pos) {
                _.done = true;
                return null;
            }
            prop.array_len = arr_len;
            prop.array_encoding = enc;
            prop.array_compressed_len = comp_len;
            prop.raw_bytes = _.src[_.pos .. _.pos + payload_len];
            _.pos += payload_len;
        }

        _.remaining -= 1;
        if (_.remaining == 0) {
            _.done = true;
        }
        return prop;
    }
}
