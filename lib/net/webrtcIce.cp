// Origin: Pure-CP Stdlib (RFC 8489 STUN Wire Codec, RFC 8445 ICE Pairing/Checklist & RFC 8866/9725 WebRTC SDP/WHEP) | Created: 2026-11-23
// Reference-hardened against algesten/str0m (MIT/Apache-2.0)
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice WebRTC / WHEP signaling & NAT traversal engine:
//   - RFC 8489 STUN binary wire codec: 20-byte header, 4-byte padded TLV attributes, XOR-MAPPED-ADDRESS (IPv4 & IPv6),
//     PRIORITY, USE-CANDIDATE, ICE-CONTROLLING, ICE-CONTROLLED, USERNAME, ERROR-CODE (class/number/reason),
//     HMAC-SHA1 MESSAGE-INTEGRITY (RFC 8489 §14.5) and IEEE CRC32 FINGERPRINT (RFC 8489 §14.7)
//   - RFC 8445 ICE candidate priority (`2^24*type + 2^8*local + (256-comp)`), pair priority (`2^32*min + 2*max + (G>D)`),
//     SDP `candidate:` parser (IPv4/IPv6/mDNS `.local`, `raddr`/`rport`, `tcptype`, `generation`, `ufrag`),
//     487 Role Conflict tie-breaker resolution, O(N log N) heapsort pairing, nomination selection & `IceCheckList`
//   - RFC 8866 / WebRTC SDP parser (`ice-ufrag`, `ice-pwd`, `fingerprint:sha-256`, `group:BUNDLE`, `mid`, `setup`,
//     `rtcp-mux`, `rtpmap` with 7-bit PT guard, `fmtp`, `extmap`, `rtcp-fb`, `ssrc`, media directions),
//     Offer/Answer codec intersection, and RFC 9725 WHEP HTTP/1.1 POST formatter

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// RFC 8489 §5 fixed STUN magic cookie (`0x2112A442`).
pub const STUN_MAGIC_COOKIE: u32 = 0x2112A442;

/// RFC 8489 §5 fixed STUN message header size in bytes.
pub const STUN_HEADER_LEN: usize = 20;

/// RFC 8489 §14 STUN attribute TLV header size in bytes (2-byte type + 2-byte length).
pub const STUN_ATTR_HEADER_LEN: usize = 4;

/// RFC 8489 §14.7 FINGERPRINT XOR mask (`"STUN"` in ASCII: `0x5354554E`).
pub const STUN_FINGERPRINT_XOR: u32 = 0x5354554E;

/// RFC 8489 §5 STUN transaction ID length in bytes (96 bits).
pub const STUN_TX_ID_LEN: usize = 12;

/// RFC 8445 §5.1.2.2 recommended type preference for host candidates.
pub const ICE_PRIORITY_HOST: u32 = 126;

/// RFC 8445 §5.1.2.2 recommended type preference for peer-reflexive candidates.
pub const ICE_PRIORITY_PRFLX: u32 = 110;

/// RFC 8445 §5.1.2.2 recommended type preference for server-reflexive candidates.
pub const ICE_PRIORITY_SRFLX: u32 = 100;

/// RFC 8445 §5.1.2.2 recommended type preference for relayed candidates.
pub const ICE_PRIORITY_RELAY: u32 = 0;

/// RFC 8445 maximum local preference value (`65535`).
pub const ICE_LOCAL_PREF_MAX: u16 = 65535;

/// Default RTP component ID (`1`).
pub const ICE_COMPONENT_RTP: u16 = 1;

/// Default RTCP component ID (`2`).
pub const ICE_COMPONENT_RTCP: u16 = 2;

const STUN_ADDR_FAMILY_IPV4: u8 = 0x01;
const STUN_ADDR_FAMILY_IPV6: u8 = 0x02;
const STUN_FINGERPRINT_ATTR_WIRE_LEN: usize = 8; // 4-byte header + 4-byte CRC32 value
const STUN_MI_ATTR_WIRE_LEN: usize = 24; // 4-byte header + 20-byte HMAC-SHA1 value

/// Error set returned by STUN, ICE, SDP, and WHEP operations.
pub err WebRtcIceError {
    PacketTooShort,
    InvalidMagicCookie,
    InvalidMessageLength,
    InvalidAttribute,
    InvalidFingerprint,
    BufferTooSmall,
    InvalidCandidate,
    InvalidSdp,
    OutputSliceTooSmall,
}

// ============================================================================
// 2. Internal Byte, IEEE CRC32, FNV-1a & ASCII Helpers
// ============================================================================

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32);
}

fn readBeU64(buf: []const u8, off: usize): u64 {
    const hi: u64 = readBeU32(buf, off) as u64;
    const lo: u64 = readBeU32(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeBeU64(buf: []u8, off: usize, v: u64) {
    writeBeU32(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeBeU32(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn alignUp4(n: usize): usize {
    return (n + 3) & (~@as(usize, 3));
}

/// Computes standard IEEE 802.3 CRC-32 (reflected polynomial `0xEDB88320`).
pub fn computeStunCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i] as u32;
        let bit: u32 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes RFC 8489 §14.7 STUN FINGERPRINT value (`CRC32(prefix) ^ 0x5354554E`).
pub fn computeStunFingerprint(header_and_attrs: []const u8): u32 {
    return computeStunCrc32(header_and_attrs) ^ STUN_FINGERPRINT_XOR;
}

fn sha1Rotl(x: u32, n: u5): u32 {
    const inv: u5 = @as(u5, @intCast(32 - @as(u32, n)));
    return (x << n) | (x >> inv);
}

struct Sha1State {
    h: [5]u32 = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];
    buf: [64]u8 = [0; 64];
    buf_len: usize = 0;
    total_len: u64 = 0;

    fn compress(*_, block: []const u8) {
        let w: [80]u32 = [0; 80];
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            w[i] = readBeU32(block, i * 4);
        }
        while (i < 80) : (i += 1) {
            w[i] = sha1Rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }
        let a: u32 = _.h[0];
        let b: u32 = _.h[1];
        let c: u32 = _.h[2];
        let d: u32 = _.h[3];
        let e: u32 = _.h[4];
        i = 0;
        while (i < 80) : (i += 1) {
            let f: u32 = 0;
            let k: u32 = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            const temp: u32 = sha1Rotl(a, 5) +% f +% e +% k +% w[i];
            e = d;
            d = c;
            c = sha1Rotl(b, 30);
            b = a;
            a = temp;
        }
        _.h[0] +%= a;
        _.h[1] +%= b;
        _.h[2] +%= c;
        _.h[3] +%= d;
        _.h[4] +%= e;
    }

    fn update(*_, data: []const u8) {
        let off: usize = 0;
        _.total_len += data.len as u64;
        if (_.buf_len > 0) {
            const need: usize = @as(usize, 64) - _.buf_len;
            const take: usize = if (data.len < need) data.len else need;
            let k: usize = 0;
            while (k < take) : (k += 1) {
                _.buf[_.buf_len + k] = data[k];
            }
            _.buf_len += take;
            off += take;
            if (_.buf_len == 64) {
                _.compress(_.buf[0..64]);
                _.buf_len = 0;
            }
        }
        while (off + 64 <= data.len) : (off += 64) {
            _.compress(data[off..(off + 64)]);
        }
        while (off < data.len) : (off += 1) {
            _.buf[_.buf_len] = data[off];
            _.buf_len += 1;
        }
    }

    fn finish(*_): [20]u8 {
        const total_bits: u64 = _.total_len * 8;
        _.buf[_.buf_len] = 0x80;
        _.buf_len += 1;
        if (_.buf_len > 56) {
            while (_.buf_len < 64) : (_.buf_len += 1) _.buf[_.buf_len] = 0;
            _.compress(_.buf[0..64]);
            _.buf_len = 0;
        }
        while (_.buf_len < 56) : (_.buf_len += 1) _.buf[_.buf_len] = 0;
        writeBeU64(_.buf[0..], 56, total_bits);
        _.compress(_.buf[0..64]);
        let out: [20]u8 = [0; 20];
        let i: usize = 0;
        while (i < 5) : (i += 1) {
            writeBeU32(out[0..], i * 4, _.h[i]);
        }
        return out;
    }
}

fn hmacSha1Parts(key: []const u8, p1: []const u8, p2: []const u8, p3: []const u8): [20]u8 {
    let kblock: [64]u8 = [0; 64];
    if (key.len > 64) {
        let kh = Sha1State{};
        kh.update(key);
        const kd = kh.finish();
        let i: usize = 0;
        while (i < 20) : (i += 1) kblock[i] = kd[i];
    } else {
        let i: usize = 0;
        while (i < key.len) : (i += 1) kblock[i] = key[i];
    }
    let ipad: [64]u8 = [0; 64];
    let opad: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        ipad[i] = kblock[i] ^ 0x36;
        opad[i] = kblock[i] ^ 0x5C;
    }
    let inner = Sha1State{};
    inner.update(ipad[0..]);
    if (p1.len > 0) inner.update(p1);
    if (p2.len > 0) inner.update(p2);
    if (p3.len > 0) inner.update(p3);
    const idig = inner.finish();

    let outer = Sha1State{};
    outer.update(opad[0..]);
    outer.update(idig[0..]);
    return outer.finish();
}

/// Computes RFC 8489 §14.5 STUN `MESSAGE-INTEGRITY` HMAC-SHA1 over `raw_packet[0..mi_attr_off]`
/// with the 16-bit header length field adjusted to `(mi_attr_off - 20) + 24`.
pub fn computeStunMessageIntegrity(raw_packet: []const u8, mi_attr_off: usize, key: []const u8): [20]u8 {
    if (mi_attr_off < STUN_HEADER_LEN || mi_attr_off > raw_packet.len) {
        return [0; 20];
    }
    let adjusted_hdr: [20]u8 = [0; 20];
    let i: usize = 0;
    while (i < STUN_HEADER_LEN) : (i += 1) {
        adjusted_hdr[i] = raw_packet[i];
    }
    const adj_body_len: usize = (mi_attr_off - STUN_HEADER_LEN) + STUN_MI_ATTR_WIRE_LEN;
    writeBeU16(adjusted_hdr[0..], 2, adj_body_len as u16);
    const empty: []const u8 = "";
    return hmacSha1Parts(key, adjusted_hdr[0..], raw_packet[STUN_HEADER_LEN..mi_attr_off], empty);
}

fn fnv1a32(data: []const u8): u32 {
    let h: u32 = 2166136261;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= data[i] as u32;
        h = h *% 16777619;
    }
    return h;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqBytes(s[0..prefix.len], prefix);
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    return eqIgnoreCase(s[(s.len - suffix.len)..s.len], suffix);
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseIpv6Bytes(s: []const u8): ([16]u8)? {
    if (s.len < 2 || s.len > 45) return null;
    let groups: [8]u16 = [0; 8];
    let head_cnt: usize = 0;
    let tail_groups: [8]u16 = [0; 8];
    let tail_cnt: usize = 0;
    let seen_double: bool = false;

    let i: usize = 0;
    if (s[0] == ':') {
        if (s.len < 2 || s[1] != ':') return null;
        seen_double = true;
        i = 2;
    }

    while (i < s.len) {
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < s.len && s[i] != ':') : (i += 1) {
            const hv = hexVal(s[i]) orelse return null;
            digits += 1;
            if (digits > 4) return null;
            val = (val << 4) | (hv as u32);
        }
        if (digits == 0) return null;
        if (!seen_double) {
            if (head_cnt >= 8) return null;
            groups[head_cnt] = val as u16;
            head_cnt += 1;
        } else {
            if (tail_cnt >= 8) return null;
            tail_groups[tail_cnt] = val as u16;
            tail_cnt += 1;
        }
        if (i == s.len) break;
        // s[i] == ':'
        i += 1;
        if (i == s.len) return null; // trailing single ':'
        if (s[i] == ':') {
            if (seen_double) return null; // only one '::' allowed
            seen_double = true;
            i += 1;
            if (i == s.len) break;
        }
    }

    if (seen_double) {
        if (head_cnt + tail_cnt >= 8) return null;
        const fill_zeros: usize = @as(usize, 8) - head_cnt - tail_cnt;
        let t: usize = 0;
        while (t < tail_cnt) : (t += 1) {
            groups[head_cnt + fill_zeros + t] = tail_groups[t];
        }
    } else {
        if (head_cnt != 8) return null;
    }

    let out: [16]u8 = [0; 16];
    let g: usize = 0;
    while (g < 8) : (g += 1) {
        out[g * 2] = ((groups[g] >> 8) & 0xFF) as u8;
        out[g * 2 + 1] = (groups[g] & 0xFF) as u8;
    }
    return out;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn hasCrOrLf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n') return true;
    }
    return false;
}

