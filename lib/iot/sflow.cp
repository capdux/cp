// Origin: Pure-CP Stdlib (Zero-Alloc sFlow v5 Network Telemetry Parser) | Created: 2026-11-22

/// sFlow 默认 UDP 端口 (6343)
pub const SFLOW_UDP_PORT: u16 = 6343;

/// sFlow 协议版本号 (v5)
pub const SFLOW_V5: u32 = 5;

/// Agent IP 地址类型：IPv4 (1) / IPv6 (2)
pub const ADDR_TYPE_IPV4: u32 = 1;
pub const ADDR_TYPE_IPV6: u32 = 2;

/// IPv4 / IPv6 Agent 报头固定字节长度
pub const SFLOW_HDR_V4_SIZE: usize = 28;
pub const SFLOW_HDR_V6_SIZE: usize = 40;

// ── Sample Format 常量 (enterprise == 0) ──
pub const SAMPLE_FLOW: u32 = 1;
pub const SAMPLE_COUNTERS: u32 = 2;
pub const SAMPLE_EXPANDED_FLOW: u32 = 3;
pub const SAMPLE_EXPANDED_COUNTERS: u32 = 4;

// ── Flow Record Format 常量 (enterprise == 0) ──
pub const REC_RAW_PACKET_HEADER: u32 = 1;
pub const REC_ETHERNET_FRAME: u32 = 2;
pub const REC_IPV4: u32 = 3;
pub const REC_IPV6: u32 = 4;
pub const REC_EXTENDED_SWITCH: u32 = 1001;
pub const REC_EXTENDED_ROUTER: u32 = 1002;

// ── Counter Record Format 常量 (enterprise == 0) ──
pub const CTR_GENERIC_IF: u32 = 1;
pub const CTR_ETHERNET_IF: u32 = 2;

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

/// sFlow v5 数据报头
pub struct SflowHeader {
    version: u32,
    addr_type: u32,
    /// Agent IP（IPv4 存于前 4 字节，其余为 0；IPv6 占满 16 字节）
    agent_ip: [16]u8,
    sub_agent_id: u32,
    seq_num: u32,
    uptime_ms: u32,
    num_samples: u32,

    fn isIpv4(): bool {
        return _.addr_type == ADDR_TYPE_IPV4;
    }

    fn isIpv6(): bool {
        return _.addr_type == ADDR_TYPE_IPV6;
    }
}

/// sFlow v5 单条 Sample 零拷贝视图
pub struct SflowSample {
    enterprise: u32,
    format: u32,
    data: []const u8,
}

/// sFlow v5 Sample 列表零分配迭代器
pub struct SflowSampleIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    had_error: bool,

    static fn init(samples_bytes: []const u8, num_samples: u32): SflowSampleIter {
        return SflowSampleIter {
            data: samples_bytes,
            pos: 0,
            remaining: num_samples,
            had_error: false,
        };
    }

    fn next(*_): SflowSample? {
        if (_.remaining == 0) {
            return null;
        }
        if (_.data.len - _.pos < 8) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const raw_fmt: u32 = readBeU32(_.data, _.pos);
        const len_u32: u32 = readBeU32(_.data, _.pos + 4);
        const val_start: usize = _.pos + 8;
        const val_len: usize = len_u32 as usize;
        if (_.data.len - val_start < val_len) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + val_len];
        _.pos = val_start + val_len;
        _.remaining -= 1;
        return SflowSample {
            enterprise: (raw_fmt >> 12) & 0xFFFFF,
            format: raw_fmt & 0xFFF,
            data: val,
        };
    }
}

/// sFlow v5 完整数据报零拷贝视图
pub struct SflowDatagram {
    header: SflowHeader,
    samples_bytes: []const u8,

    fn samples(): SflowSampleIter {
        return SflowSampleIter.init(_.samples_bytes, _.header.num_samples);
    }
}

/// sFlow v5 单条 Flow / Counter Record 零拷贝视图
pub struct SflowRecord {
    enterprise: u32,
    format: u32,
    data: []const u8,
}

/// sFlow v5 Record 列表零分配迭代器
pub struct SflowRecordIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    had_error: bool,

    static fn init(records_bytes: []const u8, num_records: u32): SflowRecordIter {
        return SflowRecordIter {
            data: records_bytes,
            pos: 0,
            remaining: num_records,
            had_error: false,
        };
    }

    fn next(*_): SflowRecord? {
        if (_.remaining == 0) {
            return null;
        }
        if (_.data.len - _.pos < 8) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const raw_fmt: u32 = readBeU32(_.data, _.pos);
        const len_u32: u32 = readBeU32(_.data, _.pos + 4);
        const val_start: usize = _.pos + 8;
        const val_len: usize = len_u32 as usize;
        if (_.data.len - val_start < val_len) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + val_len];
        _.pos = val_start + val_len;
        _.remaining -= 1;
        return SflowRecord {
            enterprise: (raw_fmt >> 12) & 0xFFFFF,
            format: raw_fmt & 0xFFF,
            data: val,
        };
    }
}

/// sFlow v5 Flow Sample 解析结果（统一承载标准 format=1 与 Expanded format=3）
pub struct SflowFlowSample {
    seq_num: u32,
    source_id_type: u32,
    source_id_index: u32,
    sampling_rate: u32,
    sample_pool: u32,
    drops: u32,
    input_if: u32,
    output_if: u32,
    num_records: u32,
    records_bytes: []const u8,

