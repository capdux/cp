// Origin: Pure-CP Stdlib (Zero-Alloc TCG TPM 2.0 Header, Attestation, Quote & Public Parser/Encoder) | Created: 2026-11-22

// TPM 2.0 Core Protocol Constants
pub const TPM_HEADER_SIZE: usize = 10;
pub const TPM_GENERATED_VALUE: u32 = 0xFF544347; // "\xFFTCG"

// Structure Tags (TPM_ST)
pub const TPM_ST_NO_SESSIONS: u16 = 0x8001;
pub const TPM_ST_SESSIONS: u16 = 0x8002;
pub const TPM_ST_ATTEST_CERTIFY: u16 = 0x8017;
pub const TPM_ST_ATTEST_QUOTE: u16 = 0x8018;
pub const TPM_ST_ATTEST_TIME: u16 = 0x8019;
pub const TPM_ST_ATTEST_CREATION: u16 = 0x801A;

// Algorithm IDs (TPM_ALG)
pub const TPM_ALG_RSA: u16 = 0x0001;
pub const TPM_ALG_SHA1: u16 = 0x0004;
pub const TPM_ALG_KEYEDHASH: u16 = 0x0008;
pub const TPM_ALG_SHA256: u16 = 0x000B;
pub const TPM_ALG_SHA384: u16 = 0x000C;
pub const TPM_ALG_SHA512: u16 = 0x000D;
pub const TPM_ALG_ECC: u16 = 0x0023;
pub const TPM_ALG_SYMCIPHER: u16 = 0x0025;

// Common Command Codes (TPM_CC)
pub const TPM_CC_STARTUP: u32 = 0x00000144;
pub const TPM_CC_QUOTE: u32 = 0x00000158;
pub const TPM_CC_GET_CAPABILITY: u32 = 0x0000017A;
pub const TPM_CC_GET_RANDOM: u32 = 0x0000017B;
pub const TPM_CC_PCR_READ: u32 = 0x0000017E;

// Response Codes (TPM_RC)
pub const TPM_RC_SUCCESS: u32 = 0x00000000;

// Object Attributes (TPMA_OBJECT)
pub const ATTR_FIXED_TPM: u32 = 0x00000002;
pub const ATTR_FIXED_PARENT: u32 = 0x00000010;
pub const ATTR_SENSITIVE_DATA_ORIGIN: u32 = 0x00000020;
pub const ATTR_USER_WITH_AUTH: u32 = 0x00000040;
pub const ATTR_RESTRICTED: u32 = 0x00010000;
pub const ATTR_DECRYPT: u32 = 0x00020000;
pub const ATTR_SIGN_ENCRYPT: u32 = 0x00040000;

fn readU16Be(data: []const u8, off: usize): u16 {
    const b0 = (data[off] as u16) << 8;
    const b1 = data[off + 1] as u16;
    return b0 | b1;
}

