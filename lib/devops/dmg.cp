// Origin: Pure-CP Stdlib (Zero-Alloc Apple UDIF .dmg koly & BLKX mish Parser) | Created: 2026-11-22

/// Apple UDIF (.dmg) 常量与 BLKX 块类型标志
pub const DMG_KOLY_MAGIC: []const u8 = "koly";
pub const DMG_KOLY_SIZE: usize = 512;
pub const DMG_MISH_MAGIC: []const u8 = "mish";
pub const DMG_MISH_HEADER_SIZE: usize = 204;
pub const DMG_STRIPE_SIZE: usize = 40;

pub const BLKX_ZERO: u32 = 0x00000000;
pub const BLKX_RAW: u32 = 0x00000001;
pub const BLKX_IGNORE: u32 = 0x00000002;
pub const BLKX_ADC: u32 = 0x80000004;
pub const BLKX_ZLIB: u32 = 0x80000005;
pub const BLKX_BZ2: u32 = 0x80000006;
pub const BLKX_LZFSE: u32 = 0x80000007;
pub const BLKX_LZMA: u32 = 0x80000008;
pub const BLKX_COMMENT: u32 = 0x7FFFFFFE;
pub const BLKX_LAST: u32 = 0xFFFFFFFF;

/// 末尾 512 字节大端 `koly` UDIF 尾部块解析结构
pub struct DmgKolyHeader {
    version: u32,
    header_size: u32,
    flags: u32,
    running_data_fork_offset: u64,
    data_fork_offset: u64,
    data_fork_length: u64,
    rsrc_fork_offset: u64,
    rsrc_fork_length: u64,
    segment_number: u32,
    segment_count: u32,
    data_checksum_type: u32,
    data_checksum_size: u32,
    xml_offset: u64,
    xml_length: u64,
    master_checksum_type: u32,
    image_variant: u32,
    sector_count: u64,

    /// 零拷贝提取内嵌 XML plist 切片（防溢出越界）
    fn xmlSlice(data: []const u8): ([]const u8)? {
        const total: u64 = data.len as u64;
        if (_.xml_offset > total || _.xml_length > total - _.xml_offset) {
            return null;
        }
        const start = _.xml_offset as usize;
        const end = (_.xml_offset + _.xml_length) as usize;
        return data[start..end];
    }

    /// 零拷贝提取数据分叉（Data Fork）切片（防溢出越界）
    fn dataForkSlice(data: []const u8): ([]const u8)? {
        const total: u64 = data.len as u64;
        if (_.data_fork_offset > total || _.data_fork_length > total - _.data_fork_offset) {
            return null;
        }
        const start = _.data_fork_offset as usize;
        const end = (_.data_fork_offset + _.data_fork_length) as usize;
        return data[start..end];
    }

    /// 计算未压缩逻辑镜像总字节数（`sector_count * 512`，防乘法溢出饱和至 `u64::MAX`）
    fn uncompressedBytes(): u64 {
        // 0xFFFFFFFFFFFFFFFF / 512 = 36028797018963967 (0x007FFFFFFFFFFFFF)
        if (_.sector_count > 36028797018963967) {
            return 0xFFFFFFFFFFFFFFFF;
        }
        return _.sector_count * 512;
    }
}

/// 204 字节大端 `mish` BLKX 分区块表头
pub struct DmgMishHeader {
    version: u32,
    sector_start: u64,
    sector_count: u64,
    data_offset: u64,
    buffers_needed: u32,
    block_descriptors: u32,
    stripe_count: u32,
}

