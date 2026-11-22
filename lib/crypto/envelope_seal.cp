// Origin: Pure-CP Stdlib (Zero-Alloc Versioned Envelope Encryption & Key-Rotation Keyring) | Created: 2026-11-22
// Pure CP zero-heap-allocation Versioned Envelope Encryption Header, Keyring & Encrypt-then-MAC Engine:
//   - Compact binary `CPE1` envelope header (`magic|ver|alg|key_version|nonce_len|salt_len|aad_len|ct_len|nonce|salt|tag`)
//   - Fixed-capacity 16-key `EnvelopeKeyring` with 4-state lifecycle (`primary | active_decrypt_only | retired | revoked`),
//     atomic primary promotion, zeroization on revoke, and `needsRewrap(key_version)` re-encryption hints
//   - Zero-C-FFI end-to-end `sealEnvelope` / `openEnvelope` using RFC 5869 HKDF-SHA256 per-message subkeys,
//     streaming HMAC-SHA256 CTR keystream, and constant-time Encrypt-then-MAC authentication over Header + AAD + Ciphertext

const std = @import("std");

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// 4-byte ASCII magic prefix identifying CP Envelope v1 (`CPE1`).
pub const ENVELOPE_MAGIC: []const u8 = "CPE1";

/// Supported binary envelope wire format version.
pub const ENVELOPE_FORMAT_VERSION: u8 = 1;

/// Algorithm ID 1: HKDF-SHA256 + HMAC-SHA256-CTR + Encrypt-then-HMAC-SHA256.
pub const ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256: u8 = 1;

/// Fixed prefix byte length before variable `nonce`, `salt`, and 32B `tag`.
pub const ENVELOPE_FIXED_HEADER_LEN: usize = 20;

/// Authentication tag length in bytes (HMAC-SHA256).
pub const ENVELOPE_TAG_LEN: usize = 32;

/// Key-Encryption-Key (KEK) symmetric key length in bytes (256-bit).
pub const ENVELOPE_KEK_LEN: usize = 32;

/// Minimum allowed nonce length in bytes (96-bit).
pub const ENVELOPE_MIN_NONCE_LEN: usize = 12;

/// Maximum allowed nonce length in bytes (256-bit).
pub const ENVELOPE_MAX_NONCE_LEN: usize = 32;

/// Minimum allowed HKDF salt length in bytes (128-bit).
pub const ENVELOPE_MIN_SALT_LEN: usize = 16;

/// Maximum allowed HKDF salt length in bytes (256-bit).
pub const ENVELOPE_MAX_SALT_LEN: usize = 32;

/// Maximum number of versioned KEKs managed inline by `EnvelopeKeyring`.
pub const ENVELOPE_KEYRING_MAX_KEYS: usize = 16;

/// Result status code for envelope header codec, keyring, and seal/open operations.
pub enum EnvelopeStatus {
    ok,
    invalid_magic,
    unsupported_version,
    unsupported_algorithm,
    invalid_header_params,
    truncated_input,
    output_buffer_too_small,
    no_primary_key,
    key_not_found,
    key_revoked,
    auth_failed,
    keyring_full,
    duplicate_key_version,
    invalid_key_material,
}

/// Four-state rotation lifecycle for each versioned KEK inside `EnvelopeKeyring`.
pub enum EnvelopeKeyState {
    primary,
    active_decrypt_only,
    retired,
    revoked,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Parsed or to-be-encoded versioned envelope header (`CPE1`).
pub struct EnvelopeHeader {
    version: u8 = ENVELOPE_FORMAT_VERSION;
    alg_id: u8 = ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256;
    key_version: u32 = 0;
    nonce_len: u8 = 16;
    salt_len: u8 = 16;
    aad_len: u32 = 0;
    ct_len: u32 = 0;
    nonce: [ENVELOPE_MAX_NONCE_LEN]u8 = [0; ENVELOPE_MAX_NONCE_LEN];
    salt: [ENVELOPE_MAX_SALT_LEN]u8 = [0; ENVELOPE_MAX_SALT_LEN];
    tag: [ENVELOPE_TAG_LEN]u8 = [0; ENVELOPE_TAG_LEN];