    fn records(): SflowRecordIter {
        return SflowRecordIter.init(_.records_bytes, _.num_records);
    }
}

/// sFlow v5 Raw Packet Header Record (enterprise=0, format=1)
pub struct SflowRawPacketHeader {
    protocol: u32,
    frame_len: u32,
    stripped: u32,
    orig_len: u32,
    header_bytes: []const u8,
}

/// sFlow v5 Extended Switch Data Record (enterprise=0, format=1001)
pub struct SflowExtendedSwitch {
    src_vlan: u32,
    src_priority: u32,
    dst_vlan: u32,
    dst_priority: u32,
}

/// 快速嗅探字节流是否满足 sFlow v5 数据报头约束
pub fn isSflow(data: []const u8): bool {
    if (data.len < SFLOW_HDR_V4_SIZE) {
        return false;
    }
    if (readBeU32(data, 0) != SFLOW_V5) {
        return false;
    }
    const addr_type: u32 = readBeU32(data, 4);
    if (addr_type == ADDR_TYPE_IPV4) {
        return data.len >= SFLOW_HDR_V4_SIZE;
    }
    if (addr_type == ADDR_TYPE_IPV6) {
        return data.len >= SFLOW_HDR_V6_SIZE;
    }
    return false;
}

/// 零拷贝解析 sFlow v5 数据报头与 Sample 切片
pub fn parseSflow(data: []const u8): SflowDatagram? {
    if (!isSflow(data)) {
        return null;
    }
    const version: u32 = readBeU32(data, 0);
    const addr_type: u32 = readBeU32(data, 4);
    let agent_ip: [16]u8 = [0; 16];
    let ip_len: usize = 4;
    if (addr_type == ADDR_TYPE_IPV6) {
        ip_len = 16;
    }
    let i: usize = 0;
    while (i < ip_len) {
        agent_ip[i] = data[8 + i];
        i += 1;
    }
    const meta_off: usize = ip_len + 8;
    const hdr_len: usize = meta_off + 16;

    return SflowDatagram {
        header: SflowHeader {
            version: version,
            addr_type: addr_type,
            agent_ip: agent_ip,
            sub_agent_id: readBeU32(data, meta_off),
            seq_num: readBeU32(data, meta_off + 4),
            uptime_ms: readBeU32(data, meta_off + 8),
            num_samples: readBeU32(data, meta_off + 12),
        },
        samples_bytes: data[hdr_len..data.len],
    };
}

/// 解析 Flow Sample（支持标准 format=1 与 Expanded format=3）
pub fn parseFlowSample(sample: SflowSample): SflowFlowSample? {
    if (sample.enterprise != 0) {
        return null;
    }
    const d = sample.data;
    if (sample.format == SAMPLE_FLOW) {
        if (d.len < 32) {
            return null;
        }
        const src_id: u32 = readBeU32(d, 4);
        return SflowFlowSample {
            seq_num: readBeU32(d, 0),
            source_id_type: (src_id >> 24) & 0xFF,
            source_id_index: src_id & 0x00FFFFFF,
            sampling_rate: readBeU32(d, 8),
            sample_pool: readBeU32(d, 12),
            drops: readBeU32(d, 16),
            input_if: readBeU32(d, 20),
            output_if: readBeU32(d, 24),
            num_records: readBeU32(d, 28),
            records_bytes: d[32..d.len],
        };
    }
    if (sample.format == SAMPLE_EXPANDED_FLOW) {
        if (d.len < 44) {
            return null;
        }
        return SflowFlowSample {
            seq_num: readBeU32(d, 0),
            source_id_type: readBeU32(d, 4),
            source_id_index: readBeU32(d, 8),
            sampling_rate: readBeU32(d, 12),
            sample_pool: readBeU32(d, 16),
            drops: readBeU32(d, 20),
            input_if: readBeU32(d, 28),
            output_if: readBeU32(d, 36),
            num_records: readBeU32(d, 40),
            records_bytes: d[44..d.len],
        };
    }
    return null;
}

/// 解析 Raw Packet Header Record (enterprise=0, format=1)
pub fn parseRawPacketHeader(rec: SflowRecord): SflowRawPacketHeader? {
    if (rec.enterprise != 0 || rec.format != REC_RAW_PACKET_HEADER || rec.data.len < 16) {
        return null;
    }
    const orig_len: u32 = readBeU32(rec.data, 12);
    const hdr_len: usize = orig_len as usize;
    if (rec.data.len - 16 < hdr_len) {
        return null;
    }
    return SflowRawPacketHeader {
        protocol: readBeU32(rec.data, 0),
        frame_len: readBeU32(rec.data, 4),
        stripped: readBeU32(rec.data, 8),
        orig_len: orig_len,
        header_bytes: rec.data[16..16 + hdr_len],
    };
}

/// 解析 Extended Switch Data Record (enterprise=0, format=1001)
pub fn parseExtendedSwitch(rec: SflowRecord): SflowExtendedSwitch? {
    if (rec.enterprise != 0 || rec.format != REC_EXTENDED_SWITCH || rec.data.len < 16) {
        return null;
    }
    return SflowExtendedSwitch {
        src_vlan: readBeU32(rec.data, 0),
        src_priority: readBeU32(rec.data, 4),
        dst_vlan: readBeU32(rec.data, 8),
        dst_priority: readBeU32(rec.data, 12),
    };
}
