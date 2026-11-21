// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IEEE 1588-2008/2019 PTPv2/v2.1 Codec)

/// IEEE 1588 Layer-2 PTP EtherType (0x88F7)
pub const PTP_ETHERTYPE: u16 = 0x88F7;

/// PTP over UDP Event Message 端口 (319)
pub const PTP_UDP_EVENT_PORT: u16 = 319;

/// PTP over UDP General Message 端口 (320)
pub const PTP_UDP_GENERAL_PORT: u16 = 320;

/// PTP 协议主版本号：PTPv2 / PTPv2.1 (2)
pub const PTP_V2: u8 = 2;

/// PTPv2 公共报头固定长度（34 字节）
pub const PTP_HEADER_SIZE: usize = 34;

/// PTPv2 时间戳固定长度（6B 48-bit 秒 + 4B 纳秒 = 10 字节）
pub const PTP_TIMESTAMP_SIZE: usize = 10;

/// PTPv2 Announce 报文体最小长度（30 字节）
pub const PTP_ANNOUNCE_BODY_SIZE: usize = 30;

/// PTPv2 Delay_Resp 报文体最小长度（20 字节）
pub const PTP_DELAY_RESP_BODY_SIZE: usize = 20;

// ── Message Type 常量（0x0..0x3 Event; 0x8..0xD General）──
pub const MSG_SYNC: u8 = 0x0;
pub const MSG_DELAY_REQ: u8 = 0x1;
pub const MSG_PDELAY_REQ: u8 = 0x2;
pub const MSG_PDELAY_RESP: u8 = 0x3;
pub const MSG_FOLLOW_UP: u8 = 0x8;
pub const MSG_DELAY_RESP: u8 = 0x9;
pub const MSG_PDELAY_RESP_FOLLOW_UP: u8 = 0xA;
pub const MSG_ANNOUNCE: u8 = 0xB;
pub const MSG_SIGNALING: u8 = 0xC;
pub const MSG_MANAGEMENT: u8 = 0xD;

// ── 16-bit Flag Field 位掩码 (IEEE 1588 §13.3.2.6) ──
pub const FLAG_ALTERNATE_MASTER: u16 = 0x0100;
pub const FLAG_TWO_STEP: u16 = 0x0200;
pub const FLAG_UNICAST: u16 = 0x0400;
pub const FLAG_LEAP61: u16 = 0x0001;
pub const FLAG_LEAP59: u16 = 0x0002;
pub const FLAG_UTC_OFFSET_VALID: u16 = 0x0004;
pub const FLAG_PTP_TIMESCALE: u16 = 0x0008;
pub const FLAG_TIME_TRACEABLE: u16 = 0x0010;
pub const FLAG_FREQ_TRACEABLE: u16 = 0x0020;

const NS_PER_SEC: u32 = 1000000000;
const NS_PER_SEC_U128: u128 = 1000000000;

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn readBeU64(data: []const u8, off: usize): u64 {
    return ((data[off] as u64) << 56) |
        ((data[off + 1] as u64) << 48) |
        ((data[off + 2] as u64) << 40) |
        ((data[off + 3] as u64) << 32) |
        ((data[off + 4] as u64) << 24) |
        ((data[off + 5] as u64) << 16) |
        ((data[off + 6] as u64) << 8) |
        (data[off + 7] as u64);
}

fn readId8(data: []const u8, off: usize): [8]u8 {
    return [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
    ];
}

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU64(buf: *[]u8, off: usize, v: u64) {
    buf.*[off] = ((v >> 56) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 48) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 40) & 0xFF) as u8;
    buf.*[off + 3] = ((v >> 32) & 0xFF) as u8;
    buf.*[off + 4] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 5] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 6] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 7] = (v & 0xFF) as u8;
}

/// IEEE 1588 10 字节高精度时间戳（48-bit 秒 + 32-bit 纳秒）
pub struct PtpTimestamp {
    seconds: u64,
    nanoseconds: u32,

