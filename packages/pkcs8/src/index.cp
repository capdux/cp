// packages/@cap/pkcs8/src/index.cp
// Zero-allocation RFC 5958 PKCS#8 PrivateKeyInfo / OneAsymmetricKey & RFC 8410 Ed25519 codec.
// Reuses:
//   - AlgorithmIdentifier, encodeAlgorithmId, decodeAlgorithmId from @cap/spki
//   - ObjectIdentifier, OID_ED25519, OID_EC_PUBLIC_KEY, OID_PRIME256V1 from @cap/const-oid
//   - encodePem, decodePem from @cap/pem-rfc7468
//   - zeroizeBytes from @cap/subtle-ct
// Clean-room MIT implementation (RustCrypto `pkcs8` parity).

import {
    OID_ED25519,
    OID_EC_PUBLIC_KEY,
    OID_PRIME256V1,
    ObjectIdentifier,
} from "../../const-oid/src/index.cp";
import {
    AlgorithmIdentifier,
    encodeAlgorithmId,
    decodeAlgorithmId,
} from "../../spki/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle/src/index.cp";
import { bytesEq } from "cap:bin";

pub const PK8_VERSION_V1: u8 = 0;
pub const PK8_VERSION_V2: u8 = 1;
pub const PK8_MAX_PRIVKEY_LEN: usize = 160;
pub const PK8_MAX_PUBKEY_LEN: usize = 64;
pub const PK8_PEM_LABEL: string = "PRIVATE KEY";

pub const PK8_TAG_INTEGER: u8 = 0x02;
pub const PK8_TAG_OCTET_STRING: u8 = 0x04;
pub const PK8_TAG_SEQUENCE: u8 = 0x30;
pub const PK8_TAG_ATTRIBUTES_CTX0: u8 = 0xA0;
pub const PK8_TAG_PUBLIC_KEY_CTX1: u8 = 0x81;

pub const PK8_OK: i64 = 0;
pub const PK8_ERR_BUFFER_TOO_SMALL: i64 = -1;
pub const PK8_ERR_INVALID_VERSION: i64 = -2;
pub const PK8_ERR_INVALID_DER: i64 = -3;
pub const PK8_ERR_INVALID_TAG: i64 = -4;
pub const PK8_ERR_TRUNCATED: i64 = -5;
pub const PK8_ERR_INVALID_ALG: i64 = -6;
pub const PK8_ERR_KEY_TOO_LONG: i64 = -7;
pub const PK8_ERR_INVALID_PUBLIC_KEY: i64 = -8;
pub const PK8_ERR_INVALID_PEM: i64 = -9;
pub const PK8_ERR_INVALID_PEM_LABEL: i64 = -10;
pub const PK8_ERR_TRAILING_DATA: i64 = -11;
pub const PK8_ERR_INVALID_SEED: i64 = -12;

struct Pk8LenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

fn pk8ZeroPrivBuf(): [160]u8 {
    let b: [160]u8 = [0; 160];
    return b;
}

fn pk8ZeroPubBuf(): [64]u8 {
    let b: [64]u8 = [0; 64];
    return b;
}

fn pk8DerLenSize(n: usize): usize {
    if (n < 128) {
        return 1;
    }
    if (n < 256) {
        return 2;
    }
    return 3;
}

fn pk8WriteDerLen(out: []u8, pos: usize, n: usize): usize {
    if (n < 128) {
        out[pos] = n as u8;
        return pos + 1;
    }
    if (n < 256) {
        out[pos] = 0x81 as u8;
        out[pos + 1] = n as u8;
        return pos + 2;
    }
    out[pos] = 0x82 as u8;
    out[pos + 1] = ((n >> (8 as u6)) & 0xFF) as u8;
    out[pos + 2] = (n & 0xFF) as u8;
    return pos + 3;
}

