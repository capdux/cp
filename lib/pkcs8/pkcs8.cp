// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5958 PKCS#8 & RFC 5280 SPKI DER Parser)

/// 常见密码学算法 OID（DER 内容字节，不含 0x06 Tag 与长度字节）
pub const OID_RSA_ENCRYPTION: [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01];
pub const OID_EC_PUBLIC_KEY: [7]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01];
pub const OID_P256: [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07];
pub const OID_P384: [5]u8 = [0x2B, 0x81, 0x04, 0x00, 0x22];
pub const OID_P521: [5]u8 = [0x2B, 0x81, 0x04, 0x00, 0x23];
pub const OID_X25519: [3]u8 = [0x2B, 0x65, 0x6E];
pub const OID_X448: [3]u8 = [0x2B, 0x65, 0x6F];
pub const OID_ED25519: [3]u8 = [0x2B, 0x65, 0x70];
pub const OID_ED448: [3]u8 = [0x2B, 0x65, 0x71];
pub const OID_PBES2: [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x05, 0x0D];
pub const OID_PBKDF2: [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x05, 0x0C];

/// 已识别的公钥/私钥算法类型
pub enum KeyAlgorithm: u8 {
    rsa = 0,
    ec_p256 = 1,
    ec_p384 = 2,
    ec_p521 = 3,
    ec_other = 4,
    ed25519 = 5,
    ed448 = 6,
    x25519 = 7,
    x448 = 8,
    unknown = 9,
}

/// RFC 5958 PKCS#8 `PrivateKeyInfo` / `OneAsymmetricKey` 零拷贝视图
pub struct PrivateKeyInfo {
    version: u8,
    alg_oid: []const u8,
    alg_params_der: []const u8,
    algorithm: KeyAlgorithm,
    private_key_bytes: []const u8,
    public_key_bytes: []const u8,

    /// 剥离 RFC 8410（Ed25519/X25519/Ed448/X448）`privateKey` 中内嵌的 `OCTET STRING`，返回裸私钥种子
    fn rawCurvePrivateKey(): ([]const u8)? {
        const expected_len: usize = match (_.algorithm) {
            .ed25519 => 32,
            .x25519 => 32,
            .x448 => 56,
            .ed448 => 57,
            else => return null,
        };
        const inner = parseDerTlvStrict(_.private_key_bytes) ?? return null;
        if (inner.tag != 0x04 || inner.total_bytes != _.private_key_bytes.len) {
            return null;
        }
        if (inner.value.len != expected_len) {
            return null;
        }
        return inner.value;
    }
}

/// RFC 5958 PKCS#8 `EncryptedPrivateKeyInfo` 零拷贝视图
pub struct EncryptedPrivateKeyInfo {
    enc_alg_oid: []const u8,
    enc_params_der: []const u8,
    encrypted_data: []const u8,

    /// 是否采用 PBES2（`1.2.840.113549.1.5.13`）加密方案
    fn isPbes2(): bool {
        return bytesEq(_.enc_alg_oid, OID_PBES2[0..9]);
    }
}

/// RFC 5280 `SubjectPublicKeyInfo` 零拷贝视图
pub struct SubjectPublicKeyInfo {
    alg_oid: []const u8,
    alg_params_der: []const u8,
    algorithm: KeyAlgorithm,
    public_key_bytes: []const u8,
}

// ---------------------------------------------------------------------------
// 内部严格 DER TLV 与 AlgorithmIdentifier 解析辅助函数
// ---------------------------------------------------------------------------

struct Tlv {
    tag: u8,
    value: []const u8,
    raw: []const u8,
    total_bytes: usize,
}

