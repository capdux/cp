// @cap/oid-registry — Zero-allocation X.509/PKCS/TLS/NIST/RFC OID registry & DER codec
// Clean-room parity with crates.io:oid-registry (MIT OR Apache-2.0)

pub struct OidEntry {
    oid: string,
    short_name: string,
    description: string,
    category: string,
}

const OID_COUNT: usize = 36;

fn oidrEntryAt(idx: usize): OidEntry {
    switch (idx) {
        0 => return OidEntry{ oid: "2.5.4.3", short_name: "CN", description: "commonName", category: "X.500" },
        1 => return OidEntry{ oid: "2.5.4.6", short_name: "C", description: "countryName", category: "X.500" },
        2 => return OidEntry{ oid: "2.5.4.7", short_name: "L", description: "localityName", category: "X.500" },
        3 => return OidEntry{ oid: "2.5.4.8", short_name: "ST", description: "stateOrProvinceName", category: "X.500" },
        4 => return OidEntry{ oid: "2.5.4.10", short_name: "O", description: "organizationName", category: "X.500" },
        5 => return OidEntry{ oid: "2.5.4.11", short_name: "OU", description: "organizationalUnitName", category: "X.500" },
        6 => return OidEntry{ oid: "2.5.4.5", short_name: "serialNumber", description: "serialNumber", category: "X.500" },
        7 => return OidEntry{ oid: "2.5.29.14", short_name: "subjectKeyIdentifier", description: "Subject Key Identifier", category: "X.509" },
        8 => return OidEntry{ oid: "2.5.29.15", short_name: "keyUsage", description: "Key Usage", category: "X.509" },
        9 => return OidEntry{ oid: "2.5.29.17", short_name: "subjectAltName", description: "Subject Alternative Name", category: "X.509" },
        10 => return OidEntry{ oid: "2.5.29.19", short_name: "basicConstraints", description: "Basic Constraints", category: "X.509" },
        11 => return OidEntry{ oid: "2.5.29.31", short_name: "cRLDistributionPoints", description: "CRL Distribution Points", category: "X.509" },
        12 => return OidEntry{ oid: "2.5.29.35", short_name: "authorityKeyIdentifier", description: "Authority Key Identifier", category: "X.509" },
        13 => return OidEntry{ oid: "2.5.29.37", short_name: "extKeyUsage", description: "Extended Key Usage", category: "X.509" },
        14 => return OidEntry{ oid: "1.3.6.1.5.5.7.3.1", short_name: "serverAuth", description: "TLS Web Server Authentication", category: "PKIX" },
        15 => return OidEntry{ oid: "1.3.6.1.5.5.7.3.2", short_name: "clientAuth", description: "TLS Web Client Authentication", category: "PKIX" },
        16 => return OidEntry{ oid: "1.3.6.1.5.5.7.3.3", short_name: "codeSigning", description: "Code Signing", category: "PKIX" },
        17 => return OidEntry{ oid: "1.2.840.113549.1.1.1", short_name: "rsaEncryption", description: "RSA Encryption", category: "PKCS1" },
        18 => return OidEntry{ oid: "1.2.840.113549.1.1.5", short_name: "sha1WithRSAEncryption", description: "SHA-1 with RSA Encryption", category: "PKCS1" },
        19 => return OidEntry{ oid: "1.2.840.113549.1.1.10", short_name: "rsassaPss", description: "RSASSA-PSS", category: "PKCS1" },
        20 => return OidEntry{ oid: "1.2.840.113549.1.1.11", short_name: "sha256WithRSAEncryption", description: "SHA-256 with RSA Encryption", category: "PKCS1" },
        21 => return OidEntry{ oid: "1.2.840.113549.1.1.12", short_name: "sha384WithRSAEncryption", description: "SHA-384 with RSA Encryption", category: "PKCS1" },
        22 => return OidEntry{ oid: "1.2.840.113549.1.1.13", short_name: "sha512WithRSAEncryption", description: "SHA-512 with RSA Encryption", category: "PKCS1" },
        23 => return OidEntry{ oid: "1.2.840.10045.2.1", short_name: "ecPublicKey", description: "Elliptic Curve Public Key", category: "X9.62" },
        24 => return OidEntry{ oid: "1.2.840.10045.3.1.7", short_name: "prime256v1", description: "NIST P-256 / secp256r1", category: "X9.62" },
        25 => return OidEntry{ oid: "1.3.132.0.34", short_name: "secp384r1", description: "NIST P-384", category: "SECG" },
        26 => return OidEntry{ oid: "1.3.132.0.35", short_name: "secp521r1", description: "NIST P-521", category: "SECG" },
        27 => return OidEntry{ oid: "1.2.840.10045.4.3.2", short_name: "ecdsa-with-SHA256", description: "ECDSA with SHA-256", category: "X9.62" },
        28 => return OidEntry{ oid: "1.2.840.10045.4.3.3", short_name: "ecdsa-with-SHA384", description: "ECDSA with SHA-384", category: "X9.62" },
        29 => return OidEntry{ oid: "1.3.101.110", short_name: "X25519", description: "Curve25519 Diffie-Hellman", category: "RFC8410" },
        30 => return OidEntry{ oid: "1.3.101.112", short_name: "Ed25519", description: "EdDSA Signature over Curve25519", category: "RFC8410" },
        31 => return OidEntry{ oid: "2.16.840.1.101.3.4.2.1", short_name: "sha256", description: "SHA-256", category: "NIST" },
        32 => return OidEntry{ oid: "2.16.840.1.101.3.4.2.2", short_name: "sha384", description: "SHA-384", category: "NIST" },
        33 => return OidEntry{ oid: "2.16.840.1.101.3.4.2.3", short_name: "sha512", description: "SHA-512", category: "NIST" },
        34 => return OidEntry{ oid: "2.16.840.1.101.3.4.1.6", short_name: "aes128-GCM", description: "AES-128-GCM", category: "NIST" },
        else => return OidEntry{ oid: "2.16.840.1.101.3.4.1.46", short_name: "aes256-GCM", description: "AES-256-GCM", category: "NIST" },
    }
}

