// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 792/4443 ICMPv4 & ICMPv6 Codec)

/// ICMP 固定报头长度（Type 1B + Code 1B + Checksum 2B + Rest of Header 4B）
pub const ICMP_HEADER_SIZE: usize = 8;

/// IPv4 协议号：ICMP (RFC 792)
pub const ICMP_IP_PROTO_V4: u8 = 1;

/// IPv6 Next Header 协议号：ICMPv6 (RFC 4443)
pub const ICMP_IP_PROTO_V6: u8 = 58;

// ── ICMPv4 Type 常量 (RFC 792) ──
pub const ICMP4_ECHO_REPLY: u8 = 0;
pub const ICMP4_DEST_UNREACH: u8 = 3;
pub const ICMP4_REDIRECT: u8 = 5;
pub const ICMP4_ECHO_REQUEST: u8 = 8;
pub const ICMP4_TIME_EXCEEDED: u8 = 11;
pub const ICMP4_PARAM_PROBLEM: u8 = 12;
pub const ICMP4_TIMESTAMP_REQ: u8 = 13;
pub const ICMP4_TIMESTAMP_REPLY: u8 = 14;

// ── ICMPv6 Type 常量 (RFC 4443 / RFC 4861) ──
pub const ICMP6_DEST_UNREACH: u8 = 1;
pub const ICMP6_PACKET_TOO_BIG: u8 = 2;
pub const ICMP6_TIME_EXCEEDED: u8 = 3;
pub const ICMP6_PARAM_PROBLEM: u8 = 4;
pub const ICMP6_ECHO_REQUEST: u8 = 128;
pub const ICMP6_ECHO_REPLY: u8 = 129;
pub const ICMP6_ROUTER_SOLICIT: u8 = 133;
pub const ICMP6_ROUTER_ADVERT: u8 = 134;
pub const ICMP6_NEIGHBOR_SOLICIT: u8 = 135;
pub const ICMP6_NEIGHBOR_ADVERT: u8 = 136;

/// 折叠 32 位一补码累加器至 16 位
fn foldChecksum(sum_in: u32): u16 {
    let acc: u32 = sum_in;
    while ((acc >> 16) != 0) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return (acc & 0xFFFF) as u16;
}

/// 将字节切片按 16-bit 大端字累加到 RFC 1071 一补码累加器（可选将字节 2..4 视为 0）
fn addBytesChecksum(acc_in: u32, data: []const u8, zero_checksum_field: bool): u32 {
    let acc: u32 = acc_in;
    let i: usize = 0;
    while (i + 1 < data.len) {
        if (zero_checksum_field && i == 2) {
            i += 2;
            continue;
        }
        const word: u32 = ((data[i] as u32) << 8) | (data[i + 1] as u32);
        acc += word;
        if ((acc & 0x80000000) != 0) {
            acc = (acc & 0xFFFF) + (acc >> 16);
        }
        i += 2;
    }
    if (i < data.len) {
        // 奇数字节末尾补 0
        acc += (data[i] as u32) << 8;
    }
    return acc;
}

/// 累加 40 字节 IPv6 伪首部（src_ip 16B + dst_ip 16B + upper_len 4B + zero 3B + next_header 58）
fn addIpv6PseudoHeader(src_ip: [16]u8, dst_ip: [16]u8, pkt_len: usize): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < 16) {
        acc += ((src_ip[i] as u32) << 8) | (src_ip[i + 1] as u32);
        acc += ((dst_ip[i] as u32) << 8) | (dst_ip[i + 1] as u32);
        i += 2;
    }
    const len_u32: u32 = pkt_len as u32;
    acc += (len_u32 >> 16) & 0xFFFF;
    acc += len_u32 & 0xFFFF;
    acc += ICMP_IP_PROTO_V6 as u32;
    return acc;
}

/// 计算 ICMPv4 报文的 RFC 1071 16-bit 一补码校验和（自动忽略字节 2..4 校验和槽位）
pub fn icmp4Checksum(packet: []const u8): u16 {
    const acc: u32 = addBytesChecksum(0, packet, true);
    const folded: u16 = foldChecksum(acc);
    return ~folded;
}

