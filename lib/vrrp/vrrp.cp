// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 3768/5798 VRRPv2 & VRRPv3 Parser)

/// IP 协议号：VRRP (IANA 112)
pub const VRRP_IP_PROTO: u8 = 112;

/// VRRP 协议版本：VRRPv2 (RFC 3768, IPv4 only)
pub const VRRP_V2: u8 = 2;

/// VRRP 协议版本：VRRPv3 (RFC 5798, IPv4 & IPv6)
pub const VRRP_V3: u8 = 3;

/// VRRP 报文类型：ADVERTISEMENT (1)
pub const VRRP_TYPE_ADVERTISEMENT: u8 = 1;

/// VRRP 固定公共报头长度（8 字节）
pub const VRRP_HEADER_SIZE: usize = 8;

/// VRRPv2 尾部固定认证数据长度（8 字节）
pub const VRRP_V2_AUTH_SIZE: usize = 8;

// ── VRRPv2 认证类型常量 (RFC 3768) ──
pub const VRRP_AUTH_NONE: u8 = 0;
pub const VRRP_AUTH_SIMPLE: u8 = 1;
pub const VRRP_AUTH_AH: u8 = 2;

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn foldChecksum(sum_in: u32): u16 {
    let acc: u32 = sum_in;
    while ((acc >> 16) != 0) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return (acc & 0xFFFF) as u16;
}

/// 内部：对 VRRP 报文做 16-bit 大端一补码累加（可选跳过字节 6..8 checksum 字段）
fn addVrrpBytes(acc_in: u32, data: []const u8, zero_checksum: bool): u32 {
    let acc: u32 = acc_in;
    let i: usize = 0;
    while (i + 1 < data.len) {
        if (zero_checksum && i == 6) {
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
        acc += (data[i] as u32) << 8;
    }
    return acc;
}

fn ipv4PseudoAcc(src_ip: [4]u8, dst_ip: [4]u8, vrrp_len: usize): u32 {
    let acc: u32 = 0;
    acc += ((src_ip[0] as u32) << 8) | (src_ip[1] as u32);
    acc += ((src_ip[2] as u32) << 8) | (src_ip[3] as u32);
    acc += ((dst_ip[0] as u32) << 8) | (dst_ip[1] as u32);
    acc += ((dst_ip[2] as u32) << 8) | (dst_ip[3] as u32);
    acc += VRRP_IP_PROTO as u32;
    acc += (vrrp_len & 0xFFFF) as u32;
    return acc;
}

fn ipv6PseudoAcc(src_ip: [16]u8, dst_ip: [16]u8, vrrp_len: usize): u32 {
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < 16) {
        acc += ((src_ip[i] as u32) << 8) | (src_ip[i + 1] as u32);
        acc += ((dst_ip[i] as u32) << 8) | (dst_ip[i + 1] as u32);
        i += 2;
    }
    const len_u32: u32 = vrrp_len as u32;
    acc += (len_u32 >> 16) & 0xFFFF;
    acc += len_u32 & 0xFFFF;
    acc += VRRP_IP_PROTO as u32;
    return acc;
}

/// 计算 VRRPv2 (RFC 3768) 一补码校验和（不含 IP 伪首部，自动忽略字节 6..8）
pub fn vrrp2Checksum(data: []const u8): u16 {
    const acc: u32 = addVrrpBytes(0, data, true);
    return ~foldChecksum(acc);
}

/// 验证 VRRPv2 (RFC 3768) 完整报文校验和
pub fn verifyVrrp2Checksum(data: []const u8): bool {
    if (data.len < VRRP_HEADER_SIZE) {
        return false;
    }
    const acc: u32 = addVrrpBytes(0, data, false);
    return foldChecksum(acc) == 0xFFFF;
}

/// 计算 VRRPv3 over IPv4 (RFC 5798) 校验和（含 12B IPv4 伪首部 + Proto=112）
pub fn vrrp3ChecksumIpv4(src_ip: [4]u8, dst_ip: [4]u8, data: []const u8): u16 {
    const pseudo: u32 = ipv4PseudoAcc(src_ip, dst_ip, data.len);
    const acc: u32 = addVrrpBytes(pseudo, data, true);
    return ~foldChecksum(acc);
}

