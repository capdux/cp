// Source: Absorbed from /data/dev/bun/src/dns/ | Synced: 2026-11-20T07:15:24Z
// lib/dns/packet.cp
// RFC 1035 DNS Wire Packet Header, Domain Label Codec & Pointer Decompression

import { NSClass, NSType } from "./_cares.cp";

pub enum DnsRCode: u8 {
    NoError = 0,
    FormErr = 1,
    ServFail = 2,
    NXDomain = 3,
    NotImp = 4,
    Refused = 5,
}

// RFC 1035 Section 4.1.1 12-byte DNS Wire Header
pub struct DnsHeader {
    id: u16 = 0;
    is_response: bool = false;
    opcode: u8 = 0;
    authoritative: bool = false;
    truncated: bool = false;
    recursion_desired: bool = true;
    recursion_available: bool = false;
    rcode: DnsRCode = .NoError;
    qd_count: u16 = 1;
    an_count: u16 = 0;
    ns_count: u16 = 0;
    ar_count: u16 = 0;

    pub fn encode(out: []u8): usize {
        if (out.len < 12) {
            return 0;
        }
        out[0] = (_.id >> 8) as u8;
        out[1] = (_.id & 0xFF) as u8;

        let flags_hi: u8 = (_.opcode & 0x0F) << 3;
        if (_.is_response) {
            flags_hi = flags_hi | 0x80;
        }
        if (_.authoritative) {
            flags_hi = flags_hi | 0x04;
        }
        if (_.truncated) {
            flags_hi = flags_hi | 0x02;
        }
        if (_.recursion_desired) {
            flags_hi = flags_hi | 0x01;
        }
        let flags_lo: u8 = (_.rcode as u8) & 0x0F;
        if (_.recursion_available) {
            flags_lo = flags_lo | 0x80;
        }
        out[2] = flags_hi;
        out[3] = flags_lo;

        out[4] = (_.qd_count >> 8) as u8;
        out[5] = (_.qd_count & 0xFF) as u8;
        out[6] = (_.an_count >> 8) as u8;
        out[7] = (_.an_count & 0xFF) as u8;
        out[8] = (_.ns_count >> 8) as u8;
        out[9] = (_.ns_count & 0xFF) as u8;
        out[10] = (_.ar_count >> 8) as u8;
        out[11] = (_.ar_count & 0xFF) as u8;
        return 12;
    }

    pub static fn decode(buf: []const u8): DnsHeader? {
        if (buf.len < 12) {
            return null;
        }
        const id: u16 = ((buf[0] as u16) << 8) | (buf[1] as u16);
        const f_hi = buf[2];
        const f_lo = buf[3];
        const rcode_raw = f_lo & 0x0F;
        const rcode = switch (rcode_raw) {
            0 => DnsRCode.NoError,
            1 => DnsRCode.FormErr,
            2 => DnsRCode.ServFail,
            3 => DnsRCode.NXDomain,
            4 => DnsRCode.NotImp,
            else => DnsRCode.Refused,
        };
        return DnsHeader{
            id: id,
            is_response: (f_hi & 0x80) != 0,
            opcode: (f_hi >> 3) & 0x0F,
            authoritative: (f_hi & 0x04) != 0,
            truncated: (f_hi & 0x02) != 0,
            recursion_desired: (f_hi & 0x01) != 0,
            recursion_available: (f_lo & 0x80) != 0,
            rcode: rcode,
            qd_count: ((buf[4] as u16) << 8) | (buf[5] as u16),
            an_count: ((buf[6] as u16) << 8) | (buf[7] as u16),
            ns_count: ((buf[8] as u16) << 8) | (buf[9] as u16),
            ar_count: ((buf[10] as u16) << 8) | (buf[11] as u16),
        };
    }
}

pub struct DecodedDomain {
    name_len: usize = 0;
    packet_bytes_consumed: usize = 0;
}

pub struct DnsQuestion {
    name_len: usize = 0;
    qtype: NSType = NSType.Invalid;
    qclass: NSClass = NSClass.In;
    next_offset: usize = 0;
}

pub struct DnsResourceRecord {
    name_len: usize = 0;
    rtype: NSType = NSType.Invalid;
    rclass: NSClass = NSClass.In;
    ttl: u32 = 0;
    rdlength: u16 = 0;
    rdata_offset: usize = 0;
    next_offset: usize = 0;
}

pub struct DnsMxRecord {
    preference: u16 = 0;
    exchange_buf: [256]u8 = [0; 256];
    exchange_len: usize = 0;

