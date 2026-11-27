// Source: cp/lib/capnp/builder.cp (Cap'n Proto zero-copy MessageBuilder, StructBuilder, and ListBuilder)
const std = @import("std");
import {
    CapnpError,
    ElementSize,
    WORD_SIZE,
    MAX_SEGMENTS,
    writeU16LE,
    writeU32LE,
    writeU64LE,
} from "./types.cp";
import {
    encodeStructPointer,
    encodeListPointer,
    encodeFarPointer,
    computeListContentWords,
} from "./pointer.cp";

pub struct SegBuf {
    data: std.ArrayList(u8),
}

pub struct StructBuilder {
    msg: *MessageBuilder,
    seg_id: u32,
    data_word_offset: u32,
    data_words: u16,
    ptr_words: u16,

    fn segSlice(*_): []u8 {
        return _.msg.segments.items[_.seg_id].data.items;
    }

    pub fn writeBool(*_, bit_offset: u32, val: bool)  {
        const byte_idx: usize = @as(usize, bit_offset) / 8;
        const limit_bytes: usize = @as(usize, _.data_words) * WORD_SIZE;
        if (byte_idx >= limit_bytes) {
            return;
        }
        const abs_off = @as(usize, _.data_word_offset) * WORD_SIZE + byte_idx;
        let slice = _.segSlice();
        const bit_shift: u3 = @intCast(bit_offset % 8);
        const mask: u8 = @as(u8, 1) << bit_shift;
        if (val) {
            slice[abs_off] |= mask;
        } else {
            slice[abs_off] &= ~mask;
        }
    }

    pub fn writeBoolOrDefault(*_, bit_offset: u32, val: bool, default_val: bool)  {
        _.writeBool(bit_offset, val != default_val);
    }

    pub fn writeU8(*_, byte_offset: u32, val: u8)  {
        const limit_bytes: usize = @as(usize, _.data_words) * WORD_SIZE;
        if (@as(usize, byte_offset) >= limit_bytes) {
            return;
        }
        const abs_off = @as(usize, _.data_word_offset) * WORD_SIZE + @as(usize, byte_offset);
        _.segSlice()[abs_off] = val;
    }

    pub fn writeU8OrDefault(*_, byte_offset: u32, val: u8, default_val: u8)  {
        _.writeU8(byte_offset, val ^ default_val);
    }

    pub fn writeU16(*_, elem_idx: u32, val: u16)  {
        const byte_off: usize = @as(usize, elem_idx) * 2;
        if (byte_off + 2 > @as(usize, _.data_words) * WORD_SIZE) {
            return;
        }
        const abs_off = @as(usize, _.data_word_offset) * WORD_SIZE + byte_off;
        writeU16LE(_.segSlice(), abs_off, val);
    }

    pub fn writeU32(*_, elem_idx: u32, val: u32)  {
        const byte_off: usize = @as(usize, elem_idx) * 4;
        if (byte_off + 4 > @as(usize, _.data_words) * WORD_SIZE) {
            return;
        }
        const abs_off = @as(usize, _.data_word_offset) * WORD_SIZE + byte_off;
        writeU32LE(_.segSlice(), abs_off, val);
    }

    pub fn writeU32OrDefault(*_, elem_idx: u32, val: u32, default_val: u32)  {
        _.writeU32(elem_idx, val ^ default_val);
    }

    pub fn writeI32(*_, elem_idx: u32, val: i32)  {
        _.writeU32(elem_idx, @bitCast(val));
    }

    pub fn writeU64(*_, elem_idx: u32, val: u64)  {
        const byte_off: usize = @as(usize, elem_idx) * 8;
        if (byte_off + 8 > @as(usize, _.data_words) * WORD_SIZE) {
            return;
        }
        const abs_off = @as(usize, _.data_word_offset) * WORD_SIZE + byte_off;
        writeU64LE(_.segSlice(), abs_off, val);
    }

