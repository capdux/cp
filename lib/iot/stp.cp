// Origin: Pure-CP Stdlib (Zero-Alloc IEEE 802.1D / 802.1w STP & RSTP BPDU Codec) | Created: 2026-11-22

pub const STP_PROTOCOL_ID: u16 = 0x0000;

pub const STP_VERSION_STP: u8 = 0;
pub const STP_VERSION_RSTP: u8 = 2;
pub const STP_VERSION_MSTP: u8 = 3;

pub const BPDU_TYPE_CONFIG: u8 = 0x00;
pub const BPDU_TYPE_RST: u8 = 0x02;
pub const BPDU_TYPE_TCN: u8 = 0x80;

pub const STP_TCN_SIZE: usize = 4;
pub const STP_CONFIG_SIZE: usize = 35;
pub const STP_RST_SIZE: usize = 36;

pub const FLAG_TC: u8 = 0x01;
pub const FLAG_PROPOSAL: u8 = 0x02;
pub const FLAG_LEARNING: u8 = 0x10;
pub const FLAG_FORWARDING: u8 = 0x20;
pub const FLAG_AGREEMENT: u8 = 0x40;
pub const FLAG_TCA: u8 = 0x80;

pub enum StpPortRole: u8 {
    unknown = 0,
    alternate_backup = 1,
    root = 2,
    designated = 3,
}

pub fn stpPortRoleFromFlags(flags: u8): StpPortRole {
    const code = (flags >> 2) & 0x03;
    if (code == 1) return StpPortRole.alternate_backup;
    if (code == 2) return StpPortRole.root;
    if (code == 3) return StpPortRole.designated;
    return StpPortRole.unknown;
}

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

/// 8-byte IEEE 802.1t Bridge Identifier: top 4-bit priority (multiple of 4096) + 12-bit system ID ext + 6B MAC.
pub struct StpBridgeId {
    priority: u16,
    system_id_ext: u16,
    mac: [6]u8,

    static fn zero(): StpBridgeId {
        return StpBridgeId{
            .priority = 0,
            .system_id_ext = 0,
            .mac = [0, 0, 0, 0, 0, 0],
        };
    }
}

/// Parses an 8-byte IEEE 802.1t Bridge Identifier.
pub fn parseBridgeId(bytes_8: []const u8): StpBridgeId? {
    if (bytes_8.len < 8) {
        return null;
    }
    const raw = readU16Be(bytes_8, 0);
    const mac: [6]u8 = [
        bytes_8[2],
        bytes_8[3],
        bytes_8[4],
        bytes_8[5],
        bytes_8[6],
        bytes_8[7],
    ];
    return StpBridgeId{
        .priority = raw & 0xF000,
        .system_id_ext = raw & 0x0FFF,
        .mac = mac,
    };
}

/// Encodes an 8-byte IEEE 802.1t Bridge Identifier into `out_8`.
pub fn encodeBridgeId(bid: StpBridgeId, out_8: []u8): bool {
    if (out_8.len < 8) {
        return false;
    }
    const combined: u16 = (bid.priority & 0xF000) | (bid.system_id_ext & 0x0FFF);
    writeU16Be(out_8, 0, combined);
    let i: usize = 0;
    while (i < 6) {
        out_8[2 + i] = bid.mac[i];
        i += 1;
    }
    return true;
}

pub struct StpBpdu {
    version: u8,
    bpdu_type: u8,
    is_tcn: bool,
    flags: u8,
    root_id: StpBridgeId,
    root_path_cost: u32,
    bridge_id: StpBridgeId,
    port_id: u16,
    message_age_ticks: u16,
    max_age_ticks: u16,
    hello_time_ticks: u16,
    forward_delay_ticks: u16,
    version_1_len: u8,

    fn isTcn(): bool {
        return _.is_tcn;
    }

    fn isRstp(): bool {
        return _.bpdu_type == BPDU_TYPE_RST;
    }

    fn portRole(): StpPortRole {
        return stpPortRoleFromFlags(_.flags);
    }

    fn isTopologyChange(): bool {
        return (_.flags & FLAG_TC) != 0;
    }

    fn isTopologyChangeAck(): bool {
        return (_.flags & FLAG_TCA) != 0;
    }

    fn isProposal(): bool {
        return (_.flags & FLAG_PROPOSAL) != 0;
    }

    fn isAgreement(): bool {
        return (_.flags & FLAG_AGREEMENT) != 0;
    }

    fn isForwarding(): bool {
        return (_.flags & FLAG_FORWARDING) != 0;
    }

    fn isLearning(): bool {
        return (_.flags & FLAG_LEARNING) != 0;
    }

    /// Top 4-bit port priority (0, 16, 32, ..., 240).
    fn portPriority(): u8 {
        return ((_.port_id >> 8) & 0xF0) as u8;
    }

    /// Low 12-bit port number (0..4095).
    fn portNumber(): u16 {
        return _.port_id & 0x0FFF;
    }

    fn messageAgeSec(): f64 {
        return (_.message_age_ticks as f64) / 256.0;
    }

