// @cap/oid — Zero-allocation ASN.1 Object Identifier (OID) X.690 DER encoder/decoder & PKI table for CP
// Clean-room MIT implementation inspired by Rust `const-oid` (690M downloads).

const MAX_ARCS: usize = 16;

pub struct Oid {
    pub valid: bool,
    pub arcs: [MAX_ARCS]u32,
    pub arc_count: usize,
}

fn invalidOid() -> Oid {
    let arcs: [MAX_ARCS]u32 = undefined;
    let i: usize = 0;
    while (i < MAX_ARCS) {
        arcs[i] = 0 as u32;
        i += 1;
    }
    return Oid{
        valid: false,
        arcs: arcs,
        arc_count: 0 as usize,
    };
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidOidShape(arcs: *[MAX_ARCS]u32, count: usize) -> bool {
    if (count < 2 || count > MAX_ARCS) {
        return false;
    }
    if (arcs[0] > (2 as u32)) {
        return false;
    }
    if (arcs[0] < (2 as u32) && arcs[1] >= (40 as u32)) {
        return false;
    }
    return true;
}

pub fn parseOid(dot_str: string) -> Oid {
    if (dot_str.len == 0) {
        return invalidOid();
    }

    let arcs: [MAX_ARCS]u32 = undefined;
    let initIdx: usize = 0;
    while (initIdx < MAX_ARCS) {
        arcs[initIdx] = 0 as u32;
        initIdx += 1;
    }

    let count: usize = 0;
    let i: usize = 0;
    while (i < dot_str.len) {
        if (count >= MAX_ARCS) {
            return invalidOid();
        }
        const segStart = i;
        let val: u64 = 0;
        while (i < dot_str.len && dot_str[i] != '.') {
            const c = dot_str[i];
            if (c < '0' || c > '9') {
                return invalidOid();
            }
            // Reject leading zero on multi-digit arc (e.g. "01")
            if (i == segStart + 1 && dot_str[segStart] == '0') {
                return invalidOid();
            }
            val = val * (10 as u64) + ((c - '0') as u64);
            if (val > (0xFFFFFFFF as u64)) {
                return invalidOid();
            }
            i += 1;
        }
        if (i == segStart) {
            return invalidOid();
        }
        arcs[count] = val as u32;
        count += 1;

        if (i < dot_str.len) {
            // Must be '.', and cannot be trailing '.'
            i += 1;
            if (i >= dot_str.len) {
                return invalidOid();
            }
        }
    }

    if (!isValidOidShape(&arcs, count)) {
        return invalidOid();
    }
    return Oid{
        valid: true,
        arcs: arcs,
        arc_count: count,
    };
}

fn appendU32(out: []u8, pos: usize, val_in: u32) -> usize {
    let tmp: [16]u8 = undefined;
    let n: usize = 0;
    let v = val_in;
    if (v == (0 as u32)) {
        tmp[0] = '0';
        n = 1 as usize;
    } else {
        while (v > (0 as u32) && n < 16) {
            tmp[n] = ((v % (10 as u32)) as u8) + '0';
            v = v / (10 as u32);
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = tmp[k];
        p += 1;
    }
    return p;
}

pub fn formatOid(oid: Oid, out: []u8) -> string {
    if (!oid.valid || oid.arc_count < 2) {
        return out[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < oid.arc_count && pos < out.len) {
        if (i > 0) {
            out[pos] = '.';
            pos += 1;
        }
        pos = appendU32(out, pos, oid.arcs[i]);
        i += 1;
    }
    return out[0..pos];
}

pub fn oidEquals(a: Oid, b: Oid) -> bool {
    if (!a.valid || !b.valid || a.arc_count != b.arc_count) {
        return false;
    }
    let i: usize = 0;
    while (i < a.arc_count) {
        if (a.arcs[i] != b.arcs[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn encodeBase128(val_in: u64, out: []u8, pos: usize) -> usize? {
    let tmp: [10]u8 = undefined;
    let n: usize = 0;
    let v = val_in;
    if (v == (0 as u64)) {
        tmp[0] = 0 as u8;
        n = 1 as usize;
    } else {
        while (v > (0 as u64) && n < 10) {
            tmp[n] = (v & (0x7F as u64)) as u8;
            v = v / (128 as u64);
            n += 1;
        }
    }
    if (pos + n > out.len) {
        return null;
    }
    let p = pos;
    let k = n;
    while (k > 0) {
        k -= 1;
        let b = tmp[k];
        if (k > 0) {
            b = b | (0x80 as u8);
        }
        out[p] = b;
        p += 1;
    }
    return p;
}

pub fn encodeOidDer(oid: Oid, out: []u8) -> string? {
    if (!oid.valid || oid.arc_count < 2 || oid.arc_count > MAX_ARCS) {
        return null;
    }
    if (oid.arcs[0] > (2 as u32) || (oid.arcs[0] < (2 as u32) && oid.arcs[1] >= (40 as u32))) {
        return null;
    }

    let pos: usize = 0;
    const firstCombined = ((oid.arcs[0] as u64) * (40 as u64)) + (oid.arcs[1] as u64);
    const nextPos0 = encodeBase128(firstCombined, out, pos);
    if (nextPos0 == null) {
        return null;
    }
    pos = nextPos0 ?? (0 as usize);

    let i: usize = 2;
    while (i < oid.arc_count) {
        const nextPos = encodeBase128(oid.arcs[i] as u64, out, pos);
        if (nextPos == null) {
            return null;
        }
        pos = nextPos ?? (0 as usize);
        i += 1;
    }

    return out[0..pos];
}

pub fn decodeOidDer(der_bytes: string) -> Oid {
    if (der_bytes.len == 0) {
        return invalidOid();
    }

    let arcs: [MAX_ARCS]u32 = undefined;
    let initIdx: usize = 0;
    while (initIdx < MAX_ARCS) {
        arcs[initIdx] = 0 as u32;
        initIdx += 1;
    }

    let count: usize = 0;
    let i: usize = 0;
    let isFirst = true;

    while (i < der_bytes.len) {
        // Reject non-canonical leading 0x80 padding (X.690 8.19.2)
        if (der_bytes[i] == (0x80 as u8)) {
            return invalidOid();
        }
        let val: u64 = 0;
        let terminated = false;
        while (i < der_bytes.len) {
            const b = der_bytes[i];
            i += 1;
            val = (val * (128 as u64)) + ((b & (0x7F as u8)) as u64);
            if (val > (0x100000050 as u64)) {
                return invalidOid();
            }
            if ((b & (0x80 as u8)) == (0 as u8)) {
                terminated = true;
                break;
            }
        }
        if (!terminated) {
            return invalidOid();
        }

        if (isFirst) {
            isFirst = false;
            if (val < (40 as u64)) {
                arcs[0] = 0 as u32;
                arcs[1] = val as u32;
            } else if (val < (80 as u64)) {
                arcs[0] = 1 as u32;
                arcs[1] = (val - (40 as u64)) as u32;
            } else {
                const second = val - (80 as u64);
                if (second > (0xFFFFFFFF as u64)) {
                    return invalidOid();
                }
                arcs[0] = 2 as u32;
                arcs[1] = second as u32;
            }
            count = 2 as usize;
        } else {
            if (count >= MAX_ARCS || val > (0xFFFFFFFF as u64)) {
                return invalidOid();
            }
            arcs[count] = val as u32;
            count += 1;
        }
    }

    if (!isValidOidShape(&arcs, count)) {
        return invalidOid();
    }
    return Oid{
        valid: true,
        arcs: arcs,
        arc_count: count,
    };
}

pub fn oidName(dot_str: string) -> string? {
    if (strEq(dot_str, "2.5.4.3")) { return "CN"; }
    if (strEq(dot_str, "2.5.4.6")) { return "C"; }
    if (strEq(dot_str, "2.5.4.7")) { return "L"; }
    if (strEq(dot_str, "2.5.4.8")) { return "ST"; }
    if (strEq(dot_str, "2.5.4.10")) { return "O"; }
    if (strEq(dot_str, "2.5.4.11")) { return "OU"; }
    if (strEq(dot_str, "1.2.840.113549.1.1.1")) { return "rsaEncryption"; }
    if (strEq(dot_str, "1.2.840.113549.1.1.11")) { return "sha256WithRSAEncryption"; }
    if (strEq(dot_str, "1.2.840.113549.1.1.12")) { return "sha384WithRSAEncryption"; }
    if (strEq(dot_str, "1.2.840.113549.1.1.13")) { return "sha512WithRSAEncryption"; }
    if (strEq(dot_str, "1.2.840.10045.2.1")) { return "ecPublicKey"; }
    if (strEq(dot_str, "1.2.840.10045.3.1.7")) { return "prime256v1"; }
    if (strEq(dot_str, "1.3.101.110")) { return "X25519"; }
    if (strEq(dot_str, "1.3.101.112")) { return "Ed25519"; }
    if (strEq(dot_str, "2.16.840.1.101.3.4.2.1")) { return "sha256"; }
    if (strEq(dot_str, "2.16.840.1.101.3.4.2.3")) { return "sha512"; }
    if (strEq(dot_str, "2.5.29.17")) { return "subjectAltName"; }
    if (strEq(dot_str, "2.5.29.19")) { return "basicConstraints"; }
    if (strEq(dot_str, "1.3.6.1.5.5.7.3.1")) { return "serverAuth"; }
    if (strEq(dot_str, "1.3.6.1.5.5.7.3.2")) { return "clientAuth"; }
    return null;
}

export {
    Oid,
    parseOid,
    formatOid,
    oidEquals,
    encodeOidDer,
    decodeOidDer,
    oidName,
};