fn pk8ReadDerLen(der: []const u8, pos: usize): Pk8LenDecode {
    if (pos >= der.len) {
        return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    const b0 = der[pos];
    if ((b0 & (0x80 as u8)) == 0) {
        const vlen = b0 as usize;
        const npos = pos + 1;
        if (vlen > der.len - npos) {
            return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
        return Pk8LenDecode { ok: true, val_len: vlen, next_pos: npos };
    }
    const nbytes = (b0 & (0x7F as u8)) as usize;
    if (nbytes == 0 || nbytes > 2 || pos + 1 + nbytes > der.len) {
        return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    let vlen: usize = 0;
    if (nbytes == 1) {
        vlen = der[pos + 1] as usize;
        if (vlen < 128) {
            return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    } else {
        vlen = ((der[pos + 1] as usize) << (8 as u6)) | (der[pos + 2] as usize);
        if (vlen < 256) {
            return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
        }
    }
    const npos = pos + 1 + nbytes;
    if (vlen > der.len - npos) {
        return Pk8LenDecode { ok: false, val_len: 0, next_pos: pos };
    }
    return Pk8LenDecode { ok: true, val_len: vlen, next_pos: npos };
}

/// RFC 5958 `PrivateKeyInfo` (v1) / `OneAsymmetricKey` (v2).
pub struct PrivateKeyInfo {
    pub version: u8,
    pub algorithm: AlgorithmIdentifier,
    pub private_key: [160]u8,
    pub private_key_len: usize,
    pub has_public_key: bool,
    pub public_key: [64]u8,
    pub public_key_len: usize,

    pub static fn empty(): PrivateKeyInfo {
        return PrivateKeyInfo {
            version: PK8_VERSION_V1,
            algorithm: AlgorithmIdentifier.empty(),
            private_key: pk8ZeroPrivBuf(),
            private_key_len: 0,
            has_public_key: false,
            public_key: pk8ZeroPubBuf(),
            public_key_len: 0,
        };
    }

    /// Constructs an RFC 8410 §7 Ed25519 PKCS#8 v1 `PrivateKeyInfo` from a 32-byte seed.
    /// Wraps the inner private key as DER OCTET STRING `04 20 || seed32` (34 bytes).
    pub static fn ed25519(seed32: []const u8): Pkcs8DecodeResult {
        if (seed32.len != 32) {
            return Pkcs8DecodeResult {
                ok: false,
                info: PrivateKeyInfo.empty(),
                err: PK8_ERR_INVALID_SEED,
            };
        }
        const oid_res = ObjectIdentifier.fromStr(OID_ED25519);
        if (!oid_res.ok) {
            return Pkcs8DecodeResult {
                ok: false,
                info: PrivateKeyInfo.empty(),
                err: PK8_ERR_INVALID_ALG,
            };
        }
        let pk = pk8ZeroPrivBuf();
        pk[0] = PK8_TAG_OCTET_STRING;
        pk[1] = 0x20 as u8;
        let i: usize = 0;
        while (i < 32) {
            pk[2 + i] = seed32[i];
            i = i + 1;
        }
        return Pkcs8DecodeResult {
            ok: true,
            info: PrivateKeyInfo {
                version: PK8_VERSION_V1,
                algorithm: AlgorithmIdentifier.fromOid(oid_res.oid),
                private_key: pk,
                private_key_len: 34,
                has_public_key: false,
                public_key: pk8ZeroPubBuf(),
                public_key_len: 0,
            },
            err: PK8_OK,
        };
    }

    /// Constructs an RFC 5958 v2 / RFC 8410 Ed25519 `OneAsymmetricKey` with embedded 32-byte public key.
    pub static fn ed25519V2(seed32: []const u8, pub32: []const u8): Pkcs8DecodeResult {
        if (seed32.len != 32 || pub32.len != 32) {
            return Pkcs8DecodeResult {
                ok: false,
                info: PrivateKeyInfo.empty(),
                err: PK8_ERR_INVALID_SEED,
            };
        }
        let base = PrivateKeyInfo.ed25519(seed32);
        if (!base.ok) {
            return base;
        }
        base.info.version = PK8_VERSION_V2;
        base.info.has_public_key = true;
        base.info.public_key_len = 32;
        let i: usize = 0;
        while (i < 32) {
            base.info.public_key[i] = pub32[i];
            i = i + 1;
        }
        return base;
    }

    /// Constructs a NIST P-256 (prime256v1) PKCS#8 v1 `PrivateKeyInfo` wrapping ECPrivateKey DER bytes.
    pub static fn p256(ec_priv_der: []const u8): Pkcs8DecodeResult {
        if (ec_priv_der.len == 0 || ec_priv_der.len > PK8_MAX_PRIVKEY_LEN) {
            return Pkcs8DecodeResult {
                ok: false,
                info: PrivateKeyInfo.empty(),
                err: PK8_ERR_KEY_TOO_LONG,
            };
        }
        const alg_oid = ObjectIdentifier.fromStr(OID_EC_PUBLIC_KEY);
        const curve_oid = ObjectIdentifier.fromStr(OID_PRIME256V1);
        if (!alg_oid.ok || !curve_oid.ok) {
            return Pkcs8DecodeResult {
                ok: false,
                info: PrivateKeyInfo.empty(),
                err: PK8_ERR_INVALID_ALG,
            };
        }
        let pk = pk8ZeroPrivBuf();
        let i: usize = 0;
        while (i < ec_priv_der.len) {
            pk[i] = ec_priv_der[i];
            i = i + 1;
        }
        return Pkcs8DecodeResult {
            ok: true,
            info: PrivateKeyInfo {
                version: PK8_VERSION_V1,
                algorithm: AlgorithmIdentifier.withParamOid(alg_oid.oid, curve_oid.oid),
                private_key: pk,
                private_key_len: ec_priv_der.len,
                has_public_key: false,
                public_key: pk8ZeroPubBuf(),
                public_key_len: 0,
            },
            err: PK8_OK,
        };
    }

    /// Securely wipes private and public key buffers using `@cap/subtle-ct` (`zeroizeBytes`).
    pub fn zeroize(*_) {
        zeroizeBytes(_.private_key[0..PK8_MAX_PRIVKEY_LEN]);
        zeroizeBytes(_.public_key[0..PK8_MAX_PUBKEY_LEN]);
        _.private_key_len = 0;
        _.public_key_len = 0;
        _.has_public_key = false;
    }
}

pub struct Pkcs8DecodeResult {
    pub ok: bool,
    pub info: PrivateKeyInfo,
    pub err: i64,
}

/// Encodes a `PrivateKeyInfo` (v1 or v2) into canonical RFC 5958 DER bytes.
/// Returns number of bytes written on success, or a negative `PK8_ERR_*` code.
pub fn encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64 {
    if (info.version > PK8_VERSION_V2) {
        return PK8_ERR_INVALID_VERSION;
    }
    if (info.version == PK8_VERSION_V1 && info.has_public_key) {
        return PK8_ERR_INVALID_PUBLIC_KEY;
    }
    if (info.private_key_len > PK8_MAX_PRIVKEY_LEN) {
        return PK8_ERR_KEY_TOO_LONG;
    }
    if (info.has_public_key && info.public_key_len > PK8_MAX_PUBKEY_LEN) {
        return PK8_ERR_KEY_TOO_LONG;
    }

    // Encode AlgorithmIdentifier via @cap/spki
    let alg_buf: [192]u8 = undefined;
    const alg_res = encodeAlgorithmId(info.algorithm, alg_buf[0..192]);
    if (alg_res < 0) {
        return PK8_ERR_INVALID_ALG;
    }
    const alg_len = alg_res as usize;

    // version INTEGER: 02 01 00 or 02 01 01 (3 bytes)
    const ver_tlv_len: usize = 3;
    // privateKey OCTET STRING
    const pk_tlv_len = (1 as usize) + pk8DerLenSize(info.private_key_len) + info.private_key_len;
    // Optional publicKey [1] IMPLICIT BIT STRING (Tag 0x81, unused_bits = 0x00)
    let pub_tlv_len: usize = 0;
    if (info.has_public_key) {
        const pub_content_len = (1 as usize) + info.public_key_len;
        pub_tlv_len = (1 as usize) + pk8DerLenSize(pub_content_len) + pub_content_len;
    }

    const seq_content_len = ver_tlv_len + alg_len + pk_tlv_len + pub_tlv_len;
    const total_len = (1 as usize) + pk8DerLenSize(seq_content_len) + seq_content_len;
    if (out.len < total_len) {
        return PK8_ERR_BUFFER_TOO_SMALL;
    }

    let pos: usize = 0;
    out[pos] = PK8_TAG_SEQUENCE;
    pos = pos + 1;
    pos = pk8WriteDerLen(out, pos, seq_content_len);

    // Write version
    out[pos] = PK8_TAG_INTEGER;
    out[pos + 1] = 0x01 as u8;
    out[pos + 2] = info.version;
    pos = pos + 3;

    // Write AlgorithmIdentifier
    let i: usize = 0;
    while (i < alg_len) {
        out[pos + i] = alg_buf[i];
        i = i + 1;
    }
    pos = pos + alg_len;

    // Write privateKey OCTET STRING
    out[pos] = PK8_TAG_OCTET_STRING;
    pos = pos + 1;
    pos = pk8WriteDerLen(out, pos, info.private_key_len);
    i = 0;
    while (i < info.private_key_len) {
        out[pos + i] = info.private_key[i];
        i = i + 1;
    }
    pos = pos + info.private_key_len;

    // Write optional publicKey [1] IMPLICIT BIT STRING
    if (info.has_public_key) {
        out[pos] = PK8_TAG_PUBLIC_KEY_CTX1;
        pos = pos + 1;
        pos = pk8WriteDerLen(out, pos, (1 as usize) + info.public_key_len);
        out[pos] = 0x00 as u8; // unused_bits = 0
        pos = pos + 1;
        i = 0;
        while (i < info.public_key_len) {
            out[pos + i] = info.public_key[i];
            i = i + 1;
        }
        pos = pos + info.public_key_len;
    }

    return pos as i64;
}

/// Decodes a canonical RFC 5958 DER `PrivateKeyInfo` (v1) or `OneAsymmetricKey` (v2).
/// Strictly validates version (0 or 1), AlgorithmIdentifier (via `@cap/spki`), privateKey OCTET STRING,
/// optional `[0]` attributes, optional `[1] IMPLICIT BIT STRING` publicKey (v2 only), and no trailing data.
pub fn decodePkcs8Der(der: []const u8): Pkcs8DecodeResult {
    const empty = PrivateKeyInfo.empty();
    if (der.len == 0) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRUNCATED };
    }
    if (der[0] != PK8_TAG_SEQUENCE) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_TAG };
    }

    const seq_len = pk8ReadDerLen(der, 1);
    if (!seq_len.ok) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_DER };
    }
    const seq_end = seq_len.next_pos + seq_len.val_len;
    if (seq_end != der.len) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRAILING_DATA };
    }

    let pos = seq_len.next_pos;

    // 1. version INTEGER (must be 0x00 for v1 or 0x01 for v2)
    if (pos + 3 > seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRUNCATED };
    }
    if (der[pos] != PK8_TAG_INTEGER) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_TAG };
    }
    if (der[pos + 1] != (0x01 as u8)) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_VERSION };
    }
    const ver = der[pos + 2];
    if (ver != PK8_VERSION_V1 && ver != PK8_VERSION_V2) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_VERSION };
    }
    pos = pos + 3;

    // 2. privateKeyAlgorithm AlgorithmIdentifier SEQUENCE (delegated to @cap/spki)
    if (pos >= seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRUNCATED };
    }
    if (der[pos] != PK8_TAG_SEQUENCE) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_TAG };
    }
    const alg_len = pk8ReadDerLen(der, pos + 1);
    if (!alg_len.ok || alg_len.next_pos + alg_len.val_len > seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_ALG };
    }
    const alg_end = alg_len.next_pos + alg_len.val_len;
    const alg_res = decodeAlgorithmId(der[pos..alg_end]);
    if (!alg_res.ok) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_ALG };
    }
    pos = alg_end;

    // 3. privateKey OCTET STRING (Tag 0x04)
    if (pos >= seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRUNCATED };
    }
    if (der[pos] != PK8_TAG_OCTET_STRING) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_TAG };
    }
    const pk_len = pk8ReadDerLen(der, pos + 1);
    if (!pk_len.ok || pk_len.next_pos + pk_len.val_len > seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_DER };
    }
    if (pk_len.val_len > PK8_MAX_PRIVKEY_LEN) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_KEY_TOO_LONG };
    }
    let pk_buf = pk8ZeroPrivBuf();
    let i: usize = 0;
    while (i < pk_len.val_len) {
        pk_buf[i] = der[pk_len.next_pos + i];
        i = i + 1;
    }
    pos = pk_len.next_pos + pk_len.val_len;

    // 4. Optional [0] IMPLICIT Attributes (Tag 0xA0) — skip if present
    if (pos < seq_end && der[pos] == PK8_TAG_ATTRIBUTES_CTX0) {
        const attr_len = pk8ReadDerLen(der, pos + 1);
        if (!attr_len.ok || attr_len.next_pos + attr_len.val_len > seq_end) {
            return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_DER };
        }
        pos = attr_len.next_pos + attr_len.val_len;
    }

    // 5. Optional [1] IMPLICIT BIT STRING publicKey (Tag 0x81, RFC 5958 v2 only)
    let has_pub = false;
    let pub_buf = pk8ZeroPubBuf();
    let pub_len: usize = 0;
    if (pos < seq_end && der[pos] == PK8_TAG_PUBLIC_KEY_CTX1) {
        if (ver != PK8_VERSION_V2) {
            return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_PUBLIC_KEY };
        }
        const pb_len = pk8ReadDerLen(der, pos + 1);
        if (!pb_len.ok || pb_len.val_len < 1 || pb_len.next_pos + pb_len.val_len > seq_end) {
            return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_PUBLIC_KEY };
        }
        const unused_bits = der[pb_len.next_pos];
        if (unused_bits != (0 as u8)) {
            return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_PUBLIC_KEY };
        }
        const raw_pub_len = pb_len.val_len - (1 as usize);
        if (raw_pub_len > PK8_MAX_PUBKEY_LEN) {
            return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_KEY_TOO_LONG };
        }
        i = 0;
        while (i < raw_pub_len) {
            pub_buf[i] = der[pb_len.next_pos + 1 + i];
            i = i + 1;
        }
        has_pub = true;
        pub_len = raw_pub_len;
        pos = pb_len.next_pos + pb_len.val_len;
    }

    if (pos != seq_end) {
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_TRAILING_DATA };
    }

    return Pkcs8DecodeResult {
        ok: true,
        info: PrivateKeyInfo {
            version: ver,
            algorithm: alg_res.alg,
            private_key: pk_buf,
            private_key_len: pk_len.val_len,
            has_public_key: has_pub,
            public_key: pub_buf,
            public_key_len: pub_len,
        },
        err: PK8_OK,
    };
}