    pub fn exchange(*_): []const u8 {
        return _.exchange_buf[0.._.exchange_len];
    }
}

pub struct DnsSrvRecord {
    priority: u16 = 0;
    weight: u16 = 0;
    port: u16 = 0;
    target_buf: [256]u8 = [0; 256];
    target_len: usize = 0;

    pub fn target(*_): []const u8 {
        return _.target_buf[0.._.target_len];
    }
}

pub struct DnsWireCodec {
    // Encodes "api.cap.dev" -> "\x03api\x03cap\x03dev\x00"
    pub static fn encodeDomainName(domain: []const u8, out: []u8): usize {
        if (domain.len == 0 || domain.len > 253) {
            return 0;
        }
        let pos: usize = 0;
        let label_start: usize = 0;
        let i: usize = 0;
        while (i <= domain.len) {
            if (i == domain.len || domain[i] == 46) { // '.' = 46
                const label_len = i - label_start;
                if (label_len == 0 || label_len > 63 || pos + 1 + label_len + 1 > out.len) {
                    return 0;
                }
                out[pos] = label_len as u8;
                pos += 1;
                let k: usize = 0;
                while (k < label_len) {
                    out[pos + k] = domain[label_start + k];
                    k += 1;
                }
                pos += label_len;
                label_start = i + 1;
            }
            i += 1;
        }
        out[pos] = 0;
        return pos + 1;
    }

    // Decodes wire domain name (supporting RFC 1035 Section 4.1.4 `0xC0` pointer compression!)
    pub static fn decodeDomainName(packet: []const u8, start_offset: usize, out_name: []u8): DecodedDomain? {
        let cur = start_offset;
        let out_pos: usize = 0;
        let consumed: usize = 0;
        let jumped = false;
        let hops: usize = 0;

        while (cur < packet.len && hops < 16) {
            const len_byte = packet[cur];
            if (len_byte == 0) {
                if (!jumped) {
                    consumed = (cur - start_offset) + 1;
                }
                return DecodedDomain{
                    name_len: out_pos,
                    packet_bytes_consumed: consumed,
                };
            }
            // Check pointer compression (top 2 bits == 11 -> 0xC0)
            if ((len_byte & 0xC0) == 0xC0) {
                if (cur + 1 >= packet.len) {
                    return null;
                }
                if (!jumped) {
                    consumed = (cur - start_offset) + 2;
                    jumped = true;
                }
                const target_offset: usize = (((len_byte & 0x3F) as usize) << 8) | (packet[cur + 1] as usize);
                if (target_offset >= packet.len) {
                    return null;
                }
                cur = target_offset;
                hops += 1;
            } else {
                const label_len = len_byte as usize;
                cur += 1;
                if (cur + label_len > packet.len) {
                    return null;
                }
                if (out_pos > 0) {
                    if (out_pos + 1 > out_name.len) {
                        return null;
                    }
                    out_name[out_pos] = 46; // '.'
                    out_pos += 1;
                }
                if (out_pos + label_len > out_name.len) {
                    return null;
                }
                let k: usize = 0;
                while (k < label_len) {
                    out_name[out_pos + k] = packet[cur + k];
                    k += 1;
                }
                out_pos += label_len;
                cur += label_len;
            }
        }
        return null;
    }

    // Builds a complete DNS query packet (12-byte header + encoded QNAME + QTYPE + QCLASS)
    pub static fn buildQueryPacket(id: u16, domain: []const u8, qtype: NSType, out: []u8): usize {
        const hdr = DnsHeader{
            id: id,
            is_response: false,
            opcode: 0,
            authoritative: false,
            truncated: false,
            recursion_desired: true,
            recursion_available: false,
            rcode: .NoError,
            qd_count: 1,
            an_count: 0,
            ns_count: 0,
            ar_count: 0,
        };
        const hdr_len = hdr.encode(out);
        if (hdr_len == 0) {
            return 0;
        }
        const qname_len = DnsWireCodec.encodeDomainName(domain, out[12..out.len]);
        if (qname_len == 0) {
            return 0;
        }
        const pos = 12 + qname_len;
        if (pos + 4 > out.len) {
            return 0;
        }
        const qt: u16 = qtype as u16;
        const qc: u16 = NSClass.In as u16;
        out[pos] = (qt >> 8) as u8;
        out[pos + 1] = (qt & 0xFF) as u8;
        out[pos + 2] = (qc >> 8) as u8;
        out[pos + 3] = (qc & 0xFF) as u8;
        return pos + 4;
    }

