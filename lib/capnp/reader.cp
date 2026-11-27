// Source: cp/lib/capnp/reader.cp (Cap'n Proto zero-copy MessageReader, StructReader, and ListReader)
const std = @import("std");
import {
    CapnpError,
    PointerKind,
    ElementSize,
    TraversalLimit,
    WORD_SIZE,
    MAX_SEGMENTS,
    readU16LE,
    readU32LE,
    readU64LE,
} from "./types.cp";
import {
    ByteSlice,
    resolvePointer,
} from "./pointer.cp";

pub struct StructReader {
    segments: []const ByteSlice,
    seg_id: u32,
    data_word_offset: u32,
    data_words: u16,
    ptr_words: u16,
    limit: *TraversalLimit,

    fn dataBytes(): []const u8 {
        const seg = _.segments[_.seg_id];
        const start: usize = @as(usize, _.data_word_offset) * WORD_SIZE;
        const len: usize = @as(usize, _.data_words) * WORD_SIZE;
        return seg[start .. start + len];
    }

    pub fn readBool(bit_offset: u32): bool {
        const byte_idx: usize = @as(usize, bit_offset) / 8;
        const db = _.dataBytes();
        if (byte_idx >= db.len) {
            return false;
        }
        const bit_shift: u3 = @intCast(bit_offset % 8);
        return ((db[byte_idx] >> bit_shift) & 1) != 0;
    }

    pub fn readBoolOrDefault(bit_offset: u32, default_val: bool): bool {
        return _.readBool(bit_offset) != default_val;
    }

    pub fn readU8(byte_offset: u32): u8 {
        const db = _.dataBytes();
        if (@as(usize, byte_offset) >= db.len) {
            return 0;
        }
        return db[byte_offset];
    }

    pub fn readU8OrDefault(byte_offset: u32, default_val: u8): u8 {
        return _.readU8(byte_offset) ^ default_val;
    }

    pub fn readI8(byte_offset: u32): i8 {
        return @bitCast(_.readU8(byte_offset));
    }

    pub fn readU16(elem_idx: u32): u16 {
        const byte_off: usize = @as(usize, elem_idx) * 2;
        const db = _.dataBytes();
        if (byte_off + 2 > db.len) {
            return 0;
        }
        return readU16LE(db, byte_off);
    }

    pub fn readU16OrDefault(elem_idx: u32, default_val: u16): u16 {
        return _.readU16(elem_idx) ^ default_val;
    }

    pub fn readI16(elem_idx: u32): i16 {
        return @bitCast(_.readU16(elem_idx));
    }

    pub fn readU32(elem_idx: u32): u32 {
        const byte_off: usize = @as(usize, elem_idx) * 4;
        const db = _.dataBytes();
        if (byte_off + 4 > db.len) {
            return 0;
        }
        return readU32LE(db, byte_off);
    }

    pub fn readU32OrDefault(elem_idx: u32, default_val: u32): u32 {
        return _.readU32(elem_idx) ^ default_val;
    }

    pub fn readI32(elem_idx: u32): i32 {
        return @bitCast(_.readU32(elem_idx));
    }

    pub fn readI32OrDefault(elem_idx: u32, default_val: i32): i32 {
        const def_u: u32 = @bitCast(default_val);
        return @bitCast(_.readU32(elem_idx) ^ def_u);
    }

    pub fn readU64(elem_idx: u32): u64 {
        const byte_off: usize = @as(usize, elem_idx) * 8;
        const db = _.dataBytes();
        if (byte_off + 8 > db.len) {
            return 0;
        }
        return readU64LE(db, byte_off);
    }

    pub fn readU64OrDefault(elem_idx: u32, default_val: u64): u64 {
        return _.readU64(elem_idx) ^ default_val;
    }

    pub fn readI64(elem_idx: u32): i64 {
        return @bitCast(_.readU64(elem_idx));
    }

    pub fn readF32(elem_idx: u32): f32 {
        return @bitCast(_.readU32(elem_idx));
    }

    pub fn readF64(elem_idx: u32): f64 {
        return @bitCast(_.readU64(elem_idx));
    }

    pub fn hasPointer(ptr_idx: u16): bool {
        if (ptr_idx >= _.ptr_words) {
            return false;
        }
        const ptr_word = _.data_word_offset + @as(u32, _.data_words) + @as(u32, ptr_idx);
        const seg = _.segments[_.seg_id];
        const byte_off: usize = @as(usize, ptr_word) * WORD_SIZE;
        if (byte_off + WORD_SIZE > seg.len) {
            return false;
        }
        return readU64LE(seg, byte_off) != 0;
    }

