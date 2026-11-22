// Origin: Pure-CP Stdlib (Zero-Alloc Apache ORC Columnar Container Parser) | Created: 2026-11-22

/// Apache ORC 3 字节魔数常量
pub const ORC_MAGIC: []const u8 = "ORC";

/// Apache ORC 压缩算法枚举（对应 `orc_proto.CompressionKind`）
pub enum OrcCompression: u8 {
    none = 0,
    zlib = 1,
    snappy = 2,
    lzo = 3,
    lz4 = 4,
    zstd = 5,
    unknown = 255,
}

/// Apache ORC PostScript 元数据（位于文件末尾 `postscript_len` 字节）
pub struct OrcPostScript {
    footer_length: u64,
    compression: OrcCompression,
    compression_block_size: u64,
    version_major: u32,
    version_minor: u32,
    metadata_length: u64,
    writer_version: u32,
    magic: []const u8,
    postscript_len: u8,
}

/// 零拷贝 Apache ORC 文件分区视图（Stripes / Metadata / Footer / PostScript）
pub struct OrcLayout {
    postscript: OrcPostScript,
    postscript_offset: usize,
    footer_offset: usize,
    footer_slice: []const u8,
    metadata_offset: usize,
    metadata_slice: []const u8,
    stripes_slice: []const u8,
}

/// 零拷贝 ORC 压缩/原始数据块视图（3 字节小端头 + 负载切片）
pub struct OrcCompressionBlock {
    is_original: bool,
    length: u32,
    payload: []const u8,
}

