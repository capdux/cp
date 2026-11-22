// Origin: Pure-CP Stdlib (Zero-Alloc Apache Arrow IPC Stream & File Parser) | Created: 2026-11-22

import std from "std";

pub const ARROW_MAGIC: []const u8 = "ARROW1";
pub const ARROW_FILE_HEADER_SIZE: usize = 8;
pub const ARROW_FILE_TRAILER_SIZE: usize = 10;
pub const ARROW_FILE_MIN_SIZE: usize = 18;
pub const ARROW_CONTINUATION: u32 = 0xFFFF_FFFF;

pub enum ArrowMetadataVersion {
    v1,
    v2,
    v3,
    v4,
    v5,
    unknown,
}

pub enum ArrowHeaderType {
    none,
    schema,
    dictionary_batch,
    record_batch,
    tensor,
    sparse_tensor,
    unknown,
}

pub struct ArrowMessageMeta {
    version: ArrowMetadataVersion,
    header_type: ArrowHeaderType,
    body_length: u64,
}

pub struct ArrowMessageHeader {
    metadata_len: u32,
    metadata: []const u8,
    header_bytes: usize,
    body_length: u64,
    body: []const u8,
    is_eos: bool,

    fn parseMeta(): ArrowMessageMeta? {
        if (_.is_eos || _.metadata.len == 0) {
            return null;
        }
        return parseArrowMessageMeta(_.metadata);
    }
}

pub struct ArrowFooterInfo {
    footer_len: u32,
    footer_offset: usize,
    footer_flatbuf: []const u8,
    stream_start: usize,
    stream_end: usize,
    stream_slice: []const u8,
}

