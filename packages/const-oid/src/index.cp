// @cap/const-oid — Pure CP zero-allocation ITU-T X.690 §8.19 Object Identifier (OID)
// Clean-room MIT implementation with parity to RustCrypto `const-oid`.
// Supports strict dotted-decimal parsing, canonical DER & TLV (0x06) encoding/decoding,
// formatting, equality, and prefix matching with zero heap allocation.

pub const OID_MAX_ARCS: usize = 16;
pub const OID_MAX_DER_LEN: usize = 80;
pub const OID_TAG: u8 = 0x06;

// Common PKI / X.509 / PKCS / RFC standard OID strings
pub const OID_RSA_ENCRYPTION: string = "1.2.840.113549.1.1.1";
pub const OID_SHA256_WITH_RSA: string = "1.2.840.113549.1.1.11";
pub const OID_EC_PUBLIC_KEY: string = "1.2.840.10045.2.1";
pub const OID_PRIME256V1: string = "1.2.840.10045.3.1.7";
pub const OID_ED25519: string = "1.3.101.112";
pub const OID_X25519: string = "1.3.101.110";
pub const OID_COMMON_NAME: string = "2.5.4.3";
pub const OID_COUNTRY_NAME: string = "2.5.4.6";
pub const OID_ORGANIZATION_NAME: string = "2.5.4.10";
pub const OID_SHA256: string = "2.16.840.1.101.3.4.2.1";
pub const OID_SHA384: string = "2.16.840.1.101.3.4.2.2";
pub const OID_SHA512: string = "2.16.840.1.101.3.4.2.3";

fn coidZeroArcs(): [16]u32 {
    let a: [16]u32 = undefined;
    let i: usize = 0;
    while (i < 16) {
        a[i] = 0 as u32;
        i = i + 1;
    }
    return a;
}

fn coidZeroDer(): [80]u8 {
    let d: [80]u8 = undefined;
    let i: usize = 0;
    while (i < 80) {
        d[i] = 0 as u8;
        i = i + 1;
    }
    return d;
}

fn coidEncodeBase128U64(val: u64, out: []u8, start_pos: usize): usize {
    if (val == 0) {
        if (start_pos >= out.len) {
            return 0;
        }
        out[start_pos] = 0 as u8;
        return start_pos + 1;
    }

    let tmp: [10]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = (v & (0x7F as u64)) as u8;
        v = v >> (7 as u6);
        n = n + 1;
    }

    if (start_pos + n > out.len) {
        return 0;
    }

    let pos = start_pos;
    let i = n;
    while (i > 0) {
        i = i - 1;
        let b = tmp[i];
        if (i > 0) {
            b = b | (0x80 as u8);
        }
        out[pos] = b;
        pos = pos + 1;
    }
    return pos;
}

fn coidEncodeArcsToDer(arcs: [16]u32, arc_len: usize, der_out: []u8): usize {
    if (arc_len < 2 || arc_len > OID_MAX_ARCS) {
        return 0;
    }
    const first_val: u64 = (arcs[0] as u64) * (40 as u64) + (arcs[1] as u64);
    let pos = coidEncodeBase128U64(first_val, der_out, 0);
    if (pos == 0) {
        return 0;
    }

    let i: usize = 2;
    while (i < arc_len) {
        pos = coidEncodeBase128U64(arcs[i] as u64, der_out, pos);
        if (pos == 0) {
            return 0;
        }
        i = i + 1;
    }
    return pos;
}

fn coidWriteU32Dec(val: u32, out: []u8, start_pos: usize): usize {
    if (val == 0) {
        if (start_pos >= out.len) {
            return 0;
        }
        out[start_pos] = 48 as u8; // '0'
        return start_pos + 1;
    }

    let tmp: [10]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        const d = (v % (10 as u32)) as u8;
        tmp[n] = (48 as u8) + d;
        v = v / (10 as u32);
        n = n + 1;
    }

    if (start_pos + n > out.len) {
        return 0;
    }

    let pos = start_pos;
    let i = n;
    while (i > 0) {
        i = i - 1;
        out[pos] = tmp[i];
        pos = pos + 1;
    }
    return pos;
}

pub struct OidParseResult {
    pub ok: bool,
    pub oid: ObjectIdentifier,
}

pub struct ObjectIdentifier {
    pub arcs: [16]u32,
    pub len: usize,
    pub der_buf: [80]u8,
    pub der_len: usize,

    pub static fn empty(): ObjectIdentifier {
        return ObjectIdentifier {
            arcs: coidZeroArcs(),
            len: 0,
            der_buf: coidZeroDer(),
            der_len: 0,
        };
    }

