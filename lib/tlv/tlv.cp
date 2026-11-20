// Source: cp/lib/tlv/tlv.cp (Binary TLV Codec, Length-Prefixed Stream Framer & Castagnoli CRC32C)
// Pure CP zero-C-dependency binary TLV encoder/decoder with 32-level nesting guard,
// streaming LengthPrefixedFramer (half-packet & sticky-packet reassembly), and Castagnoli CRC32C.

const std = @import("std");

pub const TLV_MAX_DEPTH: usize = 32;

pub err TlvError {
    Truncated,
    TagNotFound,
    InvalidLength,
    FrameTooLarge,
    InvalidChecksum,
    MaxDepthExceeded,
    TypeMismatch,
    OutOfMemory,
}

pub enum FrameHeaderKind {
    U16Be,
    U32Be,
    U32Le,
}

fn makeCrc32cTable(): [256]u32 {
    let table: [256]u32 = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let crc: u32 = i as u32;
        let bit: usize = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0x82F63B78;
            } else {
                crc = crc >> 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

const CRC32C_TABLE: [256]u32 = makeCrc32cTable();

pub fn crc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const idx: usize = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
        crc = CRC32C_TABLE[idx] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

fn readU16Be(buf: []const u8): u16 {
    return ((buf[0] as u16) << 8) | (buf[1] as u16);
}

fn readU32Be(buf: []const u8): u32 {
    return ((buf[0] as u32) << 24) |
        ((buf[1] as u32) << 16) |
        ((buf[2] as u32) << 8) |
        (buf[3] as u32);
}

fn readU32Le(buf: []const u8): u32 {
    return (buf[0] as u32) |
        ((buf[1] as u32) << 8) |
        ((buf[2] as u32) << 16) |
        ((buf[3] as u32) << 24);
}

fn readU64Be(buf: []const u8): u64 {
    return ((buf[0] as u64) << 56) |
        ((buf[1] as u64) << 48) |
        ((buf[2] as u64) << 40) |
        ((buf[3] as u64) << 32) |
        ((buf[4] as u64) << 24) |
        ((buf[5] as u64) << 16) |
        ((buf[6] as u64) << 8) |
        (buf[7] as u64);
}

fn writeU16Be(buf: []u8, v: u16) {
    buf[0] = ((v >> 8) & 0xFF) as u8;
    buf[1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, v: u32) {
    buf[0] = ((v >> 24) & 0xFF) as u8;
    buf[1] = ((v >> 16) & 0xFF) as u8;
    buf[2] = ((v >> 8) & 0xFF) as u8;
    buf[3] = (v & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, v: u32) {
    buf[0] = (v & 0xFF) as u8;
    buf[1] = ((v >> 8) & 0xFF) as u8;
    buf[2] = ((v >> 16) & 0xFF) as u8;
    buf[3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Be(buf: []u8, v: u64) {
    buf[0] = ((v >> 56) & 0xFF) as u8;
    buf[1] = ((v >> 48) & 0xFF) as u8;
    buf[2] = ((v >> 40) & 0xFF) as u8;
    buf[3] = ((v >> 32) & 0xFF) as u8;
    buf[4] = ((v >> 24) & 0xFF) as u8;
    buf[5] = ((v >> 16) & 0xFF) as u8;
    buf[6] = ((v >> 8) & 0xFF) as u8;
    buf[7] = (v & 0xFF) as u8;
}

struct ByteBuf {
    alloc: std.mem.Allocator = undefined,
    data: []u8 = [],
    len: usize = 0,

    static fn init(alloc: std.mem.Allocator): ByteBuf {
        return ByteBuf{
            alloc: alloc,
            data: [],
            len: 0,
        };
    }

    fn deinit(*_) {
        if (_.data.len > 0) {
            _.alloc.free(_.data);
            _.data = [];
        }
        _.len = 0;
    }

    fn ensureCapacity(*_, needed: usize): ! TlvError {
        if (needed <= _.data.len) {
            return;
        }
        let new_cap: usize = if (_.data.len == 0) 64 else _.data.len;
        while (new_cap < needed) {
            if (new_cap > (usize.max / 2)) {
                new_cap = needed;
                break;
            }
            new_cap = new_cap * 2;
        }
        const new_buf = _.alloc.alloc(u8, new_cap) catch return TlvError.OutOfMemory;
        if (_.len > 0) {
            std.mem.copyForwards(u8, new_buf[0.._.len], _.data[0.._.len]);
        }
        if (_.data.len > 0) {
            _.alloc.free(_.data);
        }
        _.data = new_buf;
    }

    fn appendSlice(*_, s: []const u8): ! TlvError {
        if (s.len == 0) {
            return;
        }
        _.ensureCapacity(_.len + s.len)?;
        std.mem.copyForwards(u8, _.data[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    fn slice(): []const u8 {
        return _.data[0.._.len];
    }

    fn toOwnedSlice(*_): ([]u8) ! TlvError {
        const out = _.alloc.dupe(u8, _.data[0.._.len]) catch return TlvError.OutOfMemory;
        _.deinit();
        return out;
    }
}

pub struct TlvNestToken {
    tag: u16 = 0,
    len_offset: usize = 0,
    depth: usize = 0,
}

pub struct TlvField {
    tag: u16 = 0,
    value: []const u8 = "",
    depth: usize = 0,

    pub fn asBytes(): []const u8 {
        return _.value;
    }

    pub fn asU64(): u64 ! TlvError {
        if (_.value.len != 8) {
            return TlvError.TypeMismatch;
        }
        return readU64Be(_.value);
    }

    pub fn asI64(): i64 ! TlvError {
        if (_.value.len != 8) {
            return TlvError.TypeMismatch;
        }
        const u = readU64Be(_.value);
        return @as(i64, @bitCast(u));
    }

    pub fn asBool(): bool ! TlvError {
        if (_.value.len != 1) {
            return TlvError.TypeMismatch;
        }
        if (_.value[0] == 0) return false;
        if (_.value[0] == 1) return true;
        return TlvError.TypeMismatch;
    }

    pub fn asStr(): ([]const u8) ! TlvError {
        if (!std.unicode.utf8ValidateSlice(_.value)) {
            return TlvError.TypeMismatch;
        }
        return _.value;
    }

    pub fn reader(): TlvReader ! TlvError {
        return TlvReader.initWithDepth(_.value, _.depth + 1);
    }
}

pub struct TlvWriter {
    buf: ByteBuf = ByteBuf{},
    depth: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): TlvWriter {
        return TlvWriter{
            buf: ByteBuf.init(alloc),
            depth: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit();
        _.depth = 0;
    }

    pub fn reset(*_) {
        _.buf.len = 0;
        _.depth = 0;
    }

    pub fn bytes(): []const u8 {
        return _.buf.slice();
    }

    pub fn toOwnedSlice(*_): ([]u8) ! TlvError {
        _.depth = 0;
        return _.buf.toOwnedSlice();
    }

    pub fn writeBytes(*_, tag: u16, data: []const u8): ! TlvError {
        if (data.len > 0xFFFFFFFF) {
            return TlvError.InvalidLength;
        }
        let hdr: [6]u8 = [0; 6];
        writeU16Be(hdr[0..2], tag);
        writeU32Be(hdr[2..6], data.len as u32);
        _.buf.appendSlice(&hdr)?;
        _.buf.appendSlice(data)?;
    }

    pub fn writeU64(*_, tag: u16, v: u64): ! TlvError {
        let raw: [8]u8 = [0; 8];
        writeU64Be(&raw, v);
        return _.writeBytes(tag, &raw);
    }

    pub fn writeI64(*_, tag: u16, v: i64): ! TlvError {
        const u = @as(u64, @bitCast(v));
        return _.writeU64(tag, u);
    }

    pub fn writeStr(*_, tag: u16, s: []const u8): ! TlvError {
        return _.writeBytes(tag, s);
    }

    pub fn writeBool(*_, tag: u16, b: bool): ! TlvError {
        let raw: [1]u8 = [if (b) 1 else 0];
        return _.writeBytes(tag, &raw);
    }

    pub fn beginNested(*_, tag: u16): TlvNestToken ! TlvError {
        if (_.depth >= TLV_MAX_DEPTH) {
            return TlvError.MaxDepthExceeded;
        }
        let hdr: [6]u8 = [0; 6];
        writeU16Be(hdr[0..2], tag);
        _.buf.appendSlice(&hdr)?;
        const len_offset = _.buf.len - 4;
        _.depth += 1;
        return TlvNestToken{
            tag: tag,
            len_offset: len_offset,
            depth: _.depth,
        };
    }

    pub fn endNested(*_, token: TlvNestToken): ! TlvError {
        if (_.depth == 0 || _.depth != token.depth || token.len_offset + 4 > _.buf.len) {
            return TlvError.InvalidLength;
        }
        const payload_len = _.buf.len - (token.len_offset + 4);
        if (payload_len > 0xFFFFFFFF) {
            return TlvError.InvalidLength;
        }
        writeU32Be(_.buf.data[token.len_offset .. token.len_offset + 4], payload_len as u32);
        _.depth -= 1;
    }
}

pub struct TlvReader {
    src: []const u8 = "",
    pos: usize = 0,
    depth: usize = 0,

    pub static fn init(data: []const u8): TlvReader {
        return TlvReader{
            src: data,
            pos: 0,
            depth: 0,
        };
    }

    pub static fn initWithDepth(data: []const u8, depth: usize): TlvReader ! TlvError {
        if (depth > TLV_MAX_DEPTH) {
            return TlvError.MaxDepthExceeded;
        }
        return TlvReader{
            src: data,
            pos: 0,
            depth: depth,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    pub fn remaining(): usize {
        if (_.pos >= _.src.len) return 0;
        return _.src.len - _.pos;
    }

    pub fn next(*_): TlvField? ! TlvError {
        if (_.pos >= _.src.len) {
            return null;
        }
        if (_.src.len - _.pos < 6) {
            return TlvError.Truncated;
        }
        const tag = readU16Be(_.src[_.pos .. _.pos + 2]);
        const len_u32 = readU32Be(_.src[_.pos + 2 .. _.pos + 6]);
        const len_usize: usize = len_u32 as usize;
        const val_start = _.pos + 6;
        if (len_usize > _.src.len - val_start) {
            return TlvError.Truncated;
        }
        const val_end = val_start + len_usize;
        _.pos = val_end;
        return TlvField{
            tag: tag,
            value: _.src[val_start..val_end],
            depth: _.depth,
        };
    }

    pub fn findTag(tag: u16): TlvField ! TlvError {
        let scan_pos: usize = 0;
        while (scan_pos < _.src.len) {
            if (_.src.len - scan_pos < 6) {
                return TlvError.Truncated;
            }
            const cur_tag = readU16Be(_.src[scan_pos .. scan_pos + 2]);
            const len_u32 = readU32Be(_.src[scan_pos + 2 .. scan_pos + 6]);
            const len_usize: usize = len_u32 as usize;
            const val_start = scan_pos + 6;
            if (len_usize > _.src.len - val_start) {
                return TlvError.Truncated;
            }
            const val_end = val_start + len_usize;
            if (cur_tag == tag) {
                return TlvField{
                    tag: cur_tag,
                    value: _.src[val_start..val_end],
                    depth: _.depth,
                };
            }
            scan_pos = val_end;
        }
        return TlvError.TagNotFound;
    }
}

pub struct LengthPrefixedFramer {
    kind: FrameHeaderKind = FrameHeaderKind.U32Be,
    max_frame_bytes: usize = 1048576,
    with_crc32c: bool = true,
    buf: ByteBuf = ByteBuf{},
    head: usize = 0,

    pub static fn init(
        alloc: std.mem.Allocator,
        kind: FrameHeaderKind,
        max_frame_bytes: usize,
        with_crc32c: bool,
    ): LengthPrefixedFramer {
        return LengthPrefixedFramer{
            kind: kind,
            max_frame_bytes: max_frame_bytes,
            with_crc32c: with_crc32c,
            buf: ByteBuf.init(alloc),
            head: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit();
        _.head = 0;
    }

    pub fn bufferedBytes(): usize {
        if (_.head >= _.buf.len) return 0;
        return _.buf.len - _.head;
    }

    fn headerSize(): usize {
        return switch (_.kind) {
            .U16Be => 2,
            .U32Be => 4,
            .U32Le => 4,
        };
    }

    pub fn encodeFrame(alloc: std.mem.Allocator, payload: []const u8): ([]u8) ! TlvError {
        if (payload.len > _.max_frame_bytes) {
            return TlvError.FrameTooLarge;
        }
        switch (_.kind) {
            .U16Be => {
                if (payload.len > 65535) {
                    return TlvError.FrameTooLarge;
                }
            },
            .U32Be | .U32Le => {
                if (payload.len > 0xFFFFFFFF) {
                    return TlvError.FrameTooLarge;
                }
            },
        }

        const hdr_size = _.headerSize();
        const trailer_size: usize = if (_.with_crc32c) 4 else 0;
        const total = hdr_size + payload.len + trailer_size;
        const out = alloc.alloc(u8, total) catch return TlvError.OutOfMemory;

        switch (_.kind) {
            .U16Be => writeU16Be(out[0..2], payload.len as u16),
            .U32Be => writeU32Be(out[0..4], payload.len as u32),
            .U32Le => writeU32Le(out[0..4], payload.len as u32),
        }
        if (payload.len > 0) {
            std.mem.copyForwards(u8, out[hdr_size .. hdr_size + payload.len], payload);
        }
        if (_.with_crc32c) {
            const sum = crc32c(payload);
            writeU32Be(out[hdr_size + payload.len .. total], sum);
        }
        return out;
    }

    pub fn feed(*_, chunk: []const u8): ! TlvError {
        if (chunk.len == 0) {
            return;
        }
        _.compactIfNeeded();
        _.buf.appendSlice(chunk)?;
    }

    fn compactIfNeeded(*_) {
        if (_.head >= _.buf.len) {
            _.buf.len = 0;
            _.head = 0;
        } else if (_.head > 4096 && _.head * 2 >= _.buf.len) {
            const rem = _.buf.len - _.head;
            std.mem.copyForwards(u8, _.buf.data[0..rem], _.buf.data[_.head.._.buf.len]);
            _.buf.len = rem;
            _.head = 0;
        }
    }

    pub fn nextFrame(*_, alloc: std.mem.Allocator): ([]u8)? ! TlvError {
        const avail = _.bufferedBytes();
        const hdr_size = _.headerSize();
        if (avail < hdr_size) {
            return null;
        }

        const hdr_slice = _.buf.data[_.head .. _.head + hdr_size];
        const payload_len: usize = switch (_.kind) {
            .U16Be => readU16Be(hdr_slice) as usize,
            .U32Be => readU32Be(hdr_slice) as usize,
            .U32Le => readU32Le(hdr_slice) as usize,
        };

        if (payload_len > _.max_frame_bytes) {
            return TlvError.FrameTooLarge;
        }

        const trailer_size: usize = if (_.with_crc32c) 4 else 0;
        const total_needed = hdr_size + payload_len + trailer_size;
        if (avail < total_needed) {
            return null;
        }

        const payload_start = _.head + hdr_size;
        const payload_end = payload_start + payload_len;
        const payload_slice = _.buf.data[payload_start..payload_end];

        if (_.with_crc32c) {
            const expected_crc = readU32Be(_.buf.data[payload_end .. payload_end + 4]);
            const actual_crc = crc32c(payload_slice);
            if (actual_crc != expected_crc) {
                _.head += total_needed;
                _.compactIfNeeded();
                return TlvError.InvalidChecksum;
            }
        }

        const out = alloc.alloc(u8, payload_len) catch return TlvError.OutOfMemory;
        if (payload_len > 0) {
            std.mem.copyForwards(u8, out, payload_slice);
        }
        _.head += total_needed;
        _.compactIfNeeded();
        return out;
    }
}