/// 验证 VRRPv3 over IPv4 校验和
pub fn verifyVrrp3ChecksumIpv4(src_ip: [4]u8, dst_ip: [4]u8, data: []const u8): bool {
    if (data.len < VRRP_HEADER_SIZE) {
        return false;
    }
    const pseudo: u32 = ipv4PseudoAcc(src_ip, dst_ip, data.len);
    const acc: u32 = addVrrpBytes(pseudo, data, false);
    return foldChecksum(acc) == 0xFFFF;
}

/// 计算 VRRPv3 over IPv6 (RFC 5798) 校验和（含 40B IPv6 伪首部 + NextHeader=112）
pub fn vrrp3ChecksumIpv6(src_ip: [16]u8, dst_ip: [16]u8, data: []const u8): u16 {
    const pseudo: u32 = ipv6PseudoAcc(src_ip, dst_ip, data.len);
    const acc: u32 = addVrrpBytes(pseudo, data, true);
    return ~foldChecksum(acc);
}

/// 验证 VRRPv3 over IPv6 校验和
pub fn verifyVrrp3ChecksumIpv6(src_ip: [16]u8, dst_ip: [16]u8, data: []const u8): bool {
    if (data.len < VRRP_HEADER_SIZE) {
        return false;
    }
    const pseudo: u32 = ipv6PseudoAcc(src_ip, dst_ip, data.len);
    const acc: u32 = addVrrpBytes(pseudo, data, false);
    return foldChecksum(acc) == 0xFFFF;
}

/// VRRPv2 / VRRPv3 8 字节公共报头
pub struct VrrpHeader {
    version: u8,
    pkt_type: u8,
    vrid: u8,
    priority: u8,
    count_ip: u8,
    /// VRRPv2 为 Auth Type (8-bit)；VRRPv3 为高 4-bit 保留字段 (rsvd)
    auth_type: u8,
    /// VRRPv2 为 Adver Int（单位：秒，0..255）；VRRPv3 为 12-bit Max Adver Int（单位：厘秒，0..4095）
    max_adver_int: u16,
    checksum: u16,

    /// 是否为 Master 主动释放通告（Priority == 0）
    fn isMasterShutdown(): bool {
        return _.priority == 0;
    }

    /// 是否为虚拟 IP 地址所有者（Priority == 255）
    fn isAddressOwner(): bool {
        return _.priority == 255;
    }
}

/// VRRP IPv4 虚拟地址零分配迭代器（每项 4 字节）
pub struct VrrpIpv4Iter {
    data: []const u8,
    pos: usize,

    static fn init(ip_bytes: []const u8): VrrpIpv4Iter {
        return VrrpIpv4Iter {
            data: ip_bytes,
            pos: 0,
        };
    }

    fn next(*_): ([4]u8)? {
        if (_.pos + 4 > _.data.len) {
            return null;
        }
        const ip: [4]u8 = [
            _.data[_.pos],
            _.data[_.pos + 1],
            _.data[_.pos + 2],
            _.data[_.pos + 3],
        ];
        _.pos += 4;
        return ip;
    }
}

/// VRRP IPv6 虚拟地址零分配迭代器（每项 16 字节）
pub struct VrrpIpv6Iter {
    data: []const u8,
    pos: usize,

    static fn init(ip_bytes: []const u8): VrrpIpv6Iter {
        return VrrpIpv6Iter {
            data: ip_bytes,
            pos: 0,
        };
    }

    fn next(*_): ([16]u8)? {
        if (_.pos + 16 > _.data.len) {
            return null;
        }
        let ip: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) {
            ip[i] = _.data[_.pos + i];
            i += 1;
        }
        _.pos += 16;
        return ip;
    }
}

/// VRRPv2 / VRRPv3 完整报文零拷贝视图
pub struct VrrpPacket {
    header: VrrpHeader,
    is_ipv6: bool,
    /// 连续虚拟 IP 字节切片（count_ip * 4B 或 count_ip * 16B）
    ip_bytes: []const u8,
    /// VRRPv2 尾部 8 字节认证数据切片（VRRPv3 为空切片）
    auth_data: []const u8,
    /// 本报文消耗的有效字节总长度
    total_bytes: usize,

    /// 返回 IPv4 虚拟地址零分配迭代器（仅当 `!is_ipv6` 时有效）
    fn ipv4Addrs(): VrrpIpv4Iter {
        if (_.is_ipv6) {
            return VrrpIpv4Iter.init(_.ip_bytes[0..0]);
        }
        return VrrpIpv4Iter.init(_.ip_bytes);
    }