/// 40 字节大端 BLKX 块描述符条带（Stripe）
pub struct DmgBlkStripe {
    entry_type: u32,
    sector_start: u64,
    sector_count: u64,
    comp_offset: u64,
    comp_length: u64,
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

/// 校验字节切片是否为合法 UDIF `.dmg` 镜像：
/// `data.len >= 512` 且末尾 512B 开头为 `"koly"`、版本号为 `4`、`header_size == 512`
pub fn isDmg(data: []const u8): bool {
    if (data.len < DMG_KOLY_SIZE) return false;
    const base = data.len - DMG_KOLY_SIZE;
    if (!strEq(data[base .. base + 4], DMG_KOLY_MAGIC)) return false;
    if (readU32BE(data, base + 4) != 4) return false;
    if (readU32BE(data, base + 8) != 512) return false;
    return true;
}

/// 解析 `.dmg` 末尾 512 字节大端 `koly` 尾部块
pub fn parseDmgKoly(data: []const u8): DmgKolyHeader? {
    if (!isDmg(data)) return null;
    const base = data.len - DMG_KOLY_SIZE;
    const koly = data[base .. data.len];

    return DmgKolyHeader {
        version: readU32BE(koly, 4),
        header_size: readU32BE(koly, 8),
        flags: readU32BE(koly, 12),
        running_data_fork_offset: readU64BE(koly, 16),
        data_fork_offset: readU64BE(koly, 24),
        data_fork_length: readU64BE(koly, 32),
        rsrc_fork_offset: readU64BE(koly, 40),
        rsrc_fork_length: readU64BE(koly, 48),
        segment_number: readU32BE(koly, 56),
        segment_count: readU32BE(koly, 60),
        data_checksum_type: readU32BE(koly, 80),
        data_checksum_size: readU32BE(koly, 84),
        xml_offset: readU64BE(koly, 216),
        xml_length: readU64BE(koly, 224),
        master_checksum_type: readU32BE(koly, 352),
        image_variant: readU32BE(koly, 488),
        sector_count: readU64BE(koly, 492),
    };
}

/// 解析 204 字节大端 `mish` BLKX 表头
pub fn parseDmgMish(data: []const u8): DmgMishHeader? {
    if (data.len < DMG_MISH_HEADER_SIZE) return null;
    if (!strEq(data[0..4], DMG_MISH_MAGIC)) return null;

    return DmgMishHeader {
        version: readU32BE(data, 4),
        sector_start: readU64BE(data, 8),
        sector_count: readU64BE(data, 16),
        data_offset: readU64BE(data, 24),
        buffers_needed: readU32BE(data, 32),
        block_descriptors: readU32BE(data, 36),
        stripe_count: readU32BE(data, 200),
    };
}

/// 零堆分配 BLKX 40B 条带（`DmgBlkStripe`）迭代器
pub struct DmgStripeIter {
    data: []const u8,
    pos: usize,
    index: u32 = 0,
    count: u32,
    had_error: bool = false,

    static fn init(data: []const u8): DmgStripeIter {
        if (hdr := parseDmgMish(data)) {
            return DmgStripeIter {
                data: data,
                pos: DMG_MISH_HEADER_SIZE,
                index: 0,
                count: hdr.stripe_count,
                had_error: false,
            };
        }
        return DmgStripeIter {
            data: data,
            pos: 0,
            index: 0,
            count: 0,
            had_error: true,
        };
    }

    static fn fromHeader(data: []const u8, header: DmgMishHeader): DmgStripeIter {
        if (data.len < DMG_MISH_HEADER_SIZE) {
            return DmgStripeIter {
                data: data,
                pos: data.len,
                index: 0,
                count: 0,
                had_error: true,
            };
        }
        return DmgStripeIter {
            data: data,
            pos: DMG_MISH_HEADER_SIZE,
            index: 0,
            count: header.stripe_count,
            had_error: false,
        };
    }

    fn next(*_): DmgBlkStripe? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.pos > _.data.len || DMG_STRIPE_SIZE > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const entry_type = readU32BE(_.data, off);
        const sector_start = readU64BE(_.data, off + 8);
        const sector_count = readU64BE(_.data, off + 16);
        const comp_offset = readU64BE(_.data, off + 24);
        const comp_length = readU64BE(_.data, off + 32);

        _.pos += DMG_STRIPE_SIZE;
        _.index += 1;

        return DmgBlkStripe {
            entry_type: entry_type,
            sector_start: sector_start,
            sector_count: sector_count,
            comp_offset: comp_offset,
            comp_length: comp_length,
        };
    }
}