    /// Total encoded wire length of this header in bytes.
    fn encodedLen(): usize {
        return ENVELOPE_FIXED_HEADER_LEN + (_.nonce_len as usize) + (_.salt_len as usize) + ENVELOPE_TAG_LEN;
    }

    /// Returns the active nonce bytes slice.
    fn nonceSlice(): []const u8 {
        const n: usize = if ((_.nonce_len as usize) <= ENVELOPE_MAX_NONCE_LEN) (_.nonce_len as usize) else ENVELOPE_MAX_NONCE_LEN;
        return _.nonce[0..n];
    }

    /// Returns the active HKDF salt bytes slice.
    fn saltSlice(): []const u8 {
        const s: usize = if ((_.salt_len as usize) <= ENVELOPE_MAX_SALT_LEN) (_.salt_len as usize) else ENVELOPE_MAX_SALT_LEN;
        return _.salt[0..s];
    }
}

/// Output returned by `decodeEnvelopeHeader`.
pub struct EnvelopeDecodeResult {
    status: EnvelopeStatus = EnvelopeStatus.ok;
    header: EnvelopeHeader = EnvelopeHeader{};
    header_len: usize = 0;
}

/// Output returned by `encodeEnvelopeHeader`, `sealEnvelope`, and `openEnvelope`.
pub struct EnvelopeOpResult {
    status: EnvelopeStatus = EnvelopeStatus.ok;
    bytes_written: usize = 0;
    key_version: u32 = 0;
    needs_rewrap: bool = false;
}

/// Single versioned KEK slot inside `EnvelopeKeyring`.
pub struct EnvelopeKeyEntry {
    occupied: bool = false;
    key_version: u32 = 0;
    state: EnvelopeKeyState = EnvelopeKeyState.retired;
    kek: [ENVELOPE_KEK_LEN]u8 = [0; ENVELOPE_KEK_LEN];
    created_at_ms: u64 = 0;
}

/// Operational telemetry snapshot for `EnvelopeKeyring`.
pub struct EnvelopeKeyringStats {
    total_keys: usize = 0;
    has_primary: bool = false;
    primary_version: u32 = 0;
    active_decrypt_only_count: usize = 0;
    retired_count: usize = 0;
    revoked_count: usize = 0;
}

// ============================================================================
// 3. Internal Low-Level Codec & Constant-Time Crypto Helpers
// ============================================================================

fn writeBeU32(dst: *[]u8, off: usize, v: u32) {
    dst[off] = ((v >> 24) & 0xFF) as u8;
    dst[off + 1] = ((v >> 16) & 0xFF) as u8;
    dst[off + 2] = ((v >> 8) & 0xFF) as u8;
    dst[off + 3] = (v & 0xFF) as u8;
}

fn readBeU32(src: []const u8, off: usize): u32 {
    return ((src[off] as u32) << 24) |
        ((src[off + 1] as u32) << 16) |
        ((src[off + 2] as u32) << 8) |
        (src[off + 3] as u32);
}

fn isAllZeroKey(k: [ENVELOPE_KEK_LEN]u8): bool {
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < ENVELOPE_KEK_LEN) : (i += 1) {
        acc |= k[i];
    }
    return acc == 0;
}

fn constantTimeTagEql(a: [ENVELOPE_TAG_LEN]u8, b: [ENVELOPE_TAG_LEN]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < ENVELOPE_TAG_LEN) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn validateHeaderParams(hdr: EnvelopeHeader): EnvelopeStatus {
    if (hdr.version != ENVELOPE_FORMAT_VERSION) {
        return EnvelopeStatus.unsupported_version;
    }
    if (hdr.alg_id != ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256) {
        return EnvelopeStatus.unsupported_algorithm;
    }
    const nl: usize = hdr.nonce_len as usize;
    const sl: usize = hdr.salt_len as usize;
    if (nl < ENVELOPE_MIN_NONCE_LEN || nl > ENVELOPE_MAX_NONCE_LEN) {
        return EnvelopeStatus.invalid_header_params;
    }
    if (sl < ENVELOPE_MIN_SALT_LEN || sl > ENVELOPE_MAX_SALT_LEN) {
        return EnvelopeStatus.invalid_header_params;
    }
    if (hdr.key_version == 0) {
        return EnvelopeStatus.invalid_header_params;
    }
    return EnvelopeStatus.ok;
}

