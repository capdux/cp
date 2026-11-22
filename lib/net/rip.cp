// Origin: Pure-CP Stdlib (Zero-Alloc RFC 2453 RIPv1/v2 & RFC 2080 RIPng Codec) | Created: 2026-11-22

/// RIPv1/v2 标准 UDP 端口 (RFC 2453)
pub const RIP_UDP_PORT: u16 = 520;

/// RIPng 标准 UDP 端口 (RFC 2080)
pub const RIPNG_UDP_PORT: u16 = 521;

/// RIP / RIPng 公共报头长度（4 字节）
pub const RIP_HEADER_SIZE: usize = 4;

/// RIP / RIPng 单条路由表项 (RTE) 固定长度（20 字节）
pub const RIP_RTE_SIZE: usize = 20;

// ── 协议版本号 ──
pub const RIP_V1: u8 = 1;
pub const RIP_V2: u8 = 2;
pub const RIPNG_V1: u8 = 1;

// ── Command 常量 ──
pub const RIP_CMD_REQUEST: u8 = 1;
pub const RIP_CMD_RESPONSE: u8 = 2;

// ── Address Family Identifier (AFI) 与认证常量 ──
pub const RIP_AFI_IP: u16 = 2;
pub const RIP_AFI_AUTH: u16 = 0xFFFF;
pub const RIP_AUTH_SIMPLE: u16 = 2;
pub const RIP_AUTH_CRYPTO: u16 = 3;

// ── 度量值常量 ──
pub const RIP_METRIC_INFINITY: u32 = 16;
pub const RIPNG_NEXT_HOP_METRIC: u8 = 0xFF;

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

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 3] = (v & 0xFF) as u8;
}

/// RIPv1 / RIPv2 / RIPng 4 字节公共报头
pub struct RipHeader {
    command: u8,
    version: u8,

    fn isRequest(): bool {
        return _.command == RIP_CMD_REQUEST;
    }

    fn isResponse(): bool {
        return _.command == RIP_CMD_RESPONSE;
    }
}

/// RIPv1 / RIPv2 20 字节路由表项 (RTE)
pub struct RipRte {
    afi: u16,
    route_tag: u16,
    ip: [4]u8,
    mask: [4]u8,
    next_hop: [4]u8,
    metric: u32,

    /// 是否为认证扩展条目 (AFI == 0xFFFF)
    fn isAuth(): bool {
        return _.afi == RIP_AFI_AUTH;
    }

    /// 是否为不可达路由 (metric >= 16)
    fn isUnreachable(): bool {
        return _.metric >= RIP_METRIC_INFINITY;
    }

    /// 是否为默认路由 (0.0.0.0/0, AFI == IP)
    fn isDefaultRoute(): bool {
        return _.afi == RIP_AFI_IP &&
            _.ip[0] == 0 && _.ip[1] == 0 && _.ip[2] == 0 && _.ip[3] == 0 &&
            _.mask[0] == 0 && _.mask[1] == 0 && _.mask[2] == 0 && _.mask[3] == 0;
    }
}

/// RIPv2 简单口令认证条目 (AFI == 0xFFFF, AuthType == 2)
pub struct RipAuthSimple {
    password: [16]u8,
}

/// 解析 20 字节 RIPv2 简单口令认证 RTE
pub fn parseRipAuthSimple(rte_raw: []const u8): RipAuthSimple? {
    if (rte_raw.len < RIP_RTE_SIZE) {
        return null;
    }
    const afi: u16 = readBeU16(rte_raw, 0);
    const auth_type: u16 = readBeU16(rte_raw, 2);
    if (afi != RIP_AFI_AUTH || auth_type != RIP_AUTH_SIMPLE) {
        return null;
    }
    let pw: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        pw[i] = rte_raw[4 + i];
        i += 1;
    }
    return RipAuthSimple {
        password: pw,
    };
}

/// RIPv1 / RIPv2 RTE 零分配迭代器（每项 20 字节）
pub struct RipRteIter {
    data: []const u8,
    pos: usize,

    static fn init(rte_bytes: []const u8): RipRteIter {
        return RipRteIter {
            data: rte_bytes,
            pos: 0,
        };
    }

    fn next(*_): RipRte? {
        if (_.pos + RIP_RTE_SIZE > _.data.len) {
            return null;
        }
        const off: usize = _.pos;
        _.pos += RIP_RTE_SIZE;
        return RipRte {
            afi: readBeU16(_.data, off),
            route_tag: readBeU16(_.data, off + 2),
            ip: readIp4(_.data, off + 4),
            mask: readIp4(_.data, off + 8),
            next_hop: readIp4(_.data, off + 12),
            metric: readBeU32(_.data, off + 16),
        };
    }
}

/// RIPv1 / RIPv2 完整报文零拷贝视图
pub struct RipPacket {
    header: RipHeader,
    rte_bytes: []const u8,
    rte_count: usize,

    /// 返回 RTE 零分配迭代器
    fn entries(): RipRteIter {
        return RipRteIter.init(_.rte_bytes);
    }

    /// 按下标返回原始 20 字节 RTE 切片（便于传递给 `parseRipAuthSimple`）
    fn rawRteAt(idx: usize): ([]const u8)? {
        const off: usize = idx * RIP_RTE_SIZE;
        if (off + RIP_RTE_SIZE > _.rte_bytes.len) {
            return null;
        }
        return _.rte_bytes[off..off + RIP_RTE_SIZE];
    }
}

