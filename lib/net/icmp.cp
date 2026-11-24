// Reference-hardened against smoltcp-rs/smoltcp (0BSD) & RFC 792/4443/826
// Origin: Pure-CP Stdlib (Zero-Alloc RFC 792/4443 ICMPv4 & ICMPv6 Codec) | Created: 2026-11-22

type IcmpArpVec8u16 = @Vector(8, u16);
type IcmpArpVec8u32 = @Vector(8, u32);

/// ICMP 固定报头长度（Type 1B + Code 1B + Checksum 2B + Rest of Header 4B）
pub const ICMP_HEADER_SIZE: usize = 8;

/// IPv4 最大 ICMP 报文长度上限
pub const ICMP_MAX_PACKET_SIZE: usize = 65535;

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

/// 折叠 64 位一补码累加器至 16 位（防进位溢出）
fn icmpFoldU64(sum_in: u64): u16 {
    let acc: u64 = sum_in;
    while ((acc >> 16) != 0) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return (acc & 0xFFFF) as u16;
}

/// 标量基准：RFC 1071 16-bit 大端一补码累加并折叠至 16 位（含奇数字节补零，防溢出）
pub fn scalarIcmpChecksumFold(acc_in: u32, data: []const u8): u16 {
    let acc: u64 = acc_in as u64;
    let i: usize = 0;
    while (i + 1 < data.len) : (i += 2) {
        const word: u64 = ((data[i] as u64) << 8) | (data[i + 1] as u64);
        acc += word;
    }
    if (i < data.len) {
        acc += (data[i] as u64) << 8;
    }
    return icmpFoldU64(acc);
}

/// 8-lane SIMD 批量（每次 16 字节 / 8 个 u16）RFC 1071 一补码累加并折叠至 16 位
pub fn simdIcmpChecksumFold(acc_in: u32, data: []const u8): u16 {
    let acc: u64 = acc_in as u64;
    let vec_acc: IcmpArpVec8u32 = @splat(@as(u32, 0));
    let i: usize = 0;
    let blocks: usize = 0;
    while (i + 16 <= data.len) : (i += 16) {
        const w16: IcmpArpVec8u16 = [
            ((data[i] as u16) << 8) | (data[i + 1] as u16),
            ((data[i + 2] as u16) << 8) | (data[i + 3] as u16),
            ((data[i + 4] as u16) << 8) | (data[i + 5] as u16),
            ((data[i + 6] as u16) << 8) | (data[i + 7] as u16),
            ((data[i + 8] as u16) << 8) | (data[i + 9] as u16),
            ((data[i + 10] as u16) << 8) | (data[i + 11] as u16),
            ((data[i + 12] as u16) << 8) | (data[i + 13] as u16),
            ((data[i + 14] as u16) << 8) | (data[i + 15] as u16),
        ];
        const w32: IcmpArpVec8u32 = [
            w16[0] as u32, w16[1] as u32, w16[2] as u32, w16[3] as u32,
            w16[4] as u32, w16[5] as u32, w16[6] as u32, w16[7] as u32,
        ];
        vec_acc += w32;
        blocks += 1;
        if (blocks == 1024) {
            acc += @reduce(.Add, vec_acc) as u64;
            vec_acc = @splat(@as(u32, 0));
            blocks = 0;
        }
    }
    if (blocks > 0) {
        acc += @reduce(.Add, vec_acc) as u64;
    }
    while (i + 1 < data.len) : (i += 2) {
        const word: u64 = ((data[i] as u64) << 8) | (data[i + 1] as u64);
        acc += word;
    }
    if (i < data.len) {
        acc += (data[i] as u64) << 8;
    }
    return icmpFoldU64(acc);
}

/// 累加 40 字节 IPv6 伪首部（src_ip 16B + dst_ip 16B + upper_len 4B + zero 3B + next_header 58）
fn icmpAddIpv6PseudoHeader(src_ip: [16]u8, dst_ip: [16]u8, pkt_len: usize): u32 {
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

fn icmpFoldPacketZeroingSlot(acc_in: u32, packet: []const u8): u16 {
    if (packet.len < 2) {
        return simdIcmpChecksumFold(acc_in, packet);
    }
    const head_word: u32 = ((packet[0] as u32) << 8) | (packet[1] as u32);
    const base: u32 = acc_in + head_word;
    if (packet.len <= 4) {
        return icmpFoldU64(base as u64);
    }
    return simdIcmpChecksumFold(base, packet[4..packet.len]);
}

/// 计算 ICMPv4 报文的 RFC 1071 16-bit 一补码校验和（自动忽略字节 2..4 校验和槽位，8-lane SIMD 加速）
pub fn icmp4Checksum(packet: []const u8): u16 {
    const folded: u16 = icmpFoldPacketZeroingSlot(0, packet);
    return ~folded;
}

/// 校验 ICMPv4 完整报文的 RFC 1071 一补码校验和（需满足长度 >= 8，8-lane SIMD 加速）
pub fn verifyIcmp4Checksum(packet: []const u8): bool {
    if (packet.len < ICMP_HEADER_SIZE) {
        return false;
    }
    return simdIcmpChecksumFold(0, packet) == 0xFFFF;
}

/// 计算 ICMPv6 报文的 RFC 4443 / RFC 1071 校验和（含 40B IPv6 伪首部 + NextHeader=58）
pub fn icmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): u16 {
    const pseudo: u32 = icmpAddIpv6PseudoHeader(src_ip, dst_ip, packet.len);
    const folded: u16 = icmpFoldPacketZeroingSlot(pseudo, packet);
    return ~folded;
}