struct DerivedSubkeys {
    enc_key: [32]u8 = [0; 32];
    mac_key: [32]u8 = [0; 32];
}

fn deriveMessageSubkeys(
    kek: [ENVELOPE_KEK_LEN]u8,
    alg_id: u8,
    key_version: u32,
    salt: []const u8,
    nonce: []const u8,
): DerivedSubkeys {
    const prk: [32]u8 = std.crypto.kdf.hkdf.HkdfSha256.extract(salt, kek[0..]);

    // Domain-separated HKDF info: "CPE1-SUBKEYS" (12B) | alg_id (1B) | key_version (4B BE) | nonce (<=32B)
    let info_buf: [64]u8 = [0; 64];
    const domain: []const u8 = "CPE1-SUBKEYS";
    let i: usize = 0;
    while (i < domain.len) : (i += 1) {
        info_buf[i] = domain[i];
    }
    info_buf[12] = alg_id;
    let info_slice: []u8 = info_buf[0..];
    writeBeU32(*info_slice, 13, key_version);
    let n_idx: usize = 0;
    while (n_idx < nonce.len) : (n_idx += 1) {
        info_buf[17 + n_idx] = nonce[n_idx];
    }
    const prefix_len: usize = 17;
    const info_len: usize = prefix_len + nonce.len;

    let okm: [64]u8 = [0; 64];
    std.crypto.kdf.hkdf.HkdfSha256.expand(okm[0..], info_buf[0..info_len], prk);

    let out = DerivedSubkeys{};
    let k: usize = 0;
    while (k < 32) : (k += 1) {
        out.enc_key[k] = okm[k];
        out.mac_key[k] = okm[32 + k];
    }
    return out;
}

fn xorCtrTransform(
    enc_key: [32]u8,
    nonce: []const u8,
    input: []const u8,
    out_buf: *[]u8,
    out_offset: usize,
) {
    let ctr_in: [ENVELOPE_MAX_NONCE_LEN + 4]u8 = [0; ENVELOPE_MAX_NONCE_LEN + 4];
    let i: usize = 0;
    while (i < nonce.len) : (i += 1) {
        ctr_in[i] = nonce[i];
    }
    const ctr_in_len: usize = nonce.len + 4;

    let pos: usize = 0;
    let counter: u32 = 0;
    while (pos < input.len) : (counter += 1) {
        let ctr_slice: []u8 = ctr_in[0..];
        writeBeU32(*ctr_slice, nonce.len, counter);

        let keystream: [32]u8 = [0; 32];
        std.crypto.auth.hmac.sha2.HmacSha256.create(&keystream, ctr_in[0..ctr_in_len], enc_key[0..]);

        let j: usize = 0;
        while (j < 32 && pos < input.len) : (j += 1) {
            out_buf[out_offset + pos] = input[pos] ^ keystream[j];
            pos += 1;
        }
    }
}

fn computeEnvelopeTag(
    mac_key: [32]u8,
    hdr: EnvelopeHeader,
    aad: []const u8,
    ciphertext: []const u8,
): [ENVELOPE_TAG_LEN]u8 {
    // Reconstruct canonical 20-byte fixed prefix so header tampering is cryptographically bound.
    let prefix: [ENVELOPE_FIXED_HEADER_LEN]u8 = [0; ENVELOPE_FIXED_HEADER_LEN];
    prefix[0] = 'C';
    prefix[1] = 'P';
    prefix[2] = 'E';
    prefix[3] = '1';
    prefix[4] = hdr.version;
    prefix[5] = hdr.alg_id;
    let p_slice: []u8 = prefix[0..];
    writeBeU32(*p_slice, 6, hdr.key_version);
    prefix[10] = hdr.nonce_len;
    prefix[11] = hdr.salt_len;
    writeBeU32(*p_slice, 12, hdr.aad_len);
    writeBeU32(*p_slice, 16, hdr.ct_len);

    let mac = std.crypto.auth.hmac.sha2.HmacSha256.init(mac_key[0..]);
    mac.update(prefix[0..]);
    mac.update(hdr.nonce[0..(hdr.nonce_len as usize)]);
    mac.update(hdr.salt[0..(hdr.salt_len as usize)]);
    mac.update(aad);
    mac.update(ciphertext);

    let tag: [ENVELOPE_TAG_LEN]u8 = [0; ENVELOPE_TAG_LEN];
    mac.final(&tag);
    return tag;
}