/// 校验 ICMPv4 完整报文的 RFC 1071 一补码校验和（需满足长度 >= 8）
pub fn verifyIcmp4Checksum(packet: []const u8): bool {
    if (packet.len < ICMP_HEADER_SIZE) {
        return false;
    }
    const acc: u32 = addBytesChecksum(0, packet, false);
    return foldChecksum(acc) == 0xFFFF;
}

/// 计算 ICMPv6 报文的 RFC 4443 / RFC 1071 校验和（含 40B IPv6 伪首部 + NextHeader=58）
pub fn icmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): u16 {
    const pseudo: u32 = addIpv6PseudoHeader(src_ip, dst_ip, packet.len);
    const acc: u32 = addBytesChecksum(pseudo, packet, true);
    const folded: u16 = foldChecksum(acc);
    return ~folded;
}

/// 校验 ICMPv6 完整报文（含 IPv6 伪首部）的一补码校验和（需满足长度 >= 8）
pub fn verifyIcmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): bool {
    if (packet.len < ICMP_HEADER_SIZE) {
        return false;
    }
    const pseudo: u32 = addIpv6PseudoHeader(src_ip, dst_ip, packet.len);
    const acc: u32 = addBytesChecksum(pseudo, packet, false);
    return foldChecksum(acc) == 0xFFFF;
}

/// ICMPv4 / ICMPv6 报文零拷贝解析视图
pub struct IcmpPacket {
    icmp_type: u8,
    code: u8,
    checksum: u16,
    rest_of_header: u32,
    payload: []const u8,

    /// Echo Request/Reply 标识符（Rest of Header 高 16 位）
    fn echoId(): u16 {
        return ((_.rest_of_header >> 16) & 0xFFFF) as u16;
    }

    /// Echo Request/Reply 序列号（Rest of Header 低 16 位）
    fn echoSeq(): u16 {
        return (_.rest_of_header & 0xFFFF) as u16;
    }

    /// ICMPv4 Destination Unreachable (Code 4: Fragmentation Needed) Next-Hop MTU（低 16 位）
    fn nextHopMtu(): u16 {
        return (_.rest_of_header & 0xFFFF) as u16;
    }

    /// ICMPv6 Packet Too Big (Type 2) 32-bit MTU
    fn packetTooBigMtu(): u32 {
        return _.rest_of_header;
    }

    /// 是否为 ICMPv4 Echo Request (Type 8, Code 0)
    fn isEchoRequest4(): bool {
        return _.icmp_type == ICMP4_ECHO_REQUEST && _.code == 0;
    }

    /// 是否为 ICMPv4 Echo Reply (Type 0, Code 0)
    fn isEchoReply4(): bool {
        return _.icmp_type == ICMP4_ECHO_REPLY && _.code == 0;
    }

    /// 是否为 ICMPv6 Echo Request (Type 128, Code 0)
    fn isEchoRequest6(): bool {
        return _.icmp_type == ICMP6_ECHO_REQUEST && _.code == 0;
    }

    /// 是否为 ICMPv6 Echo Reply (Type 129, Code 0)
    fn isEchoReply6(): bool {
        return _.icmp_type == ICMP6_ECHO_REPLY && _.code == 0;
    }
}

/// 零拷贝解析 ICMPv4 / ICMPv6 报文（校验长度 >= 8）
pub fn parseIcmp(data: []const u8): IcmpPacket? {
    if (data.len < ICMP_HEADER_SIZE) {
        return null;
    }
    const icmp_type: u8 = data[0];
    const code: u8 = data[1];
    const checksum: u16 = ((data[2] as u16) << 8) | (data[3] as u16);
    const rest: u32 = ((data[4] as u32) << 24) |
        ((data[5] as u32) << 16) |
        ((data[6] as u32) << 8) |
        (data[7] as u32);

    return IcmpPacket {
        icmp_type: icmp_type,
        code: code,
        checksum: checksum,
        rest_of_header: rest,
        payload: data[ICMP_HEADER_SIZE..data.len],
    };
}