/// Protobuf Varint 解码结果
pub struct VarintResult {
    value: u64,
    next_pos: usize,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 将 ORC 压缩枚举值映射为 `OrcCompression`
pub fn orcCompressionFromU64(v: u64): OrcCompression {
    if (v == 0) return OrcCompression.none;
    if (v == 1) return OrcCompression.zlib;
    if (v == 2) return OrcCompression.snappy;
    if (v == 3) return OrcCompression.lzo;
    if (v == 4) return OrcCompression.lz4;
    if (v == 5) return OrcCompression.zstd;
    return OrcCompression.unknown;
}

/// 零分配 Protobuf Varint 解码（最多 10 字节，防位移溢出与截断）
pub fn readVarint(buf: []const u8, start: usize): VarintResult? {
    let pos = start;
    let val: u64 = 0;
    let shift: u6 = 0;
    let count: usize = 0;

    while (pos < buf.len && count < 10) {
        const b = buf[pos];
        pos += 1;
        count += 1;

        if (count == 10 && b > 1) return null;
        val |= ((b & 0x7F) as u64) << shift;
        if ((b & 0x80) == 0) {
            return VarintResult {
                value: val,
                next_pos: pos,
            };
        }
        shift += 7;
    }
    return null;
}

/// 按 Protobuf wire type 跳过未知字段，返回下一字段偏移；非法 wire type 或越界返回 `null`
pub fn skipField(buf: []const u8, start: usize, wire_type: u64): usize? {
    if (wire_type == 0) {
        const r = readVarint(buf, start) ?? return null;
        return r.next_pos;
    }
    if (wire_type == 1) {
        if (buf.len - start < 8) return null;
        return start + 8;
    }
    if (wire_type == 2) {
        const r = readVarint(buf, start) ?? return null;
        if (r.value > ((buf.len - r.next_pos) as u64)) return null;
        return r.next_pos + (r.value as usize);
    }
    if (wire_type == 5) {
        if (buf.len - start < 4) return null;
        return start + 4;
    }
    return null;
}

/// 解析文件尾部 Protobuf 编码的 ORC PostScript（校验 `postscript_len >= 5` 与 `magic == "ORC"`）
pub fn parseOrcPostScript(data: []const u8): OrcPostScript? {
    if (data.len < 6) return null;
    const ps_len_u8 = data[data.len - 1];
    const ps_len: usize = ps_len_u8 as usize;
    if (ps_len < 5 || ps_len > data.len - 1) return null;

    const ps_start = data.len - 1 - ps_len;
    const ps_buf = data[ps_start .. data.len - 1];

    let footer_length: u64 = 0;
    let compression = OrcCompression.none;
    let compression_block_size: u64 = 0;
    let version_major: u32 = 0;
    let version_minor: u32 = 0;
    let version_count: usize = 0;
    let metadata_length: u64 = 0;
    let writer_version: u32 = 0;
    let magic: []const u8 = "";

    let pos: usize = 0;
    while (pos < ps_buf.len) {
        const tag_res = readVarint(ps_buf, pos) ?? return null;
        pos = tag_res.next_pos;
        const field_num = tag_res.value >> 3;
        const wire_type = tag_res.value & 7;

        if (field_num == 1 && wire_type == 0) {
            const v = readVarint(ps_buf, pos) ?? return null;
            footer_length = v.value;
            pos = v.next_pos;
        } else if (field_num == 2 && wire_type == 0) {
            const v = readVarint(ps_buf, pos) ?? return null;
            compression = orcCompressionFromU64(v.value);
            pos = v.next_pos;
        } else if (field_num == 3 && wire_type == 0) {
            const v = readVarint(ps_buf, pos) ?? return null;
            compression_block_size = v.value;
            pos = v.next_pos;
        } else if (field_num == 4 && wire_type == 0) {
            // Unpacked repeated uint32 version
            const v = readVarint(ps_buf, pos) ?? return null;
            if (version_count == 0) {
                version_major = v.value as u32;
            } else if (version_count == 1) {
                version_minor = v.value as u32;
            }
            version_count += 1;
            pos = v.next_pos;
        } else if (field_num == 4 && wire_type == 2) {
            // Packed repeated uint32 version
            const len_res = readVarint(ps_buf, pos) ?? return null;
            if (len_res.value > ((ps_buf.len - len_res.next_pos) as u64)) return null;
            const pack_end = len_res.next_pos + (len_res.value as usize);
            let p = len_res.next_pos;
            while (p < pack_end) {
                const v = readVarint(ps_buf, p) ?? return null;
                if (version_count == 0) {
                    version_major = v.value as u32;
                } else if (version_count == 1) {
                    version_minor = v.value as u32;
                }
                version_count += 1;
                p = v.next_pos;
            }
            if (p != pack_end) return null;
            pos = pack_end;
        } else if (field_num == 5 && wire_type == 0) {
            const v = readVarint(ps_buf, pos) ?? return null;
            metadata_length = v.value;
            pos = v.next_pos;
        } else if (field_num == 6 && wire_type == 0) {
            const v = readVarint(ps_buf, pos) ?? return null;
            writer_version = v.value as u32;
            pos = v.next_pos;
        } else if (field_num == 8000 && wire_type == 2) {
            const len_res = readVarint(ps_buf, pos) ?? return null;
            if (len_res.value > ((ps_buf.len - len_res.next_pos) as u64)) return null;
            const end = len_res.next_pos + (len_res.value as usize);
            magic = ps_buf[len_res.next_pos .. end];
            pos = end;
        } else {
            pos = skipField(ps_buf, pos, wire_type) ?? return null;
        }
    }

    if (!strEq(magic, ORC_MAGIC)) return null;

    return OrcPostScript {
        footer_length: footer_length,
        compression: compression,
        compression_block_size: compression_block_size,
        version_major: version_major,
        version_minor: version_minor,
        metadata_length: metadata_length,
        writer_version: writer_version,
        magic: magic,
        postscript_len: ps_len_u8,
    };
}

/// 快速探测数据是否为合法 Apache ORC 文件（校验尾部 PostScript 及 `magic == "ORC"`）
pub fn isOrc(data: []const u8): bool {
    return parseOrcPostScript(data) != null;
}

/// 计算并严格校验 ORC 文件各分区偏移与零拷贝切片（防下溢与越界）
pub fn parseOrcLayout(data: []const u8): OrcLayout? {
    const ps = parseOrcPostScript(data) ?? return null;
    const ps_offset: usize = data.len - 1 - (ps.postscript_len as usize);

    if (ps.footer_length > (ps_offset as u64)) return null;
    const footer_offset: usize = ps_offset - (ps.footer_length as usize);

    if (ps.metadata_length > (footer_offset as u64)) return null;
    const metadata_offset: usize = footer_offset - (ps.metadata_length as usize);

    let stripes_start: usize = 0;
    if (metadata_offset >= 3 && data[0] == 'O' && data[1] == 'R' && data[2] == 'C') {
        stripes_start = 3;
    }

    return OrcLayout {
        postscript: ps,
        postscript_offset: ps_offset,
        footer_offset: footer_offset,
        footer_slice: data[footer_offset..ps_offset],
        metadata_offset: metadata_offset,
        metadata_slice: data[metadata_offset..footer_offset],
        stripes_slice: data[stripes_start..metadata_offset],
    };
}

/// 零堆分配 ORC 3 字节小端压缩/原始块流式迭代器
pub struct OrcBlockIter {
    data: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    pub static fn init(stream_bytes: []const u8): OrcBlockIter {
        return OrcBlockIter {
            data: stream_bytes,
            pos: 0,
            had_error: false,
        };
    }

    /// 读取下一个 ORC 压缩/原始数据块；到达末尾返回 `null`，遇截断/越界置 `had_error = true` 并返回 `null`
    pub fn next(*_): OrcCompressionBlock? {
        if (_.had_error || _.pos >= _.data.len) return null;
        if (_.data.len - _.pos < 3) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const p = _.pos;
        const hdr: u32 = (_.data[p] as u32)
            | ((_.data[p + 1] as u32) << 8)
            | ((_.data[p + 2] as u32) << 16);
        const is_original = (hdr & 1) != 0;
        const block_len: u32 = hdr >> 1;
        const len_usize: usize = block_len as usize;

        if (len_usize > _.data.len - (p + 3)) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const payload = _.data[p + 3 .. p + 3 + len_usize];
        _.pos = p + 3 + len_usize;

        return OrcCompressionBlock {
            is_original: is_original,
            length: block_len,
            payload: payload,
        };
    }
}
