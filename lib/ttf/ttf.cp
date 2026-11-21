// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc TrueType / OpenType SFNT Font Parser)

/// SFNT 字体容器风格（sfntVersion）枚举
pub enum SfntFlavor: u8 {
    truetype = 0,
    cff = 1,
    apple_true = 2,
    unknown = 3,
}

/// TrueType/OpenType SFNT 12 字节大端偏移表（Offset Table）
pub struct SfntOffsetTable {
    flavor: SfntFlavor,
    sfnt_version: u32,
    num_tables: u16,
    search_range: u16,
    entry_selector: u16,
    range_shift: u16,
}

/// SFNT 16 字节大端表目录记录（Table Record）
pub struct SfntTableRecord {
    tag: [4]u8,
    checksum: u32,
    offset: u32,
    length: u32,

    /// 带溢出与边界检查返回该表在字体缓冲区中的零拷贝切片
    pub fn slice(data: []const u8): ([]const u8)? {
        const off = _.offset as usize;
        const len = _.length as usize;
        if (off > data.len || len > data.len - off) return null;
        return data[off .. off + len];
    }

    /// 比较 4 字节表标签是否等于给定切片（如 `"head"`、`"maxp"`）
    pub fn tagEquals(other: []const u8): bool {
        if (other.len != 4) return false;
        return _.tag[0] == other[0]
            && _.tag[1] == other[1]
            && _.tag[2] == other[2]
            && _.tag[3] == other[3];
    }
}

/// TrueType `head` 表核心度量元数据
pub struct TtfHeadMetrics {
    units_per_em: u16,
    x_min: i16,
    y_min: i16,
    x_max: i16,
    y_max: i16,
    index_to_loc_format: i16,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readI16BE(data: []const u8, off: usize): i16 {
    const raw = readU16BE(data, off);
    if (raw >= 0x8000) {
        return ((raw as i32) - 65536) as i16;
    }
    return raw as i16;
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

/// 解析 12 字节 SFNT Offset Table 并校验表目录总长度不越界
pub fn parseSfntHeader(data: []const u8): SfntOffsetTable? {
    if (data.len < 12) return null;

    const sfnt_version = readU32BE(data, 0);
    let flavor = SfntFlavor.unknown;
    if (sfnt_version == 0x00010000) {
        flavor = SfntFlavor.truetype;
    } else if (sfnt_version == 0x4F54544F) {
        flavor = SfntFlavor.cff;
    } else if (sfnt_version == 0x74727565) {
        flavor = SfntFlavor.apple_true;
    } else {
        return null;
    }

    const num_tables = readU16BE(data, 4);
    const dir_bytes: usize = (num_tables as usize) * 16;
    if (data.len - 12 < dir_bytes) return null;

    return SfntOffsetTable {
        flavor: flavor,
        sfnt_version: sfnt_version,
        num_tables: num_tables,
        search_range: readU16BE(data, 6),
        entry_selector: readU16BE(data, 8),
        range_shift: readU16BE(data, 10),
    };
}

/// 零堆分配 SFNT 表目录记录迭代器
pub struct SfntTableIter {
    data: []const u8,
    index: usize = 0,
    total: usize = 0,

    pub static fn init(data: []const u8): SfntTableIter {
        if (hdr := parseSfntHeader(data)) {
            return SfntTableIter {
                data: data,
                index: 0,
                total: hdr.num_tables as usize,
            };
        }
        return SfntTableIter {
            data: data,
            index: 0,
            total: 0,
        };
    }

    /// 读取下一条 16 字节表目录记录；到达末尾时返回 `null`
    pub fn next(*_): SfntTableRecord? {
        if (_.index >= _.total) return null;
        const off = 12 + _.index * 16;
        if (off + 16 > _.data.len) {
            _.index = _.total;
            return null;
        }

        const tag: [4]u8 = [
            _.data[off],
            _.data[off + 1],
            _.data[off + 2],
            _.data[off + 3],
        ];
        const rec = SfntTableRecord {
            tag: tag,
            checksum: readU32BE(_.data, off + 4),
            offset: readU32BE(_.data, off + 8),
            length: readU32BE(_.data, off + 12),
        };
        _.index += 1;
        return rec;
    }
}

/// 按 4 字节 tag（如 `"head"`, `"maxp"`, `"cmap"`）查找 SFNT 表记录
pub fn findSfntTable(data: []const u8, tag: []const u8): SfntTableRecord? {
    if (tag.len != 4) return null;
    let it = SfntTableIter.init(data);
    while (rec := it.next()) {
        if (rec.tagEquals(tag)) return rec;
    }
    return null;
}

/// 解析 `head` 表核心度量并严格校验 `magicNumber == 0x5F0F3CF5`
pub fn parseTtfHead(data: []const u8): TtfHeadMetrics? {
    const rec = findSfntTable(data, "head") ?? return null;
    const tbl = rec.slice(data) ?? return null;
    if (tbl.len < 54) return null;

    const magic = readU32BE(tbl, 12);
    if (magic != 0x5F0F3CF5) return null;

    return TtfHeadMetrics {
        units_per_em: readU16BE(tbl, 18),
        x_min: readI16BE(tbl, 36),
        y_min: readI16BE(tbl, 38),
        x_max: readI16BE(tbl, 40),
        y_max: readI16BE(tbl, 42),
        index_to_loc_format: readI16BE(tbl, 50),
    };
}

/// 从 `maxp` 表提取字形总数（`numGlyphs`）
pub fn parseTtfNumGlyphs(data: []const u8): u16? {
    const rec = findSfntTable(data, "maxp") ?? return null;
    const tbl = rec.slice(data) ?? return null;
    if (tbl.len < 6) return null;
    return readU16BE(tbl, 4);
}

/// 计算 OpenType/TrueType SFNT 表大端 4 字节对齐无符号回绕校验和
pub fn calcSfntChecksum(table_bytes: []const u8): u32 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i + 4 <= table_bytes.len) {
        sum = sum +% readU32BE(table_bytes, i);
        i += 4;
    }
    if (i < table_bytes.len) {
        const b0: u32 = table_bytes[i] as u32;
        const b1: u32 = if (i + 1 < table_bytes.len) (table_bytes[i + 1] as u32) else 0;
        const b2: u32 = if (i + 2 < table_bytes.len) (table_bytes[i + 2] as u32) else 0;
        const tail: u32 = (b0 << 24) | (b1 << 16) | (b2 << 8);
        sum = sum +% tail;
    }
    return sum;
}
