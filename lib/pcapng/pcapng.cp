// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc PCAPng Container & Packet Block Parser)

/// PCAPng Block 类型常量与字节序魔数
pub const BLOCK_SHB: u32 = 0x0A0D0D0A;
pub const BLOCK_IDB: u32 = 0x00000001;
pub const BLOCK_SPB: u32 = 0x00000003;
pub const BLOCK_NRB: u32 = 0x00000004;
pub const BLOCK_ISB: u32 = 0x00000005;
pub const BLOCK_EPB: u32 = 0x00000006;
pub const BYTE_ORDER_MAGIC: u32 = 0x1A2B3C4D;

/// 零拷贝 PCAPng 通用 Block 视图
pub struct PcapngBlock {
    block_type: u32,
    total_length: u32,
    is_big_endian: bool,
    /// Block 主体切片（不含首 8 字节头与尾 4 字节长度）
    body: []const u8,
}

/// Section Header Block (SHB) 核心元数据
pub struct ShbInfo {
    major_version: u16,
    minor_version: u16,
    section_length: i64,
}

/// Interface Description Block (IDB) 核心元数据
pub struct IdbInfo {
    link_type: u16,
    snap_len: u32,
}

/// Enhanced Packet Block (EPB) 零拷贝报文视图
pub struct EpbPacket {
    interface_id: u32,
    timestamp_high: u32,
    timestamp_low: u32,
    captured_len: u32,
    original_len: u32,
    packet_data: []const u8,

    /// 组合高低 32 位时间戳为完整 64 位时间戳
    pub fn timestamp64(): u64 {
        const hi: u64 = _.timestamp_high as u64;
        const lo: u64 = _.timestamp_low as u64;
        return (hi << 32) | lo;
    }
}