/// 校验 ICMPv6 完整报文（含 IPv6 伪首部）的一补码校验和（需满足长度 >= 8）
pub fn verifyIcmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): bool {
    if (packet.len < ICMP_HEADER_SIZE) {
        return false;
    }
    const pseudo: u32 = icmpAddIpv6PseudoHeader(src_ip, dst_ip, packet.len);
    return simdIcmpChecksumFold(pseudo, packet) == 0xFFFF;
}

/// 零堆严格校验 RFC 792 ICMPv4 报文（>=8B 头、RFC 1071 校验和、Echo/Unreach/TimeExceeded/Redirect/Timestamp 载荷边界）
pub fn validateIcmpStrict(data: []const u8): bool {
    if (data.len < ICMP_HEADER_SIZE || data.len > ICMP_MAX_PACKET_SIZE) {
        return false;
    }
    if (!verifyIcmp4Checksum(data)) {
        return false;
    }
    const pkt = parseIcmp(data) ?? return false;
    if (pkt.icmp_type == ICMP4_ECHO_REQUEST || pkt.icmp_type == ICMP4_ECHO_REPLY) {
        return pkt.code == 0;
    }
    if (pkt.icmp_type == ICMP4_DEST_UNREACH) {
        return pkt.code <= 15 && pkt.payload.len >= 28;
    }
    if (pkt.icmp_type == ICMP4_TIME_EXCEEDED) {
        return pkt.code <= 1 && pkt.payload.len >= 28;
    }
    if (pkt.icmp_type == ICMP4_PARAM_PROBLEM) {
        return pkt.code <= 2 && pkt.payload.len >= 28;
    }
    if (pkt.icmp_type == ICMP4_REDIRECT) {
        return pkt.code <= 3 && pkt.payload.len >= 28;
    }
    if (pkt.icmp_type == ICMP4_TIMESTAMP_REQ || pkt.icmp_type == ICMP4_TIMESTAMP_REPLY) {
        return pkt.code == 0 && pkt.payload.len == 12;
    }
    return true;
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
    if (payload.len > ICMP_MAX_PACKET_SIZE - ICMP_HEADER_SIZE) {
        return null;
    }
    const total: usize = payload.len + ICMP_HEADER_SIZE;
    if (out.len < total) {
        return null;
    }
    out[0] = icmp_type;
    out[1] = code;
    out[2] = 0;
    out[3] = 0;
    out[4] = ((rest_of_header >> 24) & 0xFF) as u8;
    out[5] = ((rest_of_header >> 16) & 0xFF) as u8;
    out[6] = ((rest_of_header >> 8) & 0xFF) as u8;
    out[7] = (rest_of_header & 0xFF) as u8;

    let i: usize = 0;
    while (i < payload.len) {
        out[ICMP_HEADER_SIZE + i] = payload[i];
        i += 1;
    }

    const csum: u16 = icmp4Checksum(out[0..total]);
    out[2] = ((csum >> 8) & 0xFF) as u8;
    out[3] = (csum & 0xFF) as u8;
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
    const t: u8 = if (is_reply) ICMP4_ECHO_REPLY else ICMP4_ECHO_REQUEST;
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
    if (payload.len > ICMP_MAX_PACKET_SIZE - ICMP_HEADER_SIZE) {
        return null;
    }
    const total: usize = payload.len + ICMP_HEADER_SIZE;
    if (out.len < total) {
        return null;
    }
    out[0] = icmp_type;
    out[1] = code;
    out[2] = 0;
    out[3] = 0;
    out[4] = ((rest_of_header >> 24) & 0xFF) as u8;
    out[5] = ((rest_of_header >> 16) & 0xFF) as u8;
    out[6] = ((rest_of_header >> 8) & 0xFF) as u8;
    out[7] = (rest_of_header & 0xFF) as u8;

    let i: usize = 0;
    while (i < payload.len) {
        out[ICMP_HEADER_SIZE + i] = payload[i];
        i += 1;
    }

    const csum: u16 = icmp6Checksum(src_ip, dst_ip, out[0..total]);
    out[2] = ((csum >> 8) & 0xFF) as u8;
    out[3] = (csum & 0xFF) as u8;
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
    const t: u8 = if (is_reply) ICMP6_ECHO_REPLY else ICMP6_ECHO_REQUEST;
    const rest: u32 = ((id as u32) << 16) | (seq as u32);
    return encodeIcmp6(src_ip, dst_ip, t, 0, rest, payload, out);
}
