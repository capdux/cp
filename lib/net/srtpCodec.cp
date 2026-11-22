// Origin: Pure-CP Stdlib (RFC 3711 SRTP/SRTCP Codec, AES-128-CM KDF, ROC 48-Bit Index, 64-Bit Replay Window & RFC 5764 DTLS-SRTP) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice WebRTC secure media engine:
//   - Self-contained FIPS 197 AES-128-ECB/CM & FIPS 180-4 SHA-1 / RFC 2104 HMAC-SHA1 primitives
//   - RFC 5764 §4.2 DTLS-SRTP Exported Keying Material (EKM) 60-byte split (`client`/`server` master key & salt)
//   - RFC 3711 §4.3.1 AES-128-CM PRF Key Derivation Function (KDF, verified against RFC 3711 Appendix B.3)
//   - RFC 3711 §3.3.1 48-bit ROC packet index estimation & 64-bit sliding anti-replay bitmap window
//   - RFC 3711 SRTP (`protectRtp`/`unprotectRtp` with CSRC + extension header parsing & constant-time HMAC tag check)
//   - RFC 3711 §3.4 SRTCP (`protectRtcp`/`unprotectRtcp` with E-flag, 31-bit index & independent replay window)
//   - Deterministic RAII `~init()` key zeroization

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// AES-128 SRTP master/session encryption key length in bytes (`16`).
pub const SRTP_MASTER_KEY_LEN: usize = 16;

/// SRTP master/session salt length in bytes (`14`).
pub const SRTP_MASTER_SALT_LEN: usize = 14;

/// HMAC-SHA1 SRTP/SRTCP session authentication key length in bytes (`20`).
pub const SRTP_AUTH_KEY_LEN: usize = 20;

/// Truncated 80-bit HMAC-SHA1 authentication tag length in bytes (`10`).
pub const SRTP_HMAC_SHA1_80_TAG_LEN: usize = 10;

/// Truncated 32-bit HMAC-SHA1 authentication tag length in bytes (`4`).
pub const SRTP_HMAC_SHA1_32_TAG_LEN: usize = 4;

/// Anti-replay sliding window width in packets (`64`).
pub const SRTP_REPLAY_WINDOW_SIZE: usize = 64;

/// RFC 5764 §4.2 DTLS-SRTP EKM length for AES-128-CM profiles (`2 * (16 + 14) = 60` bytes).
pub const SRTP_DTLS_EKM_LEN: usize = 60;

const SRTP_MIN_RTP_HEADER_LEN: usize = 12;
const SRTP_MIN_RTCP_HEADER_LEN: usize = 8;
const SRTCP_INDEX_TRAILER_LEN: usize = 4;
const SRTCP_E_FLAG_MASK: u32 = 0x80000000;
const SRTCP_INDEX_MASK: u32 = 0x7FFFFFFF;

/// Error set returned by SRTP / SRTCP cryptographic and framing operations.
pub err SrtpError {
    PacketTooShort,
    AuthTagMismatch,
    ReplayDetected,
    PacketTooOld,
    BufferTooSmall,
    InvalidProfile,
    InvalidKeyLen,
    IndexOverflow,
}

/// Supported RFC 3711 / RFC 5764 SRTP cryptographic profiles.
pub enum SrtpProfile {
    aes128_cm_hmac_sha1_80,
    aes128_cm_hmac_sha1_32,
    null_cipher_hmac_sha1_80,

    /// Returns the SRTP authentication tag length in bytes (`10` or `4`).
    fn rtpTagLen(): usize {
        return match (_) {
            SrtpProfile.aes128_cm_hmac_sha1_80 => SRTP_HMAC_SHA1_80_TAG_LEN,
            SrtpProfile.aes128_cm_hmac_sha1_32 => SRTP_HMAC_SHA1_32_TAG_LEN,
            SrtpProfile.null_cipher_hmac_sha1_80 => SRTP_HMAC_SHA1_80_TAG_LEN,
        };
    }

    /// Returns the SRTCP authentication tag length in bytes (always `10` per RFC 3711 §5.2).
    fn rtcpTagLen(): usize {
        return SRTP_HMAC_SHA1_80_TAG_LEN;
    }