    fn maxAgeSec(): f64 {
        return (_.max_age_ticks as f64) / 256.0;
    }

    fn helloTimeSec(): f64 {
        return (_.hello_time_ticks as f64) / 256.0;
    }

    fn forwardDelaySec(): f64 {
        return (_.forward_delay_ticks as f64) / 256.0;
    }
}

/// Fast check verifying protocol_id == 0x0000 and valid BPDU type & minimum length.
pub fn isBpdu(data: []const u8): bool {
    if (data.len < STP_TCN_SIZE) {
        return false;
    }
    if (readU16Be(data, 0) != STP_PROTOCOL_ID) {
        return false;
    }
    const btype = data[3];
    if (btype == BPDU_TYPE_TCN) {
        return true;
    }
    if (btype == BPDU_TYPE_CONFIG) {
        return data.len >= STP_CONFIG_SIZE;
    }
    if (btype == BPDU_TYPE_RST) {
        return data.len >= STP_RST_SIZE;
    }
    return false;
}

/// Parses an IEEE 802.1D (Config / TCN) or IEEE 802.1w (RSTP) BPDU.
pub fn parseBpdu(data: []const u8): StpBpdu? {
    if (data.len < STP_TCN_SIZE) {
        return null;
    }
    if (readU16Be(data, 0) != STP_PROTOCOL_ID) {
        return null;
    }
    const version = data[2];
    const btype = data[3];

    if (btype == BPDU_TYPE_TCN) {
        return StpBpdu{
            .version = version,
            .bpdu_type = btype,
            .is_tcn = true,
            .flags = 0,
            .root_id = StpBridgeId.zero(),
            .root_path_cost = 0,
            .bridge_id = StpBridgeId.zero(),
            .port_id = 0,
            .message_age_ticks = 0,
            .max_age_ticks = 0,
            .hello_time_ticks = 0,
            .forward_delay_ticks = 0,
            .version_1_len = 0,
        };
    }

    if (btype != BPDU_TYPE_CONFIG && btype != BPDU_TYPE_RST) {
        return null;
    }

    const min_len = if (btype == BPDU_TYPE_RST) STP_RST_SIZE else STP_CONFIG_SIZE;
    if (data.len < min_len) {
        return null;
    }

    const root_id = parseBridgeId(data[5..13]) ?? return null;
    const bridge_id = parseBridgeId(data[17..25]) ?? return null;
    const v1_len: u8 = if (btype == BPDU_TYPE_RST) data[35] else 0;

    return StpBpdu{
        .version = version,
        .bpdu_type = btype,
        .is_tcn = false,
        .flags = data[4],
        .root_id = root_id,
        .root_path_cost = readU32Be(data, 13),
        .bridge_id = bridge_id,
        .port_id = readU16Be(data, 25),
        .message_age_ticks = readU16Be(data, 27),
        .max_age_ticks = readU16Be(data, 29),
        .hello_time_ticks = readU16Be(data, 31),
        .forward_delay_ticks = readU16Be(data, 33),
        .version_1_len = v1_len,
    };
}

/// Zero-alloc BPDU encoder: writes 4B for TCN, 35B for STP Config, or 36B for RSTP.
pub fn encodeConfigBpdu(bpdu: StpBpdu, out: []u8): usize? {
    if (bpdu.bpdu_type == BPDU_TYPE_TCN || bpdu.is_tcn) {
        if (out.len < STP_TCN_SIZE) {
            return null;
        }
        writeU16Be(out, 0, STP_PROTOCOL_ID);
        out[2] = bpdu.version;
        out[3] = BPDU_TYPE_TCN;
        return STP_TCN_SIZE;
    }

    if (bpdu.bpdu_type != BPDU_TYPE_CONFIG && bpdu.bpdu_type != BPDU_TYPE_RST) {
        return null;
    }

    const need = if (bpdu.bpdu_type == BPDU_TYPE_RST) STP_RST_SIZE else STP_CONFIG_SIZE;
    if (out.len < need) {
        return null;
    }

    writeU16Be(out, 0, STP_PROTOCOL_ID);
    out[2] = bpdu.version;
    out[3] = bpdu.bpdu_type;
    out[4] = bpdu.flags;
    if (!encodeBridgeId(bpdu.root_id, out[5..13])) {
        return null;
    }
    writeU32Be(out, 13, bpdu.root_path_cost);
    if (!encodeBridgeId(bpdu.bridge_id, out[17..25])) {
        return null;
    }
    writeU16Be(out, 25, bpdu.port_id);
    writeU16Be(out, 27, bpdu.message_age_ticks);
    writeU16Be(out, 29, bpdu.max_age_ticks);
    writeU16Be(out, 31, bpdu.hello_time_ticks);
    writeU16Be(out, 33, bpdu.forward_delay_ticks);

    if (bpdu.bpdu_type == BPDU_TYPE_RST) {
        out[35] = bpdu.version_1_len;
        return STP_RST_SIZE;
    }
    return STP_CONFIG_SIZE;
}
