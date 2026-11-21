// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux RPM v3/v4 Package Header Parser)

/// RPM Lead 与 Header 魔数及结构尺寸常量
pub const RPM_LEAD_MAGIC: u32 = 0xEDABEEDB;
pub const RPM_LEAD_SIZE: usize = 96;
pub const RPM_HEADER_MAGIC: u32 = 0x8EADE8;
pub const RPM_HEADER_INTRO_SIZE: usize = 16;
pub const RPM_INDEX_ENTRY_SIZE: usize = 16;

/// RPM 常见 Header Tag 编号常量
pub const TAG_NAME: u32 = 1000;
pub const TAG_VERSION: u32 = 1001;
pub const TAG_RELEASE: u32 = 1002;
pub const TAG_SUMMARY: u32 = 1004;
pub const TAG_SIZE: u32 = 1009;
pub const TAG_LICENSE: u32 = 1014;
pub const TAG_OS: u32 = 1021;
pub const TAG_ARCH: u32 = 1022;
pub const TAG_PAYLOAD_COMPRESSOR: u32 = 1125;

/// RPM Header Tag 数据类型常量
pub const RPM_TYPE_INT16: u32 = 3;
pub const RPM_TYPE_INT32: u32 = 4;
pub const RPM_TYPE_INT64: u32 = 5;
pub const RPM_TYPE_STRING: u32 = 6;
pub const RPM_TYPE_BIN: u32 = 7;
pub const RPM_TYPE_STRING_ARRAY: u32 = 8;
pub const RPM_TYPE_I18NSTRING: u32 = 9;

/// RPM 96 字节 Lead 头部元数据
pub struct RpmLead {
    major: u8,
    minor: u8,
    /// `0` = Binary RPM, `1` = Source RPM
    pkg_type: u16,
    archnum: u16,
    name: []const u8,
    osnum: u16,
    signature_type: u16,

    /// 是否为二进制 RPM 包（`pkg_type == 0`）
    pub fn isBinary(): bool {
        return _.pkg_type == 0;
    }

    /// 是否为源码 RPM 包（`pkg_type == 1`）
    pub fn isSource(): bool {
        return _.pkg_type == 1;
    }
}

/// RPM Header / Signature 区段元信息
pub struct RpmHeaderSection {
    nindex: u32,
    hsize: u32,
    entries_offset: usize,
    store_offset: usize,
    total_bytes: usize,
    end_offset: usize,
    /// 8 字节边界填充对齐后的下一区段起始偏移（用于 Signature -> Main Header）
    next_aligned_offset: usize,
}

/// RPM 16 字节 Tag 索引条目
pub struct RpmTagEntry {
    tag: u32,
    tag_type: u32,
    offset: u32,
    count: u32,
}

/// 一站式解析后的 RPM 软件包核心元数据
pub struct RpmPackageInfo {
    lead: RpmLead,
    signature: RpmHeaderSection,
    header: RpmHeaderSection,
    payload_offset: usize,
    name: ([]const u8)?,
    version: ([]const u8)?,
    release: ([]const u8)?,
    arch: ([]const u8)?,
    payload_compressor: ([]const u8)?,
    installed_size: u32?,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn trimCString(buf: []const u8): []const u8 {
    let i: usize = 0;
    while (i < buf.len && buf[i] != 0) {
        i += 1;
    }
    return buf[0..i];
}

/// 校验数据是否以 96 字节 RPM Lead 魔数 `0xEDABEEDB` 开头
pub fn isRpm(data: []const u8): bool {
    if (data.len < RPM_LEAD_SIZE) return false;
    return readU32BE(data, 0) == RPM_LEAD_MAGIC;
}

/// 解析 96 字节大端 RPM Lead 头部
pub fn parseRpmLead(data: []const u8): RpmLead? {
    if (!isRpm(data)) return null;
    return RpmLead {
        major: data[4],
        minor: data[5],
        pkg_type: readU16BE(data, 6),
        archnum: readU16BE(data, 8),
        name: trimCString(data[10..76]),
        osnum: readU16BE(data, 76),
        signature_type: readU16BE(data, 78),
    };
}

/// 解析位于 `offset` 的 16 字节 RPM Header/Signature 区段头并严格防溢出校验索引与存储区边界
pub fn parseRpmHeaderSection(data: []const u8, offset: usize): RpmHeaderSection? {
    if (offset > data.len || data.len - offset < RPM_HEADER_INTRO_SIZE) return null;

    // 3-byte magic 0x8E 0xAD 0xE8 + version 0x01 + 4-byte reserved 0x00000000
    const magic24: u32 = ((data[offset] as u32) << 16)
        | ((data[offset + 1] as u32) << 8)
        | (data[offset + 2] as u32);
    if (magic24 != RPM_HEADER_MAGIC || data[offset + 3] != 0x01) return null;
    if (readU32BE(data, offset + 4) != 0) return null;

    const nindex = readU32BE(data, offset + 8);
    const hsize = readU32BE(data, offset + 12);

    const entries_offset: usize = offset + RPM_HEADER_INTRO_SIZE;
    const max_entries: usize = (data.len - entries_offset) / RPM_INDEX_ENTRY_SIZE;
    if ((nindex as usize) > max_entries) return null;

    const entries_bytes: usize = (nindex as usize) * RPM_INDEX_ENTRY_SIZE;
    const store_offset: usize = entries_offset + entries_bytes;
    const hsize_usize: usize = hsize as usize;
    if (hsize_usize > data.len - store_offset) return null;

    const total_bytes: usize = entries_bytes + hsize_usize + RPM_HEADER_INTRO_SIZE;
    const end_offset: usize = offset + total_bytes;
    const rem: usize = total_bytes & 7;
    const pad: usize = if (rem == 0) 0 else ((8 as usize) - rem);
    const next_aligned_offset: usize = end_offset + pad;

    return RpmHeaderSection {
        nindex: nindex,
        hsize: hsize,
        entries_offset: entries_offset,
        store_offset: store_offset,
        total_bytes: total_bytes,
        end_offset: end_offset,
        next_aligned_offset: next_aligned_offset,
    };
}

/// 零堆分配 RPM Header Tag 索引条目流式迭代器
pub struct RpmTagIter {
    data: []const u8,
    section: RpmHeaderSection,
    index: usize = 0,

