// Type definitions for CP

export class DnsHeader {
    id: u16,
    flags: u16,
    qdcount: u16,
    ancount: u16,
    nscount: u16,
    arcount: u16,
}

export class NameDecodeResult {
    name_len: usize,
    next_offset: usize,
    ok: bool,
}

export fn putBeU16(dst: []u8, offset: usize, v: u16);
export fn getBeU16(src: &[]u8, offset: usize): u16;
export fn encodeHeader(h: DnsHeader, dst: []u8): usize;
export fn decodeHeader(src: &[]u8): DnsHeader;
export fn encodeName(domain: &[]u8, dst: []u8): i64;
export fn decodeName(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult;
export fn encodeQuery(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64;
