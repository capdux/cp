// @cap/dns-packet — Zero-heap RFC 1035 DNS binary packet codec
// MIT License (clean-room CP implementation)

pub const TYPE_A: u16 = 1;
pub const TYPE_NS: u16 = 2;
pub const TYPE_CNAME: u16 = 5;
pub const TYPE_SOA: u16 = 6;
pub const TYPE_PTR: u16 = 12;
pub const TYPE_MX: u16 = 15;
pub const TYPE_TXT: u16 = 16;
pub const TYPE_AAAA: u16 = 28;
pub const TYPE_SRV: u16 = 33;
pub const CLASS_IN: u16 = 1;

pub const HEADER_LEN: usize = 12;
pub const MAX_LABEL_LEN: usize = 63;
pub const MAX_NAME_LEN: usize = 255;
pub const MAX_POINTER_JUMPS: usize = 16;

pub struct DnsHeader {
    pub id: u16,
    pub flags: u16,
    pub qdcount: u16,
    pub ancount: u16,
    pub nscount: u16,
    pub arcount: u16,
}

pub struct NameDecodeResult {
    pub name_len: usize,
    pub next_offset: usize,
    pub ok: bool,
}

fn putBeU16(dst: []u8, offset: usize, v: u16) {
    dst[offset] = ((v >> 8) & 0xFF) as u8;
    dst[offset + 1] = (v & 0xFF) as u8;
}

fn getBeU16(src: []const u8, offset: usize) -> u16 {
    const hi: u16 = (src[offset] as u16) << 8;
    const lo: u16 = src[offset + 1] as u16;
    return hi | lo;
}

pub fn encodeHeader(h: DnsHeader, dst: []u8) -> usize {
    if (dst.len < HEADER_LEN) {
        return 0;
    }
    putBeU16(dst, 0, h.id);
    putBeU16(dst, 2, h.flags);
    putBeU16(dst, 4, h.qdcount);
    putBeU16(dst, 6, h.ancount);
    putBeU16(dst, 8, h.nscount);
    putBeU16(dst, 10, h.arcount);
    return HEADER_LEN;
}

pub fn decodeHeader(src: []const u8) -> DnsHeader {
    if (src.len < HEADER_LEN) {
        return DnsHeader {
            id: 0,
            flags: 0,
            qdcount: 0,
            ancount: 0,
            nscount: 0,
            arcount: 0,
        };
    }
    return DnsHeader {
        id: getBeU16(src, 0),
        flags: getBeU16(src, 2),
        qdcount: getBeU16(src, 4),
        ancount: getBeU16(src, 6),
        nscount: getBeU16(src, 8),
        arcount: getBeU16(src, 10),
    };
}

pub fn encodeName(domain: []const u8, dst: []u8) -> i64 {
    // Root domain "" or "."
    if (domain.len == 0 || (domain.len == 1 && domain[0] == '.' as u8)) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = 0;
        return 1;
    }

    // Normalize single trailing dot
    let eff_len: usize = domain.len;
    if (domain[eff_len - 1] == '.' as u8) {
        eff_len -= 1;
    }
    if (eff_len == 0) {
        return -1;
    }

    let pos: usize = 0;
    let label_start: usize = 0;
    let i: usize = 0;
    while (i <= eff_len) {
        let is_sep: bool = false;
        if (i == eff_len) {
            is_sep = true;
        } else if (domain[i] == '.' as u8) {
            is_sep = true;
        }

        if (is_sep) {
            const label_len: usize = i - label_start;
            if (label_len == 0 || label_len > MAX_LABEL_LEN) {
                return -1;
            }
            if (pos + 1 + label_len + 1 > MAX_NAME_LEN) {
                return -1;
            }
            if (pos + 1 + label_len >= dst.len) {
                return -1;
            }
            dst[pos] = label_len as u8;
            pos += 1;
            let k: usize = 0;
            while (k < label_len) {
                dst[pos] = domain[label_start + k];
                pos += 1;
                k += 1;
            }
            label_start = i + 1;
        }
        i += 1;
    }

    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = 0;
    pos += 1;
    return pos as i64;
}

pub fn decodeName(packet: []const u8, offset: usize, dst: []u8) -> NameDecodeResult {
    const fail = NameDecodeResult { name_len: 0, next_offset: offset, ok: false };
    if (offset >= packet.len) {
        return fail;
    }

    let cur: usize = offset;
    let out_len: usize = 0;
    let jumped: bool = false;
    let next_off: usize = offset;
    let jumps: usize = 0;

    while (true) {
        if (cur >= packet.len) {
            return fail;
        }
        const tag: u8 = packet[cur];
        if (tag == 0) {
            if (!jumped) {
                next_off = cur + 1;
            }
            return NameDecodeResult {
                name_len: out_len,
                next_offset: next_off,
                ok: true,
            };
        }

        const top: u8 = tag & 0xC0;
        if (top == 0xC0) {
            if (cur + 1 >= packet.len) {
                return fail;
            }
            jumps += 1;
            if (jumps > MAX_POINTER_JUMPS) {
                return fail;
            }
            const ptr: usize = (((tag & 0x3F) as usize) << 8) | (packet[cur + 1] as usize);
            if (ptr >= packet.len) {
                return fail;
            }
            if (!jumped) {
                next_off = cur + 2;
                jumped = true;
            }
            cur = ptr;
            continue;
        }
        if (top != 0) {
            return fail;
        }

        const label_len: usize = tag as usize;
        if (cur + 1 + label_len > packet.len) {
            return fail;
        }
        if (out_len > 0) {
            if (out_len >= dst.len) {
                return fail;
            }
            dst[out_len] = '.' as u8;
            out_len += 1;
        }
        if (label_len > dst.len - out_len) {
            return fail;
        }
        let k: usize = 0;
        while (k < label_len) {
            dst[out_len] = packet[cur + 1 + k];
            out_len += 1;
            k += 1;
        }
        if (out_len > MAX_NAME_LEN) {
            return fail;
        }
        cur = cur + 1 + label_len;
    }
    return fail;
}

pub fn encodeQuery(
    id: u16,
    domain: []const u8,
    qtype: u16,
    rd: bool,
    dst: []u8,
) -> i64 {
    if (dst.len < HEADER_LEN) {
        return -1;
    }
    let flags: u16 = 0;
    if (rd) {
        flags = 0x0100;
    }
    const hdr = DnsHeader {
        id: id,
        flags: flags,
        qdcount: 1,
        ancount: 0,
        nscount: 0,
        arcount: 0,
    };
    * = encodeHeader(hdr, dst);

    const name_written: i64 = encodeName(domain, dst[HEADER_LEN..dst.len]);
    if (name_written < 0) {
        return -1;
    }
    const q_end: usize = HEADER_LEN + (name_written as usize);
    if (q_end + 4 > dst.len) {
        return -1;
    }
    putBeU16(dst, q_end, qtype);
    putBeU16(dst, q_end + 2, CLASS_IN);
    return (q_end + 4) as i64;
}
