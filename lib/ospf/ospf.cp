// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2328 OSPFv2 Routing Protocol Parser)

/// IPv4 协议号：OSPF (RFC 2328)
pub const OSPF_IP_PROTO: u8 = 89;

/// OSPFv2 协议版本号
pub const OSPF_V2: u8 = 2;

/// OSPFv2 公共报头固定长度（24 字节）
pub const OSPF_HEADER_SIZE: usize = 24;

/// OSPFv2 LSA 公共头固定长度（20 字节）
pub const OSPF_LSA_HEADER_SIZE: usize = 20;

/// OSPFv2 Hello 报文体最小固定长度（不含邻居列表，20 字节）
pub const OSPF_HELLO_MIN_SIZE: usize = 20;

// ── 认证类型常量 (RFC 2328 §D) ──
pub const AUTH_NONE: u16 = 0;
pub const AUTH_SIMPLE_PW: u16 = 1;
pub const AUTH_CRYPTO: u16 = 2;

/// OSPFv2 报文类型枚举
pub enum OspfPacketType: u8 {
    unknown = 0,
    hello = 1,
    dbd = 2,
    lsr = 3,
    lsu = 4,
    lsack = 5,
}

/// OSPFv2 LSA 类型枚举 (RFC 2328 / RFC 5250)
pub enum OspfLsaType: u8 {
    unknown = 0,
    router = 1,
    network = 2,
    summary_net = 3,
    summary_asbr = 4,
    as_external = 5,
    opaque_link = 9,
    opaque_area = 10,
    opaque_as = 11,
}

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn readIp4(data: []const u8, off: usize): [4]u8 {
    return [data[off], data[off + 1], data[off + 2], data[off + 3]];
}

fn mapPacketType(raw: u8): OspfPacketType {
    if (raw == 1) { return .hello; }
    if (raw == 2) { return .dbd; }
    if (raw == 3) { return .lsr; }
    if (raw == 4) { return .lsu; }
    if (raw == 5) { return .lsack; }
    return .unknown;
}

fn mapLsaType(raw: u8): OspfLsaType {
    if (raw == 1) { return .router; }
    if (raw == 2) { return .network; }
    if (raw == 3) { return .summary_net; }
    if (raw == 4) { return .summary_asbr; }
    if (raw == 5) { return .as_external; }
    if (raw == 9) { return .opaque_link; }
    if (raw == 10) { return .opaque_area; }
    if (raw == 11) { return .opaque_as; }
    return .unknown;
}

fn foldChecksum(sum_in: u32): u16 {
    let acc: u32 = sum_in;
    while ((acc >> 16) != 0) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return (acc & 0xFFFF) as u16;
}

/// 内部：按 RFC 2328 规则计算 OSPFv2 一补码累加和（强制跳过偏移 16..24 认证区，可选跳过偏移 12..14 校验和区）
fn ospf2ChecksumAcc(packet: []const u8, zero_checksum: bool): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i + 1 < packet.len) {
        // 跳过 12..14 校验和字段（计算模式）与 16..24 8 字节认证字段（始终跳过）
        if ((zero_checksum && i == 12) || (i >= 16 && i < 24)) {
            i += 2;
            continue;
        }
        const word: u32 = ((packet[i] as u32) << 8) | (packet[i + 1] as u32);
        acc += word;
        if ((acc & 0x80000000) != 0) {
            acc = (acc & 0xFFFF) + (acc >> 16);
        }
        i += 2;
    }
    if (i < packet.len) {
        if (!(i >= 16 && i < 24)) {
            acc += (packet[i] as u32) << 8;
        }
    }
    return acc;
}

/// 计算 OSPFv2 报文的 RFC 1071 校验和（跳过偏移 12..14 校验和字段与偏移 16..24 8 字节认证字段）
pub fn ospf2Checksum(packet: []const u8): u16 {
    if (packet.len < OSPF_HEADER_SIZE) {
        return 0;
    }
    const acc: u32 = ospf2ChecksumAcc(packet, true);
    return ~foldChecksum(acc);
}

