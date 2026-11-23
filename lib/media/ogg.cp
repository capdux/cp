// Reference-hardened against RustAudio/ogg (BSD-3-Clause)
// Origin: Pure-CP Stdlib (RFC 3533 Ogg Container Page & Multi-Stream Packet Codec) | Created: 2026-11-22

const std = @import("std");
import { Simd } from "cap:simd";

/// RFC 3533 Ogg 容器分页头部标志位常量
pub const OGG_FLAG_CONTINUATION: u8 = 0x01;
pub const OGG_FLAG_BOS: u8 = 0x02;
pub const OGG_FLAG_EOS: u8 = 0x04;
pub const OGG_GRANULE_UNKNOWN: u64 = std.math.maxInt(u64);
pub const OGG_MAX_PAGE_SEGMENTS: usize = 255;
pub const OGG_MAX_PAGE_PAYLOAD: usize = 255 * 255;

pub error OggError {
    UnexpectedEof,
    InvalidCapturePattern,
    UnsupportedVersion,
    ChecksumMismatch,
    InvalidHeaderFlags,
    InvalidPacketState,
    OutOfMemory,
}

/// 16-lane SIMD `@Vector(16, u16)` widening sum for Ogg segment lacing table.
pub fn oggSumSegmentsSimd(seg_table: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i + 16 <= seg_table.len) : (i += 16) {
        const bytes_v: @Vector(16, u8) = seg_table[i..][0..16].*;
        const wide_v: @Vector(16, u16) = @intCast(bytes_v);
        total += @reduce(.Add, wide_v) as usize;
    }
    while (i < seg_table.len) : (i += 1) {
        total += seg_table[i] as usize;
    }
    return total;
}

/// Scalar reference segment sum for 0-diff parity testing against `oggSumSegmentsSimd`.
pub fn oggSumSegmentsScalar(seg_table: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < seg_table.len) : (i += 1) {
        total += seg_table[i] as usize;
    }
    return total;
}

