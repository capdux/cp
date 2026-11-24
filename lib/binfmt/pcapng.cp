// Reference-hardened against rusticata/pcap-parser (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc PCAPng Container & Packet Block Parser + 16-Lane SIMD) | Created: 2026-11-22

import { simdPcapCopyBytes } from "./pcap.cp";

/// PCAPng Block 类型常量与字节序魔数
pub const BLOCK_SHB: u32 = 0x0A0D0D0A;
pub const BLOCK_IDB: u32 = 0x00000001;
pub const BLOCK_SPB: u32 = 0x00000003;
pub const BLOCK_NRB: u32 = 0x00000004;
pub const BLOCK_ISB: u32 = 0x00000005;
pub const BLOCK_EPB: u32 = 0x00000006;
pub const BYTE_ORDER_MAGIC: u32 = 0x1A2B3C4D;
pub const BYTE_ORDER_MAGIC_SWAPPED: u32 = 0x4D3C2B1A;

/// 安全边界常量（防死循环、畸形长度与资源耗尽）
pub const PCAPNG_MIN_BLOCK_SIZE: u32 = 12;
pub const PCAPNG_SHB_MIN_BODY_SIZE: usize = 16;
pub const PCAPNG_IDB_MIN_BODY_SIZE: usize = 8;
pub const PCAPNG_EPB_MIN_BODY_SIZE: usize = 20;
pub const PCAPNG_DEFAULT_MAX_BLOCKS: usize = 1_000_000;
pub const PCAPNG_DEFAULT_MAX_OPTIONS: usize = 4096;
pub const PCAPNG_DEFAULT_MAX_SNAPLEN: u32 = 1024 * 1024;

pub err PcapngError {
    InvalidShb,
    InvalidByteOrderMagic,
    TruncatedBlock,
    BlockTooShort,
    UnalignedBlockLength,
    LengthMismatch,
    TooManyBlocks,
    InvalidIdb,
    InvalidEpb,
    CapturedLenOutOfBounds,
    SnaplenExceeded,
    InvalidOption,
    TooManyOptions,
    BufferTooSmall,
}

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

/// 零堆严格校验统计结果
pub struct PcapngValidationStats {
    section_count: usize,
    interface_count: usize,
    packet_count: usize,
    total_blocks: usize,
    total_packet_bytes: usize,
}

fn pcapngReadU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn pcapngReadU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn pcapngReadU16(data: []const u8, off: usize, is_be: bool): u16 {
    if (is_be) return pcapngReadU16BE(data, off);
    return pcapngReadU16LE(data, off);
}

fn pcapngReadU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn pcapngReadU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn pcapngReadU32(data: []const u8, off: usize, is_be: bool): u32 {
    if (is_be) return pcapngReadU32BE(data, off);
    return pcapngReadU32LE(data, off);
}

fn pcapngReadI64(data: []const u8, off: usize, is_be: bool): i64 {
    let raw: u64 = 0;
    if (is_be) {
        const hi: u64 = pcapngReadU32BE(data, off) as u64;
        const lo: u64 = pcapngReadU32BE(data, off + 4) as u64;
        raw = (hi << 32) | lo;
    } else {
        const lo: u64 = pcapngReadU32LE(data, off) as u64;
        const hi: u64 = pcapngReadU32LE(data, off + 4) as u64;
        raw = (hi << 32) | lo;
    }
    const val: i64 = @bitCast(raw);
    return val;
}

fn pcapngWriteU16(out: []u8, off: usize, v: u16, is_be: bool) {
    if (is_be) {
        out[off] = @as(u8, @truncate(v >> 8));
        out[off + 1] = @as(u8, @truncate(v));
    } else {
        out[off] = @as(u8, @truncate(v));
        out[off + 1] = @as(u8, @truncate(v >> 8));
    }
}

fn pcapngWriteU32(out: []u8, off: usize, v: u32, is_be: bool) {
    if (is_be) {
        out[off] = @as(u8, @truncate(v >> 24));
        out[off + 1] = @as(u8, @truncate(v >> 16));
        out[off + 2] = @as(u8, @truncate(v >> 8));
        out[off + 3] = @as(u8, @truncate(v));
    } else {
        out[off] = @as(u8, @truncate(v));
        out[off + 1] = @as(u8, @truncate(v >> 8));
        out[off + 2] = @as(u8, @truncate(v >> 16));
        out[off + 3] = @as(u8, @truncate(v >> 24));
    }
}

