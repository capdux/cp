// Source: cp/lib/tlv/oid.cp (ASN.1 DER Object Identifier Bidirectional Codec)
// Pure CP zero-C-dependency dotted OID string <-> DER OID octets encoder and decoder.

const std = @import("std");
import { TlvError, ByteBuf } from "./types.cp";

fn appendBase128U64(buf: *ByteBuf, val: u64): ! TlvError {
    if (val == 0) {
        buf.appendByte(0x00)?;
        return;
    }
    let tmp: [10]u8 = [0; 10];
    let count: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        tmp[count] = (v & 0x7F) as u8;
        count += 1;
        v = v >> 7;
    }
    let i: usize = count;
    while (i > 0) {
        i -= 1;
        let b: u8 = tmp[i];
        if (i > 0) {
            b = b | 0x80;
        }
        buf.appendByte(b)?;
    }
}

fn appendDecimalU64(buf: *ByteBuf, val: u64): ! TlvError {
    if (val == 0) {
        buf.appendByte('0')?;
        return;
    }
    let tmp: [24]u8 = [0; 24];
    let count: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        tmp[count] = ('0' as u8) + ((v % 10) as u8);
        count += 1;
        v = v / 10;
    }
    let i: usize = count;
    while (i > 0) {
        i -= 1;
        buf.appendByte(tmp[i])?;
    }
}

/// Encodes a dotted OID string (e.g. `"1.2.840.113549.1.1.11"`) into DER OID value octets.
/// Caller owns the returned slice and must free it with `alloc.free`.
pub fn encodeOidBytes(alloc: std.mem.Allocator, oid_str: []const u8): ([]u8) ! TlvError {
    if (oid_str.len == 0) {
        return TlvError.InvalidOid;
    }

    let arcs: [64]u64 = [0; 64];
    let arc_count: usize = 0;
    let pos: usize = 0;

    while (pos <= oid_str.len) {
        if (pos == oid_str.len) {
            return TlvError.InvalidOid;
        }
        let acc: u64 = 0;
        let digits: usize = 0;
        while (pos < oid_str.len && oid_str[pos] != '.') : (pos += 1) {
            const c = oid_str[pos];
            if (c < '0' || c > '9') {
                return TlvError.InvalidOid;
            }
            if (digits > 0 && acc == 0) {
                // Leading zero in an arc (e.g. "01") is invalid
                return TlvError.InvalidOid;
            }
            const d: u64 = (c - '0') as u64;
            if (acc > (0xFFFFFFFFFFFFFFFF - d) / 10) {
                return TlvError.InvalidOid;
            }
            acc = acc * 10 + d;
            digits += 1;
        }
        if (digits == 0) {
            return TlvError.InvalidOid;
        }
        if (arc_count >= 64) {
            return TlvError.InvalidOid;
        }
        arcs[arc_count] = acc;
        arc_count += 1;

        if (pos == oid_str.len) {
            break;
        }
        // Skip '.'
        pos += 1;
    }

    if (arc_count < 2) {
        return TlvError.InvalidOid;
    }
    const arc0 = arcs[0];
    const arc1 = arcs[1];
    if (arc0 > 2) {
        return TlvError.InvalidOid;
    }
    if (arc0 < 2 && arc1 > 39) {
        return TlvError.InvalidOid;
    }

    let out = ByteBuf.init(alloc);
    errdefer out.deinit();

    const first_subid: u64 = arc0 * 40 + arc1;
    appendBase128U64(&out, first_subid)?;

    let i: usize = 2;
    while (i < arc_count) : (i += 1) {
        appendBase128U64(&out, arcs[i])?;
    }

    return out.toOwnedSlice();
}

/// Decodes DER OID value octets into a dotted OID string (e.g. `"1.2.840.113549.1.1.11"`).
/// Caller owns the returned slice and must free it with `alloc.free`.
pub fn decodeOid(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! TlvError {
    if (bytes.len == 0) {
        return TlvError.InvalidOid;
    }

    let out = ByteBuf.init(alloc);
    errdefer out.deinit();

    let pos: usize = 0;
    let first: bool = true;

    while (pos < bytes.len) {
        if (bytes[pos] == 0x80) {
            // DER forbids leading zero padding in Base-128 subidentifier
            return TlvError.InvalidOid;
        }
        let subid: u64 = 0;
        let terminated: bool = false;
        while (pos < bytes.len) : (pos += 1) {
            const b = bytes[pos];
            if (subid > 0x01FFFFFFFFFFFFFF) {
                return TlvError.InvalidOid;
            }
            subid = (subid << 7) | ((b & 0x7F) as u64);
            if ((b & 0x80) == 0) {
                pos += 1;
                terminated = true;
                break;
            }
        }
        if (!terminated) {
            return TlvError.UnexpectedEof;
        }

        if (first) {
            first = false;
            if (subid < 40) {
                appendDecimalU64(&out, 0)?;
                out.appendByte('.')?;
                appendDecimalU64(&out, subid)?;
            } else if (subid < 80) {
                appendDecimalU64(&out, 1)?;
                out.appendByte('.')?;
                appendDecimalU64(&out, subid - 40)?;
            } else {
                appendDecimalU64(&out, 2)?;
                out.appendByte('.')?;
                appendDecimalU64(&out, subid - 80)?;
            }
        } else {
            out.appendByte('.')?;
            appendDecimalU64(&out, subid)?;
        }
    }

    return out.toOwnedSlice();
}