// ============================================================================
// 4. Deterministic Zero-Copy Envelope Header Codec
// ============================================================================

/// Encodes `hdr` into caller-owned `out_buf` in canonical big-endian `CPE1` binary format.
pub fn encodeEnvelopeHeader(hdr: EnvelopeHeader, out_buf: *[]u8): EnvelopeOpResult {
    const valid = validateHeaderParams(hdr);
    if (valid != EnvelopeStatus.ok) {
        return EnvelopeOpResult{ status: valid };
    }
    const need: usize = hdr.encodedLen();
    if (out_buf.len < need) {
        return EnvelopeOpResult{ status: EnvelopeStatus.output_buffer_too_small };
    }

    out_buf[0] = 'C';
    out_buf[1] = 'P';
    out_buf[2] = 'E';
    out_buf[3] = '1';
    out_buf[4] = hdr.version;
    out_buf[5] = hdr.alg_id;
    writeBeU32(out_buf, 6, hdr.key_version);
    out_buf[10] = hdr.nonce_len;
    out_buf[11] = hdr.salt_len;
    writeBeU32(out_buf, 12, hdr.aad_len);
    writeBeU32(out_buf, 16, hdr.ct_len);

    let off: usize = ENVELOPE_FIXED_HEADER_LEN;
    const nl: usize = hdr.nonce_len as usize;
    let i: usize = 0;
    while (i < nl) : (i += 1) {
        out_buf[off + i] = hdr.nonce[i];
    }
    off += nl;

    const sl: usize = hdr.salt_len as usize;
    let j: usize = 0;
    while (j < sl) : (j += 1) {
        out_buf[off + j] = hdr.salt[j];
    }
    off += sl;

    let t: usize = 0;
    while (t < ENVELOPE_TAG_LEN) : (t += 1) {
        out_buf[off + t] = hdr.tag[t];
    }

    return EnvelopeOpResult{
        status: EnvelopeStatus.ok,
        bytes_written: need,
        key_version: hdr.key_version,
        needs_rewrap: false,
    };
}

/// Decodes and validates a `CPE1` binary envelope header from `buf`.
pub fn decodeEnvelopeHeader(buf: []const u8): EnvelopeDecodeResult {
    if (buf.len < ENVELOPE_FIXED_HEADER_LEN) {
        return EnvelopeDecodeResult{ status: EnvelopeStatus.truncated_input };
    }
    if (buf[0] != 'C' || buf[1] != 'P' || buf[2] != 'E' || buf[3] != '1') {
        return EnvelopeDecodeResult{ status: EnvelopeStatus.invalid_magic };
    }

    let hdr = EnvelopeHeader{
        version: buf[4],
        alg_id: buf[5],
        key_version: readBeU32(buf, 6),
        nonce_len: buf[10],
        salt_len: buf[11],
        aad_len: readBeU32(buf, 12),
        ct_len: readBeU32(buf, 16),
    };

    const valid = validateHeaderParams(hdr);
    if (valid != EnvelopeStatus.ok) {
        return EnvelopeDecodeResult{ status: valid };
    }

    const total_hdr_len: usize = hdr.encodedLen();
    if (buf.len < total_hdr_len) {
        return EnvelopeDecodeResult{ status: EnvelopeStatus.truncated_input };
    }

    let off: usize = ENVELOPE_FIXED_HEADER_LEN;
    const nl: usize = hdr.nonce_len as usize;
    let i: usize = 0;
    while (i < nl) : (i += 1) {
        hdr.nonce[i] = buf[off + i];
    }
    off += nl;

    const sl: usize = hdr.salt_len as usize;
    let j: usize = 0;
    while (j < sl) : (j += 1) {
        hdr.salt[j] = buf[off + j];
    }
    off += sl;

    let t: usize = 0;
    while (t < ENVELOPE_TAG_LEN) : (t += 1) {
        hdr.tag[t] = buf[off + t];
    }

    return EnvelopeDecodeResult{
        status: EnvelopeStatus.ok,
        header: hdr,
        header_len: total_hdr_len,
    };
}