fn pcapngWriteI64(out: []u8, off: usize, v: i64, is_be: bool) {
    const raw: u64 = @bitCast(v);
    const hi: u32 = @as(u32, @truncate(raw >> 32));
    const lo: u32 = @as(u32, @truncate(raw));
    if (is_be) {
        pcapngWriteU32(out, off, hi, true);
        pcapngWriteU32(out, off + 4, lo, true);
    } else {
        pcapngWriteU32(out, off, lo, false);
        pcapngWriteU32(out, off + 4, hi, false);
    }
}

fn detectShbEndianness(data: []const u8, off: usize): bool? {
    if (data.len - off < 12) return null;
    if (pcapngReadU32LE(data, off) != BLOCK_SHB) return null;
    const magic_le = pcapngReadU32LE(data, off + 8);
    if (magic_le == BYTE_ORDER_MAGIC) return false;
    if (magic_le == BYTE_ORDER_MAGIC_SWAPPED) return true;
    return null;
}

/// 校验数据是否以合法 PCAPng SHB (`0x0A0D0D0A`) 及 LE (`0x1A2B3C4D`) / BE (`0x4D3C2B1A`) 字节序魔数开头
pub fn isPcapng(data: []const u8): bool {
    return detectShbEndianness(data, 0) != null;
}

/// 零堆分配 PCAPng Block 流式迭代器（支持按 SHB 自动识别与切换大小端，校验首尾长度一致、4 字节对齐与块数上限）
pub struct PcapngBlockIter {
    data: []const u8,
    pos: usize = 0,
    is_big_endian: bool = false,
    initialized: bool = false,
    had_error: bool = false,
    blocks_seen: usize = 0,
    max_blocks: usize = PCAPNG_DEFAULT_MAX_BLOCKS,

    pub static fn init(data: []const u8): PcapngBlockIter {
        return PcapngBlockIter.initWithMaxBlocks(data, PCAPNG_DEFAULT_MAX_BLOCKS);
    }

    pub static fn initWithMaxBlocks(data: []const u8, max_blocks: usize): PcapngBlockIter {
        if (be := detectShbEndianness(data, 0)) {
            return PcapngBlockIter {
                data: data,
                pos: 0,
                is_big_endian: be,
                initialized: true,
                had_error: false,
                blocks_seen: 0,
                max_blocks: max_blocks,
            };
        }
        return PcapngBlockIter {
            data: data,
            pos: 0,
            is_big_endian: false,
            initialized: false,
            had_error: true,
            blocks_seen: 0,
            max_blocks: max_blocks,
        };
    }