fn parseDecU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 0xFFFFFFFF) return null;
    }
    return acc as u32;
}

fn parseDecU16(s: []const u8): u16? {
    if (v := parseDecU32(s)) {
        if (v > 65535) return null;
        return v as u16;
    }
    return null;
}

fn parseDecU8(s: []const u8): u8? {
    if (v := parseDecU32(s)) {
        if (v > 255) return null;
        return v as u8;
    }
    return null;
}

fn parseIpv4Bytes(s: []const u8): ([4]u8)? {
    let out: [4]u8 = [0, 0, 0, 0];
    let octet: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) : (i += 1) {
        if (i == s.len || s[i] == '.') {
            if (octet >= 4) return null;
            const part = s[start..i];
            if (part.len == 0 || part.len > 3) return null;
            if (part.len > 1 && part[0] == '0') return null;
            if (v := parseDecU8(part)) {
                out[octet] = v;
            } else {
                return null;
            }
            octet += 1;
            start = i + 1;
        }
    }
    if (octet != 4) return null;
    return out;
}

fn appendBytes(out_buf: []u8, pos: usize, src: []const u8): usize ! WebRtcIceError {
    if (pos + src.len > out_buf.len) {
        return WebRtcIceError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out_buf[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendUsizeDec(out_buf: []u8, pos: usize, val: usize): usize ! WebRtcIceError {
    let tmp: [32]u8 = [0; 32];
    let n: usize = 0;
    if (val == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        let cur: usize = val;
        while (cur > 0) {
            tmp[n] = ('0' as usize + (cur % 10)) as u8;
            cur = cur / 10;
            n += 1;
        }
    }
    if (pos + n > out_buf.len) {
        return WebRtcIceError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    return pos + n;
}

// ============================================================================
// 3. RFC 8489 STUN Binary Wire Protocol Codec
// ============================================================================

/// RFC 8489 / RFC 8656 STUN message type codes.
pub enum StunMsgType {
    binding_request,
    binding_response,
    binding_error,
    allocate_request,
    refresh_request,

    /// Converts the STUN message type to its 16-bit big-endian wire code.
    fn toWireU16(): u16 {
        return match (_) {
            StunMsgType.binding_request => 0x0001,
            StunMsgType.binding_response => 0x0101,
            StunMsgType.binding_error => 0x0111,
            StunMsgType.allocate_request => 0x0003,
            StunMsgType.refresh_request => 0x0004,
        };
    }

    /// Decodes a 16-bit wire code to a recognized `StunMsgType`.
    static fn fromWireU16(v: u16): StunMsgType? {
        if (v == 0x0001) return StunMsgType.binding_request;
        if (v == 0x0101) return StunMsgType.binding_response;
        if (v == 0x0111) return StunMsgType.binding_error;
        if (v == 0x0003) return StunMsgType.allocate_request;
        if (v == 0x0004) return StunMsgType.refresh_request;
        return null;
    }
}

/// RFC 8489 / RFC 8445 STUN attribute type codes.
pub enum StunAttrType {
    mapped_address,
    username,
    message_integrity,
    error_code,
    xor_mapped_address,
    priority,
    use_candidate,
    ice_controlling,
    ice_controlled,
    fingerprint,

    /// Converts the STUN attribute type to its 16-bit big-endian wire code.
    fn toWireU16(): u16 {
        return match (_) {
            StunAttrType.mapped_address => 0x0001,
            StunAttrType.username => 0x0006,
            StunAttrType.message_integrity => 0x0008,
            StunAttrType.error_code => 0x0009,
            StunAttrType.xor_mapped_address => 0x0020,
            StunAttrType.priority => 0x0024,
            StunAttrType.use_candidate => 0x0025,
            StunAttrType.ice_controlling => 0x802A,
            StunAttrType.ice_controlled => 0x8029,
            StunAttrType.fingerprint => 0x8028,
        };
    }

    /// Decodes a 16-bit wire code to a recognized `StunAttrType`.
    static fn fromWireU16(v: u16): StunAttrType? {
        if (v == 0x0001) return StunAttrType.mapped_address;
        if (v == 0x0006) return StunAttrType.username;
        if (v == 0x0008) return StunAttrType.message_integrity;
        if (v == 0x0009) return StunAttrType.error_code;
        if (v == 0x0020) return StunAttrType.xor_mapped_address;
        if (v == 0x0024) return StunAttrType.priority;
        if (v == 0x0025) return StunAttrType.use_candidate;
        if (v == 0x802A) return StunAttrType.ice_controlling;
        if (v == 0x8029) return StunAttrType.ice_controlled;
        if (v == 0x8028) return StunAttrType.fingerprint;
        return null;
    }
}

/// IPv4 address + UDP/TCP port pair used by STUN MAPPED / XOR-MAPPED attributes.
pub struct StunIpv4Addr {
    ip: [4]u8 = [0, 0, 0, 0];
    port: u16 = 0;

    static fn init(a: u8, b: u8, c: u8, d: u8, port: u16): StunIpv4Addr {
        return StunIpv4Addr{
            ip: [a, b, c, d],
            port: port,
        };
    }

    fn eql(other: StunIpv4Addr): bool {
        return port == other.port
            && ip[0] == other.ip[0]
            && ip[1] == other.ip[1]
            && ip[2] == other.ip[2]
            && ip[3] == other.ip[3];
    }

    fn toU32(): u32 {
        return ((ip[0] as u32) << 24)
            | ((ip[1] as u32) << 16)
            | ((ip[2] as u32) << 8)
            | (ip[3] as u32);
    }
}

/// IPv6 address + UDP/TCP port pair used by STUN `XOR-MAPPED-ADDRESS` (family `0x02`).
pub struct StunIpv6Addr {
    ip: [16]u8 = [0; 16];
    port: u16 = 0;

    static fn init(ip: [16]u8, port: u16): StunIpv6Addr {
        return StunIpv6Addr{ ip: ip, port: port };
    }

    fn eql(other: StunIpv6Addr): bool {
        if (port != other.port) return false;
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (ip[i] != other.ip[i]) return false;
        }
        return true;
    }
}

/// Zero-allocation parsed representation of an RFC 8489 STUN packet.
pub struct StunPacket {
    msg_type: StunMsgType = StunMsgType.binding_request;
    msg_len: u16 = 0;
    tx_id: [12]u8 = [0; 12];
    has_mapped: bool = false;
    mapped: StunIpv4Addr = StunIpv4Addr{};
    has_xor_mapped: bool = false;
    xor_mapped: StunIpv4Addr = StunIpv4Addr{};
    has_xor_mapped_v6: bool = false;
    xor_mapped_v6: StunIpv6Addr = StunIpv6Addr{};
    has_priority: bool = false;
    priority: u32 = 0;
    use_candidate: bool = false;
    has_ice_controlling: bool = false;
    ice_controlling: u64 = 0;
    has_ice_controlled: bool = false;
    ice_controlled: u64 = 0;
    has_error_code: bool = false;
    error_code: u16 = 0;
    error_reason_off: usize = 0;
    error_reason_len: usize = 0;
    has_message_integrity: bool = false;
    mi_attr_off: usize = 0;
    message_integrity: [20]u8 = [0; 20];
    has_fingerprint: bool = false;
    fingerprint_valid: bool = false;
    username_off: usize = 0;
    username_len: usize = 0;
    attr_count: usize = 0;

    /// Returns the zero-copy `USERNAME` attribute slice from the original packet buffer.
    fn usernameSlice(raw: []const u8): []const u8 {
        if (username_len == 0 || username_off + username_len > raw.len) {
            return "";
        }
        return raw[username_off..(username_off + username_len)];
    }

    /// Returns the zero-copy `ERROR-CODE` reason phrase slice from the original packet buffer.
    fn errorReasonSlice(raw: []const u8): []const u8 {
        if (error_reason_len == 0 || error_reason_off + error_reason_len > raw.len) {
            return "";
        }
        return raw[error_reason_off..(error_reason_off + error_reason_len)];
    }

    /// Checks whether the packet's 96-bit transaction ID matches `expected`.
    fn matchesTxId(expected: [12]u8): bool {
        let i: usize = 0;
        while (i < STUN_TX_ID_LEN) : (i += 1) {
            if (tx_id[i] != expected[i]) return false;
        }
        return true;
    }
}

/// Verifies RFC 8489 §14.5 STUN `MESSAGE-INTEGRITY` HMAC-SHA1 against `key` in constant time.
pub fn verifyStunMessageIntegrity(raw: []const u8, pkt: StunPacket, key: []const u8): bool {
    if (!pkt.has_message_integrity || pkt.mi_attr_off < STUN_HEADER_LEN || pkt.mi_attr_off > raw.len) {
        return false;
    }
    const calc = computeStunMessageIntegrity(raw, pkt.mi_attr_off, key);
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        diff |= calc[i] ^ pkt.message_integrity[i];
    }
    return diff == 0;
}

fn writeStunHeader(out_buf: []u8, msg_type: StunMsgType, body_len: u16, tx_id: [12]u8): void ! WebRtcIceError {
    if (out_buf.len < STUN_HEADER_LEN) {
        return WebRtcIceError.BufferTooSmall;
    }
    writeBeU16(out_buf, 0, msg_type.toWireU16());
    writeBeU16(out_buf, 2, body_len);
    writeBeU32(out_buf, 4, STUN_MAGIC_COOKIE);
    let i: usize = 0;
    while (i < STUN_TX_ID_LEN) : (i += 1) {
        out_buf[8 + i] = tx_id[i];
    }
}

fn writeStunAttrPadded(out_buf: []u8, pos: usize, attr_type: StunAttrType, value: []const u8): usize ! WebRtcIceError {
    const padded = alignUp4(value.len);
    const total = STUN_ATTR_HEADER_LEN + padded;
    if (pos + total > out_buf.len) {
        return WebRtcIceError.BufferTooSmall;
    }
    if (value.len > 65535) {
        return WebRtcIceError.InvalidAttribute;
    }
    writeBeU16(out_buf, pos, attr_type.toWireU16());
    writeBeU16(out_buf, pos + 2, value.len as u16);
    let i: usize = 0;
    while (i < value.len) : (i += 1) {
        out_buf[pos + 4 + i] = value[i];
    }
    let p: usize = value.len;
    while (p < padded) : (p += 1) {
        out_buf[pos + 4 + p] = 0;
    }
    return pos + total;
}

fn finalizeWithFingerprint(out_buf: []u8, pos: usize): usize ! WebRtcIceError {
    if (pos < STUN_HEADER_LEN || pos + STUN_FINGERPRINT_ATTR_WIRE_LEN > out_buf.len) {
        return WebRtcIceError.BufferTooSmall;
    }
    const final_body_len = (pos - STUN_HEADER_LEN) + STUN_FINGERPRINT_ATTR_WIRE_LEN;
    if (final_body_len > 65535 || (final_body_len & 3) != 0) {
        return WebRtcIceError.InvalidMessageLength;
    }
    // RFC 8489 §14.7: message length in header MUST include the FINGERPRINT attribute
    // BEFORE computing CRC-32 over the preceding bytes.
    writeBeU16(out_buf, 2, final_body_len as u16);
    const fp = computeStunFingerprint(out_buf[0..pos]);
    writeBeU16(out_buf, pos, StunAttrType.fingerprint.toWireU16());
    writeBeU16(out_buf, pos + 2, 4);
    writeBeU32(out_buf, pos + 4, fp);
    return pos + STUN_FINGERPRINT_ATTR_WIRE_LEN;
}

/// Encodes an RFC 8489 + RFC 8445 ICE STUN Binding Request with 4-byte padded attributes
/// (`USERNAME`, `PRIORITY`, `ICE-CONTROLLING`, optional `USE-CANDIDATE`) and IEEE CRC32 `FINGERPRINT`.
pub fn encodeStunBindingRequest(
    tx_id: [12]u8,
    username: []const u8,
    priority: u32,
    controlling_tie: u64,
    use_candidate: bool,
    out_buf: []u8,
): usize ! WebRtcIceError {
    writeStunHeader(out_buf, StunMsgType.binding_request, 0, tx_id)?;
    let pos: usize = STUN_HEADER_LEN;

    if (username.len > 0) {
        pos = writeStunAttrPadded(out_buf, pos, StunAttrType.username, username)?;
    }

    let prio_bytes: [4]u8 = [0; 4];
    writeBeU32(prio_bytes[0..], 0, priority);
    pos = writeStunAttrPadded(out_buf, pos, StunAttrType.priority, prio_bytes[0..])?;

    let tie_bytes: [8]u8 = [0; 8];
    writeBeU64(tie_bytes[0..], 0, controlling_tie);
    pos = writeStunAttrPadded(out_buf, pos, StunAttrType.ice_controlling, tie_bytes[0..])?;

    if (use_candidate) {
        pos = writeStunAttrPadded(out_buf, pos, StunAttrType.use_candidate, "")?;
    }

    return finalizeWithFingerprint(out_buf, pos);
}

/// Encodes an RFC 8489 + RFC 8445 authenticated ICE STUN Binding Request with `USERNAME`, `PRIORITY`,
/// `ICE-CONTROLLING` or `ICE-CONTROLLED` (selected by `role`), optional `USE-CANDIDATE`,
/// HMAC-SHA1 `MESSAGE-INTEGRITY` (when `hmac_key.len > 0`), and IEEE CRC32 `FINGERPRINT`.
pub fn encodeStunBindingRequestAuth(
    tx_id: [12]u8,
    username: []const u8,
    priority: u32,
    role: IceRole,
    tie_breaker: u64,
    use_candidate: bool,
    hmac_key: []const u8,
    out_buf: []u8,
): usize ! WebRtcIceError {
    writeStunHeader(out_buf, StunMsgType.binding_request, 0, tx_id)?;
    let pos: usize = STUN_HEADER_LEN;

    if (username.len > 0) {
        pos = writeStunAttrPadded(out_buf, pos, StunAttrType.username, username)?;
    }

    let prio_bytes: [4]u8 = [0; 4];
    writeBeU32(prio_bytes[0..], 0, priority);
    pos = writeStunAttrPadded(out_buf, pos, StunAttrType.priority, prio_bytes[0..])?;

    let tie_bytes: [8]u8 = [0; 8];
    writeBeU64(tie_bytes[0..], 0, tie_breaker);
    const role_attr = match (role) {
        IceRole.controlling => StunAttrType.ice_controlling,
        IceRole.controlled => StunAttrType.ice_controlled,
    };
    pos = writeStunAttrPadded(out_buf, pos, role_attr, tie_bytes[0..])?;

    if (use_candidate) {
        pos = writeStunAttrPadded(out_buf, pos, StunAttrType.use_candidate, "")?;
    }

    if (hmac_key.len > 0) {
        if (pos + STUN_MI_ATTR_WIRE_LEN + STUN_FINGERPRINT_ATTR_WIRE_LEN > out_buf.len) {
            return WebRtcIceError.BufferTooSmall;
        }
        const mi = computeStunMessageIntegrity(out_buf[0..pos], pos, hmac_key);
        pos = writeStunAttrPadded(out_buf, pos, StunAttrType.message_integrity, mi[0..])?;
    }

    return finalizeWithFingerprint(out_buf, pos);
}

/// Encodes an RFC 8489 STUN Binding Success Response with `XOR-MAPPED-ADDRESS` (IPv4)
/// (`port ^ (magic >> 16)`, `ip ^ magic`) and IEEE CRC32 `FINGERPRINT`.
pub fn encodeStunBindingResponse(
    tx_id: [12]u8,
    mapped: StunIpv4Addr,
    out_buf: []u8,
): usize ! WebRtcIceError {
    writeStunHeader(out_buf, StunMsgType.binding_response, 0, tx_id)?;
    let pos: usize = STUN_HEADER_LEN;

    let xma: [8]u8 = [0; 8];
    xma[0] = 0x00;
    xma[1] = STUN_ADDR_FAMILY_IPV4;
    const xport: u16 = mapped.port ^ ((STUN_MAGIC_COOKIE >> 16) as u16);
    writeBeU16(xma[0..], 2, xport);
    const xip: u32 = mapped.toU32() ^ STUN_MAGIC_COOKIE;
    writeBeU32(xma[0..], 4, xip);

    pos = writeStunAttrPadded(out_buf, pos, StunAttrType.xor_mapped_address, xma[0..])?;
    return finalizeWithFingerprint(out_buf, pos);
}

/// Encodes an RFC 8489 STUN Binding Success Response with `XOR-MAPPED-ADDRESS` (IPv6, family `0x02`)
/// XORed against `STUN_MAGIC_COOKIE || tx_id`, plus IEEE CRC32 `FINGERPRINT`.
pub fn encodeStunBindingResponseV6(
    tx_id: [12]u8,
    mapped: StunIpv6Addr,
    out_buf: []u8,
): usize ! WebRtcIceError {
    writeStunHeader(out_buf, StunMsgType.binding_response, 0, tx_id)?;
    let pos: usize = STUN_HEADER_LEN;

    let xma: [20]u8 = [0; 20];
    xma[0] = 0x00;
    xma[1] = STUN_ADDR_FAMILY_IPV6;
    const xport: u16 = mapped.port ^ ((STUN_MAGIC_COOKIE >> 16) as u16);
    writeBeU16(xma[0..], 2, xport);
    let magic_be: [4]u8 = [0; 4];
    writeBeU32(magic_be[0..], 0, STUN_MAGIC_COOKIE);
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        xma[4 + i] = mapped.ip[i] ^ magic_be[i];
    }
    while (i < 16) : (i += 1) {
        xma[4 + i] = mapped.ip[i] ^ tx_id[i - 4];
    }

    pos = writeStunAttrPadded(out_buf, pos, StunAttrType.xor_mapped_address, xma[0..])?;
    return finalizeWithFingerprint(out_buf, pos);
}

/// Encodes an RFC 8489 STUN Binding Error Response with `ERROR-CODE` and `FINGERPRINT`.
pub fn encodeStunBindingError(
    tx_id: [12]u8,
    error_code: u16,
    reason: []const u8,
    out_buf: []u8,
): usize ! WebRtcIceError {
    if (error_code < 300 || error_code > 699) {
        return WebRtcIceError.InvalidAttribute;
    }
    writeStunHeader(out_buf, StunMsgType.binding_error, 0, tx_id)?;
    let pos: usize = STUN_HEADER_LEN;

    const val_len: usize = reason.len + 4;
    const padded = alignUp4(val_len);
    if (pos + STUN_ATTR_HEADER_LEN + padded + STUN_FINGERPRINT_ATTR_WIRE_LEN > out_buf.len) {
        return WebRtcIceError.BufferTooSmall;
    }
    writeBeU16(out_buf, pos, StunAttrType.error_code.toWireU16());
    writeBeU16(out_buf, pos + 2, val_len as u16);
    out_buf[pos + 4] = 0;
    out_buf[pos + 5] = 0;
    out_buf[pos + 6] = ((error_code / 100) & 0x07) as u8;
    out_buf[pos + 7] = (error_code % 100) as u8;
    let i: usize = 0;
    while (i < reason.len) : (i += 1) {
        out_buf[pos + 8 + i] = reason[i];
    }
    let p: usize = val_len;
    while (p < padded) : (p += 1) {
        out_buf[pos + 4 + p] = 0;
    }
    pos += STUN_ATTR_HEADER_LEN + padded;
    return finalizeWithFingerprint(out_buf, pos);
}

/// Parses and validates an RFC 8489 STUN binary packet, verifying the 20-byte header,
/// Magic Cookie (`0x2112A442`), 4-byte attribute alignment, `XOR-MAPPED-ADDRESS`, ICE attributes,
/// and strict IEEE CRC32 `FINGERPRINT` integrity when present.
pub fn parseStunPacket(raw: []const u8): StunPacket ! WebRtcIceError {
    if (raw.len < STUN_HEADER_LEN) {
        return WebRtcIceError.PacketTooShort;
    }
    // RFC 8489 §5: Most significant 2 bits of STUN message MUST be zero.
    if ((raw[0] & 0xC0) != 0) {
        return WebRtcIceError.InvalidAttribute;
    }
    const raw_type = readBeU16(raw, 0);
    const msg_len = readBeU16(raw, 2);
    const cookie = readBeU32(raw, 4);
    if (cookie != STUN_MAGIC_COOKIE) {
        return WebRtcIceError.InvalidMagicCookie;
    }
    if ((msg_len & 0x03) != 0) {
        return WebRtcIceError.InvalidMessageLength;
    }
    const total_len: usize = STUN_HEADER_LEN + (msg_len as usize);
    if (total_len > raw.len) {
        return WebRtcIceError.InvalidMessageLength;
    }

    let pkt = StunPacket{};
    if (mt := StunMsgType.fromWireU16(raw_type)) {
        pkt.msg_type = mt;
    } else {
        return WebRtcIceError.InvalidAttribute;
    }
    pkt.msg_len = msg_len;
    let ti: usize = 0;
    while (ti < STUN_TX_ID_LEN) : (ti += 1) {
        pkt.tx_id[ti] = raw[8 + ti];
    }

    let off: usize = STUN_HEADER_LEN;
    let ignore_after_mi: bool = false;
    while (off < total_len) {
        if (off + STUN_ATTR_HEADER_LEN > total_len) {
            return WebRtcIceError.InvalidAttribute;
        }
        const attr_code = readBeU16(raw, off);
        const attr_len = readBeU16(raw, off + 2) as usize;
        const val_off = off + STUN_ATTR_HEADER_LEN;
        const padded_len = alignUp4(attr_len);
        if (val_off + attr_len > total_len || val_off + padded_len > total_len) {
            return WebRtcIceError.InvalidAttribute;
        }

        // RFC 8489 §14.5: ignore all attributes following MESSAGE-INTEGRITY except FINGERPRINT
        if (!ignore_after_mi || attr_code == StunAttrType.fingerprint.toWireU16()) {
            if (at := StunAttrType.fromWireU16(attr_code)) {
                match (at) {
                    StunAttrType.mapped_address => {
                        if (attr_len != 8 || raw[val_off + 1] != STUN_ADDR_FAMILY_IPV4) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_mapped = true;
                        pkt.mapped = StunIpv4Addr.init(
                            raw[val_off + 4],
                            raw[val_off + 5],
                            raw[val_off + 6],
                            raw[val_off + 7],
                            readBeU16(raw, val_off + 2),
                        );
                    },
                    StunAttrType.xor_mapped_address => {
                        if (attr_len < 4) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        const family = raw[val_off + 1];
                        const xport = readBeU16(raw, val_off + 2);
                        const port = xport ^ ((STUN_MAGIC_COOKIE >> 16) as u16);
                        if (family == STUN_ADDR_FAMILY_IPV4) {
                            if (attr_len != 8) return WebRtcIceError.InvalidAttribute;
                            const xip = readBeU32(raw, val_off + 4);
                            const ip_u32 = xip ^ STUN_MAGIC_COOKIE;
                            pkt.has_xor_mapped = true;
                            pkt.xor_mapped = StunIpv4Addr.init(
                                ((ip_u32 >> 24) & 0xFF) as u8,
                                ((ip_u32 >> 16) & 0xFF) as u8,
                                ((ip_u32 >> 8) & 0xFF) as u8,
                                (ip_u32 & 0xFF) as u8,
                                port,
                            );
                        } else if (family == STUN_ADDR_FAMILY_IPV6) {
                            if (attr_len != 20) return WebRtcIceError.InvalidAttribute;
                            let magic_be: [4]u8 = [0; 4];
                            writeBeU32(magic_be[0..], 0, STUN_MAGIC_COOKIE);
                            let ip6: [16]u8 = [0; 16];
                            let k: usize = 0;
                            while (k < 4) : (k += 1) {
                                ip6[k] = raw[val_off + 4 + k] ^ magic_be[k];
                            }
                            while (k < 16) : (k += 1) {
                                ip6[k] = raw[val_off + 4 + k] ^ pkt.tx_id[k - 4];
                            }
                            pkt.has_xor_mapped_v6 = true;
                            pkt.xor_mapped_v6 = StunIpv6Addr.init(ip6, port);
                        } else {
                            return WebRtcIceError.InvalidAttribute;
                        }
                    },
                    StunAttrType.username => {
                        pkt.username_off = val_off;
                        pkt.username_len = attr_len;
                    },
                    StunAttrType.error_code => {
                        if (attr_len < 4) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        if (raw[val_off] != 0 || raw[val_off + 1] != 0 || (raw[val_off + 2] & 0xF8) != 0) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        const class_digit = (raw[val_off + 2] & 0x07) as u16;
                        const number = raw[val_off + 3] as u16;
                        if (number >= 100) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_error_code = true;
                        pkt.error_code = class_digit * 100 + number;
                        if (attr_len > 4) {
                            pkt.error_reason_off = val_off + 4;
                            pkt.error_reason_len = attr_len - 4;
                        }
                    },
                    StunAttrType.priority => {
                        if (attr_len != 4) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_priority = true;
                        pkt.priority = readBeU32(raw, val_off);
                    },
                    StunAttrType.use_candidate => {
                        if (attr_len != 0) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.use_candidate = true;
                    },
                    StunAttrType.ice_controlling => {
                        if (attr_len != 8) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_ice_controlling = true;
                        pkt.ice_controlling = readBeU64(raw, val_off);
                    },
                    StunAttrType.ice_controlled => {
                        if (attr_len != 8) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_ice_controlled = true;
                        pkt.ice_controlled = readBeU64(raw, val_off);
                    },
                    StunAttrType.fingerprint => {
                        if (attr_len != 4) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        const wire_fp = readBeU32(raw, val_off);
                        const calc_fp = computeStunFingerprint(raw[0..off]);
                        if (wire_fp != calc_fp) {
                            return WebRtcIceError.InvalidFingerprint;
                        }
                        pkt.has_fingerprint = true;
                        pkt.fingerprint_valid = true;
                    },
                    StunAttrType.message_integrity => {
                        if (attr_len != 20) {
                            return WebRtcIceError.InvalidAttribute;
                        }
                        pkt.has_message_integrity = true;
                        pkt.mi_attr_off = off;
                        let k: usize = 0;
                        while (k < 20) : (k += 1) {
                            pkt.message_integrity[k] = raw[val_off + k];
                        }
                        ignore_after_mi = true;
                    },
                }
            } else {
                // RFC 8489 §14: comprehension-required attributes (< 0x8000) that are unknown fail parsing.
                if (attr_code < 0x8000) {
                    return WebRtcIceError.InvalidAttribute;
                }
            }
        }

        pkt.attr_count += 1;
        off = val_off + padded_len;
    }

    return pkt;
}

// ============================================================================
// 4. RFC 8445 ICE Candidate Priority, Pairing & Connectivity Check Engine
// ============================================================================

/// RFC 8445 ICE candidate type.
pub enum IceCandidateKind {
    host,
    srflx,
    prflx,
    relay,

    /// Returns the RFC 8445 §5.1.2.2 recommended type preference (`0..126`).
    fn typePreference(): u32 {
        return match (_) {
            IceCandidateKind.host => ICE_PRIORITY_HOST,
            IceCandidateKind.prflx => ICE_PRIORITY_PRFLX,
            IceCandidateKind.srflx => ICE_PRIORITY_SRFLX,
            IceCandidateKind.relay => ICE_PRIORITY_RELAY,
        };
    }
}

/// RFC 8445 ICE agent role.
pub enum IceRole {
    controlling,
    controlled,
}

/// RFC 8445 §5.7.4 ICE candidate pair connectivity check state.
pub enum IcePairState {
    frozen,
    waiting,
    in_progress,
    succeeded,
    failed,
}

/// RFC 8445 / RFC 6544 ICE transport protocol.
pub enum IceTransportProto {
    udp,
    tcp,
    ssltcp,
}

/// RFC 6544 ICE-TCP candidate type (`active`, `passive`, `so`).
pub enum IceTcpType {
    none,
    active,
    passive,
    so,
}

/// Host address kind for an ICE candidate (`ipv4`, `ipv6`, or RFC 8828 mDNS `.local`).
pub enum IceHostKind {
    ipv4,
    ipv6,
    mdns,
}

/// RFC 8445 §7.3.1.1 ICE role conflict (487) resolution action.
pub enum IceRoleResolution {
    no_conflict,
    switch_to_controlled,
    switch_to_controlling,
    send_487_conflict,
}

/// Resolves RFC 8445 §7.3.1.1 ICE role conflicts using local role, local tie-breaker,
/// and incoming STUN `ICE-CONTROLLING` / `ICE-CONTROLLED` attributes.
pub fn resolveIceRoleConflict(local_role: IceRole, local_tie: u64, pkt: StunPacket): IceRoleResolution {
    if (local_role == IceRole.controlling && pkt.has_ice_controlling) {
        if (local_tie >= pkt.ice_controlling) {
            return IceRoleResolution.send_487_conflict;
        }
        return IceRoleResolution.switch_to_controlled;
    }
    if (local_role == IceRole.controlled && pkt.has_ice_controlled) {
        if (local_tie >= pkt.ice_controlled) {
            return IceRoleResolution.switch_to_controlling;
        }
        return IceRoleResolution.send_487_conflict;
    }
    return IceRoleResolution.no_conflict;
}

/// Parsed RFC 8445 ICE transport candidate (IPv4, IPv6, or mDNS `.local`).
pub struct WebRtcIceCandidate {
    foundation_hash: u32 = 0;
    component_id: u16 = 1;
    transport: IceTransportProto = IceTransportProto.udp;
    kind: IceCandidateKind = IceCandidateKind.host;
    priority: u32 = 0;
    host_kind: IceHostKind = IceHostKind.ipv4;
    ip: [4]u8 = [0, 0, 0, 0];
    ip6: [16]u8 = [0; 16];
    host_off: usize = 0;
    host_len: usize = 0;
    port: u16 = 0;
    has_raddr: bool = false;
    raddr_ip: [4]u8 = [0, 0, 0, 0];
    raddr_port: u16 = 0;
    tcptype: IceTcpType = IceTcpType.none;
    has_generation: bool = false;
    generation: u32 = 0;
    ufrag_off: usize = 0;
    ufrag_len: usize = 0;

    /// Returns the zero-copy host address / mDNS hostname slice from `raw`.
    fn hostSlice(raw: []const u8): []const u8 {
        if (host_len == 0 || host_off + host_len > raw.len) return "";
        return raw[host_off..(host_off + host_len)];
    }

    /// Returns the zero-copy optional `ufrag` extension slice from `raw`.
    fn ufragSlice(raw: []const u8): []const u8 {
        if (ufrag_len == 0 || ufrag_off + ufrag_len > raw.len) return "";
        return raw[ufrag_off..(ufrag_off + ufrag_len)];
    }

    /// Returns `true` if this candidate uses an mDNS `.local` obfuscated hostname.
    fn isMdns(): bool {
        return host_kind == IceHostKind.mdns;
    }
}

/// Computes RFC 8445 §5.1.2.1 ICE candidate priority:
/// `priority = (2^24) * type_preference + (2^8) * local_preference + (256 - component_id)`.
pub fn computeIceCandidatePriority(kind: IceCandidateKind, local_pref: u16, component_id: u16): u32 {
    const type_pref: u32 = kind.typePreference();
    let comp: u32 = component_id as u32;
    if (comp < 1) comp = 1;
    if (comp > 256) comp = 256;
    return (type_pref << 24) | ((local_pref as u32) << 8) | (256 - comp);
}

/// Computes RFC 8445 §5.7.2 ICE candidate pair priority:
/// `pair_priority = 2^32 * MIN(G, D) + 2 * MAX(G, D) + (G > D ? 1 : 0)`.
pub fn computeIcePairPriority(controlling_prio: u32, controlled_prio: u32): u64 {
    const g: u64 = controlling_prio as u64;
    const d: u64 = controlled_prio as u64;
    const min_gd: u64 = if (g < d) g else d;
    const max_gd: u64 = if (g > d) g else d;
    const tie: u64 = if (g > d) 1 else 0;
    return (min_gd << 32) + (max_gd << 1) + tie;
}

fn nextToken(line: []const u8, start: usize): [2]usize {
    let i: usize = start;
    while (i < line.len && isSpace(line[i])) : (i += 1) {}
    const tok_start = i;
    while (i < line.len && !isSpace(line[i])) : (i += 1) {}
    return [tok_start, i];
}

/// Parses an RFC 8839 / RFC 8445 ICE candidate attribute line such as:
/// `candidate:842163049 1 udp 2122260223 192.168.1.10 54321 typ host`
/// (accepts optional leading `a=` prefix and trailing `\r\n`).
pub fn parseIceCandidateAttr(attr_line: []const u8): WebRtcIceCandidate ! WebRtcIceError {
    let end: usize = attr_line.len;
    while (end > 0 && (attr_line[end - 1] == '\r' || attr_line[end - 1] == '\n' || isSpace(attr_line[end - 1]))) {
        end -= 1;
    }
    let start: usize = 0;
    while (start < end && isSpace(attr_line[start])) : (start += 1) {}
    let line = attr_line[start..end];

    let base_off: usize = start;
    if (startsWith(line, "a=")) {
        line = line[2..];
        base_off += 2;
    }
    if (!startsWith(line, "candidate:")) {
        return WebRtcIceError.InvalidCandidate;
    }
    line = line[10..];
    base_off += 10;

    // Token 0: foundation
    const t0 = nextToken(line, 0);
    if (t0[0] == t0[1]) return WebRtcIceError.InvalidCandidate;
    const foundation = line[t0[0]..t0[1]];

    // Token 1: component-id (1..256)
    const t1 = nextToken(line, t0[1]);
    if (t1[0] == t1[1]) return WebRtcIceError.InvalidCandidate;
    let comp_id: u16 = 0;
    if (c := parseDecU16(line[t1[0]..t1[1]])) {
        if (c < 1 || c > 256) return WebRtcIceError.InvalidCandidate;
        comp_id = c;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 2: transport ("udp", "tcp", or "ssltcp")
    const t2 = nextToken(line, t1[1]);
    if (t2[0] == t2[1]) return WebRtcIceError.InvalidCandidate;
    const transport_tok = line[t2[0]..t2[1]];
    let transport: IceTransportProto = IceTransportProto.udp;
    if (eqIgnoreCase(transport_tok, "udp")) {
        transport = IceTransportProto.udp;
    } else if (eqIgnoreCase(transport_tok, "tcp")) {
        transport = IceTransportProto.tcp;
    } else if (eqIgnoreCase(transport_tok, "ssltcp")) {
        transport = IceTransportProto.ssltcp;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 3: priority
    const t3 = nextToken(line, t2[1]);
    if (t3[0] == t3[1]) return WebRtcIceError.InvalidCandidate;
    let prio: u32 = 0;
    if (p := parseDecU32(line[t3[0]..t3[1]])) {
        prio = p;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 4: connection-address (IPv4, IPv6, or RFC 8828 mDNS `.local`)
    const t4 = nextToken(line, t3[1]);
    if (t4[0] == t4[1]) return WebRtcIceError.InvalidCandidate;
    const addr_tok = line[t4[0]..t4[1]];
    let host_kind: IceHostKind = IceHostKind.ipv4;
    let ip_bytes: [4]u8 = [0, 0, 0, 0];
    let ip6_bytes: [16]u8 = [0; 16];
    if (ip := parseIpv4Bytes(addr_tok)) {
        host_kind = IceHostKind.ipv4;
        ip_bytes = ip;
    } else if (ip6 := parseIpv6Bytes(addr_tok)) {
        host_kind = IceHostKind.ipv6;
        ip6_bytes = ip6;
    } else if (addr_tok.len > 6 && endsWithIgnoreCase(addr_tok, ".local")) {
        host_kind = IceHostKind.mdns;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 5: port (1..65535)
    const t5 = nextToken(line, t4[1]);
    if (t5[0] == t5[1]) return WebRtcIceError.InvalidCandidate;
    let port_val: u16 = 0;
    if (pv := parseDecU16(line[t5[0]..t5[1]])) {
        if (pv == 0) return WebRtcIceError.InvalidCandidate;
        port_val = pv;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 6: literal "typ"
    const t6 = nextToken(line, t5[1]);
    if (t6[0] == t6[1] || !eqIgnoreCase(line[t6[0]..t6[1]], "typ")) {
        return WebRtcIceError.InvalidCandidate;
    }

    // Token 7: candidate type ("host" | "srflx" | "prflx" | "relay")
    const t7 = nextToken(line, t6[1]);
    if (t7[0] == t7[1]) return WebRtcIceError.InvalidCandidate;
    const kind_tok = line[t7[0]..t7[1]];
    let kind: IceCandidateKind = IceCandidateKind.host;
    if (eqIgnoreCase(kind_tok, "host")) {
        kind = IceCandidateKind.host;
    } else if (eqIgnoreCase(kind_tok, "srflx")) {
        kind = IceCandidateKind.srflx;
    } else if (eqIgnoreCase(kind_tok, "prflx")) {
        kind = IceCandidateKind.prflx;
    } else if (eqIgnoreCase(kind_tok, "relay")) {
        kind = IceCandidateKind.relay;
    } else {
        return WebRtcIceError.InvalidCandidate;
    }

    let cand = WebRtcIceCandidate{
        foundation_hash: fnv1a32(foundation),
        component_id: comp_id,
        transport: transport,
        kind: kind,
        priority: prio,
        host_kind: host_kind,
        ip: ip_bytes,
        ip6: ip6_bytes,
        host_off: base_off + t4[0],
        host_len: t4[1] - t4[0],
        port: port_val,
    };

    // Optional extension key-value pairs (raddr, rport, tcptype, generation, ufrag, network-id, network-cost...)
    let cursor: usize = t7[1];
    while (cursor < line.len) {
        const tk = nextToken(line, cursor);
        if (tk[0] == tk[1]) break;
        const tv = nextToken(line, tk[1]);
        if (tv[0] == tv[1]) break;
        const key = line[tk[0]..tk[1]];
        const val = line[tv[0]..tv[1]];
        if (eqIgnoreCase(key, "raddr")) {
            if (rip := parseIpv4Bytes(val)) {
                cand.has_raddr = true;
                cand.raddr_ip = rip;
            } else if (parseIpv6Bytes(val) != null) {
                cand.has_raddr = true;
            }
        } else if (eqIgnoreCase(key, "rport")) {
            if (rp := parseDecU16(val)) {
                cand.raddr_port = rp;
            }
        } else if (eqIgnoreCase(key, "tcptype")) {
            if (eqIgnoreCase(val, "active")) {
                cand.tcptype = IceTcpType.active;
            } else if (eqIgnoreCase(val, "passive")) {
                cand.tcptype = IceTcpType.passive;
            } else if (eqIgnoreCase(val, "so")) {
                cand.tcptype = IceTcpType.so;
            }
        } else if (eqIgnoreCase(key, "generation")) {
            if (gv := parseDecU32(val)) {
                cand.has_generation = true;
                cand.generation = gv;
            }
        } else if (eqIgnoreCase(key, "ufrag")) {
            cand.ufrag_off = base_off + tv[0];
            cand.ufrag_len = tv[1] - tv[0];
        }
        cursor = tv[1];
    }

    return cand;
}

/// Paired local/remote ICE candidate entry in a connectivity check list.
pub struct IceCandidatePair {
    local_idx: usize = 0;
    remote_idx: usize = 0;
    pair_priority: u64 = 0;
    state: IcePairState = IcePairState.frozen;
    nominated: bool = false;
    rtt_us: u64 = 0;
    checks_sent: u32 = 0;
    last_check_us: u64 = 0;
}

fn pairComesBefore(a: IceCandidatePair, b: IceCandidatePair): bool {
    if (a.pair_priority != b.pair_priority) {
        return a.pair_priority > b.pair_priority;
    }
    if (a.local_idx != b.local_idx) {
        return a.local_idx < b.local_idx;
    }
    return a.remote_idx < b.remote_idx;
}

// In-place O(N log N) heapsort for arbitrary external slice length (descending by pair_priority).
// Uses a min-heap under `pairComesBefore` so popping extracts worst elements to the tail,
// leaving `pairs[0..n]` sorted in descending priority order.
fn siftDownWorst(pairs: []IceCandidatePair, root: usize, end: usize) {
    let cur = root;
    while (true) {
        const left = cur * 2 + 1;
        if (left >= end) break;
        let worst = cur;
        if (pairComesBefore(pairs[worst], pairs[left])) {
            worst = left;
        }
        const right = left + 1;
        if (right < end && pairComesBefore(pairs[worst], pairs[right])) {
            worst = right;
        }
        if (worst == cur) break;
        const tmp = pairs[cur];
        pairs[cur] = pairs[worst];
        pairs[worst] = tmp;
        cur = worst;
    }
}

fn sortIcePairsDescending(pairs: []IceCandidatePair, count: usize) {
    if (count <= 1) return;
    let start: usize = count / 2;
    while (start > 0) {
        start -= 1;
        siftDownWorst(pairs, start, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = pairs[0];
        pairs[0] = pairs[end];
        pairs[end] = tmp;
        siftDownWorst(pairs, 0, end);
    }
}

/// Forms the RFC 8445 §5.7.1 Cartesian product of component-compatible local and remote candidates,
/// computes each pair's priority according to `role`, and sorts `out_pairs[0..count]` in strictly
/// descending order of `pair_priority` in O(N log N) with zero heap allocation.
pub fn buildAndSortIcePairs(
    locals: []const WebRtcIceCandidate,
    remotes: []const WebRtcIceCandidate,
    role: IceRole,
    out_pairs: []IceCandidatePair,
): usize ! WebRtcIceError {
    let count: usize = 0;
    let li: usize = 0;
    while (li < locals.len) : (li += 1) {
        let ri: usize = 0;
        while (ri < remotes.len) : (ri += 1) {
            if (locals[li].component_id != remotes[ri].component_id) {
                continue;
            }
            if (count >= out_pairs.len) {
                return WebRtcIceError.OutputSliceTooSmall;
            }
            const g: u32 = match (role) {
                IceRole.controlling => locals[li].priority,
                IceRole.controlled => remotes[ri].priority,
            };
            const d: u32 = match (role) {
                IceRole.controlling => remotes[ri].priority,
                IceRole.controlled => locals[li].priority,
            };
            out_pairs[count] = IceCandidatePair{
                local_idx: li,
                remote_idx: ri,
                pair_priority: computeIcePairPriority(g, d),
                state: IcePairState.frozen,
                nominated: false,
                rtt_us: 0,
                checks_sent: 0,
                last_check_us: 0,
            };
            count += 1;
        }
    }

    sortIcePairsDescending(out_pairs, count);
    if (count > 0) {
        let top = out_pairs[0];
        top.state = IcePairState.waiting;
        out_pairs[0] = top;
    }
    return count;
}

/// Selects the highest-priority pair in `pairs` that is both `succeeded` and `nominated`,
/// breaking ties by lowest observed `rtt_us`. Returns `null` if no valid nominated pair exists.
pub fn selectBestNominatedPair(pairs: []const IceCandidatePair): usize? {
    let best_idx: usize? = null;
    let i: usize = 0;
    while (i < pairs.len) : (i += 1) {
        const p = pairs[i];
        if (p.state != IcePairState.succeeded || !p.nominated) {
            continue;
        }
        if (bi := best_idx) {
            const cur = pairs[bi];
            if (p.pair_priority > cur.pair_priority) {
                best_idx = i;
            } else if (p.pair_priority == cur.pair_priority && p.rtt_us < cur.rtt_us) {
                best_idx = i;
            }
        } else {
            best_idx = i;
        }
    }
    return best_idx;
}

/// Caller-slice-backed RFC 8445 ICE connectivity check list state machine.
pub struct IceCheckList {
    pairs: []IceCandidatePair = [],
    count: usize = 0;
    role: IceRole = IceRole.controlling;
    tie_breaker: u64 = 0;

    /// Initializes a check list over `buf` populated via `buildAndSortIcePairs`.
    static fn init(
        locals: []const WebRtcIceCandidate,
        remotes: []const WebRtcIceCandidate,
        role: IceRole,
        tie_breaker: u64,
        buf: []IceCandidatePair,
    ): IceCheckList ! WebRtcIceError {
        const n = buildAndSortIcePairs(locals, remotes, role, buf)?;
        return IceCheckList{
            pairs: buf,
            count: n,
            role: role,
            tie_breaker: tie_breaker,
        };
    }

    /// Returns a read-only view of all active candidate pairs sorted by descending priority.
    fn activePairs(): []const IceCandidatePair {
        return pairs[0..count];
    }

    /// Picks the next `waiting` pair (or promotes the highest-priority `frozen` pair) to check.
    fn nextCheckPair(*_): usize? {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (pairs[i].state == IcePairState.waiting) {
                return i;
            }
        }
        i = 0;
        while (i < count) : (i += 1) {
            if (pairs[i].state == IcePairState.frozen) {
                pairs[i].state = IcePairState.waiting;
                return i;
            }
        }
        return null;
    }

    /// Transitions pair `idx` into `in_progress` when a STUN Binding Request is transmitted.
    fn markInProgress(*_, idx: usize, now_us: u64): bool {
        if (idx >= count) return false;
        pairs[idx].state = IcePairState.in_progress;
        pairs[idx].checks_sent += 1;
        pairs[idx].last_check_us = now_us;
        return true;
    }

    /// Records a successful STUN Binding Response for pair `idx`, updating RTT and optional nomination.
    fn onCheckSucceeded(*_, idx: usize, rtt_us: u64, nominate: bool): bool {
        if (idx >= count) return false;
        pairs[idx].state = IcePairState.succeeded;
        pairs[idx].rtt_us = rtt_us;
        if (nominate) {
            pairs[idx].nominated = true;
        }
        return true;
    }

    /// Marks pair `idx` as `failed` after retry exhaustion or ICMP/STUN error.
    fn onCheckFailed(*_, idx: usize): bool {
        if (idx >= count) return false;
        pairs[idx].state = IcePairState.failed;
        return true;
    }

    /// Explicitly nominates a succeeded pair `idx`.
    fn nominatePair(*_, idx: usize): bool {
        if (idx >= count || pairs[idx].state != IcePairState.succeeded) return false;
        pairs[idx].nominated = true;
        return true;
    }

    /// Returns the index of the best nominated & succeeded pair, if any.
    fn bestNominatedPair(): usize? {
        return selectBestNominatedPair(pairs[0..count]);
    }

    /// Returns the count of pairs currently in the `succeeded` state.
    fn succeededCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (pairs[i].state == IcePairState.succeeded) n += 1;
        }
        return n;
    }
}

// ============================================================================
// 5. RFC 8866 / WebRTC SDP Parser, Codec Negotiator & RFC 9725 WHEP Formatter
// ============================================================================

/// SDP media description kind (`m=audio`, `m=video`, `m=application`).
pub enum SdpMediaKind {
    audio,
    video,
    application,
}

/// RFC 8866 / RFC 3264 media stream direction attribute.
pub enum SdpDirection {
    sendrecv,
    sendonly,
    recvonly,
    inactive,
}

/// RFC 4145 / RFC 5763 DTLS setup role (`a=setup:actpass|active|passive`).
pub enum SdpSetupRole {
    none,
    actpass,
    active,
    passive,
}

/// Zero-copy SDP RTP header extension mapping (`a=extmap:<id>[/<dir>] <uri>`).
pub struct SdpExtmapEntry {
    id: u16 = 0;
    media: SdpMediaKind = SdpMediaKind.audio;
    uri_off: usize = 0;
    uri_len: usize = 0;

    fn uriSlice(raw: []const u8): []const u8 {
        if (uri_len == 0 || uri_off + uri_len > raw.len) return "";
        return raw[uri_off..(uri_off + uri_len)];
    }
}

/// Zero-copy SDP RTCP feedback entry (`a=rtcp-fb:<pt> <param>`).
pub struct SdpRtcpFbEntry {
    payload_type: u8 = 0;
    param_off: usize = 0;
    param_len: usize = 0;

    fn paramSlice(raw: []const u8): []const u8 {
        if (param_len == 0 || param_off + param_len > raw.len) return "";
        return raw[param_off..(param_off + param_len)];
    }
}

/// Zero-copy SDP SSRC attribute entry (`a=ssrc:<ssrc> <attr>`).
pub struct SdpSsrcEntry {
    ssrc: u32 = 0;
    media: SdpMediaKind = SdpMediaKind.audio;
    attr_off: usize = 0;
    attr_len: usize = 0;

    fn attrSlice(raw: []const u8): []const u8 {
        if (attr_len == 0 || attr_off + attr_len > raw.len) return "";
        return raw[attr_off..(attr_off + attr_len)];
    }
}

/// Zero-copy SDP RTP codec specification parsed from `a=rtpmap:` and `a=fmtp:`.
pub struct SdpCodecSpec {
    media: SdpMediaKind = SdpMediaKind.audio;
    payload_type: u8 = 0;
    clock_rate: u32 = 0;
    channels: u8 = 1;
    name_off: usize = 0;
    name_len: usize = 0;
    fmtp_off: usize = 0;
    fmtp_len: usize = 0;

    /// Returns the codec encoding name (e.g. `"opus"`, `"H264"`, `"VP8"`, `"AV1"`).
    fn nameSlice(raw: []const u8): []const u8 {
        if (name_len == 0 || name_off + name_len > raw.len) return "";
        return raw[name_off..(name_off + name_len)];
    }

    /// Returns the optional `a=fmtp:` parameter string (e.g. `"minptime=10;useinbandfec=1"`).
    fn fmtpSlice(raw: []const u8): []const u8 {
        if (fmtp_len == 0 || fmtp_off + fmtp_len > raw.len) return "";
        return raw[fmtp_off..(fmtp_off + fmtp_len)];
    }
}

/// Zero-allocation summary of a parsed WebRTC SDP Offer/Answer.
pub struct SdpSessionSummary {
    has_version: bool = false;
    ice_ufrag_off: usize = 0;
    ice_ufrag_len: usize = 0;
    ice_pwd_off: usize = 0;
    ice_pwd_len: usize = 0;
    dtls_fp_off: usize = 0;
    dtls_fp_len: usize = 0;
    dtls_setup_off: usize = 0;
    dtls_setup_len: usize = 0;
    setup_role: SdpSetupRole = SdpSetupRole.none;
    bundle_mids_off: usize = 0;
    bundle_mids_len: usize = 0;
    audio_mid_off: usize = 0;
    audio_mid_len: usize = 0;
    video_mid_off: usize = 0;
    video_mid_len: usize = 0;
    app_mid_off: usize = 0;
    app_mid_len: usize = 0;
    mid_count: usize = 0;
    extmap_count: usize = 0;
    rtcp_fb_count: usize = 0;
    ssrc_count: usize = 0;
    audio_dir: SdpDirection = SdpDirection.inactive;
    video_dir: SdpDirection = SdpDirection.inactive;
    has_audio: bool = false;
    has_video: bool = false;
    has_application: bool = false;
    codec_count: usize = 0;
    has_bundle: bool = false;
    has_rtcp_mux: bool = false;
    ice_lite: bool = false;

    /// Returns the parsed `a=ice-ufrag:` value slice from `raw`.
    fn iceUfrag(raw: []const u8): []const u8 {
        if (ice_ufrag_len == 0 || ice_ufrag_off + ice_ufrag_len > raw.len) return "";
        return raw[ice_ufrag_off..(ice_ufrag_off + ice_ufrag_len)];
    }

    /// Returns the parsed `a=ice-pwd:` value slice from `raw`.
    fn icePwd(raw: []const u8): []const u8 {
        if (ice_pwd_len == 0 || ice_pwd_off + ice_pwd_len > raw.len) return "";
        return raw[ice_pwd_off..(ice_pwd_off + ice_pwd_len)];
    }

    /// Returns the parsed `a=fingerprint:sha-256` hex fingerprint slice from `raw`.
    fn dtlsFingerprintSha256(raw: []const u8): []const u8 {
        if (dtls_fp_len == 0 || dtls_fp_off + dtls_fp_len > raw.len) return "";
        return raw[dtls_fp_off..(dtls_fp_off + dtls_fp_len)];
    }

    /// Returns the parsed `a=setup:` DTLS role (`actpass` / `active` / `passive`) from `raw`.
    fn dtlsSetup(raw: []const u8): []const u8 {
        if (dtls_setup_len == 0 || dtls_setup_off + dtls_setup_len > raw.len) return "";
        return raw[dtls_setup_off..(dtls_setup_off + dtls_setup_len)];
    }

    /// Returns the space-separated BUNDLE mids slice from `a=group:BUNDLE ...`.
    fn bundleMids(raw: []const u8): []const u8 {
        if (bundle_mids_len == 0 || bundle_mids_off + bundle_mids_len > raw.len) return "";
        return raw[bundle_mids_off..(bundle_mids_off + bundle_mids_len)];
    }

    /// Returns the `a=mid:` value for the audio media section, if present.
    fn audioMid(raw: []const u8): []const u8 {
        if (audio_mid_len == 0 || audio_mid_off + audio_mid_len > raw.len) return "";
        return raw[audio_mid_off..(audio_mid_off + audio_mid_len)];
    }

    /// Returns the `a=mid:` value for the video media section, if present.
    fn videoMid(raw: []const u8): []const u8 {
        if (video_mid_len == 0 || video_mid_off + video_mid_len > raw.len) return "";
        return raw[video_mid_off..(video_mid_off + video_mid_len)];
    }

    /// Returns the `a=mid:` value for the application (DataChannel) media section, if present.
    fn applicationMid(raw: []const u8): []const u8 {
        if (app_mid_len == 0 || app_mid_off + app_mid_len > raw.len) return "";
        return raw[app_mid_off..(app_mid_off + app_mid_len)];
    }
}

fn findByte(s: []const u8, needle: u8, start: usize): usize? {
    let i: usize = start;
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return null;
}

/// Parses an RFC 8866 / WebRTC SDP Offer or Answer into `SdpSessionSummary` and populates
/// `out_codecs`, `out_extmaps`, `out_rtcp_fbs`, and `out_ssrcs` (zero dynamic allocation).
pub fn parseWebRtcSdpExtended(
    raw: []const u8,
    out_codecs: []SdpCodecSpec,
    out_extmaps: []SdpExtmapEntry,
    out_rtcp_fbs: []SdpRtcpFbEntry,
    out_ssrcs: []SdpSsrcEntry,
): SdpSessionSummary ! WebRtcIceError {
    if (raw.len == 0) {
        return WebRtcIceError.InvalidSdp;
    }

    let summary = SdpSessionSummary{};
    let in_media: bool = false;
    let cur_media: SdpMediaKind = SdpMediaKind.audio;
    let session_dir: SdpDirection = SdpDirection.sendrecv;

    let pos: usize = 0;
    while (pos < raw.len) {
        const line_start = pos;
        while (pos < raw.len && raw[pos] != '\n') : (pos += 1) {}
        let line_end = pos;
        if (pos < raw.len && raw[pos] == '\n') {
            pos += 1;
        }
        if (line_end > line_start && raw[line_end - 1] == '\r') {
            line_end -= 1;
        }
        if (line_end == line_start) continue;

        const line = raw[line_start..line_end];
        if (line.len < 2 || line[1] != '=') {
            return WebRtcIceError.InvalidSdp;
        }

        const key = line[0];
        const val_off = line_start + 2;
        const val = line[2..];

        if (key == 'v') {
            if (!eqBytes(val, "0")) {
                return WebRtcIceError.InvalidSdp;
            }
            summary.has_version = true;
        } else if (key == 'm') {
            in_media = true;
            if (startsWith(val, "audio ")) {
                cur_media = SdpMediaKind.audio;
                summary.has_audio = true;
                summary.audio_dir = session_dir;
            } else if (startsWith(val, "video ")) {
                cur_media = SdpMediaKind.video;
                summary.has_video = true;
                summary.video_dir = session_dir;
            } else if (startsWith(val, "application ")) {
                cur_media = SdpMediaKind.application;
                summary.has_application = true;
            }
        } else if (key == 'a') {
            if (startsWith(val, "ice-ufrag:")) {
                const sub = val[10..];
                if (sub.len == 0) return WebRtcIceError.InvalidSdp;
                if (summary.ice_ufrag_len == 0) {
                    summary.ice_ufrag_off = val_off + 10;
                    summary.ice_ufrag_len = sub.len;
                }
            } else if (startsWith(val, "ice-pwd:")) {
                const sub = val[8..];
                if (sub.len == 0) return WebRtcIceError.InvalidSdp;
                if (summary.ice_pwd_len == 0) {
                    summary.ice_pwd_off = val_off + 8;
                    summary.ice_pwd_len = sub.len;
                }
            } else if (eqBytes(val, "ice-lite")) {
                summary.ice_lite = true;
            } else if (startsWithIgnoreCase(val, "fingerprint:sha-256 ")) {
                const sub = val[20..];
                if (sub.len == 0) return WebRtcIceError.InvalidSdp;
                if (summary.dtls_fp_len == 0) {
                    summary.dtls_fp_off = val_off + 20;
                    summary.dtls_fp_len = sub.len;
                }
            } else if (startsWith(val, "setup:")) {
                const sub = val[6..];
                if (sub.len > 0 && summary.dtls_setup_len == 0) {
                    summary.dtls_setup_off = val_off + 6;
                    summary.dtls_setup_len = sub.len;
                    if (eqIgnoreCase(sub, "actpass")) {
                        summary.setup_role = SdpSetupRole.actpass;
                    } else if (eqIgnoreCase(sub, "active")) {
                        summary.setup_role = SdpSetupRole.active;
                    } else if (eqIgnoreCase(sub, "passive")) {
                        summary.setup_role = SdpSetupRole.passive;
                    }
                }
            } else if (startsWith(val, "group:BUNDLE")) {
                summary.has_bundle = true;
                if (val.len > 12 && isSpace(val[12])) {
                    let ms: usize = 13;
                    while (ms < val.len && isSpace(val[ms])) : (ms += 1) {}
                    let me: usize = val.len;
                    while (me > ms && isSpace(val[me - 1])) : (me -= 1) {}
                    if (me > ms) {
                        summary.bundle_mids_off = val_off + ms;
                        summary.bundle_mids_len = me - ms;
                    }
                }
            } else if (startsWith(val, "mid:")) {
                const sub = val[4..];
                if (sub.len > 0) {
                    summary.mid_count += 1;
                    if (in_media) {
                        if (cur_media == SdpMediaKind.audio && summary.audio_mid_len == 0) {
                            summary.audio_mid_off = val_off + 4;
                            summary.audio_mid_len = sub.len;
                        } else if (cur_media == SdpMediaKind.video && summary.video_mid_len == 0) {
                            summary.video_mid_off = val_off + 4;
                            summary.video_mid_len = sub.len;
                        } else if (cur_media == SdpMediaKind.application && summary.app_mid_len == 0) {
                            summary.app_mid_off = val_off + 4;
                            summary.app_mid_len = sub.len;
                        }
                    }
                }
            } else if (startsWith(val, "extmap:")) {
                // Format: a=extmap:<id>[/<dir>] <uri>
                const body = val[7..];
                const body_abs_off = val_off + 7;
                if (sp := findByte(body, ' ', 0)) {
                    let id_end: usize = sp;
                    if (sl := findByte(body[0..sp], '/', 0)) {
                        id_end = sl;
                    }
                    if (eid := parseDecU16(body[0..id_end])) {
                        let us: usize = sp + 1;
                        while (us < body.len && isSpace(body[us])) : (us += 1) {}
                        if (us < body.len) {
                            if (summary.extmap_count < out_extmaps.len) {
                                out_extmaps[summary.extmap_count] = SdpExtmapEntry{
                                    id: eid,
                                    media: cur_media,
                                    uri_off: body_abs_off + us,
                                    uri_len: body.len - us,
                                };
                            }
                            summary.extmap_count += 1;
                        }
                    }
                }
            } else if (startsWith(val, "rtcp-fb:")) {
                // Format: a=rtcp-fb:<pt|*> <param>
                const body = val[8..];
                const body_abs_off = val_off + 8;
                if (sp := findByte(body, ' ', 0)) {
                    let pt_val: u8 = 0;
                    let valid_pt: bool = false;
                    if (sp == 1 && body[0] == '*') {
                        pt_val = 255;
                        valid_pt = true;
                    } else if (pv := parseDecU8(body[0..sp])) {
                        if (pv <= 127) {
                            pt_val = pv;
                            valid_pt = true;
                        }
                    }
                    if (valid_pt && sp + 1 < body.len) {
                        if (summary.rtcp_fb_count < out_rtcp_fbs.len) {
                            out_rtcp_fbs[summary.rtcp_fb_count] = SdpRtcpFbEntry{
                                payload_type: pt_val,
                                param_off: body_abs_off + sp + 1,
                                param_len: body.len - (sp + 1),
                            };
                        }
                        summary.rtcp_fb_count += 1;
                    }
                }
            } else if (startsWith(val, "ssrc:")) {
                // Format: a=ssrc:<ssrc_u32> <attr>
                const body = val[5..];
                const body_abs_off = val_off + 5;
                if (sp := findByte(body, ' ', 0)) {
                    if (ssrc_v := parseDecU32(body[0..sp])) {
                        if (sp + 1 < body.len) {
                            if (summary.ssrc_count < out_ssrcs.len) {
                                out_ssrcs[summary.ssrc_count] = SdpSsrcEntry{
                                    ssrc: ssrc_v,
                                    media: cur_media,
                                    attr_off: body_abs_off + sp + 1,
                                    attr_len: body.len - (sp + 1),
                                };
                            }
                            summary.ssrc_count += 1;
                        }
                    }
                }
            } else if (eqBytes(val, "rtcp-mux")) {
                summary.has_rtcp_mux = true;
            } else if (eqBytes(val, "sendrecv")) {
                if (!in_media) {
                    session_dir = SdpDirection.sendrecv;
                } else if (cur_media == SdpMediaKind.audio) {
                    summary.audio_dir = SdpDirection.sendrecv;
                } else if (cur_media == SdpMediaKind.video) {
                    summary.video_dir = SdpDirection.sendrecv;
                }
            } else if (eqBytes(val, "sendonly")) {
                if (!in_media) {
                    session_dir = SdpDirection.sendonly;
                } else if (cur_media == SdpMediaKind.audio) {
                    summary.audio_dir = SdpDirection.sendonly;
                } else if (cur_media == SdpMediaKind.video) {
                    summary.video_dir = SdpDirection.sendonly;
                }
            } else if (eqBytes(val, "recvonly")) {
                if (!in_media) {
                    session_dir = SdpDirection.recvonly;
                } else if (cur_media == SdpMediaKind.audio) {
                    summary.audio_dir = SdpDirection.recvonly;
                } else if (cur_media == SdpMediaKind.video) {
                    summary.video_dir = SdpDirection.recvonly;
                }
            } else if (eqBytes(val, "inactive")) {
                if (!in_media) {
                    session_dir = SdpDirection.inactive;
                } else if (cur_media == SdpMediaKind.audio) {
                    summary.audio_dir = SdpDirection.inactive;
                } else if (cur_media == SdpMediaKind.video) {
                    summary.video_dir = SdpDirection.inactive;
                }
            } else if (startsWith(val, "rtpmap:")) {
                // Format: a=rtpmap:<payload_type 0..127> <encoding_name>/<clock_rate>[/<channels>]
                const body = val[7..];
                const body_abs_off = val_off + 7;
                if (sp := findByte(body, ' ', 0)) {
                    let pt: u8 = 0;
                    if (pt_v := parseDecU8(body[0..sp])) {
                        if (pt_v > 127) return WebRtcIceError.InvalidSdp; // 7-bit RTP PT per RFC 3550 & str0m
                        pt = pt_v;
                    } else {
                        return WebRtcIceError.InvalidSdp;
                    }
                    const rest = body[(sp + 1)..];
                    const rest_abs_off = body_abs_off + sp + 1;
                    if (sl1 := findByte(rest, '/', 0)) {
                        if (sl1 == 0) return WebRtcIceError.InvalidSdp;
                        let clock_slice = rest[(sl1 + 1)..];
                        let channels: u8 = 1;
                        if (sl2 := findByte(rest, '/', sl1 + 1)) {
                            clock_slice = rest[(sl1 + 1)..sl2];
                            const ch_slice = rest[(sl2 + 1)..];
                            if (ch_v := parseDecU8(ch_slice)) {
                                if (ch_v == 0) return WebRtcIceError.InvalidSdp;
                                channels = ch_v;
                            } else {
                                return WebRtcIceError.InvalidSdp;
                            }
                        }
                        let clk: u32 = 0;
                        if (clk_v := parseDecU32(clock_slice)) {
                            if (clk_v == 0) return WebRtcIceError.InvalidSdp;
                            clk = clk_v;
                        } else {
                            return WebRtcIceError.InvalidSdp;
                        }

                        if (summary.codec_count >= out_codecs.len) {
                            return WebRtcIceError.OutputSliceTooSmall;
                        }
                        out_codecs[summary.codec_count] = SdpCodecSpec{
                            media: cur_media,
                            payload_type: pt,
                            clock_rate: clk,
                            channels: channels,
                            name_off: rest_abs_off,
                            name_len: sl1,
                            fmtp_off: 0,
                            fmtp_len: 0,
                        };
                        summary.codec_count += 1;
                    } else {
                        return WebRtcIceError.InvalidSdp;
                    }
                } else {
                    return WebRtcIceError.InvalidSdp;
                }
            } else if (startsWith(val, "fmtp:")) {
                // Format: a=fmtp:<payload_type> <parameters> (tolerates horizontal whitespace)
                const body = val[5..];
                const body_abs_off = val_off + 5;
                if (sp := findByte(body, ' ', 0)) {
                    if (pt_v := parseDecU8(body[0..sp])) {
                        let ps: usize = sp + 1;
                        while (ps < body.len && isSpace(body[ps])) : (ps += 1) {}
                        let pe: usize = body.len;
                        while (pe > ps && isSpace(body[pe - 1])) : (pe -= 1) {}
                        const params_off = body_abs_off + ps;
                        const params_len = pe - ps;
                        let ci: usize = 0;
                        while (ci < summary.codec_count) : (ci += 1) {
                            if (out_codecs[ci].payload_type == pt_v) {
                                let spec = out_codecs[ci];
                                spec.fmtp_off = params_off;
                                spec.fmtp_len = params_len;
                                out_codecs[ci] = spec;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!summary.has_version) {
        return WebRtcIceError.InvalidSdp;
    }
    return summary;
}

/// Parses an RFC 8866 / WebRTC SDP Offer or Answer into `SdpSessionSummary` and populates
/// `out_codecs` with every `a=rtpmap:` / `a=fmtp:` codec entry (zero dynamic allocation).
pub fn parseWebRtcSdp(raw: []const u8, out_codecs: []SdpCodecSpec): SdpSessionSummary ! WebRtcIceError {
    let no_ext: [1]SdpExtmapEntry = [SdpExtmapEntry{}];
    let no_fb: [1]SdpRtcpFbEntry = [SdpRtcpFbEntry{}];
    let no_ssrc: [1]SdpSsrcEntry = [SdpSsrcEntry{}];
    return parseWebRtcSdpExtended(raw, out_codecs, no_ext[0..0], no_fb[0..0], no_ssrc[0..0]);
}

/// Negotiates common RTP codecs between a remote SDP Offer and local supported codec names
/// (case-insensitive), preserving the Offer's preference order. Writes matched indices into
/// `out_matched_indices` and returns the number of matched codecs.
pub fn negotiateCommonCodecs(
    offer_codecs: []const SdpCodecSpec,
    offer_raw: []const u8,
    local_supported_names: []const []const u8,
    out_matched_indices: []usize,
): usize {
    let matched: usize = 0;
    let oi: usize = 0;
    while (oi < offer_codecs.len) : (oi += 1) {
        const offer_name = offer_codecs[oi].nameSlice(offer_raw);
        if (offer_name.len == 0) continue;

        let found: bool = false;
        let li: usize = 0;
        while (li < local_supported_names.len) : (li += 1) {
            if (eqIgnoreCase(offer_name, local_supported_names[li])) {
                found = true;
                break;
            }
        }

        if (found) {
            if (matched < out_matched_indices.len) {
                out_matched_indices[matched] = oi;
                matched += 1;
            } else {
                break;
            }
        }
    }
    return matched;
}

/// Formats an RFC 9725 WHEP (WebRTC-HTTP Egress Protocol) HTTP/1.1 `POST` request carrying
/// `Content-Type: application/sdp` and optional `Authorization: Bearer <token>`.
pub fn formatWhepOfferRequest(
    endpoint_path: []const u8,
    host: []const u8,
    bearer_token: []const u8,
    sdp_body: []const u8,
    out_buf: []u8,
): usize ! WebRtcIceError {
    if (endpoint_path.len == 0 || endpoint_path[0] != '/' || host.len == 0 || sdp_body.len == 0) {
        return WebRtcIceError.InvalidSdp;
    }
    if (hasCrOrLf(endpoint_path) || hasCrOrLf(host) || hasCrOrLf(bearer_token)) {
        return WebRtcIceError.InvalidSdp;
    }

    let pos: usize = 0;
    pos = appendBytes(out_buf, pos, "POST ")?;
    pos = appendBytes(out_buf, pos, endpoint_path)?;
    pos = appendBytes(out_buf, pos, " HTTP/1.1\r\nHost: ")?;
    pos = appendBytes(out_buf, pos, host)?;
    pos = appendBytes(out_buf, pos, "\r\nContent-Type: application/sdp\r\nAccept: application/sdp\r\n")?;

    if (bearer_token.len > 0) {
        pos = appendBytes(out_buf, pos, "Authorization: Bearer ")?;
        pos = appendBytes(out_buf, pos, bearer_token)?;
        pos = appendBytes(out_buf, pos, "\r\n")?;
    }

    pos = appendBytes(out_buf, pos, "Content-Length: ")?;
    pos = appendUsizeDec(out_buf, pos, sdp_body.len)?;
    pos = appendBytes(out_buf, pos, "\r\n\r\n")?;
    pos = appendBytes(out_buf, pos, sdp_body)?;
    return pos;
}
