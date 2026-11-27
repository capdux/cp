// @cap/sct — Zero-allocation RFC 6962 Signed Certificate Timestamp (SCT) parser & encoder
// Parity with rustls/sct.rs (crates.io:sct, Apache-2.0 OR MIT)

pub enum SctVersion: u8 {
    V1 = 0,
    Unknown = 255,
}

pub enum HashAlgorithm: u8 {
    None = 0,
    Md5 = 1,
    Sha1 = 2,
    Sha224 = 3,
    Sha256 = 4,
    Sha384 = 5,
    Sha512 = 6,
    Unknown = 255,
}

pub enum SignatureAlgorithm: u8 {
    Anonymous = 0,
    Rsa = 1,
    Dsa = 2,
    Ecdsa = 3,
    Ed25519 = 7,
    Ed448 = 8,
    Unknown = 255,
}

pub struct SignedCertificateTimestamp {
    valid: bool;
    version: SctVersion;
    log_id: [32]u8;
    timestamp_ms: u64;
    extensions: []const u8;
    hash_alg: HashAlgorithm;
    sig_alg: SignatureAlgorithm;
    signature: []const u8;
}

fn sctInvalid(): SignedCertificateTimestamp {
    const emptyLog: [32]u8 = @as([32]u8, [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ]);
    return SignedCertificateTimestamp{
        valid: false,
        version: SctVersion.Unknown,
        log_id: emptyLog,
        timestamp_ms: 0 as u64,
        extensions: "",
        hash_alg: HashAlgorithm.Unknown,
        sig_alg: SignatureAlgorithm.Unknown,
        signature: "",
    };
}

fn sctReadBeU16(b: []const u8, off: usize): u16 {
    return ((b[off] as u16) << 8) | (b[off + 1] as u16);
}

fn sctReadBeU64(b: []const u8, off: usize): u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        v = (v << 8) | (b[off + i] as u64);
        i += 1;
    }
    return v;
}

fn sctWriteBeU16(v: u16, out: []u8, off: usize) {
    out[off] = u8((v >> 8) & (0xFF as u16));
    out[off + 1] = u8(v & (0xFF as u16));
}

fn sctWriteBeU64(v: u64, out: []u8, off: usize) {
    let i: usize = 0;
    while (i < 8) {
        const shift = (7 - i) * 8;
        out[off + i] = u8((v >> shift) & (0xFF as u64));
        i += 1;
    }
}

fn sctDecodeHashAlg(b: u8): HashAlgorithm {
    if (b == (0 as u8)) { return HashAlgorithm.None; }
    if (b == (1 as u8)) { return HashAlgorithm.Md5; }
    if (b == (2 as u8)) { return HashAlgorithm.Sha1; }
    if (b == (3 as u8)) { return HashAlgorithm.Sha224; }
    if (b == (4 as u8)) { return HashAlgorithm.Sha256; }
    if (b == (5 as u8)) { return HashAlgorithm.Sha384; }
    if (b == (6 as u8)) { return HashAlgorithm.Sha512; }
    return HashAlgorithm.Unknown;
}

fn sctEncodeHashAlg(alg: HashAlgorithm): u8 {
    if (alg == HashAlgorithm.None) { return 0 as u8; }
    if (alg == HashAlgorithm.Md5) { return 1 as u8; }
    if (alg == HashAlgorithm.Sha1) { return 2 as u8; }
    if (alg == HashAlgorithm.Sha224) { return 3 as u8; }
    if (alg == HashAlgorithm.Sha256) { return 4 as u8; }
    if (alg == HashAlgorithm.Sha384) { return 5 as u8; }
    if (alg == HashAlgorithm.Sha512) { return 6 as u8; }
    return 255 as u8;
}

fn sctDecodeSigAlg(b: u8): SignatureAlgorithm {
    if (b == (0 as u8)) { return SignatureAlgorithm.Anonymous; }
    if (b == (1 as u8)) { return SignatureAlgorithm.Rsa; }
    if (b == (2 as u8)) { return SignatureAlgorithm.Dsa; }
    if (b == (3 as u8)) { return SignatureAlgorithm.Ecdsa; }
    if (b == (7 as u8)) { return SignatureAlgorithm.Ed25519; }
    if (b == (8 as u8)) { return SignatureAlgorithm.Ed448; }
    return SignatureAlgorithm.Unknown;
}

fn sctEncodeSigAlg(alg: SignatureAlgorithm): u8 {
    if (alg == SignatureAlgorithm.Anonymous) { return 0 as u8; }
    if (alg == SignatureAlgorithm.Rsa) { return 1 as u8; }
    if (alg == SignatureAlgorithm.Dsa) { return 2 as u8; }
    if (alg == SignatureAlgorithm.Ecdsa) { return 3 as u8; }
    if (alg == SignatureAlgorithm.Ed25519) { return 7 as u8; }
    if (alg == SignatureAlgorithm.Ed448) { return 8 as u8; }
    return 255 as u8;
}