fn oidrStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

pub fn oidEntryCount(): usize {
    return OID_COUNT;
}

pub fn lookupOid(dotted: string): OidEntry? {
    let i: usize = 0;
    while (i < OID_COUNT) : (i += 1) {
        const entry = oidrEntryAt(i);
        if (oidrStrEq(entry.oid, dotted)) {
            return entry;
        }
    }
    return null;
}

pub fn lookupShortName(sn: string): OidEntry? {
    let i: usize = 0;
    while (i < OID_COUNT) : (i += 1) {
        const entry = oidrEntryAt(i);
        if (oidrStrEq(entry.short_name, sn)) {
            return entry;
        }
    }
    return null;
}

fn oidrEncodeBase128(val: u64, out: []u8, pos: usize): usize? {
    let tmp: [10]u8 = undefined;
    let count: usize = 0;
    let v: u64 = val;
    while (true) {
        tmp[count] = u8(v & 0x7F);
        count += 1;
        v = v >> 7;
        if (v == 0) {
            break;
        }
    }
    if (pos + count > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        let b: u8 = tmp[count - 1 - i];
        if (i + 1 < count) {
            b = b | 0x80;
        }
        out[pos + i] = b;
    }
    return pos + count;
}

fn oidrWriteU64Dec(val: u64, out: []u8, pos: usize): usize? {
    let tmp: [20]u8 = undefined;
    let count: usize = 0;
    let v: u64 = val;
    while (true) {
        tmp[count] = u8('0') + u8(v % 10);
        count += 1;
        v = v / 10;
        if (v == 0) {
            break;
        }
    }
    if (pos + count > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        out[pos + i] = tmp[count - 1 - i];
    }
    return pos + count;
}