struct AlgId {
    oid: []const u8,
    params_der: []const u8,
    total_bytes: usize,
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseDerTlvStrict(data: []const u8): Tlv? {
    if (data.len < 2) return null;
    const tag: u8 = data[0];
    const len_first: u8 = data[1];
    let pos: usize = 2;
    let val_len_u64: u64 = 0;

    if ((len_first & 0x80) == 0) {
        val_len_u64 = len_first as u64;
    } else {
        if (len_first == 0x80) return null; // 拒绝不定长
        const n_octets: usize = (len_first & 0x7F) as usize;
        if (n_octets == 0 || n_octets > 8 || data.len - pos < n_octets) return null;
        if (data[pos] == 0x00) return null; // 拒绝前导零
        let k: usize = 0;
        while (k < n_octets) {
            val_len_u64 = (val_len_u64 << 8) | (data[pos + k] as u64);
            k += 1;
        }
        pos += n_octets;
        if (val_len_u64 < 128) return null; // 拒绝非最小长形式
    }

    const rem_u64: u64 = (data.len - pos) as u64;
    if (val_len_u64 > rem_u64) return null;

    const val_len: usize = val_len_u64 as usize;
    const total: usize = pos + val_len;
    return Tlv {
        tag: tag,
        value: data[pos..total],
        raw: data[0..total],
        total_bytes: total,
    };
}

fn parseAlgorithmIdentifier(data: []const u8): AlgId? {
    const seq = parseDerTlvStrict(data) ?? return null;
    if (seq.tag != 0x30) return null;

    const oid_tlv = parseDerTlvStrict(seq.value) ?? return null;
    if (oid_tlv.tag != 0x06 || oid_tlv.value.len == 0) return null;

    const empty = seq.value[0..0];
    let params_der: []const u8 = empty;
    if (oid_tlv.total_bytes < seq.value.len) {
        const rest = seq.value[oid_tlv.total_bytes..seq.value.len];
        const param_tlv = parseDerTlvStrict(rest) ?? return null;
        if (param_tlv.total_bytes != rest.len) {
            // AlgorithmIdentifier 内部不得有超过 1 个参数字段的尾随数据
            return null;
        }
        params_der = param_tlv.raw;
    }

    return AlgId {
        oid: oid_tlv.value,
        params_der: params_der,
        total_bytes: seq.total_bytes,
    };
}

fn identifyAlgorithm(oid: []const u8, params_der: []const u8): KeyAlgorithm {
    if (bytesEq(oid, OID_RSA_ENCRYPTION[0..9])) return .rsa;
    if (bytesEq(oid, OID_ED25519[0..3])) return .ed25519;
    if (bytesEq(oid, OID_ED448[0..3])) return .ed448;
    if (bytesEq(oid, OID_X25519[0..3])) return .x25519;
    if (bytesEq(oid, OID_X448[0..3])) return .x448;
    if (bytesEq(oid, OID_EC_PUBLIC_KEY[0..7])) {
        if (param := parseDerTlvStrict(params_der)) {
            if (param.tag == 0x06 && param.total_bytes == params_der.len) {
                if (bytesEq(param.value, OID_P256[0..8])) return .ec_p256;
                if (bytesEq(param.value, OID_P384[0..5])) return .ec_p384;
                if (bytesEq(param.value, OID_P521[0..5])) return .ec_p521;
            }
        }
        return .ec_other;
    }
    return .unknown;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 严格 DER 解析 RFC 5958 PKCS#8 `PrivateKeyInfo` / `OneAsymmetricKey`（v1/v2）
pub fn parsePkcs8PrivateKey(der_data: []const u8): PrivateKeyInfo? {
    const outer = parseDerTlvStrict(der_data) ?? return null;
    if (outer.tag != 0x30 || outer.total_bytes != der_data.len) {
        return null;
    }

    const body = outer.value;
    let pos: usize = 0;

    // 1. version INTEGER (0=v1, 1=v2)
    const ver_tlv = parseDerTlvStrict(body[pos..body.len]) ?? return null;
    if (ver_tlv.tag != 0x02 || ver_tlv.value.len != 1) {
        return null;
    }
    const version: u8 = ver_tlv.value[0];
    if (version > 1) {
        return null;
    }
    pos += ver_tlv.total_bytes;

    // 2. privateKeyAlgorithm AlgorithmIdentifier
    const alg = parseAlgorithmIdentifier(body[pos..body.len]) ?? return null;
    pos += alg.total_bytes;

    // 3. privateKey OCTET STRING
    const pk_tlv = parseDerTlvStrict(body[pos..body.len]) ?? return null;
    if (pk_tlv.tag != 0x04) {
        return null;
    }
    pos += pk_tlv.total_bytes;

    // 4. 可选 attributes [0] IMPLICIT (tag 0xA0)
    if (pos < body.len && body[pos] == 0xA0) {
        const attr_tlv = parseDerTlvStrict(body[pos..body.len]) ?? return null;
        pos += attr_tlv.total_bytes;
    }

    // 5. 可选 publicKey [1] IMPLICIT BIT STRING (tag 0x81，仅 v2 合法)
    const empty = body[0..0];
    const has_pub: bool = pos < body.len && body[pos] == 0x81;
    if (has_pub && version != 1) {
        return null;
    }
    const pub_opt = if (has_pub) parseDerTlvStrict(body[pos..body.len]) else null;
    if (has_pub && pub_opt == null) {
        return null;
    }
    const pub_tlv = pub_opt ?? Tlv { tag: 0, value: empty, raw: empty, total_bytes: 0 };
    if (has_pub) {
        if (pub_tlv.value.len < 1 || pub_tlv.value[0] != 0) {
            return null;
        }
        pos += pub_tlv.total_bytes;
    }
    const public_key_bytes = if (has_pub) pub_tlv.value[1..pub_tlv.value.len] else empty;

    // 严格禁止尾随垃圾
    if (pos != body.len) {
        return null;
    }

    return PrivateKeyInfo {
        version: version,
        alg_oid: alg.oid,
        alg_params_der: alg.params_der,
        algorithm: identifyAlgorithm(alg.oid, alg.params_der),
        private_key_bytes: pk_tlv.value,
        public_key_bytes: public_key_bytes,
    };
}

/// 严格 DER 解析 RFC 5958 PKCS#8 `EncryptedPrivateKeyInfo`
pub fn parsePkcs8Encrypted(der_data: []const u8): EncryptedPrivateKeyInfo? {
    const outer = parseDerTlvStrict(der_data) ?? return null;
    if (outer.tag != 0x30 || outer.total_bytes != der_data.len) {
        return null;
    }

    const body = outer.value;
    let pos: usize = 0;

    const alg = parseAlgorithmIdentifier(body[pos..body.len]) ?? return null;
    pos += alg.total_bytes;

    const enc_tlv = parseDerTlvStrict(body[pos..body.len]) ?? return null;
    if (enc_tlv.tag != 0x04) {
        return null;
    }
    pos += enc_tlv.total_bytes;

    if (pos != body.len) {
        return null;
    }

    return EncryptedPrivateKeyInfo {
        enc_alg_oid: alg.oid,
        enc_params_der: alg.params_der,
        encrypted_data: enc_tlv.value,
    };
}

/// 严格 DER 解析 RFC 5280 `SubjectPublicKeyInfo`（强制校验 BIT STRING `unused_bits == 0`）
pub fn parseSpkiPublicKey(der_data: []const u8): SubjectPublicKeyInfo? {
    const outer = parseDerTlvStrict(der_data) ?? return null;
    if (outer.tag != 0x30 || outer.total_bytes != der_data.len) {
        return null;
    }

    const body = outer.value;
    let pos: usize = 0;

    const alg = parseAlgorithmIdentifier(body[pos..body.len]) ?? return null;
    pos += alg.total_bytes;

    const spk_tlv = parseDerTlvStrict(body[pos..body.len]) ?? return null;
    if (spk_tlv.tag != 0x03 || spk_tlv.value.len < 1 || spk_tlv.value[0] != 0) {
        return null;
    }
    pos += spk_tlv.total_bytes;

    if (pos != body.len) {
        return null;
    }

    return SubjectPublicKeyInfo {
        alg_oid: alg.oid,
        alg_params_der: alg.params_der,
        algorithm: identifyAlgorithm(alg.oid, alg.params_der),
        public_key_bytes: spk_tlv.value[1..spk_tlv.value.len],
    };
}