/// 16-lane SIMD `@Vector(16, u8)` payload copy for hot-path Ogg packet assembly/writing.
pub fn oggCopyPayloadSimd(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i + 16 <= src.len) : (i += 16) {
        const chunk: @Vector(16, u8) = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
    }
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference payload copy for 0-diff parity testing against `oggCopyPayloadSimd`.
pub fn oggCopyPayloadScalar(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

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

    /// 检查保留标志位是否全 0（`(header_type & 0xF8) == 0`）
    pub fn hasValidReservedFlags(): bool {
        return (_.header_type & 0xF8) == 0;
    }

    /// 是否为未知/未完成包的 Granule Position（`0xFFFFFFFFFFFFFFFF`）
    pub fn isUnknownGranule(): bool {
        return _.granule_position == OGG_GRANULE_UNKNOWN;
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

fn writeU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64LE(buf: []u8, off: usize, v: u64) {
    writeU32LE(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32LE(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

/// 计算 RFC 3533 Ogg 分页非反射 CRC-32（多项式 `0x04C11DB7`，偏移 `22..26` 自动按 0 字节处理）
pub fn oggCrc32(page_bytes: []const u8): u32 {
    let crc: u32 = 0;
    let i: usize = 0;
    while (i < page_bytes.len) : (i += 1) {
        const b: u8 = if (i >= 22 && i < 26) 0 else page_bytes[i];
        crc ^= (b as u32) << 24;
        let bit: u8 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 0x80000000) != 0) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = crc << 1;
            }
        }
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

fn tryParsePageAt(data: []const u8, p: usize): OggPage? {
    if (p >= data.len || data.len - p < 27) return null;
    if (!isOgg(data[p..])) return null;
    const seg_count_u8 = data[p + 26];
    const seg_count: usize = seg_count_u8 as usize;
    const header_total: usize = seg_count + 27;
    if (data.len - p < header_total) return null;

    const seg_table = data[p + 27 .. p + header_total];
    const payload_len = oggSumSegmentsSimd(seg_table);
    const page_total: usize = header_total + payload_len;
    if (data.len - p < page_total) return null;

    return OggPage {
        version: data[p + 4],
        header_type: data[p + 5],
        granule_position: readU64LE(data, p + 6),
        serial_number: readU32LE(data, p + 14),
        page_sequence: readU32LE(data, p + 18),
        checksum: readU32LE(data, p + 22),
        page_segments: seg_count_u8,
        segment_table: seg_table,
        payload: data[p + header_total .. p + page_total],
        raw_page: data[p .. p + page_total],
    };
}

/// SIMD-accelerated resync scan for the next valid CRC-32 verified Ogg page starting at `start`.
pub fn findNextOggPage(data: []const u8, start: usize): usize? {
    let i = start;
    while (i + 27 <= data.len) {
        const hit = Simd.indexOfByte(data[i..], 'O') orelse return null;
        i += hit;
        if (i + 27 > data.len) return null;
        if (page := tryParsePageAt(data, i)) {
            if (page.verifyChecksum()) return i;
        }
        i += 1;
    }
    return null;
}

/// Scalar reference resync scan for 0-diff parity testing against `findNextOggPage`.
pub fn findNextOggPageScalar(data: []const u8, start: usize): usize? {
    let i = start;
    while (i + 27 <= data.len) : (i += 1) {
        if (data[i] == 'O') {
            if (page := tryParsePageAt(data, i)) {
                if (page.verifyChecksum()) return i;
            }
        }
    }
    return null;
}

/// Parses and strictly validates a single RFC 3533 Ogg page at `offset`.
pub fn parseOggPage(data: []const u8, offset: usize): OggPage! {
    if (offset >= data.len || data.len - offset < 27) return OggError.UnexpectedEof;
    if (data[offset] != 'O' || data[offset + 1] != 'g' || data[offset + 2] != 'g' || data[offset + 3] != 'S') {
        return OggError.InvalidCapturePattern;
    }
    if (data[offset + 4] != 0) return OggError.UnsupportedVersion;
    if ((data[offset + 5] & 0xF8) != 0) return OggError.InvalidHeaderFlags;

    const seg_count: usize = data[offset + 26] as usize;
    const header_total: usize = seg_count + 27;
    if (data.len - offset < header_total) return OggError.UnexpectedEof;

    const seg_table = data[offset + 27 .. offset + header_total];
    const payload_len = oggSumSegmentsSimd(seg_table);
    const page_total: usize = header_total + payload_len;
    if (data.len - offset < page_total) return OggError.UnexpectedEof;

    const page = OggPage {
        version: data[offset + 4],
        header_type: data[offset + 5],
        granule_position: readU64LE(data, offset + 6),
        serial_number: readU32LE(data, offset + 14),
        page_sequence: readU32LE(data, offset + 18),
        checksum: readU32LE(data, offset + 22),
        page_segments: data[offset + 26],
        segment_table: seg_table,
        payload: data[offset + header_total .. offset + page_total],
        raw_page: data[offset .. offset + page_total],
    };
    if (!page.verifyChecksum()) return OggError.ChecksumMismatch;
    return page;
}

/// Validates an entire Ogg physical stream (headers, bounds, reserved flags, and CRC-32).
pub fn validateOggStream(data: []const u8): !void {
    let pos: usize = 0;
    while (pos < data.len) {
        const page = try parseOggPage(data, pos);
        pos += page.raw_page.len;
    }
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
        if (_.pos >= _.data.len || _.data.len - _.pos < 27) {
            _.pos = _.data.len;
            return null;
        }
        const page = tryParsePageAt(_.data, _.pos) orelse {
            _.pos = _.data.len;
            return null;
        };
        _.pos += page.raw_page.len;
        return page;
    }
}

/// Reassembled logical packet yielded by `OggPacketReader`.
pub struct OggPacket {
    pub serial_number: u32,
    pub granule_position: u64,
    pub is_bos: bool,
    pub is_eos: bool,
    pub data: []u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.data);
    }
}

struct OggStreamAssembly {
    serial_number: u32,
    buf: std.ArrayList(u8) = .empty,
    in_packet: bool = false,
    started_bos: bool = false,
}

/// Multi-stream RFC 3533 Ogg packet reassembler with CRC-32 verification and demuxing by `serial_number`.
pub struct OggPacketReader {
    alloc: std.mem.Allocator,
    data: []const u8,
    pos: usize = 0,
    streams: std.ArrayList(OggStreamAssembly) = .empty,
    cur_page: OggPage? = null,
    cur_seg_idx: usize = 0,
    cur_payload_off: usize = 0,
    verify_crc: bool = true,