    pub fn writeU64OrDefault(*_, elem_idx: u32, val: u64, default_val: u64)  {
        _.writeU64(elem_idx, val ^ default_val);
    }

    pub fn writeI64(*_, elem_idx: u32, val: i64)  {
        _.writeU64(elem_idx, @bitCast(val));
    }

    pub fn writeF32(*_, elem_idx: u32, val: f32)  {
        _.writeU32(elem_idx, @bitCast(val));
    }

    pub fn writeF64(*_, elem_idx: u32, val: f64)  {
        _.writeU64(elem_idx, @bitCast(val));
    }

    fn ptrWordIndex(ptr_idx: u16): CapnpError!u32 {
        if (ptr_idx >= _.ptr_words) {
            return CapnpError.OutOfBounds;
        }
        return _.data_word_offset + @as(u32, _.data_words) + @as(u32, ptr_idx);
    }

    pub fn initStruct(*_, ptr_idx: u16, data_words: u16, ptr_words: u16): CapnpError!StructBuilder {
        return _.initStructInSegment(ptr_idx, _.seg_id, data_words, ptr_words);
    }

    pub fn initStructInSegment(
        *_, ptr_idx: u16,
        target_seg: u32,
        data_words: u16,
        ptr_words: u16,
    ): CapnpError!StructBuilder {
        const ptr_word = try _.ptrWordIndex(ptr_idx);
        const total_words: usize = @as(usize, data_words) + @as(usize, ptr_words);
        const alloc_words: usize = if (total_words == 0) 1 else total_words;

        if (target_seg == _.seg_id) {
            const target_word = try _.msg.allocWords(_.seg_id, alloc_words);
            const rel_off: i32 = @as(i32, @intCast(target_word)) - (@as(i32, @intCast(ptr_word)) + 1);
            const ptr_val = encodeStructPointer(rel_off, data_words, ptr_words);
            writeU64LE(_.segSlice(), @as(usize, ptr_word) * WORD_SIZE, ptr_val);
            return StructBuilder{
                .msg = _.msg,
                .seg_id = _.seg_id,
                .data_word_offset = target_word,
                .data_words = data_words,
                .ptr_words = ptr_words,
            };
        } else {
            // Allocate landing pad (1 word) + body in target_seg, and Far pointer in _.seg_id
            const pad_word = try _.msg.allocWords(target_seg, 1 + alloc_words);
            const body_word = pad_word + 1;
            const pad_ptr = encodeStructPointer(0, data_words, ptr_words);
            writeU64LE(_.msg.segments.items[target_seg].data.items, @as(usize, pad_word) * WORD_SIZE, pad_ptr);

            const far_ptr = encodeFarPointer(false, pad_word, target_seg);
            writeU64LE(_.segSlice(), @as(usize, ptr_word) * WORD_SIZE, far_ptr);
            return StructBuilder{
                .msg = _.msg,
                .seg_id = target_seg,
                .data_word_offset = body_word,
                .data_words = data_words,
                .ptr_words = ptr_words,
            };
        }
    }

    pub fn initList(*_, ptr_idx: u16, elem_size: ElementSize, elem_count: u32): CapnpError!ListBuilder {
        if (elem_size == .Composite) {
            return CapnpError.UnsupportedElementSize;
        }
        const ptr_word = try _.ptrWordIndex(ptr_idx);
        const words: usize = @intCast(computeListContentWords(elem_size, elem_count));
        const alloc_words: usize = if (words == 0) 1 else words;
        const target_word = try _.msg.allocWords(_.seg_id, alloc_words);
        const rel_off: i32 = @as(i32, @intCast(target_word)) - (@as(i32, @intCast(ptr_word)) + 1);
        const ptr_val = encodeListPointer(rel_off, elem_size, elem_count);
        writeU64LE(_.segSlice(), @as(usize, ptr_word) * WORD_SIZE, ptr_val);
        return ListBuilder{
            .msg = _.msg,
            .seg_id = _.seg_id,
            .word_offset = target_word,
            .elem_size = elem_size,
            .elem_count = elem_count,
            .struct_data_words = 0,
            .struct_ptr_words = 0,
        };
    }