// ============================================================================
// 5. Multi-Version Key-Rotation Keyring (`EnvelopeKeyring`)
// ============================================================================

/// Zero-heap-allocation fixed-capacity (16 slots) key-rotation keyring for versioned envelopes.
pub struct EnvelopeKeyring {
    entries: [ENVELOPE_KEYRING_MAX_KEYS]EnvelopeKeyEntry = [EnvelopeKeyEntry{}; ENVELOPE_KEYRING_MAX_KEYS];
    count: usize = 0;

    /// Initializes an empty `EnvelopeKeyring`.
    static fn init(): EnvelopeKeyring {
        return EnvelopeKeyring{};
    }

    /// Finds the internal slot index for `key_version`, or `null` if not registered.
    fn findSlot(key_version: u32): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].key_version == key_version) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new versioned KEK into the keyring.
    /// If `state == primary`, any existing primary key is automatically demoted to `active_decrypt_only`.
    fn registerKey(
        *_,
        key_version: u32,
        kek: [ENVELOPE_KEK_LEN]u8,
        state: EnvelopeKeyState,
        created_at_ms: u64,
    ): EnvelopeStatus {
        if (key_version == 0 || isAllZeroKey(kek)) {
            return EnvelopeStatus.invalid_key_material;
        }
        if (_.findSlot(key_version) != null) {
            return EnvelopeStatus.duplicate_key_version;
        }
        if (_.count >= ENVELOPE_KEYRING_MAX_KEYS) {
            return EnvelopeStatus.keyring_full;
        }

        if (state == EnvelopeKeyState.primary) {
            let i: usize = 0;
            while (i < _.count) : (i += 1) {
                if (_.entries[i].occupied && _.entries[i].state == EnvelopeKeyState.primary) {
                    _.entries[i].state = EnvelopeKeyState.active_decrypt_only;
                }
            }
        }

        const idx: usize = _.count;
        _.entries[idx] = EnvelopeKeyEntry{
            occupied: true,
            key_version: key_version,
            state: state,
            kek: if (state == EnvelopeKeyState.revoked) [0; ENVELOPE_KEK_LEN] else kek,
            created_at_ms: created_at_ms,
        };
        _.count += 1;
        return EnvelopeStatus.ok;
    }

    /// Atomically promotes `key_version` to `primary`, demoting any previous `primary` to `active_decrypt_only`.
    /// Revoked keys cannot be promoted.
    fn promotePrimary(*_, key_version: u32): EnvelopeStatus {
        const target_idx: usize = _.findSlot(key_version) ?? return EnvelopeStatus.key_not_found;
        if (_.entries[target_idx].state == EnvelopeKeyState.revoked) {
            return EnvelopeStatus.key_revoked;
        }

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].state == EnvelopeKeyState.primary && i != target_idx) {
                _.entries[i].state = EnvelopeKeyState.active_decrypt_only;
            }
        }
        _.entries[target_idx].state = EnvelopeKeyState.primary;
        return EnvelopeStatus.ok;
    }

    /// Marks `key_version` as `retired` (still decryptable for migration, triggers `needsRewrap == true`).
    fn retireKey(*_, key_version: u32): EnvelopeStatus {
        const target_idx: usize = _.findSlot(key_version) ?? return EnvelopeStatus.key_not_found;
        if (_.entries[target_idx].state == EnvelopeKeyState.revoked) {
            return EnvelopeStatus.key_revoked;
        }
        _.entries[target_idx].state = EnvelopeKeyState.retired;
        return EnvelopeStatus.ok;
    }

    /// Permanently revokes `key_version` and scrubs its inline KEK bytes to zero.
    fn revokeKey(*_, key_version: u32): EnvelopeStatus {
        const target_idx: usize = _.findSlot(key_version) ?? return EnvelopeStatus.key_not_found;
        _.entries[target_idx].state = EnvelopeKeyState.revoked;
        _.entries[target_idx].kek = [0; ENVELOPE_KEK_LEN];
        return EnvelopeStatus.ok;
    }

    /// Returns the lifecycle state of `key_version`, or `null` if unknown.
    fn keyState(key_version: u32): EnvelopeKeyState? {
        if (idx := _.findSlot(key_version)) {
            return _.entries[idx].state;
        }
        return null;
    }

    /// Selects the active `primary` key entry for sealing new envelopes, or `null` if none exists.
    fn selectEncryptKey(): EnvelopeKeyEntry? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].state == EnvelopeKeyState.primary) {
                return _.entries[i];
            }
        }
        return null;
    }

    /// Resolves the key entry for decrypting `key_version`.
    /// Returns `null` if unknown or `revoked`.
    fn resolveDecryptKey(key_version: u32): EnvelopeKeyEntry? {
        if (idx := _.findSlot(key_version)) {
            if (_.entries[idx].state == EnvelopeKeyState.revoked) {
                return null;
            }
            return _.entries[idx];
        }
        return null;
    }

    /// Returns `true` if envelopes sealed under `key_version` should be re-encrypted under the current `primary` key.
    fn needsRewrap(key_version: u32): bool {
        if (primary := _.selectEncryptKey()) {
            return primary.key_version != key_version;
        }
        return true;
    }

    /// Returns a telemetry snapshot of key counts and current primary version.
    fn stats(): EnvelopeKeyringStats {
        let st = EnvelopeKeyringStats{
            total_keys: _.count,
        };
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.entries[i].occupied) continue;
            const ks = _.entries[i].state;
            if (ks == EnvelopeKeyState.primary) {
                st.has_primary = true;
                st.primary_version = _.entries[i].key_version;
            } else if (ks == EnvelopeKeyState.active_decrypt_only) {
                st.active_decrypt_only_count += 1;
            } else if (ks == EnvelopeKeyState.retired) {
                st.retired_count += 1;
            } else if (ks == EnvelopeKeyState.revoked) {
                st.revoked_count += 1;
            }
        }
        return st;
    }

    /// Scrubs all inline KEK bytes across the entire keyring.
    fn zeroize(*_) {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            _.entries[i].kek = [0; ENVELOPE_KEK_LEN];
        }
    }
}

