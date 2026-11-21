// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2131/2132 DHCPv4 Wire Codec)
// Pure CP zero-heap-allocation RFC 2131 / RFC 2132 DHCPv4 wire protocol codec:
// 240-byte BOOTP/DHCP header with Magic Cookie verification, PAD-skipping / END-terminating
// Option iterator, typed Option accessors, and zero-allocation DhcpWriter encoder.

/// Standard DHCPv4 server UDP port (`67`).
pub const DHCP_SERVER_PORT: u16 = 67;

/// Standard DHCPv4 client UDP port (`68`).
pub const DHCP_CLIENT_PORT: u16 = 68;

/// Fixed BOOTP/DHCP header size before Magic Cookie (`236` bytes).
pub const DHCP_FIXED_HEADER_SIZE: usize = 236;

/// Minimum DHCPv4 packet size including 4-byte Magic Cookie (`240` bytes).
pub const DHCP_MIN_PACKET_SIZE: usize = 240;

/// RFC 2131 DHCP Magic Cookie (`0x63825363`).
pub const DHCP_MAGIC_COOKIE: u32 = 0x63825363;

/// BOOTP message op codes.
pub const DHCP_OP_REQUEST: u8 = 1;
pub const DHCP_OP_REPLY: u8 = 2;

/// BOOTP Broadcast flag bitmask (`0x8000`).
pub const DHCP_FLAG_BROADCAST: u16 = 0x8000;

// Common RFC 2132 DHCP Option Codes
pub const OPT_PAD: u8 = 0;
pub const OPT_SUBNET_MASK: u8 = 1;
pub const OPT_ROUTER: u8 = 3;
pub const OPT_DNS: u8 = 6;
pub const OPT_HOSTNAME: u8 = 12;
pub const OPT_DOMAIN_NAME: u8 = 15;
pub const OPT_REQUESTED_IP: u8 = 50;
pub const OPT_LEASE_TIME: u8 = 51;
pub const OPT_MSG_TYPE: u8 = 53;
pub const OPT_SERVER_ID: u8 = 54;
pub const OPT_PARAM_REQ_LIST: u8 = 55;
pub const OPT_MAX_MSG_SIZE: u8 = 57;
pub const OPT_RENEWAL_TIME: u8 = 58;
pub const OPT_REBINDING_TIME: u8 = 59;
pub const OPT_VENDOR_CLASS: u8 = 60;
pub const OPT_CLIENT_ID: u8 = 61;
pub const OPT_END: u8 = 255;

/// RFC 2132 Option 53 DHCP Message Types.
pub enum DhcpMsgType: u8 {
    discover = 1,
    offer = 2,
    request = 3,
    decline = 4,
    ack = 5,
    nak = 6,
    release = 7,
    inform = 8,
    unknown = 255,
}

/// Decoded fixed BOOTP/DHCPv4 header fields.
pub struct DhcpHeader {
    op: u8 = DHCP_OP_REQUEST,
    htype: u8 = 1,
    hlen: u8 = 6,
    hops: u8 = 0,
    xid: u32 = 0,
    secs: u16 = 0,
    flags: u16 = 0,
    ciaddr: [4]u8 = [0; 4],
    yiaddr: [4]u8 = [0; 4],
    siaddr: [4]u8 = [0; 4],
    giaddr: [4]u8 = [0; 4],
    chaddr: [16]u8 = [0; 16],

    fn isBroadcast(): bool {
        return (_.flags & DHCP_FLAG_BROADCAST) != 0;
    }

    /// Returns the leading 6-byte Ethernet MAC address from `chaddr`.
    fn macAddr(): [6]u8 {
        return [
            _.chaddr[0],
            _.chaddr[1],
            _.chaddr[2],
            _.chaddr[3],
            _.chaddr[4],
            _.chaddr[5],
        ];
    }
}