    pub fn initCompositeList(
        *_, ptr_idx: u16,
        elem_count: u32,
        data_words: u16,
        ptr_words: u16,
    ): CapnpError!ListBuilder {
        const ptr_word = try _.ptrWordIndex(ptr_idx);
        const elem_words: u32 = @as(u32, data_words) + @as(u32, ptr_words);
        const total_elem_words: u32 = elem_count * elem_words;
        const target_word = try _.msg.allocWords(_.seg_id, 1 + @as(usize, total_elem_words));
        const rel_off: i32 = @as(i32, @intCast(target_word)) - (@as(i32, @intCast(ptr_word)) + 1);

        const ptr_val = encodeListPointer(rel_off, .Composite, total_elem_words);
        writeU64LE(_.segSlice(), @as(usize, ptr_word) * WORD_SIZE, ptr_val);

        const tag_val = encodeStructPointer(@intCast(elem_count), data_words, ptr_words);
        writeU64LE(_.segSlice(), @as(usize, target_word) * WORD_SIZE, tag_val);

        return ListBuilder{
            .msg = _.msg,
            .seg_id = _.seg_id,
            .word_offset = target_word + 1,
            .elem_size = .Composite,
            .elem_count = elem_count,
            .struct_data_words = data_words,
            .struct_ptr_words = ptr_words,
        };
    }

    pub fn writeData(*_, ptr_idx: u16, data: []const u8): CapnpError!void {
        const lb = try _.initList(ptr_idx, .Byte, @intCast(data.len));
        let seg = _.msg.segments.items[lb.seg_id].data.items;
        const dst_off: usize = @as(usize, lb.word_offset) * WORD_SIZE;
        @memcpy(seg[dst_off .. dst_off + data.len], data);
    }

    pub fn writeText(*_, ptr_idx: u16, text: []const u8): CapnpError!void {
        const lb = try _.initList(ptr_idx, .Byte, @intCast(text.len + 1));
        let seg = _.msg.segments.items[lb.seg_id].data.items;
        const dst_off: usize = @as(usize, lb.word_offset) * WORD_SIZE;
        if (text.len > 0) {
            @memcpy(seg[dst_off .. dst_off + text.len], text);
        }
        seg[dst_off + text.len] = 0;
    }
}

pub struct ListBuilder {
    msg: *MessageBuilder,
    seg_id: u32,
    word_offset: u32,
    elem_size: ElementSize,
    elem_count: u32,
    struct_data_words: u16,
    struct_ptr_words: u16,

    pub fn len(): u32 {
        return _.elem_count;
    }

    fn segSlice(*_): []u8 {
        return _.msg.segments.items[_.seg_id].data.items;
    }

    fn baseByteOffset(): usize {
        return @as(usize, _.word_offset) * WORD_SIZE;
    }

