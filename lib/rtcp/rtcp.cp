// Origin: Pure-CP Stdlib (Zero-Alloc RFC 3550 RTCP Compound & Packet Parser) | Created: 2026-11-22

/// RTCP 协议版本号 (RFC 3550 = 2)
pub const RTCP_VERSION: u8 = 2;

/// RTCP 公共报头长度（4 字节）
pub const RTCP_HEADER_SIZE: usize = 4;

/// RTCP Reception Report Block 固定长度（24 字节）
pub const RTCP_REPORT_BLOCK_SIZE: usize = 24;

/// RTCP SR Sender Info 固定长度（20 字节，不含 4B SSRC）
pub const RTCP_SR_SENDER_INFO_SIZE: usize = 20;

// ── RTCP Payload Type 常量 (RFC 3550) ──
pub const PT_SR: u8 = 200;
pub const PT_RR: u8 = 201;
pub const PT_SDES: u8 = 202;
pub const PT_BYE: u8 = 203;
pub const PT_APP: u8 = 204;

// ── SDES Item Type 常量 (RFC 3550 §6.5) ──
pub const SDES_END: u8 = 0;
pub const SDES_CNAME: u8 = 1;
pub const SDES_NAME: u8 = 2;
pub const SDES_EMAIL: u8 = 3;
pub const SDES_PHONE: u8 = 4;
pub const SDES_LOC: u8 = 5;
pub const SDES_TOOL: u8 = 6;
pub const SDES_NOTE: u8 = 7;
pub const SDES_PRIV: u8 = 8;

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

/// 解码 24-bit 大端有符号整数（二补码符号扩展至 i32）
fn readBeI24(data: []const u8, off: usize): i32 {
    let raw: u32 = ((data[off] as u32) << 16) |
        ((data[off + 1] as u32) << 8) |
        (data[off + 2] as u32);
    if ((data[off] & 0x80) != 0) {
        raw = raw | 0xFF000000;
    }
    return raw as i32;
}

/// RTCP 4 字节公共报头
pub struct RtcpHeader {
    version: u8,
    padding: bool,
    count: u8,
    pt: u8,
    length_words: u16,

    /// 返回本 RTCP 子包在报文流中占据的总字节数 `(length_words + 1) * 4`
    fn byteLen(): usize {
        return ((_.length_words as usize) + 1) * 4;
    }
}

/// 单个 RTCP 子包零拷贝视图（`body` 已剥离尾部 Padding）
pub struct RtcpPacket {
    header: RtcpHeader,
    body: []const u8,
    total_bytes: usize,
}

/// RTCP 复合包（Compound Packet）零分配迭代器
pub struct RtcpCompoundIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(data: []const u8): RtcpCompoundIter {
        return RtcpCompoundIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): RtcpPacket? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const pkt_opt = parseRtcpPacket(_.data[_.pos.._.data.len]);
        if (pkt_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const pkt = pkt_opt ?? unreachable;
        _.pos += pkt.total_bytes;
        return pkt;
    }
}

/// RTCP 24 字节 Reception Report Block
pub struct RtcpReportBlock {
    ssrc: u32,
    fraction_lost: u8,
    cumulative_lost: i32,
    highest_seq: u32,
    jitter: u32,
    lsr: u32,
    dlsr: u32,
}

/// RTCP Reception Report Block 零分配迭代器
pub struct RtcpReportBlockIter {
    data: []const u8,
    pos: usize,

    static fn init(reports_bytes: []const u8): RtcpReportBlockIter {
        return RtcpReportBlockIter {
            data: reports_bytes,
            pos: 0,
        };
    }

    fn next(*_): RtcpReportBlock? {
        if (_.pos + RTCP_REPORT_BLOCK_SIZE > _.data.len) {
            return null;
        }
        const off: usize = _.pos;
        _.pos += RTCP_REPORT_BLOCK_SIZE;
        return RtcpReportBlock {
            ssrc: readBeU32(_.data, off),
            fraction_lost: _.data[off + 4],
            cumulative_lost: readBeI24(_.data, off + 5),
            highest_seq: readBeU32(_.data, off + 8),
            jitter: readBeU32(_.data, off + 12),
            lsr: readBeU32(_.data, off + 16),
            dlsr: readBeU32(_.data, off + 20),
        };
    }
}