/// Strictly parses a single RFC 6962 §3.2 SignedCertificateTimestamp wire message.
pub fn parseSct(data: []const u8): SignedCertificateTimestamp {
    // Minimum size: 1 (ver) + 32 (log_id) + 8 (ts) + 2 (ext_len) + 1 (hash) + 1 (sig) + 2 (sig_len) = 47
    if (data.len < 47) {
        return sctInvalid();
    }
    if (data[0] != (0 as u8)) {
        return sctInvalid();
    }
    let logId: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        logId[i] = data[1 + i];
        i += 1;
    }
    const ts = sctReadBeU64(data, 33);
    const extLen = sctReadBeU16(data, 41) as usize;
    if (43 + extLen + 4 > data.len) {
        return sctInvalid();
    }
    const extensions = data[43..(43 + extLen)];
    const pos = 43 + extLen;
    const hashAlg = sctDecodeHashAlg(data[pos]);
    const sigAlg = sctDecodeSigAlg(data[pos + 1]);
    if (hashAlg == HashAlgorithm.Unknown || sigAlg == SignatureAlgorithm.Unknown) {
        return sctInvalid();
    }
    const sigLen = sctReadBeU16(data, pos + 2) as usize;
    if (sigLen == 0 || pos + 4 + sigLen != data.len) {
        return sctInvalid();
    }
    const signature = data[(pos + 4)..(pos + 4 + sigLen)];
    return SignedCertificateTimestamp{
        valid: true,
        version: SctVersion.V1,
        log_id: logId,
        timestamp_ms: ts,
        extensions: extensions,
        hash_alg: hashAlg,
        sig_alg: sigAlg,
        signature: signature,
    };
}

/// Strictly parses an RFC 6962 §3.3 SignedCertificateTimestampList TLS extension.
/// Writes parsed SCTs into `out` and returns the number of SCTs parsed, or -1 on format/capacity error.
pub fn parseSctList(tls_ext: []const u8, out: []SignedCertificateTimestamp): i64 {
    if (tls_ext.len < 2) {
        return -1 as i64;
    }
    const totalLen = sctReadBeU16(tls_ext, 0) as usize;
    if (totalLen == 0 || 2 + totalLen != tls_ext.len) {
        return -1 as i64;
    }
    let pos: usize = 2;
    let count: usize = 0;
    while (pos < tls_ext.len) {
        if (pos + 2 > tls_ext.len) {
            return -1 as i64;
        }
        const itemLen = sctReadBeU16(tls_ext, pos) as usize;
        pos += 2;
        if (itemLen == 0 || pos + itemLen > tls_ext.len) {
            return -1 as i64;
        }
        const sct = parseSct(tls_ext[pos..(pos + itemLen)]);
        if (!sct.valid) {
            return -1 as i64;
        }
        if (count >= out.len) {
            return -1 as i64;
        }
        out[count] = sct;
        count += 1;
        pos += itemLen;
    }
    return count as i64;
}

/// Serializes a valid SignedCertificateTimestamp back into RFC 6962 §3.2 wire format.
/// Returns the number of bytes written to `out`, or 0 on error / insufficient space.
pub fn encodeSct(sct: SignedCertificateTimestamp, out: []u8): usize {
    if (!sct.valid || sct.version != SctVersion.V1) {
        return 0;
    }
    if (sct.extensions.len > 65535 || sct.signature.len == 0 || sct.signature.len > 65535) {
        return 0;
    }
    const hashByte = sctEncodeHashAlg(sct.hash_alg);
    const sigByte = sctEncodeSigAlg(sct.sig_alg);
    if (hashByte == (255 as u8) || sigByte == (255 as u8)) {
        return 0;
    }
    const needed = 47 + sct.extensions.len + sct.signature.len;
    if (needed > out.len) {
        return 0;
    }
    out[0] = 0 as u8;
    let i: usize = 0;
    while (i < 32) {
        out[1 + i] = sct.log_id[i];
        i += 1;
    }
    sctWriteBeU64(sct.timestamp_ms, out, 33);
    sctWriteBeU16(u16(sct.extensions.len), out, 41);
    let e: usize = 0;
    while (e < sct.extensions.len) {
        out[43 + e] = sct.extensions[e];
        e += 1;
    }
    const pos = 43 + sct.extensions.len;
    out[pos] = hashByte;
    out[pos + 1] = sigByte;
    sctWriteBeU16(u16(sct.signature.len), out, pos + 2);
    let s: usize = 0;
    while (s < sct.signature.len) {
        out[pos + 4 + s] = sct.signature[s];
        s += 1;
    }
    return needed;
}
