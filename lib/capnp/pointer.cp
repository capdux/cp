// Source: cp/lib/capnp/pointer.cp (Cap'n Proto 64-bit pointer encoding, decoding, and Far Pointer resolution)
import {
    CapnpError,
    PointerKind,
    ElementSize,
    TraversalLimit,
    WORD_SIZE,
    readU64LE,
    signExtend30,
    signExtend29,
} from "./types.cp";

pub type ByteSlice = []const u8;

pub struct DecodedPointer {
    is_null: bool,
    kind: PointerKind,
    offset: i32,
    data_words: u16,
    ptr_words: u16,
    elem_size: ElementSize,
    elem_count: u32,
    is_double_far: bool,
    target_word: u32,
    target_seg: u32,
}

pub struct ResolvedPointer {
    is_null: bool,
    kind: PointerKind,
    seg_id: u32,
    word_offset: u32,
    data_words: u16,
    ptr_words: u16,
    elem_size: ElementSize,
    elem_count: u32,
    content_words: u64,
}

pub fn encodeStructPointer(offset: i32, data_words: u16, ptr_words: u16): u64 {
    const off_u: u32 = @bitCast(offset);
    const b_part: u64 = @as(u64, off_u & 0x3FFF_FFFF) << 2;
    const c_part: u64 = @as(u64, data_words) << 32;
    const d_part: u64 = @as(u64, ptr_words) << 48;
    return b_part | c_part | d_part;
}

pub fn encodeListPointer(offset: i32, elem_size: ElementSize, elem_count: u32): u64 {
    const off_u: u32 = @bitCast(offset);
    const kind_part: u64 = 1;
    const b_part: u64 = @as(u64, off_u & 0x3FFF_FFFF) << 2;
    const c_part: u64 = @as(u64, @intFromEnum(elem_size)) << 32;
    const d_part: u64 = @as(u64, elem_count & 0x1FFF_FFFF) << 35;
    return kind_part | b_part | c_part | d_part;
}

pub fn encodeFarPointer(is_double_far: bool, target_word: u32, target_seg: u32): u64 {
    const kind_part: u64 = 2;
    const b_part: u64 = if (is_double_far) (@as(u64, 1) << 2) else 0;
    const c_part: u64 = @as(u64, target_word & 0x1FFF_FFFF) << 3;
    const d_part: u64 = @as(u64, target_seg) << 32;
    return kind_part | b_part | c_part | d_part;
}

pub fn decodePointer(raw: u64): CapnpError!DecodedPointer {
    if (raw == 0) {
        return DecodedPointer{
            .is_null = true,
            .kind = .Struct,
            .offset = 0,
            .data_words = 0,
            .ptr_words = 0,
            .elem_size = .Void,
            .elem_count = 0,
            .is_double_far = false,
            .target_word = 0,
            .target_seg = 0,
        };
    }

    const kind_bits: u2 = @truncate(raw & 0x3);
    const kind: PointerKind = @enumFromInt(kind_bits);

    if (kind == .Struct) {
        const off_raw: u32 = @truncate((raw >> 2) & 0x3FFF_FFFF);
        const data_w: u16 = @truncate((raw >> 32) & 0xFFFF);
        const ptr_w: u16 = @truncate((raw >> 48) & 0xFFFF);
        return DecodedPointer{
            .is_null = false,
            .kind = .Struct,
            .offset = signExtend30(off_raw),
            .data_words = data_w,
            .ptr_words = ptr_w,
            .elem_size = .Void,
            .elem_count = 0,
            .is_double_far = false,
            .target_word = 0,
            .target_seg = 0,
        };
    } else if (kind == .List) {
        const off_raw: u32 = @truncate((raw >> 2) & 0x3FFF_FFFF);
        const es_bits: u3 = @truncate((raw >> 32) & 0x7);
        const cnt: u32 = @truncate((raw >> 35) & 0x1FFF_FFFF);
        return DecodedPointer{
            .is_null = false,
            .kind = .List,
            .offset = signExtend30(off_raw),
            .data_words = 0,
            .ptr_words = 0,
            .elem_size = @enumFromInt(es_bits),
            .elem_count = cnt,
            .is_double_far = false,
            .target_word = 0,
            .target_seg = 0,
        };
    } else if (kind == .Far) {
        const dbl: bool = ((raw >> 2) & 0x1) != 0;
        const tw: u32 = @truncate((raw >> 3) & 0x1FFF_FFFF);
        const ts: u32 = @truncate((raw >> 32) & 0xFFFF_FFFF);
        return DecodedPointer{
            .is_null = false,
            .kind = .Far,
            .offset = 0,
            .data_words = 0,
            .ptr_words = 0,
            .elem_size = .Void,
            .elem_count = 0,
            .is_double_far = dbl,
            .target_word = tw,
            .target_seg = ts,
        };
    }

    return CapnpError.InvalidPointer;
}