/// 验证 OSPFv2 报文校验和（跳过偏移 16..24 8 字节认证字段；校验长度 >= 24）
pub fn verifyOspf2Checksum(packet: []const u8): bool {
    if (packet.len < OSPF_HEADER_SIZE) {
        return false;
    }
    const pkt_len: usize = readBeU16(packet, 2) as usize;
    if (pkt_len < OSPF_HEADER_SIZE || pkt_len > packet.len) {
        return false;
    }
    const acc: u32 = ospf2ChecksumAcc(packet[0..pkt_len], false);
    return foldChecksum(acc) == 0xFFFF;
}

/// OSPFv2 24 字节公共报头
pub struct OspfHeader {
    version: u8,
    pkt_type: OspfPacketType,
    raw_type: u8,
    packet_len: u16,
    router_id: [4]u8,
    area_id: [4]u8,
    checksum: u16,
    auth_type: u16,
    auth_data: [8]u8,
}

/// OSPFv2 完整报文零拷贝视图
pub struct OspfPacket {
    header: OspfHeader,
    body: []const u8,
}

/// 快速嗅探字节流是否满足 OSPFv2 最小报头与长度约束
pub fn isOspf2(data: []const u8): bool {
    if (data.len < OSPF_HEADER_SIZE) {
        return false;
    }
    if (data[0] != OSPF_V2) {
        return false;
    }
    const ptype = mapPacketType(data[1]);
    if (ptype == .unknown) {
        return false;
    }
    const pkt_len: usize = readBeU16(data, 2) as usize;
    if (pkt_len < OSPF_HEADER_SIZE || pkt_len > data.len) {
        return false;
    }
    return true;
}

/// 零拷贝解析 OSPFv2 公共报头与报文体（校验 version == 2 与 24 <= packet_len <= data.len）
pub fn parseOspf2(data: []const u8): OspfPacket? {
    if (data.len < OSPF_HEADER_SIZE) {
        return null;
    }
    const version: u8 = data[0];
    if (version != OSPF_V2) {
        return null;
    }
    const raw_type: u8 = data[1];
    const pkt_type = mapPacketType(raw_type);
    const packet_len: u16 = readBeU16(data, 2);
    const pkt_len_usize: usize = packet_len as usize;
    if (pkt_len_usize < OSPF_HEADER_SIZE || pkt_len_usize > data.len) {
        return null;
    }

    const auth_data: [8]u8 = [
        data[16], data[17], data[18], data[19],
        data[20], data[21], data[22], data[23],
    ];

    return OspfPacket {
        header: OspfHeader {
            version: version,
            pkt_type: pkt_type,
            raw_type: raw_type,
            packet_len: packet_len,
            router_id: readIp4(data, 4),
            area_id: readIp4(data, 8),
            checksum: readBeU16(data, 12),
            auth_type: readBeU16(data, 14),
            auth_data: auth_data,
        },
        body: data[OSPF_HEADER_SIZE..pkt_len_usize],
    };
}

/// OSPFv2 Hello 报文体解析结果（RFC 2328 §A.3.2）
pub struct OspfHello {
    network_mask: [4]u8,
    hello_interval: u16,
    options: u8,
    router_priority: u8,
    dead_interval: u32,
    designated_router: [4]u8,
    backup_dr: [4]u8,
    neighbors_slice: []const u8,

    /// 返回邻居 Router ID 数量
    fn neighborCount(): usize {
        return _.neighbors_slice.len / 4;
    }

    /// 按下标零分配提取第 `idx` 个邻居 Router ID
    fn neighborAt(idx: usize): ([4]u8)? {
        const off: usize = idx * 4;
        if (off + 4 > _.neighbors_slice.len) {
            return null;
        }
        return readIp4(_.neighbors_slice, off);
    }
}

/// 解析 OSPFv2 Hello 报文体（要求长度 >= 20 且邻居列表为 4 字节倍数）
pub fn parseOspfHello(body: []const u8): OspfHello? {
    if (body.len < OSPF_HELLO_MIN_SIZE) {
        return null;
    }
    const nbr_bytes: usize = body.len - OSPF_HELLO_MIN_SIZE;
    if ((nbr_bytes % 4) != 0) {
        return null;
    }

    return OspfHello {
        network_mask: readIp4(body, 0),
        hello_interval: readBeU16(body, 4),
        options: body[6],
        router_priority: body[7],
        dead_interval: readBeU32(body, 8),
        designated_router: readIp4(body, 12),
        backup_dr: readIp4(body, 16),
        neighbors_slice: body[OSPF_HELLO_MIN_SIZE..body.len],
    };
}