    /// Parse a dotted-decimal OID string (e.g. "1.2.840.113549.1.1.11").
    /// Strictly enforces ITU-T X.690 §8.19:
    /// - At least 2 arcs, at most 16 arcs
    /// - arc0 <= 2; if arc0 < 2 then arc1 <= 39
    /// - Rejects empty segments, leading zeros (e.g. "1.02"), non-digits, and u32 overflow.
    pub static fn fromStr(s: string): OidParseResult {
        const empty_oid = ObjectIdentifier.empty();
        if (s.len == 0) {
            return OidParseResult { ok: false, oid: empty_oid };
        }

        let arcs = coidZeroArcs();
        let arc_count: usize = 0;
        let pos: usize = 0;

        while (pos < s.len) {
            if (arc_count >= OID_MAX_ARCS) {
                return OidParseResult { ok: false, oid: empty_oid };
            }

            const seg_start = pos;
            let val: u64 = 0 as u64;
            let digits: usize = 0;

            while (pos < s.len && s[pos] != (46 as u8)) {
                const ch = s[pos];
                if (ch < (48 as u8) || ch > (57 as u8)) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
                // Reject leading zero in multi-digit arc (e.g. "01", "00")
                if (digits == 1 && s[seg_start] == (48 as u8)) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
                val = val * (10 as u64) + ((ch - (48 as u8)) as u64);
                if (val > (0xFFFFFFFF as u64)) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
                digits = digits + 1;
                pos = pos + 1;
            }

            if (digits == 0) {
                return OidParseResult { ok: false, oid: empty_oid };
            }

            arcs[arc_count] = val as u32;
            arc_count = arc_count + 1;

            if (pos < s.len) {
                // Current char is '.' — must not be trailing dot
                pos = pos + 1;
                if (pos == s.len) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
            }
        }

        if (arc_count < 2) {
            return OidParseResult { ok: false, oid: empty_oid };
        }
        if (arcs[0] > (2 as u32)) {
            return OidParseResult { ok: false, oid: empty_oid };
        }
        if (arcs[0] < (2 as u32) && arcs[1] > (39 as u32)) {
            return OidParseResult { ok: false, oid: empty_oid };
        }

        let der = coidZeroDer();
        const dlen = coidEncodeArcsToDer(arcs, arc_count, der[0..OID_MAX_DER_LEN]);
        if (dlen == 0) {
            return OidParseResult { ok: false, oid: empty_oid };
        }

        return OidParseResult {
            ok: true,
            oid: ObjectIdentifier {
                arcs: arcs,
                len: arc_count,
                der_buf: der,
                der_len: dlen,
            },
        };
    }

    /// Parse canonical ITU-T X.690 §8.19 DER content bytes (value only, without 0x06 tag/length).
    /// Strictly rejects empty input, non-canonical leading 0x80 bytes, truncated VLQ, and overflow.
    pub static fn fromDer(der: []const u8): OidParseResult {
        const empty_oid = ObjectIdentifier.empty();
        if (der.len == 0 || der.len > OID_MAX_DER_LEN) {
            return OidParseResult { ok: false, oid: empty_oid };
        }

        let arcs = coidZeroArcs();
        let arc_count: usize = 0;
        let pos: usize = 0;
        let is_first = true;
        const max_first: u64 = (0xFFFFFFFF as u64) + (80 as u64);

        while (pos < der.len) {
            // X.690 §8.19.2: leading 0x80 byte is non-canonical and forbidden
            if (der[pos] == (0x80 as u8)) {
                return OidParseResult { ok: false, oid: empty_oid };
            }

            let val: u64 = 0 as u64;
            let done = false;
            while (pos < der.len) {
                const b = der[pos];
                pos = pos + 1;

                // Prevent u64 overflow before shifting left by 7
                if (val > (0x01FFFFFFFFFFFFFF as u64)) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
                val = (val << (7 as u6)) | ((b & (0x7F as u8)) as u64);

                if (is_first) {
                    if (val > max_first) {
                        return OidParseResult { ok: false, oid: empty_oid };
                    }
                } else {
                    if (val > (0xFFFFFFFF as u64)) {
                        return OidParseResult { ok: false, oid: empty_oid };
                    }
                }

                if ((b & (0x80 as u8)) == 0) {
                    done = true;
                    break;
                }
            }

            // Truncated multi-byte VLQ (ended with continuation bit set)
            if (!done) {
                return OidParseResult { ok: false, oid: empty_oid };
            }

            if (is_first) {
                if (val < (40 as u64)) {
                    arcs[0] = 0 as u32;
                    arcs[1] = val as u32;
                } else if (val < (80 as u64)) {
                    arcs[0] = 1 as u32;
                    arcs[1] = (val - (40 as u64)) as u32;
                } else {
                    arcs[0] = 2 as u32;
                    arcs[1] = (val - (80 as u64)) as u32;
                }
                arc_count = 2;
                is_first = false;
            } else {
                if (arc_count >= OID_MAX_ARCS) {
                    return OidParseResult { ok: false, oid: empty_oid };
                }
                arcs[arc_count] = val as u32;
                arc_count = arc_count + 1;
            }
        }

        let der_copy = coidZeroDer();
        let i: usize = 0;
        while (i < der.len) {
            der_copy[i] = der[i];
            i = i + 1;
        }

        return OidParseResult {
            ok: true,
            oid: ObjectIdentifier {
                arcs: arcs,
                len: arc_count,
                der_buf: der_copy,
                der_len: der.len,
            },
        };
    }