    pub static fn nsTypeFromU16(raw: u16): NSType {
        return switch (raw) {
            1 => NSType.A,
            2 => NSType.NS,
            5 => NSType.CNAME,
            6 => NSType.SOA,
            12 => NSType.PTR,
            13 => NSType.HINFO,
            15 => NSType.MX,
            16 => NSType.TXT,
            28 => NSType.AAAA,
            33 => NSType.SRV,
            35 => NSType.NAPTR,
            41 => NSType.OPT,
            255 => NSType.ANY,
            257 => NSType.CAA,
            else => NSType.Invalid,
        };
    }

    pub static fn nsClassFromU16(raw: u16): NSClass {
        return switch (raw) {
            1 => NSClass.In,
            3 => NSClass.Chaos,
            4 => NSClass.Hs,
            254 => NSClass.None,
            255 => NSClass.Any,
            else => NSClass.Invalid,
        };
    }

    pub static fn decodeQuestion(packet: []const u8, offset: usize, out_name: []u8): DnsQuestion? {
        const dom = DnsWireCodec.decodeDomainName(packet, offset, out_name) ?? return null;
        const pos = offset + dom.packet_bytes_consumed;
        if (pos + 4 > packet.len) {
            return null;
        }
        const qt: u16 = ((packet[pos] as u16) << 8) | (packet[pos + 1] as u16);
        const qc: u16 = ((packet[pos + 2] as u16) << 8) | (packet[pos + 3] as u16);
        return DnsQuestion{
            name_len: dom.name_len,
            qtype: DnsWireCodec.nsTypeFromU16(qt),
            qclass: DnsWireCodec.nsClassFromU16(qc),
            next_offset: pos + 4,
        };
    }

    pub static fn decodeResourceRecord(packet: []const u8, offset: usize, out_name: []u8): DnsResourceRecord? {
        const dom = DnsWireCodec.decodeDomainName(packet, offset, out_name) ?? return null;
        const pos = offset + dom.packet_bytes_consumed;
        if (pos + 10 > packet.len) {
            return null;
        }
        const qt: u16 = ((packet[pos] as u16) << 8) | (packet[pos + 1] as u16);
        const qc: u16 = ((packet[pos + 2] as u16) << 8) | (packet[pos + 3] as u16);
        const ttl: u32 = ((packet[pos + 4] as u32) << 24)
            | ((packet[pos + 5] as u32) << 16)
            | ((packet[pos + 6] as u32) << 8)
            | (packet[pos + 7] as u32);
        const rdlen: u16 = ((packet[pos + 8] as u16) << 8) | (packet[pos + 9] as u16);
        const rdata_offset = pos + 10;
        const next_offset = rdata_offset + (rdlen as usize);
        if (next_offset > packet.len) {
            return null;
        }
        return DnsResourceRecord{
            name_len: dom.name_len,
            rtype: DnsWireCodec.nsTypeFromU16(qt),
            rclass: DnsWireCodec.nsClassFromU16(qc),
            ttl: ttl,
            rdlength: rdlen,
            rdata_offset: rdata_offset,
            next_offset: next_offset,
        };
    }

    pub static fn parseARdata(packet: []const u8, rr: DnsResourceRecord): ([4]u8)? {
        if (rr.rtype != NSType.A || rr.rdlength != 4 || rr.rdata_offset + 4 > packet.len) {
            return null;
        }
        return [
            packet[rr.rdata_offset],
            packet[rr.rdata_offset + 1],
            packet[rr.rdata_offset + 2],
            packet[rr.rdata_offset + 3],
        ];
    }

    pub static fn parseARdataU32(packet: []const u8, rr: DnsResourceRecord): u32? {
        const oct = DnsWireCodec.parseARdata(packet, rr) ?? return null;
        return ((oct[0] as u32) << 24)
            | ((oct[1] as u32) << 16)
            | ((oct[2] as u32) << 8)
            | (oct[3] as u32);
    }