/// OSPFv2 20 字节 LSA 公共头（RFC 2328 §A.4.1）
pub struct OspfLsaHeader {
    ls_age: u16,
    options: u8,
    ls_type: OspfLsaType,
    raw_ls_type: u8,
    link_state_id: [4]u8,
    advertising_router: [4]u8,
    ls_seq_num: u32,
    ls_checksum: u16,
    length: u16,
}

/// 解析单个 20 字节 LSA 头（严格校验 data.len >= 20 且 length >= 20）
pub fn parseOspfLsaHeader(data: []const u8): OspfLsaHeader? {
    if (data.len < OSPF_LSA_HEADER_SIZE) {
        return null;
    }
    const length: u16 = readBeU16(data, 18);
    if ((length as usize) < OSPF_LSA_HEADER_SIZE) {
        return null;
    }
    const raw_type: u8 = data[3];
    return OspfLsaHeader {
        ls_age: readBeU16(data, 0),
        options: data[2],
        ls_type: mapLsaType(raw_type),
        raw_ls_type: raw_type,
        link_state_id: readIp4(data, 4),
        advertising_router: readIp4(data, 8),
        ls_seq_num: readBeU32(data, 12),
        ls_checksum: readBeU16(data, 16),
        length: length,
    };
}

/// LSU 报文内单条完整 LSA 零拷贝视图
pub struct OspfLsuEntry {
    header: OspfLsaHeader,
    lsa_body: []const u8,
    raw_lsa: []const u8,
}

/// OSPFv2 LSU (Link State Update) 零分配迭代器
pub struct OspfLsuIter {
    data: []const u8,
    pos: usize,
    num_lsas: u32,
    remaining: u32,
    had_error: bool,

    /// 从 LSU 报文体初始化（读取前 4B BE `num_lsas`）
    static fn init(lsu_body: []const u8): OspfLsuIter? {
        if (lsu_body.len < 4) {
            return null;
        }
        const count: u32 = readBeU32(lsu_body, 0);
        return OspfLsuIter {
            data: lsu_body,
            pos: 4,
            num_lsas: count,
            remaining: count,
            had_error: false,
        };
    }

    /// 逐条零拷贝产出 `(header, lsa_body)`；按每条 LSA 的 `length >= 20` 步进，越界或畸形时置 `had_error = true`
    fn next(*_): OspfLsuEntry? {
        if (_.remaining == 0) {
            return null;
        }
        if (_.data.len - _.pos < OSPF_LSA_HEADER_SIZE) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const hdr_opt = parseOspfLsaHeader(_.data[_.pos.._.data.len]);
        if (hdr_opt == null) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }
        const hdr = hdr_opt ?? unreachable;
        const lsa_len: usize = hdr.length as usize;
        if (lsa_len < OSPF_LSA_HEADER_SIZE || _.data.len - _.pos < lsa_len) {
            _.had_error = true;
            _.remaining = 0;
            _.pos = _.data.len;
            return null;
        }

        const start: usize = _.pos;
        const end: usize = start + lsa_len;
        _.pos = end;
        _.remaining -= 1;

        return OspfLsuEntry {
            header: hdr,
            lsa_body: _.data[start + OSPF_LSA_HEADER_SIZE..end],
            raw_lsa: _.data[start..end],
        };
    }
}

/// OSPFv2 LSAck (Link State Acknowledgment) 20B LSA 头序列零分配迭代器
pub struct OspfLsackIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(lsack_body: []const u8): OspfLsackIter {
        return OspfLsackIter {
            data: lsack_body,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): OspfLsaHeader? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < OSPF_LSA_HEADER_SIZE) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const hdr_opt = parseOspfLsaHeader(_.data[_.pos.._.pos + OSPF_LSA_HEADER_SIZE]);
        if (hdr_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        _.pos += OSPF_LSA_HEADER_SIZE;
        return hdr_opt;
    }
}