pub fn encodeOidDer(dotted: string, out: []u8): usize {
    if (dotted.len == 0) {
        return 0;
    }
    let arcs: [32]u64 = undefined;
    let arc_count: usize = 0;
    let pos: usize = 0;

    while (pos <= dotted.len) {
        if (arc_count >= 32) {
            return 0;
        }
        let end: usize = pos;
        while (end < dotted.len && dotted[end] != '.') {
            end += 1;
        }
        if (end == pos) {
            return 0;
        }
        // Reject leading zero for multi-digit arcs
        if (end - pos > 1 && dotted[pos] == '0') {
            return 0;
        }
        let val: u64 = 0;
        let k: usize = pos;
        while (k < end) : (k += 1) {
            const c = dotted[k];
            if (c < '0' || c > '9') {
                return 0;
            }
            val = val * 10 + u64(c - '0');
        }
        arcs[arc_count] = val;
        arc_count += 1;
        if (end >= dotted.len) {
            break;
        }
        pos = end + 1;
    }

    if (arc_count < 2) {
        return 0;
    }
    const a0 = arcs[0];
    const a1 = arcs[1];
    if (a0 > 2) {
        return 0;
    }
    if (a0 < 2 && a1 >= 40) {
        return 0;
    }

    let w: usize = 0;
    if (nw := oidrEncodeBase128(a0 * 40 + a1, out, w)) {
        w = nw;
    } else {
        return 0;
    }
    let idx: usize = 2;
    while (idx < arc_count) : (idx += 1) {
        if (nw := oidrEncodeBase128(arcs[idx], out, w)) {
            w = nw;
        } else {
            return 0;
        }
    }
    return w;
}

struct OidrDecodeStep {
    val: u64,
    next_pos: usize,
    ok: bool,
}

fn oidrDecodeBase128(der: []const u8, start: usize): OidrDecodeStep {
    if (start >= der.len) {
        return OidrDecodeStep{ val: 0, next_pos: start, ok: false };
    }
    // Canonical DER forbids non-minimal leading 0x80
    if (der[start] == 0x80) {
        return OidrDecodeStep{ val: 0, next_pos: start, ok: false };
    }
    let val: u64 = 0;
    let pos: usize = start;
    let bytes: usize = 0;
    while (pos < der.len) {
        const b = der[pos];
        pos += 1;
        bytes += 1;
        if (bytes > 9) {
            return OidrDecodeStep{ val: 0, next_pos: pos, ok: false };
        }
        val = (val << 7) | u64(b & 0x7F);
        if ((b & 0x80) == 0) {
            return OidrDecodeStep{ val: val, next_pos: pos, ok: true };
        }
    }
    return OidrDecodeStep{ val: 0, next_pos: pos, ok: false };
}

pub fn decodeOidDer(der: []const u8, out: []u8): usize {
    if (der.len == 0) {
        return 0;
    }
    const first = oidrDecodeBase128(der, 0);
    if (!first.ok) {
        return 0;
    }

    let a0: u64 = 0;
    let a1: u64 = 0;
    if (first.val < 40) {
        a0 = 0;
        a1 = first.val;
    } else if (first.val < 80) {
        a0 = 1;
        a1 = first.val - 40;
    } else {
        a0 = 2;
        a1 = first.val - 80;
    }

    let w: usize = 0;
    if (w0 := oidrWriteU64Dec(a0, out, w)) {
        w = w0;
    } else {
        return 0;
    }
    if (w >= out.len) {
        return 0;
    }
    out[w] = '.';
    w += 1;
    if (w1 := oidrWriteU64Dec(a1, out, w)) {
        w = w1;
    } else {
        return 0;
    }

    let pos: usize = first.next_pos;
    while (pos < der.len) {
        const step = oidrDecodeBase128(der, pos);
        if (!step.ok) {
            return 0;
        }
        pos = step.next_pos;
        if (w >= out.len) {
            return 0;
        }
        out[w] = '.';
        w += 1;
        if (wn := oidrWriteU64Dec(step.val, out, w)) {
            w = wn;
        } else {
            return 0;
        }
    }
    return w;
}