/// 零分配编码 ICMPv4 报文并自动填入 RFC 1071 一补码校验和；缓冲不足时返回 `null`
pub fn encodeIcmp4(
    icmp_type: u8,
    code: u8,
    rest_of_header: u32,
    payload: []const u8,
    out: *[]u8,
): usize? {
    const total: usize = payload.len + ICMP_HEADER_SIZE;
    if (out.*.len < total) {
        return null;
    }
    out.*[0] = icmp_type;
    out.*[1] = code;
    out.*[2] = 0;
    out.*[3] = 0;
    out.*[4] = ((rest_of_header >> 24) & 0xFF) as u8;
    out.*[5] = ((rest_of_header >> 16) & 0xFF) as u8;
    out.*[6] = ((rest_of_header >> 8) & 0xFF) as u8;
    out.*[7] = (rest_of_header & 0xFF) as u8;

    let i: usize = 0;
    while (i < payload.len) {
        out.*[ICMP_HEADER_SIZE + i] = payload[i];
        i += 1;
    }

    const csum: u16 = icmp4Checksum(out.*[0..total]);
    out.*[2] = ((csum >> 8) & 0xFF) as u8;
    out.*[3] = (csum & 0xFF) as u8;
    return total;
}

/// 零分配编码 ICMPv4 Echo Request / Reply 报文
pub fn encodeIcmp4Echo(
    is_reply: bool,
    id: u16,
    seq: u16,
    payload: []const u8,
    out: *[]u8,
): usize? {
    const t: u8 = if (is_reply) { ICMP4_ECHO_REPLY } else { ICMP4_ECHO_REQUEST };
    const rest: u32 = ((id as u32) << 16) | (seq as u32);
    return encodeIcmp4(t, 0, rest, payload, out);
}

/// 零分配编码 ICMPv6 报文并自动计算填入含 IPv6 伪首部的校验和；缓冲不足时返回 `null`
pub fn encodeIcmp6(
    src_ip: [16]u8,
    dst_ip: [16]u8,
    icmp_type: u8,
    code: u8,
    rest_of_header: u32,
    payload: []const u8,
    out: *[]u8,
): usize? {
    const total: usize = payload.len + ICMP_HEADER_SIZE;
    if (out.*.len < total) {
        return null;
    }
    out.*[0] = icmp_type;
    out.*[1] = code;
    out.*[2] = 0;
    out.*[3] = 0;
    out.*[4] = ((rest_of_header >> 24) & 0xFF) as u8;
    out.*[5] = ((rest_of_header >> 16) & 0xFF) as u8;
    out.*[6] = ((rest_of_header >> 8) & 0xFF) as u8;
    out.*[7] = (rest_of_header & 0xFF) as u8;

    let i: usize = 0;
    while (i < payload.len) {
        out.*[ICMP_HEADER_SIZE + i] = payload[i];
        i += 1;
    }

    const csum: u16 = icmp6Checksum(src_ip, dst_ip, out.*[0..total]);
    out.*[2] = ((csum >> 8) & 0xFF) as u8;
    out.*[3] = (csum & 0xFF) as u8;
    return total;
}

/// 零分配编码 ICMPv6 Echo Request / Reply 报文（含 40B IPv6 伪首部校验和）
pub fn encodeIcmp6Echo(
    src_ip: [16]u8,
    dst_ip: [16]u8,
    is_reply: bool,
    id: u16,
    seq: u16,
    payload: []const u8,
    out: *[]u8,
): usize? {
    const t: u8 = if (is_reply) { ICMP6_ECHO_REPLY } else { ICMP6_ECHO_REQUEST };
    const rest: u32 = ((id as u32) << 16) | (seq as u32);
    return encodeIcmp6(src_ip, dst_ip, t, 0, rest, payload, out);
}