// ============================================================================
// 6. End-to-End Zero-Alloc `sealEnvelope` & `openEnvelope`
// ============================================================================

/// Seals `plaintext` + `aad` under `keyring`'s active `primary` key into caller-owned `out_envelope`.
/// Layout written to `out_envelope`: `[CPE1 Header (with MAC tag)][Ciphertext]`.
pub fn sealEnvelope(
    keyring: EnvelopeKeyring,
    nonce: []const u8,
    salt: []const u8,
    aad: []const u8,
    plaintext: []const u8,
    out_envelope: *[]u8,
): EnvelopeOpResult {
    const key_entry: EnvelopeKeyEntry = keyring.selectEncryptKey() ?? return EnvelopeOpResult{
        status: EnvelopeStatus.no_primary_key,
    };

    if (nonce.len < ENVELOPE_MIN_NONCE_LEN || nonce.len > ENVELOPE_MAX_NONCE_LEN) {
        return EnvelopeOpResult{ status: EnvelopeStatus.invalid_header_params };
    }
    if (salt.len < ENVELOPE_MIN_SALT_LEN || salt.len > ENVELOPE_MAX_SALT_LEN) {
        return EnvelopeOpResult{ status: EnvelopeStatus.invalid_header_params };
    }

    let hdr = EnvelopeHeader{
        version: ENVELOPE_FORMAT_VERSION,
        alg_id: ENVELOPE_ALG_HKDF_CTR_HMAC_SHA256,
        key_version: key_entry.key_version,
        nonce_len: nonce.len as u8,
        salt_len: salt.len as u8,
        aad_len: aad.len as u32,
        ct_len: plaintext.len as u32,
    };

    let i: usize = 0;
    while (i < nonce.len) : (i += 1) {
        hdr.nonce[i] = nonce[i];
    }
    let j: usize = 0;
    while (j < salt.len) : (j += 1) {
        hdr.salt[j] = salt[j];
    }

    const hdr_len: usize = hdr.encodedLen();
    const total_len: usize = hdr_len + plaintext.len;
    if (out_envelope.len < total_len) {
        return EnvelopeOpResult{ status: EnvelopeStatus.output_buffer_too_small };
    }

    // 1. Derive per-message encryption & MAC subkeys via HKDF-SHA256.
    const subkeys = deriveMessageSubkeys(
        key_entry.kek,
        hdr.alg_id,
        hdr.key_version,
        salt,
        nonce,
    );

    // 2. Encrypt plaintext directly into `out_envelope[hdr_len..total_len]` using XOR-CTR.
    xorCtrTransform(subkeys.enc_key, nonce, plaintext, out_envelope, hdr_len);

    // 3. Encrypt-then-MAC over canonical header prefix + nonce + salt + AAD + ciphertext.
    hdr.tag = computeEnvelopeTag(
        subkeys.mac_key,
        hdr,
        aad,
        out_envelope[hdr_len..total_len],
    );

    // 4. Write encoded header into `out_envelope[0..hdr_len]`.
    const enc_res = encodeEnvelopeHeader(hdr, out_envelope);
    if (enc_res.status != EnvelopeStatus.ok) {
        return enc_res;
    }

    return EnvelopeOpResult{
        status: EnvelopeStatus.ok,
        bytes_written: total_len,
        key_version: key_entry.key_version,
        needs_rewrap: false,
    };
}

