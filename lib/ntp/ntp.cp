// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5905 NTPv4 / SNTP Wire Codec & Clock Offset)

pub const NTP_UDP_PORT: u16 = 123;
pub const NTP_PACKET_SIZE: usize = 48;
pub const NTP_V3: u8 = 3;
pub const NTP_V4: u8 = 4;
pub const NTP_UNIX_EPOCH_DELTA: u64 = 2_208_988_800;

// Leap Indicator (LI)
pub const LI_NO_WARNING: u8 = 0;
pub const LI_61_SEC: u8 = 1;
pub const LI_59_SEC: u8 = 2;
pub const LI_ALARM: u8 = 3;

// Association Mode
pub const MODE_RESERVED: u8 = 0;
pub const MODE_SYMMETRIC_ACTIVE: u8 = 1;
pub const MODE_SYMMETRIC_PASSIVE: u8 = 2;
pub const MODE_CLIENT: u8 = 3;
pub const MODE_SERVER: u8 = 4;
pub const MODE_BROADCAST: u8 = 5;
pub const MODE_CONTROL: u8 = 6;
pub const MODE_PRIVATE: u8 = 7;

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

/// 64-bit RFC 5905 NTP timestamp (32-bit seconds since 1900-01-01 + 32-bit fractional second).
pub struct NtpTimestamp {
    seconds: u32,
    fraction: u32,

    fn zero(): NtpTimestamp {
        return NtpTimestamp{ .seconds = 0, .fraction = 0 };
    }

    fn isZero(): bool {
        return _.seconds == 0 && _.fraction == 0;
    }

    /// Converts Unix epoch milliseconds to an NTP 64-bit timestamp.
    fn fromUnixMs(ms: i64): NtpTimestamp {
        let sec_i64 = ms / 1000;
        let rem_ms = ms % 1000;
        if (rem_ms < 0) {
            rem_ms += 1000;
            sec_i64 -= 1;
        }
        const ntp_sec_i64 = sec_i64 + (NTP_UNIX_EPOCH_DELTA as i64);
        if (ntp_sec_i64 < 0) {
            return NtpTimestamp.zero();
        }
        const frac_u64 = (((rem_ms as u64) << 32) + 500) / 1000;
        return NtpTimestamp{
            .seconds = ntp_sec_i64 as u32,
            .fraction = frac_u64 as u32,
        };
    }

    /// Converts NTP 64-bit timestamp to Unix epoch nanoseconds (signed i128).
    fn toUnixNs(): i128 {
        const unix_sec: i128 = (_.seconds as i128) - (NTP_UNIX_EPOCH_DELTA as i128);
        const frac_ns: i128 = ((_.fraction as i128) * 1_000_000_000) >> 32;
        return unix_sec * 1_000_000_000 + frac_ns;
    }

    /// Converts NTP 64-bit timestamp to Unix epoch milliseconds (rounded).
    fn toUnixMs(): i64 {
        const unix_sec: i64 = (_.seconds as i64) - (NTP_UNIX_EPOCH_DELTA as i64);
        const half_lsb: u64 = 1 << 31;
        const frac_ms: i64 = ((((_.fraction as u64) * 1000) + half_lsb) >> 32) as i64;
        return unix_sec * 1000 + frac_ms;
    }
}

fn readTimestamp(buf: []const u8, off: usize): NtpTimestamp {
    return NtpTimestamp{
        .seconds = readU32Be(buf, off),
        .fraction = readU32Be(buf, off + 4),
    };
}

fn writeTimestamp(buf: []u8, off: usize, ts: NtpTimestamp) {
    writeU32Be(buf, off, ts.seconds);
    writeU32Be(buf, off + 4, ts.fraction);
}

pub struct NtpPacket {
    li: u8,
    version: u8,
    mode: u8,
    stratum: u8,
    poll: i8,
    precision: i8,
    root_delay: u32,
    root_dispersion: u32,
    ref_id: [4]u8,
    ref_ts: NtpTimestamp,
    orig_ts: NtpTimestamp,
    rx_ts: NtpTimestamp,
    tx_ts: NtpTimestamp,

    /// Stratum 0 indicates unspecified or Kiss-o'-Death (KoD) packet.
    fn isKissOfDeath(): bool {
        return _.stratum == 0;
    }

    /// Stratum 1 indicates a primary reference server (e.g., GPS, NIST, ATOM).
    fn isPrimary(): bool {
        return _.stratum == 1;
    }

    /// Converts 16.16 fixed-point root_delay to milliseconds.
    fn rootDelayMs(): f64 {
        return (_.root_delay as f64) * 1000.0 / 65536.0;
    }

