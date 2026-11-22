// Origin: Pure-CP Stdlib (Zero-Alloc Google FlatBuffers Binary Parser) | Created: 2026-11-22

/// FlatBuffers 带尺寸前缀消息的前缀字节数（4B LE）
pub const FB_SIZE_PREFIX_BYTES: usize = 4;

/// FlatBuffers 4 字节文件标识偏移与长度
pub const FB_FILE_ID_OFFSET: usize = 4;
pub const FB_FILE_ID_LEN: usize = 4;

/// 零拷贝 FlatBuffers 向量视图（支持标量、字节切片、字符串与嵌套表向量）
pub struct FbVector {
    buf: []const u8,
    data_pos: usize,
    len: usize,

    /// 以只读字节切片形式返回 `[u8]` 向量（严格校验边界）
    fn byteSlice(): ([]const u8)? {
        if (_.data_pos > _.buf.len || _.len > _.buf.len - _.data_pos) return null;
        return _.buf[_.data_pos .. _.data_pos + _.len];
    }

    fn getU8(idx: usize): u8? {
        if (idx >= _.len) return null;
        if (_.data_pos > _.buf.len || idx >= _.buf.len - _.data_pos) return null;
        return _.buf[_.data_pos + idx];
    }

    fn getU16(idx: usize): u16? {
        const off = _.elemOffset(idx, 2) ?? return null;
        return readU16LE(_.buf, off);
    }

    fn getU32(idx: usize): u32? {
        const off = _.elemOffset(idx, 4) ?? return null;
        return readU32LE(_.buf, off);
    }

    fn getI32(idx: usize): i32? {
        const u = _.getU32(idx) ?? return null;
        return u as i32;
    }

    fn getU64(idx: usize): u64? {
        const off = _.elemOffset(idx, 8) ?? return null;
        return readU64LE(_.buf, off);
    }

    fn getF32(idx: usize): f32? {
        const u = _.getU32(idx) ?? return null;
        return @as(f32, @bitCast(u));
    }

    /// 零拷贝读取字符串向量的第 `idx` 项
    fn getString(idx: usize): ([]const u8)? {
        const off = _.elemOffset(idx, 4) ?? return null;
        return readStringAtOffsetField(_.buf, off);
    }

    /// 零拷贝读取嵌套表向量的第 `idx` 项
    fn getTable(idx: usize): FbTable? {
        const off = _.elemOffset(idx, 4) ?? return null;
        const target = derefUOffset(_.buf, off) ?? return null;
        return makeValidatedTable(_.buf, target);
    }

    fn elemOffset(idx: usize, elem_size: usize): usize? {
        if (idx >= _.len) return null;
        if (_.data_pos > _.buf.len) return null;
        const rem = _.buf.len - _.data_pos;
        if (idx > rem / elem_size) return null;
        const rel: usize = idx * elem_size;
        if (elem_size > rem - rel) return null;
        return _.data_pos + rel;
    }
}

/// 零拷贝 FlatBuffers Table 视图（基于 VTable 槽位定位与严格越界/负数偏移防护）
pub struct FbTable {
    buf: []const u8,
    pos: usize,

    /// 计算并校验当前表的 VTable 起始偏移（`vtable_pos = pos - soffset_t`）
    fn vtablePos(): usize? {
        if (_.pos > _.buf.len || _.buf.len - _.pos < 4) return null;
        const soff: i32 = readU32LE(_.buf, _.pos) as i32;
        const vpos_i64: i64 = (_.pos as i64) - (soff as i64);
        if (vpos_i64 < 0) return null;
        const vpos_u64: u64 = vpos_i64 as u64;
        if (vpos_u64 > (_.buf.len as u64)) return null;
        const vpos: usize = vpos_u64 as usize;
        if (_.buf.len - vpos < 4) return null;

        const vtable_len: usize = readU16LE(_.buf, vpos) as usize;
        if (vtable_len < 4 || vtable_len > _.buf.len - vpos) return null;
        return vpos;
    }

    /// 返回当前 VTable 声明的字段槽位总数（`(vtable_len - 4) / 2`）
    fn fieldCount(): usize? {
        const vpos = _.vtablePos() ?? return null;
        const vtable_len: usize = readU16LE(_.buf, vpos) as usize;
        return (vtable_len - 4) / 2;
    }

    /// 查询 VTable 中偏移 `vtable_slot_offset`（4, 6, 8, ...）处的字段绝对字节偏移；缺失或为 0 返回 `null`
    fn fieldOffset(vtable_slot_offset: usize): usize? {
        if (vtable_slot_offset < 4) return null;
        const vpos = _.vtablePos() ?? return null;
        const vtable_len: usize = readU16LE(_.buf, vpos) as usize;
        if (vtable_slot_offset + 2 > vtable_len) return null;

        const rel_off: usize = readU16LE(_.buf, vpos + vtable_slot_offset) as usize;
        if (rel_off == 0) return null;
        if (rel_off > _.buf.len - _.pos) return null;
        return _.pos + rel_off;
    }

    fn getBool(slot: usize): bool? {
        const b = _.getU8(slot) ?? return null;
        return b != 0;
    }

    fn getU8(slot: usize): u8? {
        const off = _.fieldOffset(slot) ?? return null;
        if (_.buf.len - off < 1) return null;
        return _.buf[off];
    }