/// Verifies and opens `envelope` with `aad` using `keyring`, writing plaintext into `out_plaintext`.
/// Rejects revoked keys, truncated payloads, header/AAD/ciphertext tampering, and signals `needs_rewrap`
/// whenever the envelope was encrypted with a non-primary (rotated/retired) key version.
pub fn openEnvelope(
    keyring: EnvelopeKeyring,
    envelope: []const u8,
    aad: []const u8,
    out_plaintext: *[]u8,
): EnvelopeOpResult {
    const decoded = decodeEnvelopeHeader(envelope);
    if (decoded.status != EnvelopeStatus.ok) {
        return EnvelopeOpResult{ status: decoded.status };
    }

    const hdr = decoded.header;
    const ct_len: usize = hdr.ct_len as usize;
    const total_expected: usize = decoded.header_len + ct_len;
    if (envelope.len < total_expected) {
        return EnvelopeOpResult{ status: EnvelopeStatus.truncated_input };
    }
    if (out_plaintext.len < ct_len) {
        return EnvelopeOpResult{ status: EnvelopeStatus.output_buffer_too_small };
    }

    // Distinguish `key_not_found` vs `key_revoked` explicitly for operational security telemetry.
    const kstate = keyring.keyState(hdr.key_version) ?? return EnvelopeOpResult{
        status: EnvelopeStatus.key_not_found,
        key_version: hdr.key_version,
    };
    if (kstate == EnvelopeKeyState.revoked) {
        return EnvelopeOpResult{
            status: EnvelopeStatus.key_revoked,
            key_version: hdr.key_version,
            needs_rewrap: true,
        };
    }

    const key_entry = keyring.resolveDecryptKey(hdr.key_version) ?? return EnvelopeOpResult{
        status: EnvelopeStatus.key_not_found,
        key_version: hdr.key_version,
    };

    // Fast-fail if AAD length mismatches header metadata.
    if ((hdr.aad_len as usize) != aad.len) {
        return EnvelopeOpResult{
            status: EnvelopeStatus.auth_failed,
            key_version: hdr.key_version,
        };
    }

    const nonce_slice: []const u8 = hdr.nonce[0..(hdr.nonce_len as usize)];
    const salt_slice: []const u8 = hdr.salt[0..(hdr.salt_len as usize)];
    const ct_slice: []const u8 = envelope[decoded.header_len..total_expected];

    const subkeys = deriveMessageSubkeys(
        key_entry.kek,
        hdr.alg_id,
        hdr.key_version,
        salt_slice,
        nonce_slice,
    );

    // Constant-time Encrypt-then-MAC verification BEFORE decrypting any bytes.
    const expected_tag = computeEnvelopeTag(subkeys.mac_key, hdr, aad, ct_slice);
    if (!constantTimeTagEql(expected_tag, hdr.tag)) {
        return EnvelopeOpResult{
            status: EnvelopeStatus.auth_failed,
            key_version: hdr.key_version,
        };
    }

    xorCtrTransform(subkeys.enc_key, nonce_slice, ct_slice, out_plaintext, 0);

    return EnvelopeOpResult{
        status: EnvelopeStatus.ok,
        bytes_written: ct_len,
        key_version: hdr.key_version,
        needs_rewrap: keyring.needsRewrap(hdr.key_version),
    };
}