    pub fn setBool(*_, idx: u32, val: bool): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Bit) {
            return CapnpError.UnsupportedElementSize;
        }
        const byte_off = _.baseByteOffset() + (@as(usize, idx) / 8);
        const bit_shift: u3 = @intCast(idx % 8);
        const mask: u8 = @as(u8, 1) << bit_shift;
        let slice = _.segSlice();
        if (val) {
            slice[byte_off] |= mask;
        } else {
            slice[byte_off] &= ~mask;
        }
    }

    pub fn setU8(*_, idx: u32, val: u8): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Byte) {
            return CapnpError.UnsupportedElementSize;
        }
        _.segSlice()[_.baseByteOffset() + @as(usize, idx)] = val;
    }

    pub fn setU16(*_, idx: u32, val: u16): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .TwoBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        writeU16LE(_.segSlice(), _.baseByteOffset() + @as(usize, idx) * 2, val);
    }

    pub fn setU32(*_, idx: u32, val: u32): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .FourBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        writeU32LE(_.segSlice(), _.baseByteOffset() + @as(usize, idx) * 4, val);
    }

    pub fn setI32(*_, idx: u32, val: i32): CapnpError!void {
        try _.setU32(idx, @bitCast(val));
    }

    pub fn setU64(*_, idx: u32, val: u64): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .EightBytes) {
            return CapnpError.UnsupportedElementSize;
        }
        writeU64LE(_.segSlice(), _.baseByteOffset() + @as(usize, idx) * 8, val);
    }

    pub fn setF64(*_, idx: u32, val: f64): CapnpError!void {
        try _.setU64(idx, @bitCast(val));
    }

    pub fn getStruct(*_, idx: u32): CapnpError!StructBuilder {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Composite) {
            return CapnpError.UnsupportedElementSize;
        }
        const elem_words: u32 = @as(u32, _.struct_data_words) + @as(u32, _.struct_ptr_words);
        return StructBuilder{
            .msg = _.msg,
            .seg_id = _.seg_id,
            .data_word_offset = _.word_offset + idx * elem_words,
            .data_words = _.struct_data_words,
            .ptr_words = _.struct_ptr_words,
        };
    }

    pub fn initStruct(*_, idx: u32, data_words: u16, ptr_words: u16): CapnpError!StructBuilder {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Pointer) {
            return CapnpError.UnsupportedElementSize;
        }
        const ptr_word = _.word_offset + idx;
        const total_words: usize = @as(usize, data_words) + @as(usize, ptr_words);
        const alloc_words: usize = if (total_words == 0) 1 else total_words;
        const target_word = try _.msg.allocWords(_.seg_id, alloc_words);
        const rel_off: i32 = @as(i32, @intCast(target_word)) - (@as(i32, @intCast(ptr_word)) + 1);
        const ptr_val = encodeStructPointer(rel_off, data_words, ptr_words);
        writeU64LE(_.segSlice(), @as(usize, ptr_word) * WORD_SIZE, ptr_val);
        return StructBuilder{
            .msg = _.msg,
            .seg_id = _.seg_id,
            .data_word_offset = target_word,
            .data_words = data_words,
            .ptr_words = ptr_words,
        };
    }

    pub fn setText(*_, idx: u32, text: []const u8): CapnpError!void {
        if (idx >= _.elem_count) {
            return CapnpError.OutOfBounds;
        }
        if (_.elem_size != .Pointer) {
            return CapnpError.UnsupportedElementSize;
        }
        const ptr_word = _.word_offset + idx;
        const byte_count: u32 = @intCast(text.len + 1);
        const words: usize = @intCast(computeListContentWords(.Byte, byte_count));
        const target_word = try _.msg.allocWords(_.seg_id, words);
        const rel_off: i32 = @as(i32, @intCast(target_word)) - (@as(i32, @intCast(ptr_word)) + 1);
        const ptr_val = encodeListPointer(rel_off, .Byte, byte_count);
        let seg = _.segSlice();
        writeU64LE(seg, @as(usize, ptr_word) * WORD_SIZE, ptr_val);
        const dst_off: usize = @as(usize, target_word) * WORD_SIZE;
        if (text.len > 0) {
            @memcpy(seg[dst_off .. dst_off + text.len], text);
        }
        seg[dst_off + text.len] = 0;
    }
}

pub struct MessageBuilder {
    alloc: std.mem.Allocator,
    segments: std.ArrayList(SegBuf),

    pub static fn init(alloc: std.mem.Allocator): MessageBuilder {
        return MessageBuilder{
            .alloc = alloc,
            .segments = std.ArrayList(SegBuf).empty,
        };
    }

    pub fn deinit(*_)  {
        let i: usize = 0;
        while (i < _.segments.items.len) {
            _.segments.items[i].data.deinit(_.alloc);
            i += 1;
        }
        _.segments.deinit(_.alloc);
    }