    /// 读取下一 PCAPng Block；到达流尾返回 `null`，遇长度不匹配/非 4B 对齐/越界/超块数上限时置 `had_error = true` 并返回 `null`
    pub fn next(*_): PcapngBlock? {
        if (!_.initialized || _.had_error || _.pos >= _.data.len) return null;
        if (_.blocks_seen >= _.max_blocks) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        if (_.data.len - _.pos < 12) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const p = _.pos;
        // If this block is a new SHB, detect its endianness first
        if (pcapngReadU32LE(_.data, p) == BLOCK_SHB) {
            if (be := detectShbEndianness(_.data, p)) {
                _.is_big_endian = be;
            } else {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
        }

        const btype = pcapngReadU32(_.data, p, _.is_big_endian);
        const total_len = pcapngReadU32(_.data, p + 4, _.is_big_endian);
        const len_usize: usize = total_len as usize;

        if (total_len < PCAPNG_MIN_BLOCK_SIZE || (total_len & 3) != 0 || len_usize > _.data.len - p) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const trailing_len = pcapngReadU32(_.data, p + len_usize - 4, _.is_big_endian);
        if (trailing_len != total_len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const body = _.data[p + 8 .. p + len_usize - 4];
        _.pos = p + len_usize;
        _.blocks_seen += 1;

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
    if (body.len < PCAPNG_SHB_MIN_BODY_SIZE) return null;
    const magic = pcapngReadU32(body, 0, is_be);
    if (magic != BYTE_ORDER_MAGIC) return null;

    return ShbInfo {
        major_version: pcapngReadU16(body, 4, is_be),
        minor_version: pcapngReadU16(body, 6, is_be),
        section_length: pcapngReadI64(body, 8, is_be),
    };
}

/// 解析 Interface Description Block (IDB) 主体元数据
pub fn parseIdb(body: []const u8, is_be: bool): IdbInfo? {
    if (body.len < PCAPNG_IDB_MIN_BODY_SIZE) return null;
    return IdbInfo {
        link_type: pcapngReadU16(body, 0, is_be),
        snap_len: pcapngReadU32(body, 4, is_be),
    };
}

/// 解析 Enhanced Packet Block (EPB) 主体并返回零拷贝报文数据切片（严格边界与 4B 对齐填充校验）
pub fn parseEpb(body: []const u8, is_be: bool): EpbPacket? {
    if (body.len < PCAPNG_EPB_MIN_BODY_SIZE) return null;

    const iface_id = pcapngReadU32(body, 0, is_be);
    const ts_hi = pcapngReadU32(body, 4, is_be);
    const ts_lo = pcapngReadU32(body, 8, is_be);
    const cap_len = pcapngReadU32(body, 12, is_be);
    const orig_len = pcapngReadU32(body, 16, is_be);
    const cap_usize: usize = cap_len as usize;
    const avail: usize = body.len - PCAPNG_EPB_MIN_BODY_SIZE;

    if (cap_usize > avail) return null;
    const rem: usize = cap_usize & 3;
    const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
    if (pad > avail - cap_usize) return null;

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
    had_error: bool = false,
    options_seen: usize = 0,
    max_options: usize = PCAPNG_DEFAULT_MAX_OPTIONS,

    pub static fn init(options_bytes: []const u8, is_be: bool): PcapngOptionIter {
        return PcapngOptionIter {
            data: options_bytes,
            pos: 0,
            is_big_endian: is_be,
            finished: false,
            had_error: false,
            options_seen: 0,
            max_options: PCAPNG_DEFAULT_MAX_OPTIONS,
        };
    }

    /// 读取下一条 TLV Option；遇 `opt_endofopt` (`code == 0`) 或边界不足时返回 `null`
    pub fn next(*_): PcapngOption? {
        if (_.finished || _.pos >= _.data.len) {
            _.finished = true;
            return null;
        }
        if (_.options_seen >= _.max_options) {
            _.had_error = true;
            _.finished = true;
            return null;
        }
        if (_.data.len - _.pos < 4) {
            _.had_error = true;
            _.finished = true;
            return null;
        }

        const code = pcapngReadU16(_.data, _.pos, _.is_big_endian);
        const opt_len = pcapngReadU16(_.data, _.pos + 2, _.is_big_endian);
        if (code == 0) {
            _.finished = true;
            return null;
        }

        const len_usize: usize = opt_len as usize;
        const rem: usize = len_usize & 3;
        const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
        const step: usize = len_usize + pad + 4;

        if (len_usize > _.data.len - (_.pos + 4) || step > _.data.len - _.pos) {
            _.had_error = true;
            _.finished = true;
            return null;
        }

        const val = _.data[_.pos + 4 .. _.pos + 4 + len_usize];
        _.pos += step;
        _.options_seen += 1;

        return PcapngOption {
            code: code,
            value: val,
        };
    }
}

// ============================================================================
// Zero-Alloc Strict Validator (PCAPng IETF Spec + rusticata/pcap-parser)
// ============================================================================

fn validatePcapngOptions(opt_bytes: []const u8, is_be: bool): PcapngError!void {
    if (opt_bytes.len == 0) return;
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < opt_bytes.len) {
        if (count >= PCAPNG_DEFAULT_MAX_OPTIONS) {
            return PcapngError.TooManyOptions;
        }
        if (opt_bytes.len - pos < 4) {
            return PcapngError.InvalidOption;
        }
        const code = pcapngReadU16(opt_bytes, pos, is_be);
        const opt_len = pcapngReadU16(opt_bytes, pos + 2, is_be);
        if (code == 0) {
            if (opt_len != 0) {
                return PcapngError.InvalidOption;
            }
            return;
        }
        const len_usize: usize = opt_len as usize;
        const rem: usize = len_usize & 3;
        const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
        const step: usize = len_usize + pad + 4;
        if (len_usize > opt_bytes.len - (pos + 4) || step > opt_bytes.len - pos) {
            return PcapngError.InvalidOption;
        }
        pos += step;
        count += 1;
    }
}

/// Zero-alloc strict validator for PCAPng streams using default resource limits.
pub fn validatePcapngStrict(data: []const u8): PcapngError!PcapngValidationStats {
    return validatePcapngStrictWithLimits(data, PCAPNG_DEFAULT_MAX_BLOCKS, PCAPNG_DEFAULT_MAX_SNAPLEN);
}

/// Zero-alloc strict validator for PCAPng streams with explicit max_blocks and max_snaplen bounds.
/// Verifies SHB leading magic (`0x1A2B3C4D`/`0x4D3C2B1A`), 12B min block size, 4-byte alignment,
/// matching leading & trailing `block_total_length`, IDB/EPB bounds, and option framing.
pub fn validatePcapngStrictWithLimits(
    data: []const u8,
    max_blocks: usize,
    max_snaplen: u32,
): PcapngError!PcapngValidationStats {
    if (data.len < 12) {
        return PcapngError.TruncatedBlock;
    }
    if (pcapngReadU32LE(data, 0) != BLOCK_SHB) {
        return PcapngError.InvalidShb;
    }
    const init_be = detectShbEndianness(data, 0) orelse return PcapngError.InvalidByteOrderMagic;

    let pos: usize = 0;
    let is_be: bool = init_be;
    let section_count: usize = 0;
    let interface_count: usize = 0;
    let packet_count: usize = 0;
    let total_blocks: usize = 0;
    let total_packet_bytes: usize = 0;

    while (pos < data.len) {
        if (total_blocks >= max_blocks) {
            return PcapngError.TooManyBlocks;
        }
        if (data.len - pos < 12) {
            return PcapngError.TruncatedBlock;
        }

        if (pcapngReadU32LE(data, pos) == BLOCK_SHB) {
            is_be = detectShbEndianness(data, pos) orelse return PcapngError.InvalidByteOrderMagic;
        }

        const btype = pcapngReadU32(data, pos, is_be);
        const total_len = pcapngReadU32(data, pos + 4, is_be);
        if (total_len < PCAPNG_MIN_BLOCK_SIZE) {
            return PcapngError.BlockTooShort;
        }
        if ((total_len & 3) != 0) {
            return PcapngError.UnalignedBlockLength;
        }
        const len_usize: usize = total_len as usize;
        if (len_usize > data.len - pos) {
            return PcapngError.TruncatedBlock;
        }

        const trailing_len = pcapngReadU32(data, pos + len_usize - 4, is_be);
        if (trailing_len != total_len) {
            return PcapngError.LengthMismatch;
        }

        const body = data[pos + 8 .. pos + len_usize - 4];

        if (btype == BLOCK_SHB) {
            if (body.len < PCAPNG_SHB_MIN_BODY_SIZE) {
                return PcapngError.InvalidShb;
            }
            if (pcapngReadU32(body, 0, is_be) != BYTE_ORDER_MAGIC) {
                return PcapngError.InvalidByteOrderMagic;
            }
            if (body.len > PCAPNG_SHB_MIN_BODY_SIZE) {
                validatePcapngOptions(body[PCAPNG_SHB_MIN_BODY_SIZE..], is_be)?;
            }
            section_count += 1;
        } else if (btype == BLOCK_IDB) {
            if (body.len < PCAPNG_IDB_MIN_BODY_SIZE) {
                return PcapngError.InvalidIdb;
            }
            const snap_len = pcapngReadU32(body, 4, is_be);
            if (snap_len > max_snaplen) {
                return PcapngError.SnaplenExceeded;
            }
            if (body.len > PCAPNG_IDB_MIN_BODY_SIZE) {
                validatePcapngOptions(body[PCAPNG_IDB_MIN_BODY_SIZE..], is_be)?;
            }
            interface_count += 1;
        } else if (btype == BLOCK_EPB) {
            if (body.len < PCAPNG_EPB_MIN_BODY_SIZE) {
                return PcapngError.InvalidEpb;
            }
            const cap_len = pcapngReadU32(body, 12, is_be);
            if (cap_len > max_snaplen) {
                return PcapngError.SnaplenExceeded;
            }
            const cap_usize: usize = cap_len as usize;
            const avail: usize = body.len - PCAPNG_EPB_MIN_BODY_SIZE;
            if (cap_usize > avail) {
                return PcapngError.CapturedLenOutOfBounds;
            }
            const rem: usize = cap_usize & 3;
            const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
            if (pad > avail - cap_usize) {
                return PcapngError.CapturedLenOutOfBounds;
            }
            const opt_off = PCAPNG_EPB_MIN_BODY_SIZE + cap_usize + pad;
            if (opt_off < body.len) {
                validatePcapngOptions(body[opt_off..], is_be)?;
            }
            packet_count += 1;
            total_packet_bytes += cap_usize;
        }

        pos += len_usize;
        total_blocks += 1;
    }

    if (section_count == 0) {
        return PcapngError.InvalidShb;
    }

    return PcapngValidationStats {
        section_count: section_count,
        interface_count: interface_count,
        packet_count: packet_count,
        total_blocks: total_blocks,
        total_packet_bytes: total_packet_bytes,
    };
}

// ============================================================================
// Zero-Alloc PCAPng Block Encoders (SHB / IDB / EPB with 16-Lane SIMD Copy)
// ============================================================================

/// Writes a 28-byte Section Header Block (SHB) into `out` and returns 28.
pub fn writePcapngShb(
    out: []u8,
    major: u16,
    minor: u16,
    section_len: i64,
    is_be: bool,
): PcapngError!usize {
    const total_len: usize = 28;
    if (out.len < total_len) return PcapngError.BufferTooSmall;
    pcapngWriteU32(out, 0, BLOCK_SHB, is_be);
    pcapngWriteU32(out, 4, 28, is_be);
    pcapngWriteU32(out, 8, BYTE_ORDER_MAGIC, is_be);
    pcapngWriteU16(out, 12, major, is_be);
    pcapngWriteU16(out, 14, minor, is_be);
    pcapngWriteI64(out, 16, section_len, is_be);
    pcapngWriteU32(out, 24, 28, is_be);
    return total_len;
}

/// Writes a 20-byte Interface Description Block (IDB) into `out` and returns 20.
pub fn writePcapngIdb(
    out: []u8,
    link_type: u16,
    snap_len: u32,
    is_be: bool,
): PcapngError!usize {
    const total_len: usize = 20;
    if (out.len < total_len) return PcapngError.BufferTooSmall;
    pcapngWriteU32(out, 0, BLOCK_IDB, is_be);
    pcapngWriteU32(out, 4, 20, is_be);
    pcapngWriteU16(out, 8, link_type, is_be);
    pcapngWriteU16(out, 10, 0, is_be);
    pcapngWriteU32(out, 12, snap_len, is_be);
    pcapngWriteU32(out, 16, 20, is_be);
    return total_len;
}

/// Writes an Enhanced Packet Block (EPB) into `out` using 16-lane SIMD payload copy and returns total block bytes.
pub fn writePcapngEpb(
    out: []u8,
    iface_id: u32,
    ts_hi: u32,
    ts_lo: u32,
    orig_len: u32,
    payload: []const u8,
    is_be: bool,
): PcapngError!usize {
    const rem: usize = payload.len & 3;
    const pad: usize = if (rem == 0) 0 else ((4 as usize) - rem);
    const total_len: usize = (32 as usize) + payload.len + pad;
    if (out.len < total_len) return PcapngError.BufferTooSmall;
    const total_u32: u32 = @intCast(total_len);
    const cap_u32: u32 = @intCast(payload.len);

    pcapngWriteU32(out, 0, BLOCK_EPB, is_be);
    pcapngWriteU32(out, 4, total_u32, is_be);
    pcapngWriteU32(out, 8, iface_id, is_be);
    pcapngWriteU32(out, 12, ts_hi, is_be);
    pcapngWriteU32(out, 16, ts_lo, is_be);
    pcapngWriteU32(out, 20, cap_u32, is_be);
    pcapngWriteU32(out, 24, orig_len, is_be);
    if (payload.len > 0) {
        simdPcapCopyBytes(out[28 .. 28 + payload.len], payload);
    }
    let p: usize = 0;
    while (p < pad) : (p += 1) {
        out[28 + payload.len + p] = 0;
    }
    pcapngWriteU32(out, total_len - 4, total_u32, is_be);
    return total_len;
}