    pub static fn init(data: []const u8, section: RpmHeaderSection): RpmTagIter {
        return RpmTagIter {
            data: data,
            section: section,
            index: 0,
        };
    }

    /// 读取下一条 16 字节 Tag 索引项；到达末尾时返回 `null`
    pub fn next(*_): RpmTagEntry? {
        if (_.index >= (_.section.nindex as usize)) return null;
        const off: usize = _.section.entries_offset + _.index * RPM_INDEX_ENTRY_SIZE;
        if (off + RPM_INDEX_ENTRY_SIZE > _.data.len) {
            _.index = _.section.nindex as usize;
            return null;
        }
        _.index += 1;
        return RpmTagEntry {
            tag: readU32BE(_.data, off),
            tag_type: readU32BE(_.data, off + 4),
            offset: readU32BE(_.data, off + 8),
            count: readU32BE(_.data, off + 12),
        };
    }
}

/// 从 Header 数据存储区安全读取以 `\0` 结尾的字符串 Tag 值
pub fn readRpmString(store: []const u8, entry: RpmTagEntry): ([]const u8)? {
    if (entry.tag_type != RPM_TYPE_STRING
        && entry.tag_type != RPM_TYPE_I18NSTRING
        && entry.tag_type != RPM_TYPE_STRING_ARRAY) {
        return null;
    }
    const off: usize = entry.offset as usize;
    if (off >= store.len) return null;

    let end = off;
    while (end < store.len && store[end] != 0) {
        end += 1;
    }
    if (end >= store.len) return null;
    return store[off..end];
}

/// 从 Header 数据存储区安全读取大端 32 位整数 Tag 值
pub fn readRpmU32(store: []const u8, entry: RpmTagEntry): u32? {
    if (entry.tag_type != RPM_TYPE_INT32 || entry.count == 0) return null;
    const off: usize = entry.offset as usize;
    if (off > store.len || store.len - off < 4) return null;
    return readU32BE(store, off);
}

/// 一站式解析 RPM 软件包 Lead + 8B 对齐 Signature + Main Header + Payload 偏移与核心元数据
pub fn parseRpmPackage(data: []const u8): RpmPackageInfo? {
    const lead = parseRpmLead(data) ?? return null;
    const sig = parseRpmHeaderSection(data, RPM_LEAD_SIZE) ?? return null;
    if (sig.next_aligned_offset > data.len) return null;

    const hdr = parseRpmHeaderSection(data, sig.next_aligned_offset) ?? return null;
    const store = data[hdr.store_offset .. hdr.end_offset];

    let name: ([]const u8)? = null;
    let version: ([]const u8)? = null;
    let release: ([]const u8)? = null;
    let arch: ([]const u8)? = null;
    let payload_compressor: ([]const u8)? = null;
    let installed_size: u32? = null;

    let it = RpmTagIter.init(data, hdr);
    while (entry := it.next()) {
        if (entry.tag == TAG_NAME) {
            name = readRpmString(store, entry);
        } else if (entry.tag == TAG_VERSION) {
            version = readRpmString(store, entry);
        } else if (entry.tag == TAG_RELEASE) {
            release = readRpmString(store, entry);
        } else if (entry.tag == TAG_ARCH) {
            arch = readRpmString(store, entry);
        } else if (entry.tag == TAG_PAYLOAD_COMPRESSOR) {
            payload_compressor = readRpmString(store, entry);
        } else if (entry.tag == TAG_SIZE) {
            installed_size = readRpmU32(store, entry);
        }
    }

    return RpmPackageInfo {
        lead: lead,
        signature: sig,
        header: hdr,
        payload_offset: hdr.end_offset,
        name: name,
        version: version,
        release: release,
        arch: arch,
        payload_compressor: payload_compressor,
        installed_size: installed_size,
    };
}