    /// Parse a complete DER TLV-encoded OID (Tag 0x06 + definite length + DER value).
    pub static fn fromDerTlv(tlv: []const u8): OidParseResult {
        const empty_oid = ObjectIdentifier.empty();
        if (tlv.len < 3 || tlv[0] != OID_TAG) {
            return OidParseResult { ok: false, oid: empty_oid };
        }
        const len_byte = tlv[1];
        // Since OID_MAX_DER_LEN == 80 < 128, canonical DER length is always single-byte short form
        if ((len_byte & (0x80 as u8)) != 0) {
            return OidParseResult { ok: false, oid: empty_oid };
        }
        const vlen = len_byte as usize;
        if (vlen == 0 || (2 as usize) + vlen != tlv.len) {
            return OidParseResult { ok: false, oid: empty_oid };
        }
        return ObjectIdentifier.fromDer(tlv[2..((2 as usize) + vlen)]);
    }

    /// Encode OID into X.690 DER content bytes (without tag/length).
    /// Returns number of bytes written, or 0 if `out` is too small or OID is empty.
    pub fn toDer(out: []u8): usize {
        if (_.der_len == 0 || out.len < _.der_len) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.der_len) {
            out[i] = _.der_buf[i];
            i = i + 1;
        }
        return _.der_len;
    }

    /// Encode OID into DER TLV bytes: Tag `0x06` + DER Length + DER Value.
    /// Returns total bytes written (`2 + derLen()`), or 0 if `out` is too small or OID is empty.
    pub fn toDerTlv(out: []u8): usize {
        if (_.der_len == 0 || _.der_len >= 128) {
            return 0;
        }
        const total = (2 as usize) + _.der_len;
        if (out.len < total) {
            return 0;
        }
        out[0] = OID_TAG;
        out[1] = _.der_len as u8;
        let i: usize = 0;
        while (i < _.der_len) {
            out[2 + i] = _.der_buf[i];
            i = i + 1;
        }
        return total;
    }

    /// Format OID as dotted-decimal ASCII string into `out`.
    /// Returns number of bytes written, or 0 if `out` is too small or OID is empty.
    pub fn toStr(out: []u8): usize {
        if (_.len == 0) {
            return 0;
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                if (pos >= out.len) {
                    return 0;
                }
                out[pos] = 46 as u8; // '.'
                pos = pos + 1;
            }
            pos = coidWriteU32Dec(_.arcs[i], out, pos);
            if (pos == 0) {
                return 0;
            }
            i = i + 1;
        }
        return pos;
    }

    /// Exact OID equality comparison.
    pub fn eq(other: ObjectIdentifier): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.arcs[i] != other.arcs[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    /// Return the number of arcs in this OID.
    pub fn arcCount(): usize {
        return _.len;
    }

    /// Return the arc at `idx`, or 0 if out of bounds.
    pub fn arcAt(idx: usize): u32 {
        if (idx >= _.len) {
            return 0 as u32;
        }
        return _.arcs[idx];
    }

    /// Return the DER content byte length.
    pub fn derLen(): usize {
        return _.der_len;
    }

    /// Return `true` if `self` is a prefix of `other` (including equal OIDs).
    pub fn isPrefixOf(other: ObjectIdentifier): bool {
        if (_.len == 0 || _.len > other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (_.arcs[i] != other.arcs[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }
}

export {
    OID_MAX_ARCS,
    OID_MAX_DER_LEN,
    OID_TAG,
    OID_RSA_ENCRYPTION,
    OID_SHA256_WITH_RSA,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    OID_ED25519,
    OID_X25519,
    OID_COMMON_NAME,
    OID_COUNTRY_NAME,
    OID_ORGANIZATION_NAME,
    OID_SHA256,
    OID_SHA384,
    OID_SHA512,
    OidParseResult,
    ObjectIdentifier,
};