    /// Converts 16.16 fixed-point root_dispersion to milliseconds.
    fn rootDispersionMs(): f64 {
        return (_.root_dispersion as f64) * 1000.0 / 65536.0;
    }
}

/// Fast validation checking len >= 48, version in 1..4, and mode in 1..7.
pub fn isNtp(data: []const u8): bool {
    if (data.len < NTP_PACKET_SIZE) {
        return false;
    }
    const b0 = data[0];
    const ver = (b0 >> 3) & 0x07;
    const mode = b0 & 0x07;
    return ver >= 1 && ver <= 4 && mode >= 1 && mode <= 7;
}

/// Parses a 48-byte RFC 5905 NTPv1-v4 / SNTP packet.
pub fn parseNtp(data: []const u8): NtpPacket? {
    if (!isNtp(data)) {
        return null;
    }
    const b0 = data[0];
    const ref_id: [4]u8 = [data[12], data[13], data[14], data[15]];
    return NtpPacket{
        .li = (b0 >> 6) & 0x03,
        .version = (b0 >> 3) & 0x07,
        .mode = b0 & 0x07,
        .stratum = data[1],
        .poll = data[2] as i8,
        .precision = data[3] as i8,
        .root_delay = readU32Be(data, 4),
        .root_dispersion = readU32Be(data, 8),
        .ref_id = ref_id,
        .ref_ts = readTimestamp(data, 16),
        .orig_ts = readTimestamp(data, 24),
        .rx_ts = readTimestamp(data, 32),
        .tx_ts = readTimestamp(data, 40),
    };
}

/// Encodes an NtpPacket into a 48-byte buffer. Returns 48 on success, null if out.len < 48.
pub fn encodeNtp(pkt: NtpPacket, out: []u8): usize? {
    if (out.len < NTP_PACKET_SIZE) {
        return null;
    }
    out[0] = (((pkt.li & 0x03) << 6) | ((pkt.version & 0x07) << 3) | (pkt.mode & 0x07)) as u8;
    out[1] = pkt.stratum;
    out[2] = pkt.poll as u8;
    out[3] = pkt.precision as u8;
    writeU32Be(out, 4, pkt.root_delay);
    writeU32Be(out, 8, pkt.root_dispersion);
    out[12] = pkt.ref_id[0];
    out[13] = pkt.ref_id[1];
    out[14] = pkt.ref_id[2];
    out[15] = pkt.ref_id[3];
    writeTimestamp(out, 16, pkt.ref_ts);
    writeTimestamp(out, 24, pkt.orig_ts);
    writeTimestamp(out, 32, pkt.rx_ts);
    writeTimestamp(out, 40, pkt.tx_ts);
    return NTP_PACKET_SIZE;
}

/// Constructs a standard NTPv4 client request packet with the given transmit timestamp.
pub fn makeClientRequest(tx_ts: NtpTimestamp): NtpPacket {
    return NtpPacket{
        .li = LI_NO_WARNING,
        .version = NTP_V4,
        .mode = MODE_CLIENT,
        .stratum = 0,
        .poll = 6,
        .precision = -20,
        .root_delay = 0,
        .root_dispersion = 0,
        .ref_id = [0, 0, 0, 0],
        .ref_ts = NtpTimestamp.zero(),
        .orig_ts = NtpTimestamp.zero(),
        .rx_ts = NtpTimestamp.zero(),
        .tx_ts = tx_ts,
    };
}

/// RFC 5905 clock offset theta = ((T2 - T1) + (T3 - T4)) / 2 in nanoseconds.
pub fn calcOffsetNs(t1: NtpTimestamp, t2: NtpTimestamp, t3: NtpTimestamp, t4: NtpTimestamp): i128 {
    const ns1 = t1.toUnixNs();
    const ns2 = t2.toUnixNs();
    const ns3 = t3.toUnixNs();
    const ns4 = t4.toUnixNs();
    return ((ns2 - ns1) + (ns3 - ns4)) / 2;
}

/// RFC 5905 round-trip delay delta = (T4 - T1) - (T3 - T2) in nanoseconds.
pub fn calcDelayNs(t1: NtpTimestamp, t2: NtpTimestamp, t3: NtpTimestamp, t4: NtpTimestamp): i128 {
    const ns1 = t1.toUnixNs();
    const ns2 = t2.toUnixNs();
    const ns3 = t3.toUnixNs();
    const ns4 = t4.toUnixNs();
    return (ns4 - ns1) - (ns3 - ns2);
}