    fn getI8(slot: usize): i8? {
        const u = _.getU8(slot) ?? return null;
        return u as i8;
    }

    fn getU16(slot: usize): u16? {
        const off = _.fieldOffset(slot) ?? return null;
        if (_.buf.len - off < 2) return null;
        return readU16LE(_.buf, off);
    }

    fn getI16(slot: usize): i16? {
        const u = _.getU16(slot) ?? return null;
        return u as i16;
    }

    fn getU32(slot: usize): u32? {
        const off = _.fieldOffset(slot) ?? return null;
        if (_.buf.len - off < 4) return null;
        return readU32LE(_.buf, off);
    }

    fn getI32(slot: usize): i32? {
        const u = _.getU32(slot) ?? return null;
        return u as i32;
    }

    fn getU64(slot: usize): u64? {
        const off = _.fieldOffset(slot) ?? return null;
        if (_.buf.len - off < 8) return null;
        return readU64LE(_.buf, off);
    }

    fn getI64(slot: usize): i64? {
        const u = _.getU64(slot) ?? return null;
        return u as i64;
    }

    fn getF32(slot: usize): f32? {
        const u = _.getU32(slot) ?? return null;
        return @as(f32, @bitCast(u));
    }

    fn getF64(slot: usize): f64? {
        const u = _.getU64(slot) ?? return null;
        return @as(f64, @bitCast(u));
    }

    /// 零拷贝读取字符串字段（4B `uoffset` -> 4B `len` + UTF-8 切片）
    fn getString(slot: usize): ([]const u8)? {
        const off = _.fieldOffset(slot) ?? return null;
        return readStringAtOffsetField(_.buf, off);
    }

    /// 零拷贝读取嵌套 Table 字段
    fn getTable(slot: usize): FbTable? {
        const off = _.fieldOffset(slot) ?? return null;
        const target = derefUOffset(_.buf, off) ?? return null;
        return makeValidatedTable(_.buf, target);
    }

    /// 零拷贝读取通用 Vector 字段
    fn getVector(slot: usize): FbVector? {
        const off = _.fieldOffset(slot) ?? return null;
        const vec_pos = derefUOffset(_.buf, off) ?? return null;
        if (_.buf.len - vec_pos < 4) return null;
        const len: usize = readU32LE(_.buf, vec_pos) as usize;
        return FbVector {
            buf: _.buf,
            data_pos: vec_pos + 4,
            len: len,
        };
    }
}

fn readU16LE(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn readU64LE(buf: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(buf, off) as u64;
    const hi: u64 = readU32LE(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn derefUOffset(buf: []const u8, field_pos: usize): usize? {
    if (field_pos > buf.len || buf.len - field_pos < 4) return null;
    const rel: usize = readU32LE(buf, field_pos) as usize;
    if (rel > buf.len - field_pos) return null;
    return field_pos + rel;
}

fn readStringAtOffsetField(buf: []const u8, field_pos: usize): ([]const u8)? {
    const str_pos = derefUOffset(buf, field_pos) ?? return null;
    if (buf.len - str_pos < 4) return null;
    const len: usize = readU32LE(buf, str_pos) as usize;
    const start: usize = str_pos + 4;
    if (len > buf.len - start) return null;
    return buf[start .. start + len];
}

fn makeValidatedTable(buf: []const u8, pos: usize): FbTable? {
    const tbl = FbTable {
        buf: buf,
        pos: pos,
    };
    * = tbl.vtablePos() ?? return null;
    return tbl;
}

/// 读取 FlatBuffers 偏移 `4..8` 的 4 字节文件标识（File Identifier）
pub fn fbFileIdentifier(buf: []const u8): ([]const u8)? {
    if (buf.len < FB_FILE_ID_OFFSET + FB_FILE_ID_LEN) return null;
    return buf[FB_FILE_ID_OFFSET .. FB_FILE_ID_OFFSET + FB_FILE_ID_LEN];
}

/// 校验 FlatBuffers 缓冲区是否匹配指定 4 字节 File Identifier
pub fn fbHasIdentifier(buf: []const u8, id: []const u8): bool {
    if (id.len != FB_FILE_ID_LEN) return false;
    const found = fbFileIdentifier(buf) ?? return false;
    return found[0] == id[0]
        && found[1] == id[1]
        && found[2] == id[2]
        && found[3] == id[3];
}

/// 剥离带 4B LE 尺寸前缀的 FlatBuffers 消息（严格校验 `size <= buf.len - 4`）
pub fn fbStripSizePrefix(buf: []const u8): ([]const u8)? {
    if (buf.len < FB_SIZE_PREFIX_BYTES) return null;
    const size: usize = readU32LE(buf, 0) as usize;
    if (size > buf.len - FB_SIZE_PREFIX_BYTES) return null;
    return buf[FB_SIZE_PREFIX_BYTES .. FB_SIZE_PREFIX_BYTES + size];
}

/// 读取偏移 0 的 4B LE `uoffset` 并构造校验后的根 `FbTable`
pub fn fbRootTable(buf: []const u8): FbTable? {
    if (buf.len < 4) return null;
    const root_pos: usize = readU32LE(buf, 0) as usize;
    return makeValidatedTable(buf, root_pos);
}