    /// 转换为总纳秒数 (u128)
    fn toNanoseconds(): u128 {
        return ((_.seconds as u128) * NS_PER_SEC_U128) + (_.nanoseconds as u128);
    }

    /// 从总纳秒数构造 48-bit 秒 + 纳秒时间戳
    static fn fromNanoseconds(ns: u128): PtpTimestamp {
        const secs: u64 = ((ns / NS_PER_SEC_U128) & 0x0000FFFFFFFFFFFF) as u64;
        const rem_ns: u32 = (ns % NS_PER_SEC_U128) as u32;
        return PtpTimestamp {
            seconds: secs,
            nanoseconds: rem_ns,
        };
    }
}

/// IEEE 1588 10 字节端口标识（8B ClockIdentity + 2B PortNumber）
pub struct PtpPortIdentity {
    clock_id: [8]u8,
    port_number: u16,
}

/// IEEE 1588 34 字节公共报头
pub struct PtpHeader {
    major_sdo_id: u8,
    msg_type: u8,
    minor_version: u8,
    version: u8,
    message_length: u16,
    domain_number: u8,
    flags: u16,
    /// 带符号缩放纳秒修正字段（单位 2^-16 ns）
    correction_field: i64,
    source_port_id: PtpPortIdentity,
    sequence_id: u16,
    control_field: u8,
    log_message_interval: i8,

    /// 返回整数纳秒修正值（`correction_field >> 16`）
    fn correctionNs(): i64 {
        return _.correction_field >> 16;
    }

    /// 是否为 Event 报文（`msg_type < 0x8`）
    fn isEvent(): bool {
        return (_.msg_type & 0x08) == 0;
    }

    /// 是否为两步时钟 (Two-Step)
    fn isTwoStep(): bool {
        return (_.flags & FLAG_TWO_STEP) != 0;
    }

    /// 是否为单播模式 (Unicast)
    fn isUnicast(): bool {
        return (_.flags & FLAG_UNICAST) != 0;
    }
}

/// IEEE 1588 完整报文零拷贝视图
pub struct PtpPacket {
    header: PtpHeader,
    body: []const u8,
}

/// IEEE 1588 Announce 报文体 (30 字节)
pub struct PtpAnnounceBody {
    origin_ts: PtpTimestamp,
    current_utc_offset: i16,
    gm_priority1: u8,
    gm_clock_class: u8,
    gm_clock_accuracy: u8,
    gm_offset_scaled_log_variance: u16,
    gm_priority2: u8,
    gm_identity: [8]u8,
    steps_removed: u16,
    time_source: u8,
}

/// IEEE 1588 Delay_Resp 报文体 (20 字节)
pub struct PtpDelayRespBody {
    receive_ts: PtpTimestamp,
    requesting_port_id: PtpPortIdentity,
}

/// 解析 10 字节 PTP 时间戳（严格校验 `nanoseconds < 1_000_000_000`）
pub fn parsePtpTimestamp(data: []const u8): PtpTimestamp? {
    if (data.len < PTP_TIMESTAMP_SIZE) {
        return null;
    }
    const secs: u64 = ((data[0] as u64) << 40) |
        ((data[1] as u64) << 32) |
        ((data[2] as u64) << 24) |
        ((data[3] as u64) << 16) |
        ((data[4] as u64) << 8) |
        (data[5] as u64);
    const nanos: u32 = readBeU32(data, 6);
    if (nanos >= NS_PER_SEC) {
        return null;
    }
    return PtpTimestamp {
        seconds: secs,
        nanoseconds: nanos,
    };
}

/// 快速嗅探字节流是否满足 PTPv2/v2.1 最小头部与长度约束
pub fn isPtp(data: []const u8): bool {
    if (data.len < PTP_HEADER_SIZE) {
        return false;
    }
    const version: u8 = data[1] & 0x0F;
    if (version != PTP_V2) {
        return false;
    }
    const msg_len: usize = readBeU16(data, 2) as usize;
    if (msg_len < PTP_HEADER_SIZE || msg_len > data.len) {
        return false;
    }
    return true;
}