    /// Returns `true` when AES-128-CM payload encryption is enabled.
    fn usesEncryption(): bool {
        return match (_) {
            SrtpProfile.aes128_cm_hmac_sha1_80 => true,
            SrtpProfile.aes128_cm_hmac_sha1_32 => true,
            SrtpProfile.null_cipher_hmac_sha1_80 => false,
        };
    }
}

// ============================================================================
// 2. Self-Contained Pure-CP FIPS 197 AES-128-ECB/CM & SHA-1 / HMAC-SHA1
// ============================================================================

const AES_SBOX: [256]u8 = [
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
];

const AES_RCON: [10]u8 = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36];

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32);
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn rotl32(x: u32, n: u5): u32 {
    return (x << n) | (x >> (@as(u5, @intCast(32 - @as(u32, n)))));
}

fn gfMul2(x: u8): u8 {
    const hi: u8 = if ((x & 0x80) != 0) 0x1B else 0x00;
    return (x << 1) ^ hi;
}

struct Aes128Ctx {
    rk: [176]u8 = [0; 176];

    static fn init(key: [16]u8): Aes128Ctx {
        let ctx = Aes128Ctx{};
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            ctx.rk[i] = key[i];
        }
        let round: usize = 0;
        while (round < 10) : (round += 1) {
            const prev: usize = round * 16;
            const cur: usize = (round + 1) * 16;
            // RotWord + SubWord + Rcon on last 4 bytes of previous round key
            ctx.rk[cur + 0] = ctx.rk[prev + 0] ^ AES_SBOX[ctx.rk[prev + 13] as usize] ^ AES_RCON[round];
            ctx.rk[cur + 1] = ctx.rk[prev + 1] ^ AES_SBOX[ctx.rk[prev + 14] as usize];
            ctx.rk[cur + 2] = ctx.rk[prev + 2] ^ AES_SBOX[ctx.rk[prev + 15] as usize];
            ctx.rk[cur + 3] = ctx.rk[prev + 3] ^ AES_SBOX[ctx.rk[prev + 12] as usize];

            let b: usize = 4;
            while (b < 16) : (b += 1) {
                ctx.rk[cur + b] = ctx.rk[prev + b] ^ ctx.rk[cur + b - 4];
            }
        }
        return ctx;
    }

    fn encryptBlock(input: [16]u8): [16]u8 {
        let s: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            s[i] = input[i] ^ rk[i];
        }

        let round: usize = 1;
        while (round <= 10) : (round += 1) {
            // SubBytes
            i = 0;
            while (i < 16) : (i += 1) {
                s[i] = AES_SBOX[s[i] as usize];
            }

            // ShiftRows (column-major state)
            let tmp: u8 = s[1];
            s[1] = s[5];
            s[5] = s[9];
            s[9] = s[13];
            s[13] = tmp;

            tmp = s[2];
            s[2] = s[10];
            s[10] = tmp;
            tmp = s[6];
            s[6] = s[14];
            s[14] = tmp;

            tmp = s[15];
            s[15] = s[11];
            s[11] = s[7];
            s[7] = s[3];
            s[3] = tmp;

            // MixColumns (rounds 1..9)
            if (round < 10) {
                let col: usize = 0;
                while (col < 4) : (col += 1) {
                    const o: usize = col * 4;
                    const a0 = s[o];
                    const a1 = s[o + 1];
                    const a2 = s[o + 2];
                    const a3 = s[o + 3];
                    const t: u8 = a0 ^ a1 ^ a2 ^ a3;
                    s[o + 0] = a0 ^ t ^ gfMul2(a0 ^ a1);
                    s[o + 1] = a1 ^ t ^ gfMul2(a1 ^ a2);
                    s[o + 2] = a2 ^ t ^ gfMul2(a2 ^ a3);
                    s[o + 3] = a3 ^ t ^ gfMul2(a3 ^ a0);
                }
            }

            // AddRoundKey
            const rk_off: usize = round * 16;
            i = 0;
            while (i < 16) : (i += 1) {
                s[i] ^= rk[rk_off + i];
            }
        }
        return s;
    }
}