fn readU16LE(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn readI32LE(buf: []const u8, off: usize): i32 {
    return @bitCast(readU32LE(buf, off));
}

fn readI16LE(buf: []const u8, off: usize): i16 {
    return @bitCast(readU16LE(buf, off));
}

fn readU64LE(buf: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(buf, off) as u64;
    const hi: u64 = readU32LE(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn readI64LE(buf: []const u8, off: usize): i64 {
    return @bitCast(readU64LE(buf, off));
}

fn align8(n: usize): usize {
    return (n + 7) & (~(7 as usize));
}

fn hasMagicAt(data: []const u8, off: usize): bool {
    if (off + 6 > data.len) {
        return false;
    }
    return data[off] == 'A' &&
        data[off + 1] == 'R' &&
        data[off + 2] == 'R' &&
        data[off + 3] == 'O' &&
        data[off + 4] == 'W' &&
        data[off + 5] == '1';
}

// Low-level safe FlatBuffers helpers
pub fn readFbU32(buf: []const u8, off: usize): u32? {
    if (off + 4 > buf.len) {
        return null;
    }
    return readU32LE(buf, off);
}

pub fn readFbRootTable(buf: []const u8): usize? {
    if (buf.len < 4) {
        return null;
    }
    const root: usize = readU32LE(buf, 0) as usize;
    if (root + 4 > buf.len) {
        return null;
    }
    return root;
}

pub fn readFbVtableOffset(buf: []const u8, table_pos: usize, field_voffset: usize): usize? {
    if (table_pos + 4 > buf.len) {
        return null;
    }
    const vtable_rel: i32 = readI32LE(buf, table_pos);
    const vtable_pos_i64: i64 = (table_pos as i64) - (vtable_rel as i64);
    if (vtable_pos_i64 < 0) {
        return null;
    }
    const vtable_pos: usize = vtable_pos_i64 as usize;
    if (vtable_pos + 4 > buf.len) {
        return null;
    }
    const vtable_size: usize = readU16LE(buf, vtable_pos) as usize;
    if (vtable_size < 4 || vtable_pos + vtable_size > buf.len) {
        return null;
    }
    if (field_voffset + 2 > vtable_size) {
        return null;
    }
    const field_rel: usize = readU16LE(buf, vtable_pos + field_voffset) as usize;
    if (field_rel == 0) {
        return null;
    }
    const field_pos = table_pos + field_rel;
    if (field_pos >= buf.len) {
        return null;
    }
    return field_pos;
}

fn mapMetadataVersion(raw: i16): ArrowMetadataVersion {
    if (raw == 0) return ArrowMetadataVersion.v1;
    if (raw == 1) return ArrowMetadataVersion.v2;
    if (raw == 2) return ArrowMetadataVersion.v3;
    if (raw == 3) return ArrowMetadataVersion.v4;
    if (raw == 4) return ArrowMetadataVersion.v5;
    return ArrowMetadataVersion.unknown;
}

fn mapHeaderType(raw: u8): ArrowHeaderType {
    if (raw == 0) return ArrowHeaderType.none;
    if (raw == 1) return ArrowHeaderType.schema;
    if (raw == 2) return ArrowHeaderType.dictionary_batch;
    if (raw == 3) return ArrowHeaderType.record_batch;
    if (raw == 4) return ArrowHeaderType.tensor;
    if (raw == 5) return ArrowHeaderType.sparse_tensor;
    return ArrowHeaderType.unknown;
}

pub fn parseArrowMessageMeta(metadata: []const u8): ArrowMessageMeta? {
    const root = readFbRootTable(metadata) ?? return null;

    // Verify vtable itself is valid
    const vtable_rel: i32 = readI32LE(metadata, root);
    const vtable_pos_i64: i64 = (root as i64) - (vtable_rel as i64);
    if (vtable_pos_i64 < 0) {
        return null;
    }
    const vtable_pos: usize = vtable_pos_i64 as usize;
    if (vtable_pos + 4 > metadata.len) {
        return null;
    }
    const vtable_size: usize = readU16LE(metadata, vtable_pos) as usize;
    if (vtable_size < 4 || vtable_pos + vtable_size > metadata.len) {
        return null;
    }

    // Slot 0 (voffset 4): version (i16, default 0 = V1)
    let raw_ver: i16 = 0;
    if (ver_pos := readFbVtableOffset(metadata, root, 4)) {
        if (ver_pos + 2 > metadata.len) return null;
        raw_ver = readI16LE(metadata, ver_pos);
    }

    // Slot 1 (voffset 6): header_type (u8, default 0 = NONE)
    let raw_hdr: u8 = 0;
    if (hdr_pos := readFbVtableOffset(metadata, root, 6)) {
        raw_hdr = metadata[hdr_pos];
    }

    // Slot 3 (voffset 10): bodyLength (i64, default 0)
    let body_len_i64: i64 = 0;
    if (bl_pos := readFbVtableOffset(metadata, root, 10)) {
        if (bl_pos + 8 > metadata.len) return null;
        body_len_i64 = readI64LE(metadata, bl_pos);
    }
    if (body_len_i64 < 0) {
        return null;
    }

    return ArrowMessageMeta{
        .version = mapMetadataVersion(raw_ver),
        .header_type = mapHeaderType(raw_hdr),
        .body_length = body_len_i64 as u64,
    };
}

pub fn isArrowFile(data: []const u8): bool {
    if (data.len < ARROW_FILE_MIN_SIZE) {
        return false;
    }
    return hasMagicAt(data, 0) && hasMagicAt(data, data.len - 6);
}

pub fn isArrowStream(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    if (isArrowFile(data)) {
        return false;
    }
    const first = readU32LE(data, 0);
    if (first == ARROW_CONTINUATION) {
        if (data.len < 8) {
            return false;
        }
        const meta_len = readU32LE(data, 4);
        if (meta_len == 0) {
            return true;
        }
        return (8 as u64) + (meta_len as u64) <= (data.len as u64);
    }
    // Legacy format (pre-0.15): 4-byte LE metadata_len without continuation marker
    if (first == 0 || hasMagicAt(data, 0)) {
        return false;
    }
    const end_u64: u64 = (4 as u64) + (first as u64);
    if (end_u64 > (data.len as u64)) {
        return false;
    }
    return readFbRootTable(data[4..(end_u64 as usize)]) != null;
}

pub fn parseArrowFileFooter(data: []const u8): ArrowFooterInfo? {
    if (!isArrowFile(data)) {
        return null;
    }
    const footer_end = data.len - ARROW_FILE_TRAILER_SIZE;
    const footer_len = readU32LE(data, footer_end);
    const max_footer = footer_end - ARROW_FILE_HEADER_SIZE;
    if ((footer_len as usize) > max_footer) {
        return null;
    }
    const footer_start = footer_end - (footer_len as usize);

    return ArrowFooterInfo{
        .footer_len = footer_len,
        .footer_offset = footer_start,
        .footer_flatbuf = data[footer_start..footer_end],
        .stream_start = ARROW_FILE_HEADER_SIZE,
        .stream_end = footer_start,
        .stream_slice = data[ARROW_FILE_HEADER_SIZE..footer_start],
    };
}

pub struct ArrowMessageIter {
    data: []const u8,
    pos: usize,
    done: bool,

    fn init(data: []const u8): ArrowMessageIter {
        return ArrowMessageIter{
            .data = data,
            .pos = 0,
            .done = false,
        };
    }

    fn skipBody(*_, body_length: u64): bool {
        if (_.done) {
            return false;
        }
        const avail: u64 = (_.data.len - _.pos) as u64;
        if (body_length > avail) {
            _.done = true;
            return false;
        }
        const end_pos = _.pos + (body_length as usize);
        const aligned = align8(end_pos);
        if (aligned <= _.data.len) {
            _.pos = aligned;
        } else {
            _.pos = end_pos;
        }
        return true;
    }

    fn next(*_): ArrowMessageHeader? {
        if (_.done || _.pos + 4 > _.data.len) {
            _.done = true;
            return null;
        }

        const start = _.pos;
        const first = readU32LE(_.data, start);
        let meta_len: u32 = 0;
        let meta_start: usize = 0;

        if (first == ARROW_CONTINUATION) {
            if (start + 8 > _.data.len) {
                _.done = true;
                return null;
            }
            meta_len = readU32LE(_.data, start + 4);
            meta_start = start + 8;
        } else {
            meta_len = first;
            meta_start = start + 4;
        }

        if (meta_len == 0) {
            _.pos = meta_start;
            _.done = true;
            return ArrowMessageHeader{
                .metadata_len = 0,
                .metadata = _.data[meta_start..meta_start],
                .header_bytes = meta_start - start,
                .body_length = 0,
                .body = _.data[meta_start..meta_start],
                .is_eos = true,
            };
        }

        const avail_meta: u64 = (_.data.len - meta_start) as u64;
        if ((meta_len as u64) > avail_meta) {
            _.done = true;
            return null;
        }

        const meta_end = meta_start + (meta_len as usize);
        const metadata = _.data[meta_start..meta_end];
        const header_bytes = meta_end - start;

        let body_length: u64 = 0;
        let body_start = meta_end;
        let body_end = meta_end;

        if (msg_meta := parseArrowMessageMeta(metadata)) {
            if (msg_meta.body_length > 0) {
                body_length = msg_meta.body_length;
                const avail_body: u64 = (_.data.len - body_start) as u64;
                if (body_length > avail_body) {
                    _.done = true;
                    return null;
                }
                body_end = body_start + (body_length as usize);
            }
        }

        const aligned_next = align8(body_end);
        if (body_length > 0 && aligned_next <= _.data.len) {
            _.pos = aligned_next;
        } else {
            _.pos = body_end;
        }

        return ArrowMessageHeader{
            .metadata_len = meta_len,
            .metadata = metadata,
            .header_bytes = header_bytes,
            .body_length = body_length,
            .body = _.data[body_start..body_end],
            .is_eos = false,
        };
    }
}