    pub fn addSegment(*_, initial_words: usize): CapnpError!u32 {
        if (_.segments.items.len >= @as(usize, MAX_SEGMENTS)) {
            return CapnpError.TooManySegments;
        }
        let buf = SegBuf{ .data = std.ArrayList(u8).empty };
        if (initial_words > 0) {
            const bytes_needed = initial_words * WORD_SIZE;
            buf.data.resize(_.alloc, bytes_needed) catch {
                buf.data.deinit(_.alloc);
                return CapnpError.AllocationFailed;
            };
            @memset(buf.data.items, 0);
        }
        const seg_id: u32 = @intCast(_.segments.items.len);
        _.segments.append(_.alloc, buf) catch {
            buf.data.deinit(_.alloc);
            return CapnpError.AllocationFailed;
        };
        return seg_id;
    }

    pub fn allocWords(*_, seg_id: u32, word_count: usize): CapnpError!u32 {
        if (seg_id >= _.segments.items.len) {
            return CapnpError.OutOfBounds;
        }
        const seg = &_.segments.items[seg_id];
        const old_len = seg.data.items.len;
        const word_off: u32 = @intCast(old_len / WORD_SIZE);
        const add_bytes = word_count * WORD_SIZE;
        seg.data.resize(_.alloc, old_len + add_bytes) catch {
            return CapnpError.AllocationFailed;
        };
        @memset(seg.data.items[old_len .. old_len + add_bytes], 0);
        return word_off;
    }

    pub fn initRoot(*_, data_words: u16, ptr_words: u16): CapnpError!StructBuilder {
        if (_.segments.items.len == 0) {
            * = try _.addSegment(1);
        } else if (_.segments.items[0].data.items.len < WORD_SIZE) {
            * = try _.allocWords(0, 1);
        }
        const total_words: usize = @as(usize, data_words) + @as(usize, ptr_words);
        const alloc_words: usize = if (total_words == 0) 1 else total_words;
        const body_word = try _.allocWords(0, alloc_words);
        const rel_off: i32 = @as(i32, @intCast(body_word)) - 1;
        const ptr_val = encodeStructPointer(rel_off, data_words, ptr_words);
        writeU64LE(_.segments.items[0].data.items, 0, ptr_val);
        return StructBuilder{
            .msg = self,
            .seg_id = 0,
            .data_word_offset = body_word,
            .data_words = data_words,
            .ptr_words = ptr_words,
        };
    }

    pub fn toBytes(alloc: std.mem.Allocator): CapnpError![]u8 {
        const seg_count = _.segments.items.len;
        if (seg_count == 0 or seg_count > @as(usize, MAX_SEGMENTS)) {
            return CapnpError.InvalidFrame;
        }
        const raw_header_bytes: usize = @as(usize, 4) + seg_count * @as(usize, 4);
        const padding_bytes: usize = if ((seg_count % 2) == 0) @as(usize, 4) else @as(usize, 0);
        const header_bytes: usize = raw_header_bytes + padding_bytes;

        let total_payload_bytes: usize = 0;
        let i: usize = 0;
        while (i < seg_count) {
            total_payload_bytes += _.segments.items[i].data.items.len;
            i += 1;
        }

        const out = alloc.alloc(u8, header_bytes + total_payload_bytes) catch {
            return CapnpError.AllocationFailed;
        };
        @memset(out[0..header_bytes], 0);
        writeU32LE(out, 0, @intCast(seg_count - 1));

        let cursor: usize = header_bytes;
        i = 0;
        while (i < seg_count) {
            const seg_bytes = _.segments.items[i].data.items;
            const seg_words: u32 = @intCast(seg_bytes.len / WORD_SIZE);
            writeU32LE(out, 4 + i * 4, seg_words);
            if (seg_bytes.len > 0) {
                @memcpy(out[cursor .. cursor + seg_bytes.len], seg_bytes);
            }
            cursor += seg_bytes.len;
            i += 1;
        }
        return out;
    }
}
