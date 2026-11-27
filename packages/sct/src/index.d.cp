// Type definitions for CP

export enum SctVersion {
    V1 = 0,
    Unknown = 255
}

export enum HashAlgorithm {
    None = 0,
    Md5 = 1,
    Sha1 = 2,
    Sha224 = 3,
    Sha256 = 4,
    Sha384 = 5,
    Sha512 = 6,
    Unknown = 255
}

export enum SignatureAlgorithm {
    Anonymous = 0,
    Rsa = 1,
    Dsa = 2,
    Ecdsa = 3,
    Ed25519 = 7,
    Ed448 = 8,
    Unknown = 255
}

export class SignedCertificateTimestamp {
    valid: bool,
    version: SctVersion,
    log_id: [32]u8,
    timestamp_ms: u64,
    extensions: &[]u8,
    hash_alg: HashAlgorithm,
    sig_alg: SignatureAlgorithm,
    signature: &[]u8,
}

export fn sctInvalid(): SignedCertificateTimestamp;
export fn sctReadBeU16(b: &[]u8, off: usize): u16;
export fn sctReadBeU64(b: &[]u8, off: usize): u64;
export fn sctWriteBeU16(v: u16, out: []u8, off: usize);
export fn sctWriteBeU64(v: u64, out: []u8, off: usize);
export fn sctDecodeHashAlg(b: u8): HashAlgorithm;
export fn sctEncodeHashAlg(alg: HashAlgorithm): u8;
export fn sctDecodeSigAlg(b: u8): SignatureAlgorithm;
export fn sctEncodeSigAlg(alg: SignatureAlgorithm): u8;
export fn parseSct(data: &[]u8): SignedCertificateTimestamp;
export fn parseSctList(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64;
export fn encodeSct(sct: SignedCertificateTimestamp, out: []u8): usize;