/// Applies AES-128-CM keystream (16-bit big-endian block counter in `iv[14..16]`) XORing `src` into `dst`.
fn aes128CmXor(key: [16]u8, iv_base: [16]u8, src: []const u8, dst: []u8) {
    const aes = Aes128Ctx.init(key);
    let ctr_block: [16]u8 = iv_base;
    let block_idx: u16 = 0;
    let off: usize = 0;
    while (off < src.len) {
        writeBeU16(ctr_block[0..], 14, block_idx);
        const ks: [16]u8 = aes.encryptBlock(ctr_block);
        let j: usize = 0;
        while (j < 16 && off + j < src.len) : (j += 1) {
            dst[off + j] = src[off + j] ^ ks[j];
        }
        off += 16;
        block_idx = block_idx +% 1;
    }
}

/// Generates `out.len` keystream bytes from AES-128-CM (equivalent to encrypting all-zero plaintext).
fn aes128CmKeystream(key: [16]u8, iv_base: [16]u8, out: []u8) {
    const aes = Aes128Ctx.init(key);
    let ctr_block: [16]u8 = iv_base;
    let block_idx: u16 = 0;
    let off: usize = 0;
    while (off < out.len) {
        writeBeU16(ctr_block[0..], 14, block_idx);
        const ks: [16]u8 = aes.encryptBlock(ctr_block);
        let j: usize = 0;
        while (j < 16 && off + j < out.len) : (j += 1) {
            out[off + j] = ks[j];
        }
        off += 16;
        block_idx = block_idx +% 1;
    }
}

struct Sha1State {
    h: [5]u32 = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];
    buf: [64]u8 = [0; 64];
    buf_len: usize = 0;
    total_bytes: u64 = 0;

    static fn init(): Sha1State {
        return Sha1State{};
    }

    fn compress(*_, block: []const u8) {
        let w: [80]u32 = [0; 80];
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            w[i] = readBeU32(block, i * 4);
        }
        while (i < 80) : (i += 1) {
            w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        let a: u32 = h[0];
        let b: u32 = h[1];
        let c: u32 = h[2];
        let d: u32 = h[3];
        let e: u32 = h[4];

        i = 0;
        while (i < 80) : (i += 1) {
            let f: u32 = 0;
            let k: u32 = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            const temp: u32 = rotl32(a, 5) +% f +% e +% k +% w[i];
            e = d;
            d = c;
            c = rotl32(b, 30);
            b = a;
            a = temp;
        }

        h[0] = h[0] +% a;
        h[1] = h[1] +% b;
        h[2] = h[2] +% c;
        h[3] = h[3] +% d;
        h[4] = h[4] +% e;
    }

    fn update(*_, data: []const u8) {
        total_bytes += data.len as u64;
        let off: usize = 0;
        if (buf_len > 0) {
            while (buf_len < 64 && off < data.len) {
                buf[buf_len] = data[off];
                buf_len += 1;
                off += 1;
            }
            if (buf_len == 64) {
                const blk: [64]u8 = buf;
                compress(blk[0..]);
                buf_len = 0;
            }
        }
        while (off + 64 <= data.len) {
            compress(data[off .. off + 64]);
            off += 64;
        }
        while (off < data.len) {
            buf[buf_len] = data[off];
            buf_len += 1;
            off += 1;
        }
    }

    fn final(*_): [20]u8 {
        const bit_len: u64 = total_bytes * 8;
        const pad_one: [1]u8 = [0x80];
        update(pad_one[0..]);

        const zero_byte: [1]u8 = [0x00];
        while (buf_len != 56) {
            update(zero_byte[0..]);
        }

        let len_be: [8]u8 = [0; 8];
        writeBeU32(len_be[0..], 0, ((bit_len >> 32) & 0xFFFFFFFF) as u32);
        writeBeU32(len_be[0..], 4, (bit_len & 0xFFFFFFFF) as u32);
        update(len_be[0..]);

        let out: [20]u8 = [0; 20];
        let i: usize = 0;
        while (i < 5) : (i += 1) {
            writeBeU32(out[0..], i * 4, h[i]);
        }
        return out;
    }
}

