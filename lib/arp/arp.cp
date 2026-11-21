// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 826 / RFC 5227 ARP Packet Codec)

pub const ARP_ETHERTYPE: u16 = 0x0806;
pub const ARP_MIN_HEADER_SIZE: usize = 8;
pub const ARP_IPV4_PACKET_SIZE: usize = 28;

pub const ARP_HTYPE_ETHERNET: u16 = 1;
pub const ARP_PTYPE_IPV4: u16 = 0x0800;
pub const ARP_HLEN_ETHERNET: u8 = 6;
pub const ARP_PLEN_IPV4: u8 = 4;

pub enum ArpOp: u16 {
    unknown = 0,
    request = 1,
    reply = 2,
    rarp_request = 3,
    rarp_reply = 4,
    inarp_request = 8,
    inarp_reply = 9,
}

pub fn arpOpFromU16(v: u16): ArpOp {
    if (v == 1) return ArpOp.request;
    if (v == 2) return ArpOp.reply;
    if (v == 3) return ArpOp.rarp_request;
    if (v == 4) return ArpOp.rarp_reply;
    if (v == 8) return ArpOp.inarp_request;
    if (v == 9) return ArpOp.inarp_reply;
    return ArpOp.unknown;
}

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn isZeroIp(ip: [4]u8): bool {
    return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

fn ipsEqual(a: [4]u8, b: [4]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

pub struct ArpPacket {
    htype: u16,
    ptype: u16,
    hlen: u8,
    plen: u8,
    op: ArpOp,
    raw_op: u16,
    sender_mac: [6]u8,
    sender_ip: [4]u8,
    target_mac: [6]u8,
    target_ip: [4]u8,

    fn isRequest(): bool {
        return _.op == ArpOp.request;
    }

    fn isReply(): bool {
        return _.op == ArpOp.reply;
    }

    /// RFC 5227 Section 2.1.1: ARP Probe has op == request and sender IP == 0.0.0.0.
    fn isProbe(): bool {
        return _.op == ArpOp.request && isZeroIp(_.sender_ip);
    }

    /// RFC 5227 / Gratuitous ARP: sender IP == target IP and sender IP != 0.0.0.0.
    fn isGratuitous(): bool {
        return ipsEqual(_.sender_ip, _.target_ip) && !isZeroIp(_.sender_ip);
    }

    /// RFC 5227 Section 2.3: ARP Announcement is a gratuitous ARP request.
    fn isAnnouncement(): bool {
        return _.op == ArpOp.request && _.isGratuitous();
    }
}

pub struct ArpRawHeader {
    htype: u16,
    ptype: u16,
    hlen: u8,
    plen: u8,
    op: ArpOp,
    raw_op: u16,
    sha: []const u8,
    spa: []const u8,
    tha: []const u8,
    tpa: []const u8,
}

/// Fast check verifying an Ethernet (htype=1, hlen=6) + IPv4 (ptype=0x0800, plen=4) 28-byte ARP packet.
pub fn isArpIpv4(data: []const u8): bool {
    if (data.len < ARP_IPV4_PACKET_SIZE) {
        return false;
    }
    return readU16Be(data, 0) == ARP_HTYPE_ETHERNET &&
        readU16Be(data, 2) == ARP_PTYPE_IPV4 &&
        data[4] == ARP_HLEN_ETHERNET &&
        data[5] == ARP_PLEN_IPV4;
}

/// Parses a standard 28-byte Ethernet + IPv4 ARP packet.
pub fn parseArpIpv4(data: []const u8): ArpPacket? {
    if (!isArpIpv4(data)) {
        return null;
    }
    const raw_op = readU16Be(data, 6);
    const smac: [6]u8 = [data[8], data[9], data[10], data[11], data[12], data[13]];
    const sip: [4]u8 = [data[14], data[15], data[16], data[17]];
    const tmac: [6]u8 = [data[18], data[19], data[20], data[21], data[22], data[23]];
    const tip: [4]u8 = [data[24], data[25], data[26], data[27]];

    return ArpPacket{
        .htype = ARP_HTYPE_ETHERNET,
        .ptype = ARP_PTYPE_IPV4,
        .hlen = ARP_HLEN_ETHERNET,
        .plen = ARP_PLEN_IPV4,
        .op = arpOpFromU16(raw_op),
        .raw_op = raw_op,
        .sender_mac = smac,
        .sender_ip = sip,
        .target_mac = tmac,
        .target_ip = tip,
    };
}

/// Generic variable-length ARP parser supporting arbitrary hlen/plen with strict bounds checking.
pub fn parseArpRaw(data: []const u8): ArpRawHeader? {
    if (data.len < ARP_MIN_HEADER_SIZE) {
        return null;
    }
    const htype = readU16Be(data, 0);
    const ptype = readU16Be(data, 2);
    const hlen = data[4];
    const plen = data[5];
    const raw_op = readU16Be(data, 6);

    const hlen_u = hlen as usize;
    const plen_u = plen as usize;
    const total = ARP_MIN_HEADER_SIZE + hlen_u * 2 + plen_u * 2;
    if (data.len < total) {
        return null;
    }

    const sha_start = ARP_MIN_HEADER_SIZE;
    const spa_start = sha_start + hlen_u;
    const tha_start = spa_start + plen_u;
    const tpa_start = tha_start + hlen_u;
    const end = tpa_start + plen_u;

    return ArpRawHeader{
        .htype = htype,
        .ptype = ptype,
        .hlen = hlen,
        .plen = plen,
        .op = arpOpFromU16(raw_op),
        .raw_op = raw_op,
        .sha = data[sha_start..spa_start],
        .spa = data[spa_start..tha_start],
        .tha = data[tha_start..tpa_start],
        .tpa = data[tpa_start..end],
    };
}

/// Encodes a 28-byte Ethernet + IPv4 ARP packet into `out`.
pub fn encodeArpIpv4(
    op: ArpOp,
    sender_mac: [6]u8,
    sender_ip: [4]u8,
    target_mac: [6]u8,
    target_ip: [4]u8,
    out: []u8,
): usize? {
    if (out.len < ARP_IPV4_PACKET_SIZE) {
        return null;
    }
    writeU16Be(out, 0, ARP_HTYPE_ETHERNET);
    writeU16Be(out, 2, ARP_PTYPE_IPV4);
    out[4] = ARP_HLEN_ETHERNET;
    out[5] = ARP_PLEN_IPV4;
    writeU16Be(out, 6, op as u16);

    let i: usize = 0;
    while (i < 6) {
        out[8 + i] = sender_mac[i];
        out[18 + i] = target_mac[i];
        i += 1;
    }
    let j: usize = 0;
    while (j < 4) {
        out[14 + j] = sender_ip[j];
        out[24 + j] = target_ip[j];
        j += 1;
    }
    return ARP_IPV4_PACKET_SIZE;
}

/// Encodes a standard ARP Request (target_mac = 00:00:00:00:00:00).
pub fn encodeArpRequest(sender_mac: [6]u8, sender_ip: [4]u8, target_ip: [4]u8, out: []u8): usize? {
    const zero_mac: [6]u8 = [0, 0, 0, 0, 0, 0];
    return encodeArpIpv4(ArpOp.request, sender_mac, sender_ip, zero_mac, target_ip, out);
}

/// Encodes a standard ARP Reply.
pub fn encodeArpReply(
    sender_mac: [6]u8,
    sender_ip: [4]u8,
    target_mac: [6]u8,
    target_ip: [4]u8,
    out: []u8,
): usize? {
    return encodeArpIpv4(ArpOp.reply, sender_mac, sender_ip, target_mac, target_ip, out);
}

/// Encodes an RFC 5227 ARP Probe (op = request, sender_ip = 0.0.0.0, target_mac = 00:00:00:00:00:00).
pub fn encodeArpProbe(sender_mac: [6]u8, target_ip: [4]u8, out: []u8): usize? {
    const zero_mac: [6]u8 = [0, 0, 0, 0, 0, 0];
    const zero_ip: [4]u8 = [0, 0, 0, 0];
    return encodeArpIpv4(ArpOp.request, sender_mac, zero_ip, zero_mac, target_ip, out);
}

/// Encodes an RFC 5227 Gratuitous ARP Announcement (op = request, sender_ip = target_ip = ip).
pub fn encodeGratuitousArp(sender_mac: [6]u8, ip: [4]u8, out: []u8): usize? {
    const zero_mac: [6]u8 = [0, 0, 0, 0, 0, 0];
    return encodeArpIpv4(ArpOp.request, sender_mac, ip, zero_mac, ip, out);
}