/// 零拷贝解析 IEEE 1588 PTPv2/v2.1 公共报头与报文体
pub fn parsePtp(data: []const u8): PtpPacket? {
    if (!isPtp(data)) {
        return null;
    }
    const msg_len: u16 = readBeU16(data, 2);
    const msg_len_usize: usize = msg_len as usize;

    const corr_u64: u64 = readBeU64(data, 8);
    const hdr = PtpHeader {
        major_sdo_id: (data[0] >> 4) & 0x0F,
        msg_type: data[0] & 0x0F,
        minor_version: (data[1] >> 4) & 0x0F,
        version: data[1] & 0x0F,
        message_length: msg_len,
        domain_number: data[4],
        flags: readBeU16(data, 6),
        correction_field: corr_u64 as i64,
        source_port_id: PtpPortIdentity {
            clock_id: readId8(data, 20),
            port_number: readBeU16(data, 28),
        },
        sequence_id: readBeU16(data, 30),
        control_field: data[32],
        log_message_interval: data[33] as i8,
    };

    return PtpPacket {
        header: hdr,
        body: data[PTP_HEADER_SIZE..msg_len_usize],
    };
}

/// 解析 Announce 报文体（要求长度 >= 30 且 origin_ts 纳秒字段合法）
pub fn parsePtpAnnounce(body: []const u8): PtpAnnounceBody? {
    if (body.len < PTP_ANNOUNCE_BODY_SIZE) {
        return null;
    }
    const ts = parsePtpTimestamp(body[0..10]) ?? return null;
    const utc_u16: u16 = readBeU16(body, 10);
    return PtpAnnounceBody {
        origin_ts: ts,
        current_utc_offset: utc_u16 as i16,
        gm_priority1: body[13],
        gm_clock_class: body[14],
        gm_clock_accuracy: body[15],
        gm_offset_scaled_log_variance: readBeU16(body, 16),
        gm_priority2: body[18],
        gm_identity: readId8(body, 19),
        steps_removed: readBeU16(body, 27),
        time_source: body[29],
    };
}

/// 解析 Delay_Resp 报文体（要求长度 >= 20 且 receive_ts 纳秒字段合法）
pub fn parsePtpDelayResp(body: []const u8): PtpDelayRespBody? {
    if (body.len < PTP_DELAY_RESP_BODY_SIZE) {
        return null;
    }
    const ts = parsePtpTimestamp(body[0..10]) ?? return null;
    return PtpDelayRespBody {
        receive_ts: ts,
        requesting_port_id: PtpPortIdentity {
            clock_id: readId8(body, 10),
            port_number: readBeU16(body, 18),
        },
    };
}

/// 零分配编码 34 字节 PTPv2/v2.1 公共报头；目标缓冲不足 34B 时返回 `null`
pub fn encodePtpHeader(hdr: PtpHeader, out: *[]u8): usize? {
    if (out.*.len < PTP_HEADER_SIZE) {
        return null;
    }
    out.*[0] = ((hdr.major_sdo_id & 0x0F) << 4) | (hdr.msg_type & 0x0F);
    out.*[1] = ((hdr.minor_version & 0x0F) << 4) | (hdr.version & 0x0F);
    writeBeU16(out, 2, hdr.message_length);
    out.*[4] = hdr.domain_number;
    out.*[5] = 0; // minorSdoId / reserved
    writeBeU16(out, 6, hdr.flags);
    writeBeU64(out, 8, hdr.correction_field as u64);
    out.*[16] = 0; out.*[17] = 0; out.*[18] = 0; out.*[19] = 0;
    let i: usize = 0;
    while (i < 8) {
        out.*[20 + i] = hdr.source_port_id.clock_id[i];
        i += 1;
    }
    writeBeU16(out, 28, hdr.source_port_id.port_number);
    writeBeU16(out, 30, hdr.sequence_id);
    out.*[32] = hdr.control_field;
    out.*[33] = hdr.log_message_interval as u8;
    return PTP_HEADER_SIZE;
}