fn hmacSha1WithOptionalSuffix(key: [20]u8, msg: []const u8, has_roc: bool, roc: u32): [20]u8 {
    let ipad: [64]u8 = [0x36; 64];
    let opad: [64]u8 = [0x5C; 64];
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        ipad[i] ^= key[i];
        opad[i] ^= key[i];
    }

    let inner = Sha1State.init();
    inner.update(ipad[0..]);
    inner.update(msg);
    if (has_roc) {
        let roc_be: [4]u8 = [0; 4];
        writeBeU32(roc_be[0..], 0, roc);
        inner.update(roc_be[0..]);
    }
    const inner_hash: [20]u8 = inner.final();

    let outer = Sha1State.init();
    outer.update(opad[0..]);
    outer.update(inner_hash[0..]);
    return outer.final();
}

fn ctEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

// ============================================================================
// 3. Key Material, RFC 5764 DTLS-SRTP EKM Split & RFC 3711 §4.3.1 KDF
// ============================================================================

/// Master key (16B) and master salt (14B) for an SRTP/SRTCP direction.
pub struct SrtpKeyMaterial {
    master_key: [16]u8 = [0; 16];
    master_salt: [14]u8 = [0; 14];
}

/// Client and server SRTP master key material extracted from DTLS-SRTP EKM.
pub struct DtlsSrtpKeyPair {
    client: SrtpKeyMaterial = SrtpKeyMaterial{},
    server: SrtpKeyMaterial = SrtpKeyMaterial{},
}

/// Splits a 60-byte RFC 5764 §4.2 DTLS-SRTP Exported Keying Material (EKM) buffer into
/// `client_write_key (16B)`, `server_write_key (16B)`, `client_write_salt (14B)`, and `server_write_salt (14B)`.
pub fn splitDtlsExportedKeyingMaterial(ekm: []const u8): DtlsSrtpKeyPair ! SrtpError {
    if (ekm.len < SRTP_DTLS_EKM_LEN) {
        return SrtpError.InvalidKeyLen;
    }
    let pair = DtlsSrtpKeyPair{};
    let i: usize = 0;
    while (i < SRTP_MASTER_KEY_LEN) : (i += 1) {
        pair.client.master_key[i] = ekm[i];
        pair.server.master_key[i] = ekm[16 + i];
    }
    let s: usize = 0;
    while (s < SRTP_MASTER_SALT_LEN) : (s += 1) {
        pair.client.master_salt[s] = ekm[32 + s];
        pair.server.master_salt[s] = ekm[46 + s];
    }
    return pair;
}

/// Derived RFC 3711 §4.3.1 SRTP & SRTCP session keys (cipher, auth, and salt for both RTP and RTCP).
pub struct SrtpSessionKeys {
    enc_key: [16]u8 = [0; 16];
    auth_key: [20]u8 = [0; 20];
    salt_key: [14]u8 = [0; 14];
    srtcp_enc_key: [16]u8 = [0; 16];
    srtcp_auth_key: [20]u8 = [0; 20];
    srtcp_salt_key: [14]u8 = [0; 14];

    /// Zeroizes all derived session key material in place.
    fn zeroize(*_) {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            enc_key[i] = 0;
            srtcp_enc_key[i] = 0;
        }
        i = 0;
        while (i < 20) : (i += 1) {
            auth_key[i] = 0;
            srtcp_auth_key[i] = 0;
        }
        i = 0;
        while (i < 14) : (i += 1) {
            salt_key[i] = 0;
            srtcp_salt_key[i] = 0;
        }
    }
}

fn kdfDeriveLabel(master: SrtpKeyMaterial, label: u8, out: []u8) {
    // RFC 3711 §4.3.1 (kdr = 0 => r = 0):
    // key_id = label || 48-bit-zero, x = master_salt XOR key_id (aligned at LSB => byte 7)
    // IV = x || 0x0000
    let iv: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < SRTP_MASTER_SALT_LEN) : (i += 1) {
        iv[i] = master.master_salt[i];
    }
    iv[7] ^= label;
    aes128CmKeystream(master.master_key, iv, out);
}

