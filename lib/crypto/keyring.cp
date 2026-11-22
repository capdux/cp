// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Version KeyRing Rotation & Envelope MAC Verifier) | Created: 2026-11-22
// 100% pure-CP zero-heap-allocation key-version rotation manager with embedded RFC 6234 SHA-256 /
// RFC 2104 HMAC-SHA256 envelope signing, kid domain binding, constant-time MAC verification,
// graceful Active -> VerifyOnly rotation, expiry enforcement, and secret zeroization on revoke.

/// Fixed HMAC-SHA256 authentication tag length in bytes.
pub const KEYRING_TAG_LEN: usize = 32;

/// Fixed symmetric key material length in bytes (256-bit).
pub const KEYRING_SECRET_LEN: usize = 32;

// ============================================================================
// Pure-CP Zero-Alloc RFC 6234 SHA-256 & RFC 2104 HMAC-SHA256
// ============================================================================

const SHA256_K: [64]u32 = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

fn rotr32(x: u32, n: u5): u32 {
    const r: u5 = (32 - (n as u8)) as u5;
    return (x >> n) | (x << r);
}

fn readBeU32(b: []const u8, off: usize): u32 {
    return ((b[off] as u32) << 24) |
        ((b[off + 1] as u32) << 16) |
        ((b[off + 2] as u32) << 8) |
        (b[off + 3] as u32);
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
}

struct Sha256State {
    h: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    block: [64]u8 = [0; 64];
    block_len: usize = 0;
    total_bytes: u64 = 0;

    static fn init(): Sha256State {
        return Sha256State{};
    }

    fn compress(*_, blk: []const u8) {
        let w: [64]u32 = [0; 64];
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            w[i] = readBeU32(blk, i * 4);
        }
        while (i < 64) : (i += 1) {
            const s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] +% s0 +% w[i - 7] +% s1;
        }

        let a = _.h[0];
        let b = _.h[1];
        let c = _.h[2];
        let d = _.h[3];
        let e = _.h[4];
        let f = _.h[5];
        let g = _.h[6];
        let hh = _.h[7];

        let r: usize = 0;
        while (r < 64) : (r += 1) {
            const s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const ch = (e & f) ^ ((~e) & g);
            const temp1 = hh +% s1 +% ch +% SHA256_K[r] +% w[r];
            const s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            const maj = (a & b) ^ (a & c) ^ (b & c);
            const temp2 = s0 +% maj;

            hh = g;
            g = f;
            f = e;
            e = d +% temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 +% temp2;
        }

        _.h[0] = _.h[0] +% a;
        _.h[1] = _.h[1] +% b;
        _.h[2] = _.h[2] +% c;
        _.h[3] = _.h[3] +% d;
        _.h[4] = _.h[4] +% e;
        _.h[5] = _.h[5] +% f;
        _.h[6] = _.h[6] +% g;
        _.h[7] = _.h[7] +% hh;
    }

    fn update(*_, data: []const u8) {
        let pos: usize = 0;
        while (pos < data.len) : (pos += 1) {
            _.block[_.block_len] = data[pos];
            _.block_len += 1;
            _.total_bytes += 1;
            if (_.block_len == 64) {
                _.compress(_.block[0..64]);
                _.block_len = 0;
            }
        }
    }

    fn finish(*_, out: []u8) {
        const bit_len: u64 = _.total_bytes *% 8;
        _.block[_.block_len] = 0x80;
        _.block_len += 1;

        if (_.block_len > 56) {
            while (_.block_len < 64) : (_.block_len += 1) {
                _.block[_.block_len] = 0;
            }
            _.compress(_.block[0..64]);
            _.block_len = 0;
        }
        while (_.block_len < 56) : (_.block_len += 1) {
            _.block[_.block_len] = 0;
        }

        // Write 64-bit big-endian bit length
        _.block[56] = ((bit_len >> 56) & 0xFF) as u8;
        _.block[57] = ((bit_len >> 48) & 0xFF) as u8;
        _.block[58] = ((bit_len >> 40) & 0xFF) as u8;
        _.block[59] = ((bit_len >> 32) & 0xFF) as u8;
        _.block[60] = ((bit_len >> 24) & 0xFF) as u8;
        _.block[61] = ((bit_len >> 16) & 0xFF) as u8;
        _.block[62] = ((bit_len >> 8) & 0xFF) as u8;
        _.block[63] = (bit_len & 0xFF) as u8;
        _.compress(_.block[0..64]);

        let i: usize = 0;
        while (i < 8) : (i += 1) {
            writeBeU32(out, i * 4, _.h[i]);
        }
    }
}