/// RTCP Sender Report (PT = 200) 解析结果
pub struct RtcpSr {
    ssrc: u32,
    ntp_sec: u32,
    ntp_frac: u32,
    rtp_ts: u32,
    packet_count: u32,
    octet_count: u32,
    report_count: u8,
    reports_bytes: []const u8,

    fn reports(): RtcpReportBlockIter {
        return RtcpReportBlockIter.init(_.reports_bytes);
    }
}

/// RTCP Receiver Report (PT = 201) 解析结果
pub struct RtcpRr {
    ssrc: u32,
    report_count: u8,
    reports_bytes: []const u8,

    fn reports(): RtcpReportBlockIter {
        return RtcpReportBlockIter.init(_.reports_bytes);
    }
}

/// RTCP Goodbye (PT = 203) 解析结果
pub struct RtcpBye {
    src_count: u8,
    ssrc_bytes: []const u8,
    reason: []const u8,

    /// 按下标提取离开会话的 SSRC
    fn ssrcAt(idx: usize): u32? {
        const off: usize = idx * 4;
        if (off + 4 > _.ssrc_bytes.len) {
            return null;
        }
        return readBeU32(_.ssrc_bytes, off);
    }
}

/// RTCP Application-Defined (PT = 204) 解析结果
pub struct RtcpApp {
    subtype: u8,
    ssrc: u32,
    name: [4]u8,
    data: []const u8,
}

/// RTCP SDES 单条 Item（CNAME / NAME / TOOL 等）
pub struct RtcpSdesItem {
    item_type: u8,
    value: []const u8,
}

/// RTCP SDES Chunk 内的 Item 零分配迭代器
pub struct RtcpSdesItemIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(items_bytes: []const u8): RtcpSdesItemIter {
        return RtcpSdesItemIter {
            data: items_bytes,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): RtcpSdesItem? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const itype: u8 = _.data[_.pos];
        if (itype == SDES_END) {
            _.pos = _.data.len;
            return null;
        }
        if (_.data.len - _.pos < 2) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const ilen: usize = _.data[_.pos + 1] as usize;
        const val_start: usize = _.pos + 2;
        if (_.data.len - val_start < ilen) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + ilen];
        _.pos = val_start + ilen;
        return RtcpSdesItem {
            item_type: itype,
            value: val,
        };
    }
}

/// RTCP SDES Chunk（SSRC + Items 切片）
pub struct RtcpSdesChunk {
    ssrc: u32,
    items_bytes: []const u8,

    fn items(): RtcpSdesItemIter {
        return RtcpSdesItemIter.init(_.items_bytes);
    }
}

/// 快速嗅探字节流是否以合法 RTCP v2 包开头（校验版本 2、PT 范围 200..204 及长度边界）
pub fn isRtcp(data: []const u8): bool {
    if (data.len < RTCP_HEADER_SIZE) {
        return false;
    }
    const version: u8 = (data[0] >> 6) & 0x03;
    if (version != RTCP_VERSION) {
        return false;
    }
    const pt: u8 = data[1];
    if (pt < PT_SR || pt > PT_APP) {
        return false;
    }
    const byte_len: usize = ((readBeU16(data, 2) as usize) + 1) * 4;
    if (byte_len > data.len) {
        return false;
    }
    return true;
}