/// Derives all 6 SRTP and SRTCP session keys (`label = 0x00..0x05`) using the RFC 3711 §4.3.1 AES-128-CM PRF.
pub fn deriveSrtpSessionKeys(master: SrtpKeyMaterial): SrtpSessionKeys {
    let keys = SrtpSessionKeys{};
    kdfDeriveLabel(master, 0x00, keys.enc_key[0..]);
    kdfDeriveLabel(master, 0x01, keys.auth_key[0..]);
    kdfDeriveLabel(master, 0x02, keys.salt_key[0..]);
    kdfDeriveLabel(master, 0x03, keys.srtcp_enc_key[0..]);
    kdfDeriveLabel(master, 0x04, keys.srtcp_auth_key[0..]);
    kdfDeriveLabel(master, 0x05, keys.srtcp_salt_key[0..]);
    return keys;
}

// ============================================================================
// 4. RFC 3711 §3.3.1 ROC 48-Bit Packet Index & 64-Bit Anti-Replay Window
// ============================================================================

/// Estimates the 48-bit SRTP packet index `(v << 16) | seq` from current `roc`,
/// highest previously received 16-bit sequence `s_l`, and incoming `seq` (RFC 3711 Appendix A).
pub fn estimateSrtpPacketIndex(roc: u32, s_l: u16, seq: u16): u64 {
    let v: u32 = roc;
    if (s_l < 32768) {
        if (seq > s_l && (seq - s_l) > 32768) {
            v = if (roc > 0) (roc - 1) else 0;
        }
    } else {
        if (s_l > seq && (s_l - seq) > 32768) {
            v = roc +% 1;
        }
    }
    return ((v as u64) << 16) | (seq as u64);
}

/// RFC 3711 §3.3.2 64-packet sliding bitmap anti-replay detector.
pub struct SrtpReplayWindow {
    highest_index: u64 = 0;
    bitmap: u64 = 0;
    initialized: bool = false;

    /// Pre-validates `index` without mutating state; rejects duplicates (`ReplayDetected`)
    /// and packets older than 64 behind `highest_index` (`PacketTooOld`).
    fn check(index: u64): void ! SrtpError {
        if (!initialized) return;
        if (index > highest_index) return;
        const delta: u64 = highest_index - index;
        if (delta >= @as(u64, SRTP_REPLAY_WINDOW_SIZE)) {
            return SrtpError.PacketTooOld;
        }
        const bit: u64 = @as(u64, 1) << (@as(u6, @intCast(delta)));
        if ((bitmap & bit) != 0) {
            return SrtpError.ReplayDetected;
        }
    }

    /// Commits an authenticated packet `index` into the 64-bit sliding replay bitmap.
    fn accept(*_, index: u64) {
        if (!initialized) {
            highest_index = index;
            bitmap = 1;
            initialized = true;
            return;
        }
        if (index > highest_index) {
            const shift: u64 = index - highest_index;
            if (shift >= @as(u64, SRTP_REPLAY_WINDOW_SIZE)) {
                bitmap = 1;
            } else {
                bitmap = (bitmap << (@as(u6, @intCast(shift)))) | 1;
            }
            highest_index = index;
        } else {
            const delta: u64 = highest_index - index;
            if (delta < @as(u64, SRTP_REPLAY_WINDOW_SIZE)) {
                bitmap |= (@as(u64, 1) << (@as(u6, @intCast(delta))));
            }
        }
    }
}

// ============================================================================
// 5. SRTP / SRTCP Protection & Unprotection Context (`SrtpContext`)
// ============================================================================

fn parseRtpHeaderLen(pkt: []const u8): usize ! SrtpError {
    if (pkt.len < SRTP_MIN_RTP_HEADER_LEN) {
        return SrtpError.PacketTooShort;
    }
    const cc: usize = @as(usize, pkt[0] & 0x0F);
    let hdr_len: usize = SRTP_MIN_RTP_HEADER_LEN + cc * 4;
    if (hdr_len > pkt.len) {
        return SrtpError.PacketTooShort;
    }
    const has_ext: bool = (pkt[0] & 0x10) != 0;
    if (has_ext) {
        if (hdr_len + 4 > pkt.len) {
            return SrtpError.PacketTooShort;
        }
        const ext_words: usize = @as(usize, readBeU16(pkt, hdr_len + 2));
        hdr_len += 4 + ext_words * 4;
        if (hdr_len > pkt.len) {
            return SrtpError.PacketTooShort;
        }
    }
    return hdr_len;
}