    pub fn readStruct(*_, ptr_idx: u16): CapnpError!StructReader? {
        if (ptr_idx >= _.ptr_words) {
            return null;
        }
        const next_nest = try _.limit.enterNesting();
        let child_limit = TraversalLimit.init(_.limit.words_remaining, next_nest);
        const ptr_word = _.data_word_offset + @as(u32, _.data_words) + @as(u32, ptr_idx);
        const res = try resolvePointer(_.segments, _.seg_id, ptr_word, &child_limit);
        _.limit.words_remaining = child_limit.words_remaining;
        if (res.is_null) {
            return null;
        }
        if (res.kind != .Struct) {
            return CapnpError.InvalidPointer;
        }
        return StructReader{
            .segments = _.segments,
            .seg_id = res.seg_id,
            .data_word_offset = res.word_offset,
            .data_words = res.data_words,
            .ptr_words = res.ptr_words,
            .limit = _.limit,
        };
    }

    pub fn readList(*_, ptr_idx: u16): CapnpError!ListReader? {
        if (ptr_idx >= _.ptr_words) {
            return null;
        }
        const next_nest = try _.limit.enterNesting();
        let child_limit = TraversalLimit.init(_.limit.words_remaining, next_nest);
        const ptr_word = _.data_word_offset + @as(u32, _.data_words) + @as(u32, ptr_idx);
        const res = try resolvePointer(_.segments, _.seg_id, ptr_word, &child_limit);
        _.limit.words_remaining = child_limit.words_remaining;
        if (res.is_null) {
            return null;
        }
        if (res.kind != .List) {
            return CapnpError.InvalidPointer;
        }
        return ListReader{
            .segments = _.segments,
            .seg_id = res.seg_id,
            .word_offset = res.word_offset,
            .elem_size = res.elem_size,
            .elem_count = res.elem_count,
            .struct_data_words = res.data_words,
            .struct_ptr_words = res.ptr_words,
            .limit = _.limit,
        };
    }

    pub fn readData(*_, ptr_idx: u16): CapnpError![]const u8 {
        const list_opt = try _.readList(ptr_idx);
        if (list_opt == null) {
            return "";
        }
        const lr = list_opt.?;
        if (lr.elem_size != .Byte) {
            return CapnpError.InvalidPointer;
        }
        const seg = _.segments[lr.seg_id];
        const start: usize = @as(usize, lr.word_offset) * WORD_SIZE;
        return seg[start .. start + @as(usize, lr.elem_count)];
    }

    pub fn readText(*_, ptr_idx: u16): CapnpError![]const u8 {
        const raw = try _.readData(ptr_idx);
        if (raw.len == 0) {
            return "";
        }
        if (raw[raw.len - 1] != 0) {
            return CapnpError.InvalidText;
        }
        return raw[0 .. raw.len - 1];
    }
}

pub struct ListReader {
    segments: []const ByteSlice,
    seg_id: u32,
    word_offset: u32,
    elem_size: ElementSize,
    elem_count: u32,
    struct_data_words: u16,
    struct_ptr_words: u16,
    limit: *TraversalLimit,

    pub fn len(): u32 {
        return _.elem_count;
    }

    fn baseByteOffset(): usize {
        return @as(usize, _.word_offset) * WORD_SIZE;
    }

    pub fn getBool(idx: u32): CapnpError!bool {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Bit) {
            return CapnpError.UnsupportedElementSize;
        }
        const seg = _.segments[_.seg_id];
        const byte_off = _.baseByteOffset() + (@as(usize, idx) / 8);
        const bit_shift: u3 = @intCast(idx % 8);
        return ((seg[byte_off] >> bit_shift) & 1) != 0;
    }

    pub fn getU8(idx: u32): CapnpError!u8 {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Byte) {
            return CapnpError.UnsupportedElementSize;
        }
        const seg = _.segments[_.seg_id];
        return seg[_.baseByteOffset() + @as(usize, idx)];
    }

    pub fn getU16(idx: u32): CapnpError!u16 {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .TwoBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        const seg = _.segments[_.seg_id];
        return readU16LE(seg, _.baseByteOffset() + @as(usize, idx) * 2);
    }

    pub fn getU32(idx: u32): CapnpError!u32 {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .FourBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        const seg = _.segments[_.seg_id];
        return readU32LE(seg, _.baseByteOffset() + @as(usize, idx) * 4);
    }

    pub fn getI32(idx: u32): CapnpError!i32 {
        return @bitCast(try _.getU32(idx));
    }

    pub fn getU64(idx: u32): CapnpError!u64 {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .EightBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        const seg = _.segments[_.seg_id];
        return readU64LE(seg, _.baseByteOffset() + @as(usize, idx) * 8);
    }

    pub fn getF64(idx: u32): CapnpError!f64 {
        return @bitCast(try _.getU64(idx));
    }

    pub fn getStruct(*_, idx: u32): CapnpError!StructReader {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size == .Composite) {
            const elem_words: u32 = @as(u32, _.struct_data_words) + @as(u32, _.struct_ptr_words);
            return StructReader{
                .segments = _.segments,
                .seg_id = _.seg_id,
                .data_word_offset = _.word_offset + idx * elem_words,
                .data_words = _.struct_data_words,
                .ptr_words = _.struct_ptr_words,
                .limit = _.limit,
            };
        } else if (_.elem_size == .Pointer) {
            const next_nest = try _.limit.enterNesting();
            let child_limit = TraversalLimit.init(_.limit.words_remaining, next_nest);
            const ptr_word = _.word_offset + idx;
            const res = try resolvePointer(_.segments, _.seg_id, ptr_word, &child_limit);
            _.limit.words_remaining = child_limit.words_remaining;
            if (res.is_null or res.kind != .Struct) {
                return CapnpError.InvalidPointer;
            }
            return StructReader{
                .segments = _.segments,
                .seg_id = res.seg_id,
                .data_word_offset = res.word_offset,
                .data_words = res.data_words,
                .ptr_words = res.ptr_words,
                .limit = _.limit,
            };
        }
        return CapnpError.UnsupportedElementSize;
    }

    pub fn getText(*_, idx: u32): CapnpError![]const u8 {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Pointer) {
            return CapnpError.UnsupportedElementSize;
        }
        const next_nest = try _.limit.enterNesting();
        let child_limit = TraversalLimit.init(_.limit.words_remaining, next_nest);
        const ptr_word = _.word_offset + idx;
        const res = try resolvePointer(_.segments, _.seg_id, ptr_word, &child_limit);
        _.limit.words_remaining = child_limit.words_remaining;
        if (res.is_null) {
            return "";
        }
        if (res.kind != .List or res.elem_size != .Byte) {
            return CapnpError.InvalidPointer;
        }
        const seg = _.segments[res.seg_id];
        const start: usize = @as(usize, res.word_offset) * WORD_SIZE;
        const raw = seg[start .. start + @as(usize, res.elem_count)];
        if (raw.len == 0) {
            return "";
        }
        if (raw[raw.len - 1] != 0) {
            return CapnpError.InvalidText;
        }
        return raw[0 .. raw.len - 1];
    }
}