/// 零拷贝解析单个 RTCP 子包（严格校验 version == 2、总长度边界及 Padding 尾字节）
pub fn parseRtcpPacket(data: []const u8): RtcpPacket? {
    if (data.len < RTCP_HEADER_SIZE) {
        return null;
    }
    const b0: u8 = data[0];
    const version: u8 = (b0 >> 6) & 0x03;
    if (version != RTCP_VERSION) {
        return null;
    }
    const padding: bool = (b0 & 0x20) != 0;
    const count: u8 = b0 & 0x1F;
    const pt: u8 = data[1];
    const length_words: u16 = readBeU16(data, 2);
    const byte_len: usize = ((length_words as usize) + 1) * 4;
    if (byte_len > data.len) {
        return null;
    }

    let body_end: usize = byte_len;
    if (padding) {
        if (byte_len <= RTCP_HEADER_SIZE) {
            return null;
        }
        const pad_len: usize = data[byte_len - 1] as usize;
        if (pad_len == 0 || pad_len > byte_len - RTCP_HEADER_SIZE) {
            return null;
        }
        body_end = byte_len - pad_len;
    }

    return RtcpPacket {
        header: RtcpHeader {
            version: version,
            padding: padding,
            count: count,
            pt: pt,
            length_words: length_words,
        },
        body: data[RTCP_HEADER_SIZE..body_end],
        total_bytes: byte_len,
    };
}

/// 返回 RTCP 复合包零分配迭代器
pub fn iterRtcpCompound(data: []const u8): RtcpCompoundIter {
    return RtcpCompoundIter.init(data);
}

/// 解析 RTCP Sender Report (PT = 200)
pub fn parseRtcpSr(pkt: RtcpPacket): RtcpSr? {
    if (pkt.header.pt != PT_SR) {
        return null;
    }
    const rc: usize = pkt.header.count as usize;
    const min_len: usize = RTCP_SR_SENDER_INFO_SIZE + 4 + (rc * RTCP_REPORT_BLOCK_SIZE);
    if (pkt.body.len < min_len) {
        return null;
    }
    const b = pkt.body;
    return RtcpSr {
        ssrc: readBeU32(b, 0),
        ntp_sec: readBeU32(b, 4),
        ntp_frac: readBeU32(b, 8),
        rtp_ts: readBeU32(b, 12),
        packet_count: readBeU32(b, 16),
        octet_count: readBeU32(b, 20),
        report_count: pkt.header.count,
        reports_bytes: b[24..24 + (rc * RTCP_REPORT_BLOCK_SIZE)],
    };
}

/// 解析 RTCP Receiver Report (PT = 201)
pub fn parseRtcpRr(pkt: RtcpPacket): RtcpRr? {
    if (pkt.header.pt != PT_RR) {
        return null;
    }
    const rc: usize = pkt.header.count as usize;
    const min_len: usize = (rc * RTCP_REPORT_BLOCK_SIZE) + 4;
    if (pkt.body.len < min_len) {
        return null;
    }
    const b = pkt.body;
    return RtcpRr {
        ssrc: readBeU32(b, 0),
        report_count: pkt.header.count,
        reports_bytes: b[4..4 + (rc * RTCP_REPORT_BLOCK_SIZE)],
    };
}

/// 解析 RTCP Goodbye (PT = 203)
pub fn parseRtcpBye(pkt: RtcpPacket): RtcpBye? {
    if (pkt.header.pt != PT_BYE) {
        return null;
    }
    const sc: usize = pkt.header.count as usize;
    const ssrc_len: usize = sc * 4;
    if (pkt.body.len < ssrc_len) {
        return null;
    }
    let reason: []const u8 = pkt.body[0..0];
    if (pkt.body.len > ssrc_len) {
        const rlen: usize = pkt.body[ssrc_len] as usize;
        const rstart: usize = ssrc_len + 1;
        if (pkt.body.len - rstart < rlen) {
            return null;
        }
        reason = pkt.body[rstart..rstart + rlen];
    }
    return RtcpBye {
        src_count: pkt.header.count,
        ssrc_bytes: pkt.body[0..ssrc_len],
        reason: reason,
    };
}

/// 解析 RTCP Application-Defined (PT = 204)
pub fn parseRtcpApp(pkt: RtcpPacket): RtcpApp? {
    if (pkt.header.pt != PT_APP || pkt.body.len < 8) {
        return null;
    }
    const b = pkt.body;
    return RtcpApp {
        subtype: pkt.header.count,
        ssrc: readBeU32(b, 0),
        name: [b[4], b[5], b[6], b[7]],
        data: b[8..b.len],
    };
}
