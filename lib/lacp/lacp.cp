// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IEEE 802.1AX / 802.3ad LACPDU Codec)

/// IEEE 802.3 Slow Protocols EtherType (0x8809)
pub const SLOW_PROTO_ETHERTYPE: u16 = 0x8809;

/// Slow Protocols LACP Subtype (0x01)
pub const LACP_SUBTYPE: u8 = 0x01;

/// LACP 协议版本号 (0x01)
pub const LACP_VERSION: u8 = 0x01;

/// LACPDU 标准最小载荷字节数（不含 14B Ethernet 头，含 50B 尾部 Reserved）
pub const LACP_MIN_LEN: usize = 110;

/// LACP 完整以太网帧长度（14B Ethernet 头 + 110B LACPDU，不含 4B FCS）
pub const LACP_FULL_LEN: usize = 124;

// ── LACP TLV Type 与 Length 常量 ──
pub const TLV_TERMINATOR: u8 = 0x00;
pub const TLV_ACTOR: u8 = 0x01;
pub const TLV_PARTNER: u8 = 0x02;
pub const TLV_COLLECTOR: u8 = 0x03;

pub const TLV_TERMINATOR_LEN: u8 = 0;
pub const TLV_ACTOR_LEN: u8 = 20;
pub const TLV_PARTNER_LEN: u8 = 20;
pub const TLV_COLLECTOR_LEN: u8 = 16;

// ── Actor / Partner State 位掩码 (IEEE 802.1AX §6.4.2.3) ──
pub const STATE_ACTIVITY: u8 = 0x01;
pub const STATE_TIMEOUT: u8 = 0x02;
pub const STATE_AGGREGATION: u8 = 0x04;
pub const STATE_SYNC: u8 = 0x08;
pub const STATE_COLLECTING: u8 = 0x10;
pub const STATE_DISTRIBUTING: u8 = 0x20;
pub const STATE_DEFAULTED: u8 = 0x40;
pub const STATE_EXPIRED: u8 = 0x80;

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn readMac(data: []const u8, off: usize): [6]u8 {
    return [
        data[off],
        data[off + 1],
        data[off + 2],
        data[off + 3],
        data[off + 4],
        data[off + 5],
    ];
}

/// LACP Actor / Partner 对端状态信息
pub struct LacpPeerInfo {
    system_priority: u16,
    system_id: [6]u8,
    key: u16,
    port_priority: u16,
    port_number: u16,
    state: u8,

    /// bit 0: Active LACP (1) vs Passive LACP (0)
    fn isActive(): bool {
        return (_.state & STATE_ACTIVITY) != 0;
    }

    /// bit 1: Short Timeout 1s (1) vs Long Timeout 30s (0)
    fn isShortTimeout(): bool {
        return (_.state & STATE_TIMEOUT) != 0;
    }

    /// bit 2: Aggregatable (1) vs Individual (0)
    fn isAggregatable(): bool {
        return (_.state & STATE_AGGREGATION) != 0;
    }

    /// bit 3: In Synchronization (1) vs Out of Sync (0)
    fn isInSync(): bool {
        return (_.state & STATE_SYNC) != 0;
    }

    /// bit 4: Collecting enabled
    fn isCollecting(): bool {
        return (_.state & STATE_COLLECTING) != 0;
    }

    /// bit 5: Distributing enabled
    fn isDistributing(): bool {
        return (_.state & STATE_DISTRIBUTING) != 0;
    }

    /// bit 6: Defaulted partner info
    fn isDefaulted(): bool {
        return (_.state & STATE_DEFAULTED) != 0;
    }

    /// bit 7: Expired state
    fn isExpired(): bool {
        return (_.state & STATE_EXPIRED) != 0;
    }
}

/// IEEE 802.1AX LACPDU 解析结果
pub struct LacpPdu {
    subtype: u8,
    version: u8,
    actor: LacpPeerInfo,
    partner: LacpPeerInfo,
    /// Collector Max Delay（单位：10 微秒）
    collector_max_delay: u16,
}