fn computeEnvelopeMac(secret: [32]u8, kid: u32, payload: []const u8, out_tag: []u8) {
    let ipad: [64]u8 = [0x36; 64];
    let opad: [64]u8 = [0x5C; 64];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        ipad[i] = secret[i] ^ 0x36;
        opad[i] = secret[i] ^ 0x5C;
    }

    // Domain-bind kid (4-byte little-endian) into MAC input to prevent cross-version confusion
    let kid_le: [4]u8 = [
        (kid & 0xFF) as u8,
        ((kid >> 8) & 0xFF) as u8,
        ((kid >> 16) & 0xFF) as u8,
        ((kid >> 24) & 0xFF) as u8,
    ];

    let inner_hasher = Sha256State.init();
    inner_hasher.update(ipad[0..64]);
    inner_hasher.update(kid_le[0..4]);
    inner_hasher.update(payload);
    let inner_digest: [32]u8 = [0; 32];
    inner_hasher.finish(inner_digest[0..32]);

    let outer_hasher = Sha256State.init();
    outer_hasher.update(opad[0..64]);
    outer_hasher.update(inner_digest[0..32]);
    outer_hasher.finish(out_tag[0..32]);
}

fn constantTimeEq32(a: []const u8, b: []const u8): bool {
    if (a.len != 32 || b.len != 32) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

// ============================================================================
// Multi-Version KeyRing Rotation & Envelope Verifier
// ============================================================================

/// Lifecycle state of a key version stored in `KeyRing`.
pub enum KeyVersionState {
    Active,
    VerifyOnly,
    Revoked,
}

/// Single versioned symmetric key slot in `KeyRing`.
/// `expires_ms == 0` disables expiration; otherwise expired once `now_ms >= expires_ms`.
pub struct KeyRingEntry {
    kid: u32 = 0;
    secret: [32]u8 = [0; 32];
    state: KeyVersionState = KeyVersionState.VerifyOnly;
    created_ms: i64 = 0;
    expires_ms: i64 = 0;

    /// Returns `true` if this key has expired at `now_ms`.
    fn isExpired(now_ms: i64): bool {
        return _.expires_ms != 0 && now_ms >= _.expires_ms;
    }
}

/// Aggregate health snapshot of a `KeyRing` at a given timestamp.
pub struct KeyRingStats {
    total_keys: usize = 0;
    active_count: usize = 0;
    verify_only_count: usize = 0;
    revoked_count: usize = 0;
    expired_count: usize = 0;
}

/// Zero-heap-allocation multi-version keyring manager supporting zero-downtime key rotation,
/// kid-bound HMAC-SHA256 envelope signing, and constant-time verification.
pub struct KeyRing {
    slots: []KeyRingEntry;
    len: usize = 0;

    /// Initializes a `KeyRing` over caller-provided `slots`.
    static fn init(slots: []KeyRingEntry): KeyRing {
        return KeyRing{
            slots: slots,
            len: 0,
        };
    }

    /// Adds a new key version `(kid, secret)` to the ring.
    /// When `make_active == true`, any existing `Active` key is gracefully demoted to `VerifyOnly`.
    /// Rejects duplicate `kid`, full capacity, or invalid expiry (`expires_ms != 0 && expires_ms <= created_ms`).
    fn addKey(
        *_,
        kid: u32,
        secret: [32]u8,
        make_active: bool,
        created_ms: i64,
        expires_ms: i64,
    ): bool {
        if (_.len >= _.slots.len) return false;
        if (expires_ms != 0 && expires_ms <= created_ms) return false;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.slots[i].kid == kid) {
                return false;
            }
        }

        if (make_active) {
            let j: usize = 0;
            while (j < _.len) : (j += 1) {
                if (_.slots[j].state == KeyVersionState.Active) {
                    _.slots[j].state = KeyVersionState.VerifyOnly;
                }
            }
        }

        _.slots[_.len] = KeyRingEntry{
            kid: kid,
            secret: secret,
            state: if (make_active) KeyVersionState.Active else KeyVersionState.VerifyOnly,
            created_ms: created_ms,
            expires_ms: expires_ms,
        };
        _.len += 1;
        return true;
    }

    /// Revokes the key version matching `kid` and zeroes its in-memory secret bytes.
    /// Returns `false` if `kid` is not found.
    fn revokeKey(*_, kid: u32): bool {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.slots[i].kid == kid) {
                _.slots[i].state = KeyVersionState.Revoked;
                let k: usize = 0;
                while (k < 32) : (k += 1) {
                    _.slots[i].secret[k] = 0;
                }
                return true;
            }
        }
        return false;
    }

    /// Returns the currently active, non-expired signing key at `now_ms`, or `null` if none exists.
    fn activeKey(now_ms: i64): KeyRingEntry? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const e = _.slots[i];
            if (e.state == KeyVersionState.Active && !e.isExpired(now_ms)) {
                return e;
            }
        }
        return null;
    }

    /// Finds a verifiable key (`Active` or `VerifyOnly`, and not expired at `now_ms`) by `kid`.
    /// Returns `null` if not found, revoked, or expired.
    fn findVerifiableKey(kid: u32, now_ms: i64): KeyRingEntry? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const e = _.slots[i];
            if (e.kid == kid) {
                if (e.state == KeyVersionState.Revoked || e.isExpired(now_ms)) {
                    return null;
                }
                return e;
            }
        }
        return null;
    }

    /// Signs `payload` using the current non-expired `Active` key at `now_ms`, writing the 32-byte
    /// kid-bound HMAC-SHA256 tag into `out_tag_buf[0..32]` and returning the signing `kid`.
    /// Returns `null` if `out_tag_buf.len < 32` or no active valid key exists.
    fn signEnvelope(payload: []const u8, now_ms: i64, out_tag_buf: []u8): u32? {
        if (out_tag_buf.len < KEYRING_TAG_LEN) {
            return null;
        }
        const entry = _.activeKey(now_ms) ?? return null;
        computeEnvelopeMac(entry.secret, entry.kid, payload, out_tag_buf[0..32]);
        return entry.kid;
    }

    /// Verifies `expected_tag` (32 bytes) against `payload` using `kid` at `now_ms` in constant time.
    /// Accepts both `Active` and grace-period `VerifyOnly` keys; rejects `Revoked` or expired keys.
    fn verifyEnvelope(kid: u32, payload: []const u8, expected_tag: []const u8, now_ms: i64): bool {
        if (expected_tag.len != KEYRING_TAG_LEN) {
            return false;
        }
        const entry = _.findVerifiableKey(kid, now_ms) ?? return false;
        let computed: [32]u8 = [0; 32];
        computeEnvelopeMac(entry.secret, entry.kid, payload, computed[0..32]);
        return constantTimeEq32(computed[0..32], expected_tag);
    }

    /// Returns a snapshot of keyring version states at `now_ms`.
    fn stats(now_ms: i64): KeyRingStats {
        let st = KeyRingStats{ total_keys: _.len };
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const e = _.slots[i];
            switch (e.state) {
                KeyVersionState.Active => { st.active_count += 1; }
                KeyVersionState.VerifyOnly => { st.verify_only_count += 1; }
                KeyVersionState.Revoked => { st.revoked_count += 1; }
            }
            if (e.isExpired(now_ms)) {
                st.expired_count += 1;
            }
        }
        return st;
    }

    /// Returns the number of key versions currently stored.
    fn count(): usize {
        return _.len;
    }

    /// Returns the maximum capacity of the keyring.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Zeroes all stored secrets and clears the keyring for buffer reuse.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            let k: usize = 0;
            while (k < 32) : (k += 1) {
                _.slots[i].secret[k] = 0;
            }
        }
        _.len = 0;
    }
}
