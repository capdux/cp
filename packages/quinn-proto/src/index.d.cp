// Type definitions for CP

export class QuicVarint {
    val: u64,
    bytes_read: usize,
    ok: bool,
}

export class QuicLongHeader {
    is_long: bool,
    packet_type: u8,
    version: u32,
    dcid_len: u8,
    scid_len: u8,
    header_len: usize,
    ok: bool,
}

export fn encodedLen(val: u64): usize;
export fn encodeVarint(val: u64, out: []u8): usize;
export fn decodeVarint(buf: []u8): QuicVarint;
export fn qvInvalidHeader(): QuicLongHeader;
export fn parseLongHeader(pkt: []u8): QuicLongHeader;