pub struct MessageReader {
    alloc: std.mem.Allocator,
    segments: []ByteSlice,
    limit: TraversalLimit,

    pub static fn init(alloc: std.mem.Allocator, frame_bytes: []const u8, limit: TraversalLimit): CapnpError!MessageReader {
        if (frame_bytes.len < 4) {
            return CapnpError.TruncatedFrame;
        }
        const seg_count_minus_1 = readU32LE(frame_bytes, 0);
        const seg_count_u64: u64 = @as(u64, seg_count_minus_1) + 1;
        if (seg_count_u64 > @as(u64, MAX_SEGMENTS)) {
            return CapnpError.TooManySegments;
        }
        const seg_count: usize = @intCast(seg_count_u64);

        const raw_header_bytes: usize = @as(usize, 4) + seg_count * @as(usize, 4);
        const padding_bytes: usize = if ((seg_count % 2) == 0) 4 else 0;
        const header_bytes: usize = raw_header_bytes + padding_bytes;
        if (frame_bytes.len < header_bytes) {
            return CapnpError.TruncatedFrame;
        }

        let segs = alloc.alloc(ByteSlice, seg_count) catch {
            return CapnpError.AllocationFailed;
        };
        errdefer alloc.free(segs);

        let cursor: usize = header_bytes;
        let i: usize = 0;
        while (i < seg_count) {
            const seg_words: usize = @intCast(readU32LE(frame_bytes, 4 + i * 4));
            const seg_bytes: usize = seg_words * WORD_SIZE;
            if (cursor + seg_bytes > frame_bytes.len) {
                return CapnpError.TruncatedFrame;
            }
            segs[i] = frame_bytes[cursor .. cursor + seg_bytes];
            cursor += seg_bytes;
            i += 1;
        }

        return MessageReader{
            .alloc = alloc,
            .segments = segs,
            .limit = limit,
        };
    }

    pub static fn initDefault(alloc: std.mem.Allocator, frame_bytes: []const u8): CapnpError!MessageReader {
        return MessageReader.init(alloc, frame_bytes, TraversalLimit.defaultLimit());
    }

    pub fn deinit(*_)  {
        _.alloc.free(_.segments);
    }

    pub fn segmentCount(): usize {
        return _.segments.len;
    }

    pub fn segmentWords(seg_id: u32): usize {
        if (seg_id >= _.segments.len) {
            return 0;
        }
        return _.segments[seg_id].len / WORD_SIZE;
    }

    pub fn getRoot(*_): CapnpError!StructReader {
        if (_.segments.len == 0 or _.segments[0].len < WORD_SIZE) {
            return CapnpError.TruncatedFrame;
        }
        const res = try resolvePointer(_.segments, 0, 0, &_.limit);
        if (res.is_null) {
            return CapnpError.NullPointer;
        }
        if (res.kind != .Struct) {
            return CapnpError.InvalidPointer;
        }
        return StructReader{
            .segments = _.segments,
            .seg_id = res.seg_id,
            .data_word_offset = res.word_offset,
            .data_words = res.data_words,
            .ptr_words = res.ptr_words,
            .limit = &_.limit,
        };
    }
}