fn buildSrtpIv(salt: [14]u8, ssrc: u32, pkt_index: u64): [16]u8 {
    let iv: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < SRTP_MASTER_SALT_LEN) : (i += 1) {
        iv[i] = salt[i];
    }
    // XOR SSRC at bytes 4..8
    iv[4] ^= ((ssrc >> 24) & 0xFF) as u8;
    iv[5] ^= ((ssrc >> 16) & 0xFF) as u8;
    iv[6] ^= ((ssrc >> 8) & 0xFF) as u8;
    iv[7] ^= (ssrc & 0xFF) as u8;
    // XOR 48-bit packet index at bytes 8..14
    iv[8] ^= ((pkt_index >> 40) & 0xFF) as u8;
    iv[9] ^= ((pkt_index >> 32) & 0xFF) as u8;
    iv[10] ^= ((pkt_index >> 24) & 0xFF) as u8;
    iv[11] ^= ((pkt_index >> 16) & 0xFF) as u8;
    iv[12] ^= ((pkt_index >> 8) & 0xFF) as u8;
    iv[13] ^= (pkt_index & 0xFF) as u8;
    return iv;
}

/// Production RFC 3711 SRTP / SRTCP cryptographic session context with ROC tracking,
/// independent RTP/RTCP 64-bit anti-replay windows, and RAII `~init()` key zeroization.
pub struct SrtpContext {
    profile: SrtpProfile = SrtpProfile.aes128_cm_hmac_sha1_80;
    keys: SrtpSessionKeys = SrtpSessionKeys{},

    // Outbound RTP ROC tracking
    tx_roc: u32 = 0;
    tx_s_l: u16 = 0;
    tx_initialized: bool = false;

    // Inbound RTP ROC & replay tracking
    rx_roc: u32 = 0;
    rx_s_l: u16 = 0;
    rx_initialized: bool = false;
    rx_rtp_window: SrtpReplayWindow = SrtpReplayWindow{},

    // Outbound & inbound SRTCP 31-bit index & replay tracking
    tx_srtcp_index: u32 = 0;
    rx_rtcp_window: SrtpReplayWindow = SrtpReplayWindow{},

    /// Initializes an `SrtpContext` by deriving all 6 session keys from `master`.
    static fn init(master: SrtpKeyMaterial, profile: SrtpProfile): SrtpContext {
        return SrtpContext{
            profile: profile,
            keys: deriveSrtpSessionKeys(master),
        };
    }

    /// Deterministic RAII destructor: zeroizes all derived session keys on scope exit.
    ~init() {
        keys.zeroize();
    }

    /// Encrypts and authenticates an outbound RTP packet into `out_buf` (appending the HMAC-SHA1 tag).
    fn protectRtp(*_, rtp_packet: []const u8, out_buf: []u8): usize ! SrtpError {
        const hdr_len: usize = parseRtpHeaderLen(rtp_packet)?;
        const tag_len: usize = profile.rtpTagLen();
        const total_len: usize = rtp_packet.len + tag_len;
        if (out_buf.len < total_len) {
            return SrtpError.BufferTooSmall;
        }

        const seq: u16 = readBeU16(rtp_packet, 2);
        const ssrc: u32 = readBeU32(rtp_packet, 8);

        let pkt_index: u64 = 0;
        let used_roc: u32 = tx_roc;
        if (!tx_initialized) {
            tx_initialized = true;
            tx_s_l = seq;
            pkt_index = seq as u64;
            used_roc = 0;
        } else {
            pkt_index = estimateSrtpPacketIndex(tx_roc, tx_s_l, seq);
            used_roc = ((pkt_index >> 16) & 0xFFFFFFFF) as u32;
            const cur_highest: u64 = ((tx_roc as u64) << 16) | (tx_s_l as u64);
            if (pkt_index > cur_highest) {
                tx_roc = used_roc;
                tx_s_l = seq;
            }
        }

        // Copy RTP header in clear
        let i: usize = 0;
        while (i < hdr_len) : (i += 1) {
            out_buf[i] = rtp_packet[i];
        }

        // Encrypt payload (if profile uses AES-128-CM)
        if (profile.usesEncryption()) {
            const iv: [16]u8 = buildSrtpIv(keys.salt_key, ssrc, pkt_index);
            aes128CmXor(keys.enc_key, iv, rtp_packet[hdr_len..], out_buf[hdr_len .. rtp_packet.len]);
        } else {
            let p: usize = hdr_len;
            while (p < rtp_packet.len) : (p += 1) {
                out_buf[p] = rtp_packet[p];
            }
        }

        // Compute HMAC-SHA1(auth_key, encrypted_rtp || ROC) and append truncated tag
        const full_tag: [20]u8 = hmacSha1WithOptionalSuffix(keys.auth_key, out_buf[0 .. rtp_packet.len], true, used_roc);
        let t: usize = 0;
        while (t < tag_len) : (t += 1) {
            out_buf[rtp_packet.len + t] = full_tag[t];
        }
        return total_len;
    }