    pub static fn parseAAAARdata(packet: []const u8, rr: DnsResourceRecord): ([16]u8)? {
        if (rr.rtype != NSType.AAAA || rr.rdlength != 16 || rr.rdata_offset + 16 > packet.len) {
            return null;
        }
        let addr: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) {
            addr[i] = packet[rr.rdata_offset + i];
            i += 1;
        }
        return addr;
    }

    pub static fn parseNameRdata(packet: []const u8, rr: DnsResourceRecord, out_name: []u8): DecodedDomain? {
        if (rr.rdata_offset + (rr.rdlength as usize) > packet.len) {
            return null;
        }
        return DnsWireCodec.decodeDomainName(packet, rr.rdata_offset, out_name);
    }

    pub static fn parseMxRdata(packet: []const u8, rr: DnsResourceRecord): DnsMxRecord? {
        if (rr.rtype != NSType.MX || rr.rdlength < 3 || rr.rdata_offset + (rr.rdlength as usize) > packet.len) {
            return null;
        }
        const pref: u16 = ((packet[rr.rdata_offset] as u16) << 8) | (packet[rr.rdata_offset + 1] as u16);
        let rec = DnsMxRecord{ preference: pref };
        const dom = DnsWireCodec.decodeDomainName(packet, rr.rdata_offset + 2, rec.exchange_buf[0..256]) ?? return null;
        rec.exchange_len = dom.name_len;
        return rec;
    }

    pub static fn parseSrvRdata(packet: []const u8, rr: DnsResourceRecord): DnsSrvRecord? {
        if (rr.rtype != NSType.SRV || rr.rdlength < 7 || rr.rdata_offset + (rr.rdlength as usize) > packet.len) {
            return null;
        }
        const off = rr.rdata_offset;
        const prio: u16 = ((packet[off] as u16) << 8) | (packet[off + 1] as u16);
        const w: u16 = ((packet[off + 2] as u16) << 8) | (packet[off + 3] as u16);
        const port: u16 = ((packet[off + 4] as u16) << 8) | (packet[off + 5] as u16);
        let rec = DnsSrvRecord{
            priority: prio,
            weight: w,
            port: port,
        };
        const dom = DnsWireCodec.decodeDomainName(packet, off + 6, rec.target_buf[0..256]) ?? return null;
        rec.target_len = dom.name_len;
        return rec;
    }

    /// Parses RFC 1035 TXT RDATA (one or more length-prefixed character strings) into `out`.
    pub static fn parseTxtRdata(packet: []const u8, rr: DnsResourceRecord, out: []u8): usize? {
        if (rr.rtype != NSType.TXT || rr.rdata_offset + (rr.rdlength as usize) > packet.len) {
            return null;
        }
        let cur = rr.rdata_offset;
        const end = rr.rdata_offset + (rr.rdlength as usize);
        let out_pos: usize = 0;
        while (cur < end) {
            const seg_len = packet[cur] as usize;
            cur += 1;
            if (cur + seg_len > end || out_pos + seg_len > out.len) {
                return null;
            }
            let k: usize = 0;
            while (k < seg_len) {
                out[out_pos + k] = packet[cur + k];
                k += 1;
            }
            out_pos += seg_len;
            cur += seg_len;
        }
        return out_pos;
    }

    pub static fn formatIPv4(addr: [4]u8, out: []u8): usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 4) {
            if (i > 0) {
                if (pos >= out.len) return 0;
                out[pos] = 46; // '.'
                pos += 1;
            }
            let v = addr[i] as usize;
            if (v >= 100) {
                if (pos + 3 > out.len) return 0;
                out[pos] = (48 + (v / 100)) as u8;
                out[pos + 1] = (48 + ((v / 10) % 10)) as u8;
                out[pos + 2] = (48 + (v % 10)) as u8;
                pos += 3;
            } else if (v >= 10) {
                if (pos + 2 > out.len) return 0;
                out[pos] = (48 + (v / 10)) as u8;
                out[pos + 1] = (48 + (v % 10)) as u8;
                pos += 2;
            } else {
                if (pos + 1 > out.len) return 0;
                out[pos] = (48 + v) as u8;
                pos += 1;
            }
            i += 1;
        }
        return pos;
    }

    pub static fn formatIPv6(addr: [16]u8, out: []u8): usize {
        const hex_l: []const u8 = "0123456789abcdef";
        if (out.len < 39) return 0;
        let pos: usize = 0;
        let g: usize = 0;
        while (g < 8) {
            if (g > 0) {
                out[pos] = 58; // ':'
                pos += 1;
            }
            const hi = addr[g * 2];
            const lo = addr[g * 2 + 1];
            const word: u16 = ((hi as u16) << 8) | (lo as u16);
            let started = false;
            let nib_idx: usize = 0;
            while (nib_idx < 4) {
                const nib: usize = switch (nib_idx) {
                    0 => ((word >> 12) & 0xF) as usize,
                    1 => ((word >> 8) & 0xF) as usize,
                    2 => ((word >> 4) & 0xF) as usize,
                    else => (word & 0xF) as usize,
                };
                if (nib != 0 || started || nib_idx == 3) {
                    out[pos] = hex_l[nib];
                    pos += 1;
                    started = true;
                }
                nib_idx += 1;
            }
            g += 1;
        }
        return pos;
    }
}