/// 快速嗅探字节流是否满足 RIPv1 / RIPv2 报文格式
pub fn isRip(data: []const u8): bool {
    if (data.len < RIP_HEADER_SIZE) {
        return false;
    }
    const cmd: u8 = data[0];
    const ver: u8 = data[1];
    if ((cmd != RIP_CMD_REQUEST && cmd != RIP_CMD_RESPONSE) || (ver != RIP_V1 && ver != RIP_V2)) {
        return false;
    }
    if (data[2] != 0 || data[3] != 0) {
        return false;
    }
    if (((data.len - RIP_HEADER_SIZE) % RIP_RTE_SIZE) != 0) {
        return false;
    }
    return true;
}

/// 零拷贝解析 RIPv1 / RIPv2 报文（校验 command in 1..2, version in 1..2, reserved == 0, 20B RTE 对齐）
pub fn parseRip(data: []const u8): RipPacket? {
    if (!isRip(data)) {
        return null;
    }
    const rte_slice = data[RIP_HEADER_SIZE..data.len];
    return RipPacket {
        header: RipHeader {
            command: data[0],
            version: data[1],
        },
        rte_bytes: rte_slice,
        rte_count: rte_slice.len / RIP_RTE_SIZE,
    };
}

/// RIPng (RFC 2080) 20 字节 IPv6 路由表项
pub struct RipngRte {
    prefix: [16]u8,
    route_tag: u16,
    prefix_len: u8,
    metric: u8,

    /// 是否为 Next-Hop 指定条目 (metric == 0xFF)
    fn isNextHop(): bool {
        return _.metric == RIPNG_NEXT_HOP_METRIC;
    }

    /// 是否为不可达路由 (metric == 16)
    fn isUnreachable(): bool {
        return _.metric == 16;
    }
}

/// RIPng RTE 零分配迭代器（每项 20 字节）
pub struct RipngRteIter {
    data: []const u8,
    pos: usize,

    static fn init(rte_bytes: []const u8): RipngRteIter {
        return RipngRteIter {
            data: rte_bytes,
            pos: 0,
        };
    }

    fn next(*_): RipngRte? {
        if (_.pos + RIP_RTE_SIZE > _.data.len) {
            return null;
        }
        const off: usize = _.pos;
        _.pos += RIP_RTE_SIZE;
        let prefix: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) {
            prefix[i] = _.data[off + i];
            i += 1;
        }
        return RipngRte {
            prefix: prefix,
            route_tag: readBeU16(_.data, off + 16),
            prefix_len: _.data[off + 18],
            metric: _.data[off + 19],
        };
    }
}

/// RIPng (RFC 2080) 完整报文零拷贝视图
pub struct RipngPacket {
    header: RipHeader,
    rte_bytes: []const u8,
    rte_count: usize,

    /// 返回 RIPng RTE 零分配迭代器
    fn entries(): RipngRteIter {
        return RipngRteIter.init(_.rte_bytes);
    }
}

/// 零拷贝解析 RIPng (RFC 2080) 报文（校验 command in 1..2, version == 1, reserved == 0, 20B RTE 对齐）
pub fn parseRipng(data: []const u8): RipngPacket? {
    if (data.len < RIP_HEADER_SIZE) {
        return null;
    }
    const cmd: u8 = data[0];
    const ver: u8 = data[1];
    if ((cmd != RIP_CMD_REQUEST && cmd != RIP_CMD_RESPONSE) || ver != RIPNG_V1) {
        return null;
    }
    if (data[2] != 0 || data[3] != 0) {
        return null;
    }
    if (((data.len - RIP_HEADER_SIZE) % RIP_RTE_SIZE) != 0) {
        return null;
    }
    const rte_slice = data[RIP_HEADER_SIZE..data.len];
    return RipngPacket {
        header: RipHeader {
            command: cmd,
            version: ver,
        },
        rte_bytes: rte_slice,
        rte_count: rte_slice.len / RIP_RTE_SIZE,
    };
}

/// 零分配编码 4 字节 RIP / RIPng 公共报头；缓冲不足 4B 返回 `null`
pub fn encodeRipHeader(command: u8, version: u8, out: *[]u8): usize? {
    if (out.*.len < RIP_HEADER_SIZE) {
        return null;
    }
    out.*[0] = command;
    out.*[1] = version;
    out.*[2] = 0;
    out.*[3] = 0;
    return RIP_HEADER_SIZE;
}

/// 零分配编码 20 字节 RIPv1/v2 RTE；缓冲不足 20B 返回 `null`
pub fn encodeRipRte(rte: RipRte, out: *[]u8): usize? {
    if (out.*.len < RIP_RTE_SIZE) {
        return null;
    }
    writeBeU16(out, 0, rte.afi);
    writeBeU16(out, 2, rte.route_tag);
    out.*[4] = rte.ip[0]; out.*[5] = rte.ip[1]; out.*[6] = rte.ip[2]; out.*[7] = rte.ip[3];
    out.*[8] = rte.mask[0]; out.*[9] = rte.mask[1]; out.*[10] = rte.mask[2]; out.*[11] = rte.mask[3];
    out.*[12] = rte.next_hop[0]; out.*[13] = rte.next_hop[1]; out.*[14] = rte.next_hop[2]; out.*[15] = rte.next_hop[3];
    writeBeU32(out, 16, rte.metric);
    return RIP_RTE_SIZE;
}