fn readU32Be(data: []const u8, off: usize): u32 {
    const b0 = (data[off] as u32) << 24;
    const b1 = (data[off + 1] as u32) << 16;
    const b2 = (data[off + 2] as u32) << 8;
    const b3 = data[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

fn readU64Be(data: []const u8, off: usize): u64 {
    const hi = (readU32Be(data, off) as u64) << 32;
    const lo = readU32Be(data, off + 4) as u64;
    return hi | lo;
}

fn writeU16Be(out: []u8, off: usize, v: u16) {
    out[off] = ((v >> 8) & 0xFF) as u8;
    out[off + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
}

/// Parsed 10-byte TPM 2.0 command/response header and zero-copy body slice.
pub struct TpmHeader {
    tag: u16,
    size: u32,
    code: u32,
    body: []const u8,

    /// Returns `true` if `tag == TPM_ST_SESSIONS (0x8002)`.
    fn hasSessions(): bool {
        return _.tag == TPM_ST_SESSIONS;
    }

    /// Returns `true` if `code == TPM_RC_SUCCESS (0x00000000)`.
    fn isSuccess(): bool {
        return _.code == TPM_RC_SUCCESS;
    }
}

/// Parses a 10-byte TPM 2.0 command/response header with strict validation:
/// - `tag == TPM_ST_NO_SESSIONS (0x8001)` or `TPM_ST_SESSIONS (0x8002)`
/// - `10 <= size <= data.len`
pub fn parseTpmHeader(data: []const u8): TpmHeader? {
    if (data.len < TPM_HEADER_SIZE) {
        return null;
    }
    const tag = readU16Be(data, 0);
    if (tag != TPM_ST_NO_SESSIONS && tag != TPM_ST_SESSIONS) {
        return null;
    }
    const size = readU32Be(data, 2);
    if (size < (TPM_HEADER_SIZE as u32) || (size as usize) > data.len) {
        return null;
    }
    const code = readU32Be(data, 6);
    const total = size as usize;
    return TpmHeader{
        .tag = tag,
        .size = size,
        .code = code,
        .body = data[TPM_HEADER_SIZE..total],
    };
}

/// Zero-alloc encoder for a 10-byte TPM 2.0 command/response header.
/// Returns `TPM_HEADER_SIZE` (10) on success, or `null` if `tag` is invalid,
/// `out.len < 10`, or `body_len + 10` overflows `u32`.
pub fn encodeTpmHeader(tag: u16, code: u32, body_len: usize, out: []u8): usize? {
    if (tag != TPM_ST_NO_SESSIONS && tag != TPM_ST_SESSIONS) {
        return null;
    }
    if (out.len < TPM_HEADER_SIZE) {
        return null;
    }
    const max_body: u64 = (0xFFFFFFFF as u64) - (TPM_HEADER_SIZE as u64);
    if ((body_len as u64) > max_body) {
        return null;
    }
    const total_size = ((TPM_HEADER_SIZE as u64) + (body_len as u64)) as u32;
    writeU16Be(out, 0, tag);
    writeU32Be(out, 2, total_size);
    writeU32Be(out, 6, code);
    return TPM_HEADER_SIZE;
}

/// Parsed 17-byte `TPMS_CLOCK_INFO` structure.
pub struct TpmClockInfo {
    clock: u64,
    reset_count: u32,
    restart_count: u32,
    safe: bool,
}

/// Parsed `TPMS_ATTEST` remote attestation structure (zero-copy slices).
pub struct TpmAttest {
    attest_type: u16,
    qualified_signer: []const u8,
    extra_data: []const u8,
    clock_info: TpmClockInfo,
    firmware_version: u64,
    attested_body: []const u8,

    /// Returns `true` if `attest_type == TPM_ST_ATTEST_QUOTE (0x8018)`.
    fn isQuote(): bool {
        return _.attest_type == TPM_ST_ATTEST_QUOTE;
    }

    /// Returns `true` if `attest_type == TPM_ST_ATTEST_CERTIFY (0x8017)`.
    fn isCertify(): bool {
        return _.attest_type == TPM_ST_ATTEST_CERTIFY;
    }
}

/// Returns `true` if `data` starts with the 4-byte `TPM_GENERATED_VALUE` (`0xFF544347`).
pub fn isTpmAttest(data: []const u8): bool {
    return data.len >= 4 && readU32Be(data, 0) == TPM_GENERATED_VALUE;
}

/// Parses a `TPMS_ATTEST` buffer with strict validation of `0xFF544347` magic,
/// 2-byte length-prefixed `TPM2B` fields (`qualifiedSigner`, `extraData`),
/// 17-byte `TPMS_CLOCK_INFO` (`safe` in `{0, 1}`), and 8-byte `firmwareVersion`.
pub fn parseTpmAttest(data: []const u8): TpmAttest? {
    if (data.len < 6 || !isTpmAttest(data)) {
        return null;
    }
    const attest_type = readU16Be(data, 4);
    let pos: usize = 6;

    // qualifiedSigner (TPM2B_NAME)
    if (data.len - pos < 2) {
        return null;
    }
    const qs_len = readU16Be(data, pos) as usize;
    pos += 2;
    if (data.len - pos < qs_len) {
        return null;
    }
    const qualified_signer = data[pos..(pos + qs_len)];
    pos += qs_len;

    // extraData (TPM2B_DATA)
    if (data.len - pos < 2) {
        return null;
    }
    const ed_len = readU16Be(data, pos) as usize;
    pos += 2;
    if (data.len - pos < ed_len) {
        return null;
    }
    const extra_data = data[pos..(pos + ed_len)];
    pos += ed_len;

    // TPMS_CLOCK_INFO (17B) + firmwareVersion (8B) = 25B
    if (data.len - pos < 25) {
        return null;
    }
    const clock = readU64Be(data, pos);
    const reset_count = readU32Be(data, pos + 8);
    const restart_count = readU32Be(data, pos + 12);
    const safe_byte = data[pos + 16];
    if (safe_byte > 1) {
        return null;
    }
    pos += 17;

    const firmware_version = readU64Be(data, pos);
    pos += 8;

    return TpmAttest{
        .attest_type = attest_type,
        .qualified_signer = qualified_signer,
        .extra_data = extra_data,
        .clock_info = TpmClockInfo{
            .clock = clock,
            .reset_count = reset_count,
            .restart_count = restart_count,
            .safe = safe_byte == 1,
        },
        .firmware_version = firmware_version,
        .attested_body = data[pos..data.len],
    };
}

/// Single `TPMS_PCR_SELECTION` entry (hash algorithm + PCR selection bitmask).
pub struct TpmPcrSelection {
    hash_alg: u16,
    pcr_select: []const u8,

    /// Returns `true` if PCR index `pcr_idx` is selected in the bitmask.
    fn isPcrSelected(pcr_idx: u8): bool {
        const byte_idx = (pcr_idx >> 3) as usize;
        if (byte_idx >= _.pcr_select.len) {
            return false;
        }
        const bit_mask = (1 as u8) << ((pcr_idx & 7) as u3);
        return (_.pcr_select[byte_idx] & bit_mask) != 0;
    }
}

/// Zero-copy iterator over `TPMS_PCR_SELECTION` entries inside `TPML_PCR_SELECTION`.
pub struct TpmPcrSelectIter {
    data: []const u8,
    pos: usize,
    remaining: u32,

    pub static fn init(data: []const u8, count: u32): TpmPcrSelectIter {
        return TpmPcrSelectIter{
            .data = data,
            .pos = 0,
            .remaining = count,
        };
    }

    fn next(*_): TpmPcrSelection? {
        if (_.remaining == 0 || _.data.len - _.pos < 3) {
            return null;
        }
        const hash_alg = readU16Be(_.data, _.pos);
        const sel_len = _.data[_.pos + 2] as usize;
        _.pos += 3;
        if (_.data.len - _.pos < sel_len) {
            _.remaining = 0;
            return null;
        }
        const pcr_select = _.data[_.pos..(_.pos + sel_len)];
        _.pos += sel_len;
        _.remaining -= 1;
        return TpmPcrSelection{
            .hash_alg = hash_alg,
            .pcr_select = pcr_select,
        };
    }
}

/// Parsed `TPMS_QUOTE_INFO` structure (`TPML_PCR_SELECTION` + `TPM2B_DIGEST`).
pub struct TpmQuoteInfo {
    pcr_select_count: u32,
    pcr_select_bytes: []const u8,
    pcr_digest: []const u8,

    /// Returns a zero-copy iterator over the `TPMS_PCR_SELECTION` entries.
    fn pcrSelections(): TpmPcrSelectIter {
        return TpmPcrSelectIter.init(_.pcr_select_bytes, _.pcr_select_count);
    }
}

/// Parses `TPMS_QUOTE_INFO` from `TPMS_ATTEST.attested_body`.
pub fn parseTpmQuoteInfo(attested_body: []const u8): TpmQuoteInfo? {
    if (attested_body.len < 4) {
        return null;
    }
    const count = readU32Be(attested_body, 0);
    // Each selection requires at least 3 bytes (2B hash + 1B sizeofSelect)
    if ((count as u64) * 3 > ((attested_body.len - 4) as u64)) {
        return null;
    }

    let pos: usize = 4;
    let i: u32 = 0;
    while (i < count) {
        if (attested_body.len - pos < 3) {
            return null;
        }
        const sel_len = attested_body[pos + 2] as usize;
        pos += 3;
        if (attested_body.len - pos < sel_len) {
            return null;
        }
        pos += sel_len;
        i += 1;
    }
    const pcr_select_bytes = attested_body[4..pos];

    // pcrDigest (TPM2B_DIGEST)
    if (attested_body.len - pos < 2) {
        return null;
    }
    const digest_len = readU16Be(attested_body, pos) as usize;
    pos += 2;
    if (attested_body.len - pos < digest_len) {
        return null;
    }
    const pcr_digest = attested_body[pos..(pos + digest_len)];

    return TpmQuoteInfo{
        .pcr_select_count = count,
        .pcr_select_bytes = pcr_select_bytes,
        .pcr_digest = pcr_digest,
    };
}

/// Parsed `TPMT_PUBLIC` header (algorithm type, nameAlg, objectAttributes, authPolicy, and remaining params/unique).
pub struct TpmPublicHeader {
    alg_type: u16,
    name_alg: u16,
    object_attributes: u32,
    auth_policy: []const u8,
    params_and_unique: []const u8,

    /// Returns `true` if `ATTR_RESTRICTED` (`0x00010000`) is set.
    fn isRestricted(): bool {
        return (_.object_attributes & ATTR_RESTRICTED) != 0;
    }

    /// Returns `true` if `ATTR_FIXED_TPM` (`0x00000002`) is set.
    fn isFixedTpm(): bool {
        return (_.object_attributes & ATTR_FIXED_TPM) != 0;
    }

    /// Returns `true` if `ATTR_SIGN_ENCRYPT` (`0x00040000`) is set.
    fn isSigningKey(): bool {
        return (_.object_attributes & ATTR_SIGN_ENCRYPT) != 0;
    }
}

/// Parses a `TPMT_PUBLIC` structure header (`type`, `nameAlg`, `objectAttributes`, `authPolicy`).
pub fn parseTpmPublicHeader(data: []const u8): TpmPublicHeader? {
    if (data.len < 10) {
        return null;
    }
    const alg_type = readU16Be(data, 0);
    const name_alg = readU16Be(data, 2);
    const object_attributes = readU32Be(data, 4);
    const policy_len = readU16Be(data, 8) as usize;
    if (data.len - 10 < policy_len) {
        return null;
    }
    const auth_policy = data[10..(10 + policy_len)];
    const params_and_unique = data[(10 + policy_len)..data.len];

    return TpmPublicHeader{
        .alg_type = alg_type,
        .name_alg = name_alg,
        .object_attributes = object_attributes,
        .auth_policy = auth_policy,
        .params_and_unique = params_and_unique,
    };
}
