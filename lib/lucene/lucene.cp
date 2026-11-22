// Origin: Pure-CP Stdlib (Zero-Alloc Apache Lucene CodecUtil Parser) | Created: 2026-11-22

/// Apache Lucene CodecUtil 魔数与结构常量
pub const LUCENE_CODEC_MAGIC: u32 = 0x3FD76C17;
pub const LUCENE_FOOTER_MAGIC: u32 = 0xC02893E8;
pub const LUCENE_FOOTER_LENGTH: usize = 16;
pub const LUCENE_ID_LENGTH: usize = 16;

/// Lucene 32-bit VInt 解码结果
pub struct LuceneVInt {
    value: u32,
    next_pos: usize,
}

/// Lucene 63-bit 非负 VLong 解码结果
pub struct LuceneVLong {
    value: u64,
    next_pos: usize,
}

/// Lucene VInt 长度前缀字符串零拷贝视图
pub struct LuceneString {
    text: []const u8,
    next_pos: usize,
}

/// Lucene 基础 CodecHeader 解析结果
pub struct LuceneCodecHeader {
    codec_name: []const u8,
    version: i32,
    body_offset: usize,
}

/// Lucene 5.0+ 标准段文件 IndexHeader 解析结果
pub struct LuceneIndexHeader {
    codec_name: []const u8,
    version: i32,
    object_id: [16]u8,
    object_suffix: []const u8,
    body_offset: usize,
}

/// Lucene 16 字节尾部校验帧（Footer）解析结果
pub struct LuceneFooter {
    algorithm_id: u32,
    checksum: u64,
    crc32: u32,
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU64BE(data: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(data, off) as u64;
    const lo: u64 = readU32BE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

fn u32ToI32(u: u32): i32 {
    if (u >= 0x80000000) {
        const inv = 0xFFFFFFFF - u;
        return -(inv as i32) - 1;
    }
    return u as i32;
}

fn isAsciiSlice(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] >= 0x80) return false;
        i += 1;
    }
    return true;
}

/// 解码 Lucene VInt（小端 7-bit 续位 MSB 0x80，最多 5 字节覆盖完整 u32）
pub fn readLuceneVInt(data: []const u8, pos: usize): LuceneVInt? {
    if (pos >= data.len) return null;

    let acc: u32 = 0;
    let i: usize = 0;
    while (i < 5) {
        if (pos + i >= data.len) return null;
        const b = data[pos + i];
        if (i == 4) {
            // 第 5 字节仅允许低 4 位有效且不能继续置位 0x80
            if ((b & 0xF0) != 0) return null;
            acc |= (b as u32) << 28;
            return LuceneVInt { value: acc, next_pos: pos + 5 };
        }
        const shift: u5 = (i * 7) as u5;
        acc |= ((b & 0x7F) as u32) << shift;
        if ((b & 0x80) == 0) {
            return LuceneVInt { value: acc, next_pos: pos + i + 1 };
        }
        i += 1;
    }
    return null;
}

/// 解码 Lucene 非负 VLong（小端 7-bit 续位 MSB 0x80，最多 9 字节覆盖 u63，拒绝负值或超长）
pub fn readLuceneVLong(data: []const u8, pos: usize): LuceneVLong? {
    if (pos >= data.len) return null;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < 9) {
        if (pos + i >= data.len) return null;
        const b = data[pos + i];
        if (i == 8) {
            // 第 9 字节最高位必须为 0（非负 63-bit long，禁止第 10 字节）
            if ((b & 0x80) != 0) return null;
            acc |= (b as u64) << 56;
            return LuceneVLong { value: acc, next_pos: pos + 9 };
        }
        const shift: u6 = (i * 7) as u6;
        acc |= ((b & 0x7F) as u64) << shift;
        if ((b & 0x80) == 0) {
            return LuceneVLong { value: acc, next_pos: pos + i + 1 };
        }
        i += 1;
    }
    return null;
}

/// 32-bit ZigZag 解码（无溢出覆盖完整 i32）
pub fn luceneZigZagDecode32(v: u32): i32 {
    const half = v >> 1;
    if ((v & 1) == 0) return half as i32;
    return -(half as i32) - 1;
}