    /// 返回 IPv6 虚拟地址零分配迭代器（仅当 `is_ipv6` 时有效）
    fn ipv6Addrs(): VrrpIpv6Iter {
        if (!_.is_ipv6) {
            return VrrpIpv6Iter.init(_.ip_bytes[0..0]);
        }
        return VrrpIpv6Iter.init(_.ip_bytes);
    }
}

/// 快速嗅探字节流是否满足 VRRPv2 或 VRRPv3 最小头部格式
pub fn isVrrp(data: []const u8): bool {
    if (data.len < VRRP_HEADER_SIZE) {
        return false;
    }
    const version: u8 = (data[0] >> 4) & 0x0F;
    const pkt_type: u8 = data[0] & 0x0F;
    const count_ip: u8 = data[3];
    if ((version != VRRP_V2 && version != VRRP_V3) || pkt_type != VRRP_TYPE_ADVERTISEMENT || count_ip == 0) {
        return false;
    }
    if (version == VRRP_V2) {
        const min_len: usize = VRRP_HEADER_SIZE + ((count_ip as usize) * 4) + VRRP_V2_AUTH_SIZE;
        return data.len >= min_len;
    }
    // VRRPv3 至少需满足 IPv4 最小长度 (8 + count_ip * 4)
    const min_v3_len: usize = VRRP_HEADER_SIZE + ((count_ip as usize) * 4);
    return data.len >= min_v3_len;
}

/// 零拷贝解析 VRRPv2 (RFC 3768) 或 VRRPv3 (RFC 5798) 报文
/// 严格校验版本 2/3、type == 1、count_ip >= 1、VRRPv2 禁止 IPv6 及完整长度边界
pub fn parseVrrp(data: []const u8, is_ipv6: bool): VrrpPacket? {
    if (data.len < VRRP_HEADER_SIZE) {
        return null;
    }
    const version: u8 = (data[0] >> 4) & 0x0F;
    const pkt_type: u8 = data[0] & 0x0F;
    if ((version != VRRP_V2 && version != VRRP_V3) || pkt_type != VRRP_TYPE_ADVERTISEMENT) {
        return null;
    }
    const vrid: u8 = data[1];
    const priority: u8 = data[2];
    const count_ip: u8 = data[3];
    if (count_ip == 0) {
        return null;
    }
    const checksum: u16 = readBeU16(data, 6);

    if (version == VRRP_V2) {
        // VRRPv2 仅支持 IPv4
        if (is_ipv6) {
            return null;
        }
        const ip_len: usize = (count_ip as usize) * 4;
        const total: usize = VRRP_HEADER_SIZE + ip_len + VRRP_V2_AUTH_SIZE;
        if (data.len < total) {
            return null;
        }
        const auth_type: u8 = data[4];
        const adver_int: u16 = data[5] as u16;
        return VrrpPacket {
            header: VrrpHeader {
                version: version,
                pkt_type: pkt_type,
                vrid: vrid,
                priority: priority,
                count_ip: count_ip,
                auth_type: auth_type,
                max_adver_int: adver_int,
                checksum: checksum,
            },
            is_ipv6: false,
            ip_bytes: data[VRRP_HEADER_SIZE..VRRP_HEADER_SIZE + ip_len],
            auth_data: data[VRRP_HEADER_SIZE + ip_len..total],
            total_bytes: total,
        };
    }

    // VRRPv3 (IPv4 or IPv6)
    let addr_size: usize = 4;
    if (is_ipv6) {
        addr_size = 16;
    }
    const ip_len: usize = (count_ip as usize) * addr_size;
    const total: usize = VRRP_HEADER_SIZE + ip_len;
    if (data.len < total) {
        return null;
    }
    const rsvd: u8 = (data[4] >> 4) & 0x0F;
    const max_adver_int: u16 = (((data[4] & 0x0F) as u16) << 8) | (data[5] as u16);

    return VrrpPacket {
        header: VrrpHeader {
            version: version,
            pkt_type: pkt_type,
            vrid: vrid,
            priority: priority,
            count_ip: count_ip,
            auth_type: rsvd,
            max_adver_int: max_adver_int,
            checksum: checksum,
        },
        is_ipv6: is_ipv6,
        ip_bytes: data[VRRP_HEADER_SIZE..total],
        auth_data: data[0..0],
        total_bytes: total,
    };
}