pub fn computeListContentWords(elem_size: ElementSize, elem_count: u32): u64 {
    const cnt: u64 = @as(u64, elem_count);
    return switch (elem_size) {
        .Void => 0,
        .Bit => (cnt + 63) / 64,
        .Byte => (cnt + 7) / 8,
        .TwoBytes => (cnt + 3) / 4,
        .FourBytes => (cnt + 1) / 2,
        .EightBytes => cnt,
        .Pointer => cnt,
        .Composite => cnt + 1, // +1 for the tag word
    };
}

pub fn resolvePointer(
    segments: []const ByteSlice,
    seg_id: u32,
    ptr_word_idx: u32,
    limit: *TraversalLimit,
): CapnpError!ResolvedPointer {
    if (seg_id >= segments.len) {
        return CapnpError.OutOfBounds;
    }
    const seg = segments[seg_id];
    const ptr_byte_off: usize = @as(usize, ptr_word_idx) * WORD_SIZE;
    if (ptr_byte_off + WORD_SIZE > seg.len) {
        return CapnpError.OutOfBounds;
    }

    const raw = readU64LE(seg, ptr_byte_off);
    const decoded = try decodePointer(raw);
    if (decoded.is_null) {
        return ResolvedPointer{
            .is_null = true,
            .kind = .Struct,
            .seg_id = seg_id,
            .word_offset = 0,
            .data_words = 0,
            .ptr_words = 0,
            .elem_size = .Void,
            .elem_count = 0,
            .content_words = 0,
        };
    }

    let active_seg: u32 = seg_id;
    let active_ptr_word: u32 = ptr_word_idx;
    let active_dec = decoded;
    let is_double_far_resolved = false;
    let dbl_content_seg: u32 = 0;
    let dbl_content_word: u32 = 0;

    if (active_dec.kind == .Far) {
        if (active_dec.target_seg >= segments.len) {
            return CapnpError.InvalidFarPointer;
        }
        const t_seg = segments[active_dec.target_seg];
        const t_word = active_dec.target_word;
        const t_byte: usize = @as(usize, t_word) * WORD_SIZE;

        if (!active_dec.is_double_far) {
            if (t_byte + WORD_SIZE > t_seg.len) {
                return CapnpError.OutOfBounds;
            }
            try limit.consume(1);
            const pad_raw = readU64LE(t_seg, t_byte);
            const pad_dec = try decodePointer(pad_raw);
            if (pad_dec.is_null or pad_dec.kind == .Far) {
                return CapnpError.InvalidFarPointer;
            }
            active_seg = active_dec.target_seg;
            active_ptr_word = t_word;
            active_dec = pad_dec;
        } else {
            // Double-far landing pad: 2 words
            if (t_byte + 2 * WORD_SIZE > t_seg.len) {
                return CapnpError.OutOfBounds;
            }
            try limit.consume(2);
            const w0_raw = readU64LE(t_seg, t_byte);
            const w1_raw = readU64LE(t_seg, t_byte + WORD_SIZE);
            const w0_dec = try decodePointer(w0_raw);
            const w1_dec = try decodePointer(w1_raw);
            if (w0_dec.is_null or w0_dec.kind != .Far or w0_dec.is_double_far) {
                return CapnpError.InvalidFarPointer;
            }
            if (w1_dec.is_null or w1_dec.kind == .Far or w1_dec.offset != 0) {
                return CapnpError.InvalidFarPointer;
            }
            if (w0_dec.target_seg >= segments.len) {
                return CapnpError.InvalidFarPointer;
            }
            is_double_far_resolved = true;
            dbl_content_seg = w0_dec.target_seg;
            dbl_content_word = w0_dec.target_word;
            active_dec = w1_dec;
        }
    }

    let target_seg_id: u32 = active_seg;
    let target_word_off: u32 = 0;

    if (is_double_far_resolved) {
        target_seg_id = dbl_content_seg;
        target_word_off = dbl_content_word;
    } else {
        const base_i64: i64 = @as(i64, active_ptr_word) + 1 + @as(i64, active_dec.offset);
        if (base_i64 < 0) {
            return CapnpError.OutOfBounds;
        }
        target_word_off = @intCast(base_i64);
    }

    const target_seg = segments[target_seg_id];
    const total_seg_words: u64 = @as(u64, target_seg.len) / WORD_SIZE;

    if (active_dec.kind == .Struct) {
        const words: u64 = @as(u64, active_dec.data_words) + @as(u64, active_dec.ptr_words);
        if (@as(u64, target_word_off) + words > total_seg_words) {
            return CapnpError.OutOfBounds;
        }
        const consume_words: u64 = if (words == 0) 1 else words;
        try limit.consume(consume_words);
        return ResolvedPointer{
            .is_null = false,
            .kind = .Struct,
            .seg_id = target_seg_id,
            .word_offset = target_word_off,
            .data_words = active_dec.data_words,
            .ptr_words = active_dec.ptr_words,
            .elem_size = .Void,
            .elem_count = 0,
            .content_words = words,
        };
    } else if (active_dec.kind == .List) {
        const words = computeListContentWords(active_dec.elem_size, active_dec.elem_count);
        if (@as(u64, target_word_off) + words > total_seg_words) {
            return CapnpError.OutOfBounds;
        }
        const consume_words: u64 = if (words == 0) 1 else words;
        try limit.consume(consume_words);

        let comp_data_w: u16 = 0;
        let comp_ptr_w: u16 = 0;
        let comp_count: u32 = active_dec.elem_count;
        let actual_word_off: u32 = target_word_off;

        if (active_dec.elem_size == .Composite) {
            if (words < 1) {
                return CapnpError.InvalidPointer;
            }
            const tag_raw = readU64LE(target_seg, @as(usize, target_word_off) * WORD_SIZE);
            const tag_dec = try decodePointer(tag_raw);
            if (tag_dec.is_null or tag_dec.kind != .Struct) {
                return CapnpError.InvalidPointer;
            }
            comp_data_w = tag_dec.data_words;
            comp_ptr_w = tag_dec.ptr_words;
            const elem_words: u64 = @as(u64, comp_data_w) + @as(u64, comp_ptr_w);
            if (tag_dec.offset < 0) {
                return CapnpError.InvalidPointer;
            }
            comp_count = @intCast(tag_dec.offset);
            const expected_words: u64 = @as(u64, comp_count) * elem_words;
            if (expected_words != @as(u64, active_dec.elem_count)) {
                return CapnpError.OutOfBounds;
            }
            actual_word_off = target_word_off + 1;
        }

        return ResolvedPointer{
            .is_null = false,
            .kind = .List,
            .seg_id = target_seg_id,
            .word_offset = actual_word_off,
            .data_words = comp_data_w,
            .ptr_words = comp_ptr_w,
            .elem_size = active_dec.elem_size,
            .elem_count = comp_count,
            .content_words = words,
        };
    }

    return CapnpError.InvalidPointer;
}