    /// Authenticates, checks anti-replay, and decrypts an inbound SRTP packet into `out_buf`.
    fn unprotectRtp(*_, srtp_packet: []const u8, out_buf: []u8): usize ! SrtpError {
        const tag_len: usize = profile.rtpTagLen();
        if (srtp_packet.len < SRTP_MIN_RTP_HEADER_LEN + tag_len) {
            return SrtpError.PacketTooShort;
        }
        const rtp_len: usize = srtp_packet.len - tag_len;
        const hdr_len: usize = parseRtpHeaderLen(srtp_packet[0..rtp_len])?;
        if (out_buf.len < rtp_len) {
            return SrtpError.BufferTooSmall;
        }

        const seq: u16 = readBeU16(srtp_packet, 2);
        const ssrc: u32 = readBeU32(srtp_packet, 8);

        // Estimate 48-bit index & pre-check anti-replay window BEFORE crypto work
        const pkt_index: u64 = if (!rx_initialized)
            (seq as u64)
        else
            estimateSrtpPacketIndex(rx_roc, rx_s_l, seq);
        const est_roc: u32 = ((pkt_index >> 16) & 0xFFFFFFFF) as u32;

        rx_rtp_window.check(pkt_index)?;

        // Constant-time HMAC-SHA1 tag verification
        const expected_tag: [20]u8 = hmacSha1WithOptionalSuffix(keys.auth_key, srtp_packet[0..rtp_len], true, est_roc);
        if (!ctEqBytes(expected_tag[0..tag_len], srtp_packet[rtp_len..])) {
            return SrtpError.AuthTagMismatch;
        }

        // Copy header + decrypt payload
        let i: usize = 0;
        while (i < hdr_len) : (i += 1) {
            out_buf[i] = srtp_packet[i];
        }
        if (profile.usesEncryption()) {
            const iv: [16]u8 = buildSrtpIv(keys.salt_key, ssrc, pkt_index);
            aes128CmXor(keys.enc_key, iv, srtp_packet[hdr_len..rtp_len], out_buf[hdr_len..rtp_len]);
        } else {
            let p: usize = hdr_len;
            while (p < rtp_len) : (p += 1) {
                out_buf[p] = srtp_packet[p];
            }
        }

        // Commit replay window & update ROC / s_l only AFTER authentication succeeds
        rx_rtp_window.accept(pkt_index);
        if (!rx_initialized) {
            rx_initialized = true;
            rx_roc = est_roc;
            rx_s_l = seq;
        } else {
            const cur_highest: u64 = ((rx_roc as u64) << 16) | (rx_s_l as u64);
            if (pkt_index > cur_highest) {
                rx_roc = est_roc;
                rx_s_l = seq;
            }
        }
        return rtp_len;
    }