/// Zero-copy decoded RFC 2132 DHCP Option TLV.
pub struct DhcpOption {
    code: u8 = 0,
    value: []const u8 = "",
}

/// Zero-allocation iterator over RFC 2132 DHCP options.
/// Automatically skips `OPT_PAD (0)`, stops cleanly at `OPT_END (255)`, and flags truncated options.
pub struct DhcpOptIter {
    data: []const u8 = "",
    pos: usize = 0,
    seen_end: bool = false,
    failed: bool = false,

    pub fn init(options_bytes: []const u8): DhcpOptIter {
        return DhcpOptIter {
            data: options_bytes,
            pos: 0,
            seen_end: false,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn hasEndOption(): bool {
        return _.seen_end;
    }

    fn next(*_): DhcpOption? {
        if (_.failed || _.seen_end) {
            return null;
        }
        while (_.pos < _.data.len) {
            const code = _.data[_.pos];
            if (code == OPT_PAD) {
                _.pos += 1;
                continue;
            }
            if (code == OPT_END) {
                _.pos += 1;
                _.seen_end = true;
                return null;
            }
            if (_.data.len - _.pos < 2) {
                _.failed = true;
                return null;
            }
            const opt_len = _.data[_.pos + 1] as usize;
            if (_.data.len - (_.pos + 2) < opt_len) {
                _.failed = true;
                return null;
            }
            const val = _.data[(_.pos + 2)..(_.pos + 2 + opt_len)];
            _.pos += 2 + opt_len;
            return DhcpOption {
                code: code,
                value: val,
            };
        }
        return null;
    }
}

/// Zero-copy decoded DHCPv4 packet.
pub struct DhcpPacket {
    header: DhcpHeader = DhcpHeader {},
    sname: []const u8 = "",
    boot_file: []const u8 = "",
    options_bytes: []const u8 = "",

    fn options(): DhcpOptIter {
        return DhcpOptIter.init(_.options_bytes);
    }

    fn findOption(code: u8): DhcpOption? {
        let it = _.options();
        while (opt := it.next()) {
            if (opt.code == code) {
                return opt;
            }
        }
        return null;
    }

    fn msgType(): DhcpMsgType {
        if (opt := _.findOption(OPT_MSG_TYPE)) {
            if (opt.value.len == 1) {
                return classifyMsgType(opt.value[0]);
            }
        }
        return DhcpMsgType.unknown;
    }

    fn leaseTime(): u32? {
        if (opt := _.findOption(OPT_LEASE_TIME)) {
            if (opt.value.len == 4) {
                return readBeU32(opt.value, 0);
            }
        }
        return null;
    }

    fn subnetMask(): ([4]u8)? {
        if (opt := _.findOption(OPT_SUBNET_MASK)) {
            if (opt.value.len == 4) {
                return [opt.value[0], opt.value[1], opt.value[2], opt.value[3]];
            }
        }
        return null;
    }

    fn serverId(): ([4]u8)? {
        if (opt := _.findOption(OPT_SERVER_ID)) {
            if (opt.value.len == 4) {
                return [opt.value[0], opt.value[1], opt.value[2], opt.value[3]];
            }
        }
        return null;
    }

    fn requestedIp(): ([4]u8)? {
        if (opt := _.findOption(OPT_REQUESTED_IP)) {
            if (opt.value.len == 4) {
                return [opt.value[0], opt.value[1], opt.value[2], opt.value[3]];
            }
        }
        return null;
    }

    fn hostname(): ([]const u8)? {
        if (opt := _.findOption(OPT_HOSTNAME)) {
            return opt.value;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn writeBeU16(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn trimCString(field: []const u8): []const u8 {
    let i: usize = 0;
    while (i < field.len) {
        if (field[i] == 0) {
            return field[0..i];
        }
        i += 1;
    }
    return field;
}

fn classifyMsgType(raw: u8): DhcpMsgType {
    if (raw == 1) { return DhcpMsgType.discover; }
    if (raw == 2) { return DhcpMsgType.offer; }
    if (raw == 3) { return DhcpMsgType.request; }
    if (raw == 4) { return DhcpMsgType.decline; }
    if (raw == 5) { return DhcpMsgType.ack; }
    if (raw == 6) { return DhcpMsgType.nak; }
    if (raw == 7) { return DhcpMsgType.release; }
    if (raw == 8) { return DhcpMsgType.inform; }
    return DhcpMsgType.unknown;
}

fn msgTypeToByte(t: DhcpMsgType): u8 {
    if (t == DhcpMsgType.discover) { return 1; }
    if (t == DhcpMsgType.offer) { return 2; }
    if (t == DhcpMsgType.request) { return 3; }
    if (t == DhcpMsgType.decline) { return 4; }
    if (t == DhcpMsgType.ack) { return 5; }
    if (t == DhcpMsgType.nak) { return 6; }
    if (t == DhcpMsgType.release) { return 7; }
    if (t == DhcpMsgType.inform) { return 8; }
    return 0;
}

// ---------------------------------------------------------------------------
// Public DHCPv4 Parser
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 2131 DHCPv4 packet.
/// Validates `data.len >= 240`, `op in 1..2`, `hlen <= 16`, and Magic Cookie `0x63825363`.
pub fn parseDhcp(data: []const u8): DhcpPacket? {
    if (data.len < DHCP_MIN_PACKET_SIZE) {
        return null;
    }
    const op = data[0];
    if (op != DHCP_OP_REQUEST && op != DHCP_OP_REPLY) {
        return null;
    }
    const htype = data[1];
    const hlen = data[2];
    if (hlen > 16) {
        return null;
    }
    const hops = data[3];
    if (readBeU32(data, DHCP_FIXED_HEADER_SIZE) != DHCP_MAGIC_COOKIE) {
        return null;
    }

    let chaddr: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        chaddr[i] = data[28 + i];
        i += 1;
    }

    return DhcpPacket {
        header: DhcpHeader {
            op: op,
            htype: htype,
            hlen: hlen,
            hops: hops,
            xid: readBeU32(data, 4),
            secs: readBeU16(data, 8),
            flags: readBeU16(data, 10),
            ciaddr: [data[12], data[13], data[14], data[15]],
            yiaddr: [data[16], data[17], data[18], data[19]],
            siaddr: [data[20], data[21], data[22], data[23]],
            giaddr: [data[24], data[25], data[26], data[27]],
            chaddr: chaddr,
        },
        sname: trimCString(data[44..108]),
        boot_file: trimCString(data[108..236]),
        options_bytes: data[DHCP_MIN_PACKET_SIZE..data.len],
    };
}

/// Returns `true` if `data` starts with a structurally valid RFC 2131 DHCPv4 packet.
pub fn isDhcp(data: []const u8): bool {
    if (p := parseDhcp(data)) {
        * = p;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Zero-Alloc DHCPv4 Writer
// ---------------------------------------------------------------------------

/// Zero-allocation RFC 2131/2132 DHCPv4 packet encoder writing into a caller-supplied buffer.
pub struct DhcpWriter {
    buf: []u8,
    pos: usize = 0,
    failed: bool = false,

    /// Initializes the 240-byte BOOTP/DHCP header (with Ethernet htype=1/hlen=6 and Magic Cookie).
    pub fn init(buf: []u8, op: u8, xid: u32, mac: [6]u8, broadcast: bool): DhcpWriter {
        if (buf.len < DHCP_MIN_PACKET_SIZE || (op != DHCP_OP_REQUEST && op != DHCP_OP_REPLY)) {
            return DhcpWriter {
                buf: buf,
                pos: 0,
                failed: true,
            };
        }
        let i: usize = 0;
        while (i < DHCP_MIN_PACKET_SIZE) {
            buf[i] = 0;
            i += 1;
        }
        buf[0] = op;
        buf[1] = 1; // Ethernet
        buf[2] = 6; // MAC length
        writeBeU32(buf, 4, xid);
        if (broadcast) {
            writeBeU16(buf, 10, DHCP_FLAG_BROADCAST);
        }
        let m: usize = 0;
        while (m < 6) {
            buf[28 + m] = mac[m];
            m += 1;
        }
        writeBeU32(buf, DHCP_FIXED_HEADER_SIZE, DHCP_MAGIC_COOKIE);

        return DhcpWriter {
            buf: buf,
            pos: DHCP_MIN_PACKET_SIZE,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn setCiaddr(*_, ip: [4]u8) {
        if (!_.failed) {
            _.buf[12] = ip[0]; _.buf[13] = ip[1]; _.buf[14] = ip[2]; _.buf[15] = ip[3];
        }
    }

    fn setYiaddr(*_, ip: [4]u8) {
        if (!_.failed) {
            _.buf[16] = ip[0]; _.buf[17] = ip[1]; _.buf[18] = ip[2]; _.buf[19] = ip[3];
        }
    }

    fn setSiaddr(*_, ip: [4]u8) {
        if (!_.failed) {
            _.buf[20] = ip[0]; _.buf[21] = ip[1]; _.buf[22] = ip[2]; _.buf[23] = ip[3];
        }
    }

    fn setGiaddr(*_, ip: [4]u8) {
        if (!_.failed) {
            _.buf[24] = ip[0]; _.buf[25] = ip[1]; _.buf[26] = ip[2]; _.buf[27] = ip[3];
        }
    }

    /// Appends a single `OPT_PAD (0)` byte.
    fn writePad(*_): bool {
        if (_.failed || _.buf.len - _.pos < 1) {
            _.failed = true;
            return false;
        }
        _.buf[_.pos] = OPT_PAD;
        _.pos += 1;
        return true;
    }

    /// Appends a standard TLV DHCP option (`code` != 0, 255; `value.len <= 255`).
    fn writeOption(*_, code: u8, value: []const u8): bool {
        if (_.failed || code == OPT_PAD || code == OPT_END || value.len > 255) {
            _.failed = true;
            return false;
        }
        const need = 2 + value.len;
        if (_.buf.len - _.pos < need) {
            _.failed = true;
            return false;
        }
        _.buf[_.pos] = code;
        _.buf[_.pos + 1] = value.len as u8;
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + 2 + i] = value[i];
            i += 1;
        }
        _.pos += need;
        return true;
    }

    /// Appends Option 53 (`OPT_MSG_TYPE`).
    fn writeMsgType(*_, msg_type: DhcpMsgType): bool {
        const b = msgTypeToByte(msg_type);
        if (b == 0) {
            _.failed = true;
            return false;
        }
        let v: [1]u8 = [b];
        return _.writeOption(OPT_MSG_TYPE, v[0..1]);
    }

    /// Appends a 4-byte big-endian `u32` option (e.g. `OPT_LEASE_TIME`).
    fn writeU32Option(*_, code: u8, val: u32): bool {
        let v: [4]u8 = [0; 4];
        writeBeU32(v[0..4], 0, val);
        return _.writeOption(code, v[0..4]);
    }

    /// Appends a 4-byte IPv4 address option (e.g. `OPT_SUBNET_MASK`, `OPT_SERVER_ID`, `OPT_REQUESTED_IP`).
    fn writeIpOption(*_, code: u8, ip: [4]u8): bool {
        return _.writeOption(code, ip[0..4]);
    }

    /// Appends `OPT_END (255)` and returns the finalized DHCPv4 packet slice.
    fn finish(*_): ([]const u8)? {
        if (_.failed || _.buf.len - _.pos < 1) {
            _.failed = true;
            return null;
        }
        _.buf[_.pos] = OPT_END;
        _.pos += 1;
        return _.buf[0.._.pos];
    }
}