/// 64-bit ZigZag 解码（无溢出覆盖完整 i64）
pub fn luceneZigZagDecode64(v: u64): i64 {
    const half = v >> 1;
    if ((v & 1) == 0) return half as i64;
    return -(half as i64) - 1;
}

/// 读取 Lucene 字符串（VInt 字节长度前缀 + 零拷贝 UTF-8 切片）
pub fn readLuceneString(data: []const u8, pos: usize): LuceneString? {
    let len_v = LuceneVInt { value: 0, next_pos: 0 };
    if (v := readLuceneVInt(data, pos)) {
        len_v = v;
    } else {
        return null;
    }

    const start = len_v.next_pos;
    if ((len_v.value as u64) > ((data.len - start) as u64)) return null;
    const len_usize = len_v.value as usize;

    return LuceneString {
        text: data[start .. start + len_usize],
        next_pos: start + len_usize,
    };
}

/// 解析 Lucene 基础 CodecHeader（4B 魔数 + codec_name + 4B version）
pub fn parseLuceneHeader(data: []const u8): LuceneCodecHeader? {
    if (data.len < 4) return null;
    if (readU32BE(data, 0) != LUCENE_CODEC_MAGIC) return null;

    let name = LuceneString { text: "", next_pos: 0 };
    if (s := readLuceneString(data, 4)) {
        if (s.text.len >= 128 || !isAsciiSlice(s.text)) return null;
        name = s;
    } else {
        return null;
    }

    if (data.len - name.next_pos < 4) return null;
    const version = u32ToI32(readU32BE(data, name.next_pos));

    return LuceneCodecHeader {
        codec_name: name.text,
        version: version,
        body_offset: name.next_pos + 4,
    };
}

/// 解析 Lucene 5.0+ 标准段文件 IndexHeader（CodecHeader + 16B object_id + 1B suffix_len + ASCII suffix）
pub fn parseLuceneIndexHeader(data: []const u8): LuceneIndexHeader? {
    let base = LuceneCodecHeader { codec_name: "", version: 0, body_offset: 0 };
    if (h := parseLuceneHeader(data)) {
        base = h;
    } else {
        return null;
    }

    const p = base.body_offset;
    if (data.len - p < LUCENE_ID_LENGTH + 1) return null;

    let obj_id: [16]u8 = undefined;
    let i: usize = 0;
    while (i < LUCENE_ID_LENGTH) {
        obj_id[i] = data[p + i];
        i += 1;
    }

    const suffix_len = data[p + LUCENE_ID_LENGTH] as usize;
    const suffix_start = p + LUCENE_ID_LENGTH + 1;
    if (data.len - suffix_start < suffix_len) return null;

    const suffix = data[suffix_start .. suffix_start + suffix_len];
    if (!isAsciiSlice(suffix)) return null;

    return LuceneIndexHeader {
        codec_name: base.codec_name,
        version: base.version,
        object_id: obj_id,
        object_suffix: suffix,
        body_offset: suffix_start + suffix_len,
    };
}

/// 解析 Lucene 末尾 16 字节 Footer（4B magic `0xC02893E8` + 4B algorithm_id==0 + 8B checksum 高32位为0）
pub fn parseLuceneFooter(data: []const u8): LuceneFooter? {
    if (data.len < LUCENE_FOOTER_LENGTH) return null;
    const base = data.len - LUCENE_FOOTER_LENGTH;

    if (readU32BE(data, base) != LUCENE_FOOTER_MAGIC) return null;
    const alg_id = readU32BE(data, base + 4);
    if (alg_id != 0) return null;

    const checksum = readU64BE(data, base + 8);
    if ((checksum >> 32) != 0) return null;

    return LuceneFooter {
        algorithm_id: alg_id,
        checksum: checksum,
        crc32: (checksum & 0xFFFFFFFF) as u32,
    };
}

/// 同时校验头部 `0x3FD76C17`、尾部 `0xC02893E8` 与首尾不重叠合法性
pub fn isLuceneFile(data: []const u8): bool {
    if (hdr := parseLuceneHeader(data)) {
        if (parseLuceneFooter(data) != null) {
            return hdr.body_offset <= data.len - LUCENE_FOOTER_LENGTH;
        }
    }
    return false;
}