/// 快速嗅探字节流是否为合法 LACPDU（校验长度 >= 110、subtype/version 及核心 TLV 头）
pub fn isLacp(data: []const u8): bool {
    if (data.len < LACP_MIN_LEN) {
        return false;
    }
    if (data[0] != LACP_SUBTYPE || data[1] != LACP_VERSION) {
        return false;
    }
    if (data[2] != TLV_ACTOR || data[3] != TLV_ACTOR_LEN) {
        return false;
    }
    if (data[22] != TLV_PARTNER || data[23] != TLV_PARTNER_LEN) {
        return false;
    }
    if (data[42] != TLV_COLLECTOR || data[43] != TLV_COLLECTOR_LEN) {
        return false;
    }
    if (data[58] != TLV_TERMINATOR || data[59] != TLV_TERMINATOR_LEN) {
        return false;
    }
    return true;
}

/// 零拷贝解析 IEEE 802.1AX LACPDU（严格校验 subtype/version/Actor/Partner/Collector/Terminator TLV 与长度 >= 110）
pub fn parseLacp(data: []const u8): LacpPdu? {
    if (!isLacp(data)) {
        return null;
    }

    const actor = LacpPeerInfo {
        system_priority: readBeU16(data, 4),
        system_id: readMac(data, 6),
        key: readBeU16(data, 12),
        port_priority: readBeU16(data, 14),
        port_number: readBeU16(data, 16),
        state: data[18],
    };

    const partner = LacpPeerInfo {
        system_priority: readBeU16(data, 24),
        system_id: readMac(data, 26),
        key: readBeU16(data, 32),
        port_priority: readBeU16(data, 34),
        port_number: readBeU16(data, 36),
        state: data[38],
    };

    const collector_max_delay: u16 = readBeU16(data, 44);

    return LacpPdu {
        subtype: data[0],
        version: data[1],
        actor: actor,
        partner: partner,
        collector_max_delay: collector_max_delay,
    };
}

/// 零分配编码标准 110 字节 LACPDU；目标缓冲不足 110 字节时返回 `null`
pub fn encodeLacp(pdu: LacpPdu, out: *[]u8): usize? {
    if (out.*.len < LACP_MIN_LEN) {
        return null;
    }
    // 清零 110 字节（自动填充所有 Reserved 区域及 Terminator TLV）
    let i: usize = 0;
    while (i < LACP_MIN_LEN) {
        out.*[i] = 0;
        i += 1;
    }

    // Header
    out.*[0] = LACP_SUBTYPE;
    out.*[1] = LACP_VERSION;

    // Actor Information TLV (offset 2..22)
    out.*[2] = TLV_ACTOR;
    out.*[3] = TLV_ACTOR_LEN;
    writeBeU16(out, 4, pdu.actor.system_priority);
    let m: usize = 0;
    while (m < 6) {
        out.*[6 + m] = pdu.actor.system_id[m];
        m += 1;
    }
    writeBeU16(out, 12, pdu.actor.key);
    writeBeU16(out, 14, pdu.actor.port_priority);
    writeBeU16(out, 16, pdu.actor.port_number);
    out.*[18] = pdu.actor.state;

    // Partner Information TLV (offset 22..42)
    out.*[22] = TLV_PARTNER;
    out.*[23] = TLV_PARTNER_LEN;
    writeBeU16(out, 24, pdu.partner.system_priority);
    m = 0;
    while (m < 6) {
        out.*[26 + m] = pdu.partner.system_id[m];
        m += 1;
    }
    writeBeU16(out, 32, pdu.partner.key);
    writeBeU16(out, 34, pdu.partner.port_priority);
    writeBeU16(out, 36, pdu.partner.port_number);
    out.*[38] = pdu.partner.state;

    // Collector Information TLV (offset 42..58)
    out.*[42] = TLV_COLLECTOR;
    out.*[43] = TLV_COLLECTOR_LEN;
    writeBeU16(out, 44, pdu.collector_max_delay);

    // Terminator TLV (offset 58..60) = 0x00, 0x00 + 50B Reserved (60..110) = 0
    return LACP_MIN_LEN;
}