    pub static fn init(alloc: std.mem.Allocator, data: []const u8): OggPacketReader {
        return OggPacketReader {
            .alloc = alloc,
            .data = data,
            .pos = 0,
            .streams = .empty,
            .cur_page = null,
            .cur_seg_idx = 0,
            .cur_payload_off = 0,
            .verify_crc = true,
        };
    }

    pub fn deinit(*_) {
        for (*st of _.streams.items) {
            st.buf.deinit(_.alloc);
        }
        _.streams.deinit(_.alloc);
    }

    fn getOrCreateStream(*_, serial: u32): !*OggStreamAssembly {
        let i: usize = 0;
        while (i < _.streams.items.len) : (i += 1) {
            if (_.streams.items[i].serial_number == serial) return &_.streams.items[i];
        }
        _.streams.append(_.alloc, OggStreamAssembly { .serial_number = serial }) catch return OggError.OutOfMemory;
        return &_.streams.items[_.streams.items.len - 1];
    }

    /// Reads the next complete reassembled `OggPacket`, returning `null` at EOF.
    pub fn nextPacket(*_): OggPacket?! {
        while (true) {
            if (_.cur_page == null) {
                if (_.pos >= _.data.len) return null;
                const page = try parseOggPage(_.data, _.pos);
                _.pos += page.raw_page.len;
                _.cur_page = page;
                _.cur_seg_idx = 0;
                _.cur_payload_off = 0;

                const st = try _.getOrCreateStream(page.serial_number);
                if (page.isContinuation() && !st.in_packet) {
                    return OggError.InvalidPacketState;
                }
                if (!page.isContinuation() && st.in_packet) {
                    // Reset incomplete packet if stream dropped a page
                    st.buf.clearRetainingCapacity();
                    st.in_packet = false;
                }
                if (page.page_segments == 0) {
                    _.cur_page = null;
                    continue;
                }
            }

            const page = _.cur_page.?;
            const st = try _.getOrCreateStream(page.serial_number);

            while (_.cur_seg_idx < page.segment_table.len) {
                const seg_len: usize = page.segment_table[_.cur_seg_idx] as usize;
                _.cur_seg_idx += 1;
                if (!st.in_packet) {
                    st.in_packet = true;
                    st.started_bos = page.isBos();
                }
                if (seg_len > 0) {
                    const chunk = page.payload[_.cur_payload_off .. _.cur_payload_off + seg_len];
                    _.cur_payload_off += seg_len;
                    if (seg_len >= 16) {
                        st.buf.ensureUnusedCapacity(_.alloc, seg_len) catch return OggError.OutOfMemory;
                        const old_len = st.buf.items.len;
                        oggCopyPayloadSimd(st.buf.items.ptr[old_len .. old_len + seg_len], chunk);
                        st.buf.items.len = old_len + seg_len;
                    } else {
                        st.buf.appendSlice(_.alloc, chunk) catch return OggError.OutOfMemory;
                    }
                }
                if (seg_len < 255) {
                    const pkt_data = st.buf.toOwnedSlice(_.alloc) catch return OggError.OutOfMemory;
                    const is_last_seg = (_.cur_seg_idx == page.segment_table.len);
                    const pkt = OggPacket {
                        .serial_number = page.serial_number,
                        .granule_position = if (is_last_seg) page.granule_position else OGG_GRANULE_UNKNOWN,
                        .is_bos = st.started_bos,
                        .is_eos = is_last_seg && page.isEos(),
                        .data = pkt_data,
                    };
                    st.in_packet = false;
                    st.started_bos = false;
                    if (is_last_seg) {
                        _.cur_page = null;
                    }
                    return pkt;
                }
            }
            _.cur_page = null;
        }
    }
}