    /// Encrypts and authenticates an outbound RTCP packet into `out_buf` per RFC 3711 §3.4.
    fn protectRtcp(*_, rtcp_packet: []const u8, out_buf: []u8): usize ! SrtpError {
        if (rtcp_packet.len < SRTP_MIN_RTCP_HEADER_LEN) {
            return SrtpError.PacketTooShort;
        }
        if (tx_srtcp_index > SRTCP_INDEX_MASK) {
            return SrtpError.IndexOverflow;
        }
        const tag_len: usize = profile.rtcpTagLen();
        const auth_end: usize = rtcp_packet.len + SRTCP_INDEX_TRAILER_LEN;
        const total_len: usize = auth_end + tag_len;
        if (out_buf.len < total_len) {
            return SrtpError.BufferTooSmall;
        }

        const ssrc: u32 = readBeU32(rtcp_packet, 4);
        const idx: u32 = tx_srtcp_index;

        // Copy 8-byte RTCP header in clear
        let i: usize = 0;
        while (i < SRTP_MIN_RTCP_HEADER_LEN) : (i += 1) {
            out_buf[i] = rtcp_packet[i];
        }

        // Encrypt payload beyond byte 8 if cipher enabled
        let trailer: u32 = idx & SRTCP_INDEX_MASK;
        if (profile.usesEncryption()) {
            trailer |= SRTCP_E_FLAG_MASK;
            const iv: [16]u8 = buildSrtpIv(keys.srtcp_salt_key, ssrc, idx as u64);
            aes128CmXor(keys.srtcp_enc_key, iv, rtcp_packet[SRTP_MIN_RTCP_HEADER_LEN..], out_buf[SRTP_MIN_RTCP_HEADER_LEN .. rtcp_packet.len]);
        } else {
            let p: usize = SRTP_MIN_RTCP_HEADER_LEN;
            while (p < rtcp_packet.len) : (p += 1) {
                out_buf[p] = rtcp_packet[p];
            }
        }

        writeBeU32(out_buf, rtcp_packet.len, trailer);

        // Compute HMAC-SHA1(srtcp_auth_key, encrypted_rtcp || E|srtcp_index)
        const full_tag: [20]u8 = hmacSha1WithOptionalSuffix(keys.srtcp_auth_key, out_buf[0..auth_end], false, 0);
        let t: usize = 0;
        while (t < tag_len) : (t += 1) {
            out_buf[auth_end + t] = full_tag[t];
        }

        tx_srtcp_index += 1;
        return total_len;
    }

    /// Authenticates, checks SRTCP anti-replay, and decrypts an inbound SRTCP packet into `out_buf`.
    fn unprotectRtcp(*_, srtcp_packet: []const u8, out_buf: []u8): usize ! SrtpError {
        const tag_len: usize = profile.rtcpTagLen();
        if (srtcp_packet.len < SRTP_MIN_RTCP_HEADER_LEN + SRTCP_INDEX_TRAILER_LEN + tag_len) {
            return SrtpError.PacketTooShort;
        }
        const auth_end: usize = srtcp_packet.len - tag_len;
        const rtcp_len: usize = auth_end - SRTCP_INDEX_TRAILER_LEN;
        if (out_buf.len < rtcp_len) {
            return SrtpError.BufferTooSmall;
        }

        const ssrc: u32 = readBeU32(srtcp_packet, 4);
        const trailer: u32 = readBeU32(srtcp_packet, rtcp_len);
        const encrypted: bool = (trailer & SRTCP_E_FLAG_MASK) != 0;
        const idx: u64 = (trailer & SRTCP_INDEX_MASK) as u64;

        rx_rtcp_window.check(idx)?;

        const expected_tag: [20]u8 = hmacSha1WithOptionalSuffix(keys.srtcp_auth_key, srtcp_packet[0..auth_end], false, 0);
        if (!ctEqBytes(expected_tag[0..tag_len], srtcp_packet[auth_end..])) {
            return SrtpError.AuthTagMismatch;
        }

        let i: usize = 0;
        while (i < SRTP_MIN_RTCP_HEADER_LEN) : (i += 1) {
            out_buf[i] = srtcp_packet[i];
        }
        if (encrypted) {
            const iv: [16]u8 = buildSrtpIv(keys.srtcp_salt_key, ssrc, idx);
            aes128CmXor(keys.srtcp_enc_key, iv, srtcp_packet[SRTP_MIN_RTCP_HEADER_LEN..rtcp_len], out_buf[SRTP_MIN_RTCP_HEADER_LEN..rtcp_len]);
        } else {
            let p: usize = SRTP_MIN_RTCP_HEADER_LEN;
            while (p < rtcp_len) : (p += 1) {
                out_buf[p] = srtcp_packet[p];
            }
        }

        rx_rtcp_window.accept(idx);
        return rtcp_len;
    }
}
