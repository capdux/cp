// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 3533 Ogg Container Page Parser)

/// RFC 3533 Ogg 容器分页头部标志位常量
pub const OGG_FLAG_CONTINUATION: u8 = 0x01;
pub const OGG_FLAG_BOS: u8 = 0x02;
pub const OGG_FLAG_EOS: u8 = 0x04;

/// 零拷贝 RFC 3533 Ogg 分页视图（27 字节页头 + 段表 + 负载）
pub struct OggPage {
    version: u8,
    header_type: u8,
    granule_position: u64,
    serial_number: u32,
    page_sequence: u32,
    checksum: u32,
    page_segments: u8,
    segment_table: []const u8,
    payload: []const u8,
    raw_page: []const u8,

    /// 是否为上一页未完数据包的续页（`header_type & 0x01`）
    pub fn isContinuation(): bool {
        return (_.header_type & OGG_FLAG_CONTINUATION) != 0;
    }

    /// 是否为逻辑比特流首页（Beginning of Stream，`header_type & 0x02`）
    pub fn isBos(): bool {
        return (_.header_type & OGG_FLAG_BOS) != 0;
    }

    /// 是否为逻辑比特流尾页（End of Stream，`header_type & 0x04`）
    pub fn isEos(): bool {
        return (_.header_type & OGG_FLAG_EOS) != 0;
    }

    /// 校验整页的 RFC 3533 非反射 CRC-32 是否与页头 `checksum` 一致
    pub fn verifyChecksum(): bool {
        return oggCrc32(_.raw_page) == _.checksum;
    }
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU64LE(data: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(data, off) as u64;
    const hi: u64 = readU32LE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

/// 计算 RFC 3533 Ogg 分页非反射 CRC-32（多项式 `0x04C11DB7`，偏移 `22..26` 自动按 0 字节处理）
pub fn oggCrc32(page_bytes: []const u8): u32 {
    let crc: u32 = 0;
    let i: usize = 0;
    while (i < page_bytes.len) {
        const b: u8 = if (i >= 22 && i < 26) 0 else page_bytes[i];
        crc ^= (b as u32) << 24;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 0x80000000) != 0) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = crc << 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// 探测缓冲区是否以合法 Ogg `"OggS"` 魔数及 `version == 0` 开头
pub fn isOgg(data: []const u8): bool {
    if (data.len < 5) return false;
    return data[0] == 'O'
        && data[1] == 'g'
        && data[2] == 'g'
        && data[3] == 'S'
        && data[4] == 0;
}

/// 零堆分配 RFC 3533 Ogg 分页流式迭代器
pub struct OggPageIter {
    data: []const u8,
    pos: usize = 0,

    pub static fn init(data: []const u8): OggPageIter {
        return OggPageIter {
            data: data,
            pos: 0,
        };
    }

    /// 解析下一 Ogg 分页；到达流尾、魔数不匹配或缓冲区截断时安全返回 `null`
    pub fn next(*_): OggPage? {
        if (_.data.len - _.pos < 27) {
            _.pos = _.data.len;
            return null;
        }

        const p = _.pos;
        if (_.data[p] != 'O'
            || _.data[p + 1] != 'g'
            || _.data[p + 2] != 'g'
            || _.data[p + 3] != 'S'
            || _.data[p + 4] != 0) {
            _.pos = _.data.len;
            return null;
        }

        const seg_count_u8 = _.data[p + 26];
        const seg_count: usize = seg_count_u8 as usize;
        const header_total: usize = seg_count + 27;
        if (_.data.len - p < header_total) {
            _.pos = _.data.len;
            return null;
        }

        const seg_table = _.data[p + 27 .. p + header_total];
        let payload_len: usize = 0;
        let i: usize = 0;
        while (i < seg_table.len) {
            payload_len += seg_table[i] as usize;
            i += 1;
        }

        const page_total: usize = header_total + payload_len;
        if (_.data.len - p < page_total) {
            _.pos = _.data.len;
            return null;
        }

        const page = OggPage {
            version: _.data[p + 4],
            header_type: _.data[p + 5],
            granule_position: readU64LE(_.data, p + 6),
            serial_number: readU32LE(_.data, p + 14),
            page_sequence: readU32LE(_.data, p + 18),
            checksum: readU32LE(_.data, p + 22),
            page_segments: seg_count_u8,
            segment_table: seg_table,
            payload: _.data[p + header_total .. p + page_total],
            raw_page: _.data[p .. p + page_total],
        };

        _.pos = p + page_total;
        return page;
    }
}