/// RFC 3533 Ogg page & multi-page packet encoder with exact CRC-32 calculation.
pub struct OggPacketWriter {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator): OggPacketWriter {
        return OggPacketWriter {
            .alloc = alloc,
            .buf = .empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn slice(): []const u8 {
        return _.buf.items;
    }

    pub fn toOwnedSlice(*_): ([]u8)! {
        return _.buf.toOwnedSlice(_.alloc) catch return OggError.OutOfMemory;
    }

    fn emitPage(*_, header_type: u8, granule: u64, serial: u32, seq: u32, segs: []const u8, payload: []const u8): ! {
        const page_start = _.buf.items.len;
        const page_len = 27 + segs.len + payload.len;
        _.buf.ensureUnusedCapacity(_.alloc, page_len) catch return OggError.OutOfMemory;
        _.buf.items.len += page_len;
        let out = _.buf.items[page_start .. page_start + page_len];

        out[0] = 'O';
        out[1] = 'g';
        out[2] = 'g';
        out[3] = 'S';
        out[4] = 0; // version
        out[5] = header_type;
        writeU64LE(out, 6, granule);
        writeU32LE(out, 14, serial);
        writeU32LE(out, 18, seq);
        writeU32LE(out, 22, 0); // checksum placeholder
        out[26] = segs.len as u8;

        let idx: usize = 0;
        while (idx < segs.len) : (idx += 1) {
            out[27 + idx] = segs[idx];
        }
        if (payload.len > 0) {
            oggCopyPayloadSimd(out[27 + segs.len .. page_len], payload);
        }
        const crc = oggCrc32(out);
        writeU32LE(out, 22, crc);
    }

    /// Encodes a logical packet (0 bytes, 255-byte boundary, or multi-page >65KB) into RFC 3533 Ogg pages.
    pub fn writePacket(
        *_,
        serial_number: u32,
        page_sequence: *u32,
        granule_position: u64,
        is_bos: bool,
        is_eos: bool,
        packet: []const u8,
    ): ! {
        if (packet.len == 0) {
            let flags: u8 = 0;
            if (is_bos) flags |= OGG_FLAG_BOS;
            if (is_eos) flags |= OGG_FLAG_EOS;
            const segs: [1]u8 = [0];
            try _.emitPage(flags, granule_position, serial_number, page_sequence.*, &segs, "");
            page_sequence.* += 1;
            return;
        }

        let offset: usize = 0;
        let is_cont = false;
        while (true) {
            const remaining = packet.len - offset;
            let segs: [255]u8 = undefined;
            let seg_count: usize = 0;
            let chunk_len: usize = 0;

            while (seg_count < 255 && offset + chunk_len < packet.len) {
                const rem = packet.len - (offset + chunk_len);
                if (rem >= 255) {
                    segs[seg_count] = 255;
                    chunk_len += 255;
                    seg_count += 1;
                } else {
                    segs[seg_count] = rem as u8;
                    chunk_len += rem;
                    seg_count += 1;
                    break;
                }
            }

            const at_end = (offset + chunk_len == packet.len);
            const need_trailing_zero = at_end && (seg_count < 255) && (chunk_len > 0) && ((packet.len % 255) == 0);
            if (need_trailing_zero) {
                segs[seg_count] = 0;
                seg_count += 1;
            }

            const is_final_page = at_end && (!((packet.len % 255) == 0) || need_trailing_zero);
            let flags: u8 = 0;
            if (is_cont) flags |= OGG_FLAG_CONTINUATION;
            if (!is_cont && is_bos) flags |= OGG_FLAG_BOS;
            if (is_final_page && is_eos) flags |= OGG_FLAG_EOS;

            const gpos = if (is_final_page) granule_position else OGG_GRANULE_UNKNOWN;
            try _.emitPage(
                flags,
                gpos,
                serial_number,
                page_sequence.*,
                segs[0..seg_count],
                packet[offset .. offset + chunk_len],
            );
            page_sequence.* += 1;
            offset += chunk_len;
            is_cont = true;

            if (is_final_page) break;
            // Edge case: exact 255*255 bytes filled all 255 segments -> emit 1 final page with [0] segment
            if (offset == packet.len && !need_trailing_zero) {
                let end_flags: u8 = OGG_FLAG_CONTINUATION;
                if (is_eos) end_flags |= OGG_FLAG_EOS;
                const z_seg: [1]u8 = [0];
                try _.emitPage(end_flags, granule_position, serial_number, page_sequence.*, &z_seg, "");
                page_sequence.* += 1;
                break;
            }
            if (remaining == 0) break;
        }
    }
}