/// Encodes a `PrivateKeyInfo` into RFC 7468 PEM (`-----BEGIN PRIVATE KEY-----`) via `@cap/pem-rfc7468`.
/// Returns number of bytes written on success, or a negative error code.
pub fn encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64 {
    let der_buf: [512]u8 = undefined;
    const dlen = encodePkcs8Der(info, der_buf[0..512]);
    if (dlen < 0) {
        return dlen;
    }
    const res = encodePem(PK8_PEM_LABEL, der_buf[0..(dlen as usize)], crlf, out);
    zeroizeBytes(der_buf[0..512]);
    return res;
}

/// Decodes an RFC 7468 PEM `PRIVATE KEY` block into `PrivateKeyInfo` via `@cap/pem-rfc7468`.
/// Strictly verifies that the PEM label is `"PRIVATE KEY"`.
pub fn decodePkcs8Pem(pem: string): Pkcs8DecodeResult {
    const empty = PrivateKeyInfo.empty();
    let label_buf: [64]u8 = undefined;
    let label_len: usize = 0;
    let der_buf: [512]u8 = undefined;

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..512]);
    if (dlen < 0) {
        zeroizeBytes(der_buf[0..512]);
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_PEM };
    }
    if (!bytesEq(PK8_PEM_LABEL, label_buf[0..label_len])) {
        zeroizeBytes(der_buf[0..512]);
        return Pkcs8DecodeResult { ok: false, info: empty, err: PK8_ERR_INVALID_PEM_LABEL };
    }

    const res = decodePkcs8Der(der_buf[0..(dlen as usize)]);
    zeroizeBytes(der_buf[0..512]);
    return res;
}