/// 零拷贝 PCAPng TLV Option 视图
pub struct PcapngOption {
    code: u16,
    value: []const u8,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU16(data: []const u8, off: usize, is_be: bool): u16 {
    if (is_be) return readU16BE(data, off);
    return readU16LE(data, off);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU32(data: []const u8, off: usize, is_be: bool): u32 {
    if (is_be) return readU32BE(data, off);
    return readU32LE(data, off);
}

fn readI64(data: []const u8, off: usize, is_be: bool): i64 {
    let raw: u64 = 0;
    if (is_be) {
        const hi: u64 = readU32BE(data, off) as u64;
        const lo: u64 = readU32BE(data, off + 4) as u64;
        raw = (hi << 32) | lo;
    } else {
        const lo: u64 = readU32LE(data, off) as u64;
        const hi: u64 = readU32LE(data, off + 4) as u64;
        raw = (hi << 32) | lo;
    }
    const val: i64 = @bitCast(raw);
    return val;
}

fn detectShbEndianness(data: []const u8, off: usize): bool? {
    if (data.len - off < 12) return null;
    if (readU32LE(data, off) != BLOCK_SHB) return null;
    const magic_le = readU32LE(data, off + 8);
    if (magic_le == BYTE_ORDER_MAGIC) return false;
    const magic_be = readU32BE(data, off + 8);
    if (magic_be == BYTE_ORDER_MAGIC) return true;
    return null;
}

/// 校验数据是否以合法 PCAPng SHB (`0x0A0D0D0A`) 及 LE/BE `0x1A2B3C4D` 字节序魔数开头
pub fn isPcapng(data: []const u8): bool {
    return detectShbEndianness(data, 0) != null;
}

/// 零堆分配 PCAPng Block 流式迭代器（支持按 SHB 自动识别与切换大小端，校验首尾长度一致与 4 字节对齐）
pub struct PcapngBlockIter {
    data: []const u8,
    pos: usize = 0,
    is_big_endian: bool = false,
    initialized: bool = false,
    had_error: bool = false,

    pub static fn init(data: []const u8): PcapngBlockIter {
        if (be := detectShbEndianness(data, 0)) {
            return PcapngBlockIter {
                data: data,
                pos: 0,
                is_big_endian: be,
                initialized: true,
                had_error: false,
            };
        }
        return PcapngBlockIter {
            data: data,
            pos: 0,
            is_big_endian: false,
            initialized: false,
            had_error: true,
        };
    }

    /// 读取下一 PCAPng Block；到达流尾返回 `null`，遇长度不匹配/非 4B 对齐/越界时置 `had_error = true` 并返回 `null`
    pub fn next(*_): PcapngBlock? {
        if (!_.initialized || _.had_error || _.pos >= _.data.len) return null;
        if (_.data.len - _.pos < 12) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const p = _.pos;
        // If this block is a new SHB, detect its endianness first
        if (readU32LE(_.data, p) == BLOCK_SHB) {
            if (be := detectShbEndianness(_.data, p)) {
                _.is_big_endian = be;
            } else {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
        }

        const btype = readU32(_.data, p, _.is_big_endian);
        const total_len = readU32(_.data, p + 4, _.is_big_endian);
        const len_usize: usize = total_len as usize;

        if (total_len < 12 || (total_len & 3) != 0 || len_usize > _.data.len - p) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const trailing_len = readU32(_.data, p + len_usize - 4, _.is_big_endian);
        if (trailing_len != total_len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const body = _.data[p + 8 .. p + len_usize - 4];
        _.pos = p + len_usize;

        return PcapngBlock {
            block_type: btype,
            total_length: total_len,
            is_big_endian: _.is_big_endian,
            body: body,
        };
    }
}

/// 解析 Section Header Block (SHB) 主体元数据
pub fn parseShb(body: []const u8, is_be: bool): ShbInfo? {
    if (body.len < 16) return null;
    const magic = readU32(body, 0, is_be);
    if (magic != BYTE_ORDER_MAGIC) return null;

    return ShbInfo {
        major_version: readU16(body, 4, is_be),
        minor_version: readU16(body, 6, is_be),
        section_length: readI64(body, 8, is_be),
    };
}

/// 解析 Interface Description Block (IDB) 主体元数据
pub fn parseIdb(body: []const u8, is_be: bool): IdbInfo? {
    if (body.len < 8) return null;
    return IdbInfo {
        link_type: readU16(body, 0, is_be),
        snap_len: readU32(body, 4, is_be),
    };
}

/// 解析 Enhanced Packet Block (EPB) 主体并返回零拷贝报文数据切片（严格边界校验）
pub fn parseEpb(body: []const u8, is_be: bool): EpbPacket? {
    if (body.len < 20) return null;

    const iface_id = readU32(body, 0, is_be);
    const ts_hi = readU32(body, 4, is_be);
    const ts_lo = readU32(body, 8, is_be);
    const cap_len = readU32(body, 12, is_be);
    const orig_len = readU32(body, 16, is_be);
    const cap_usize: usize = cap_len as usize;

    if (cap_usize > body.len - 20) return null;

    return EpbPacket {
        interface_id: iface_id,
        timestamp_high: ts_hi,
        timestamp_low: ts_lo,
        captured_len: cap_len,
        original_len: orig_len,
        packet_data: body[20 .. 20 + cap_usize],
    };
}

/// 零堆分配 PCAPng 4 字节对齐 TLV Option 流式迭代器（遇 `code == 0` `opt_endofopt` 终止）
pub struct PcapngOptionIter {
    data: []const u8,
    pos: usize = 0,
    is_big_endian: bool = false,
    finished: bool = false,

    pub static fn init(options_bytes: []const u8, is_be: bool): PcapngOptionIter {
        return PcapngOptionIter {
            data: options_bytes,
            pos: 0,
            is_big_endian: is_be,
            finished: false,
        };
    }

    /// 读取下一条 TLV Option；遇 `opt_endofopt` (`code == 0`) 或边界不足时返回 `null`
    pub fn next(*_): PcapngOption? {
        if (_.finished || _.data.len - _.pos < 4) {
            _.finished = true;
            return null;
        }

        const code = readU16(_.data, _.pos, _.is_big_endian);
        const opt_len = readU16(_.data, _.pos + 2, _.is_big_endian);
        if (code == 0) {
            _.finished = true;
            return null;
        }

        const len_usize: usize = opt_len as usize;
        const rem: usize = len_usize & 3;
        const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
        const step: usize = len_usize + pad + 4;

        if (len_usize > _.data.len - (_.pos + 4)) {
            _.finished = true;
            return null;
        }

        const val = _.data[_.pos + 4 .. _.pos + 4 + len_usize];
        if (step <= _.data.len - _.pos) {
            _.pos += step;
        } else {
            _.pos = _.data.len;
        }

        return PcapngOption {
            code: code,
            value: val,
        };
    }
}
