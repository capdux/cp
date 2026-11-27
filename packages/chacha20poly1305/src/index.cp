// @cap/chacha20-poly1305 — Pure CP zero-allocation RFC 8439 ChaCha20-Poly1305 & XChaCha20-Poly1305 AEAD
// Clean-room MIT implementation with parity to RustCrypto `chacha20poly1305`.
// Accelerated with 16-lane @Vector(16, u8) SIMD keystream XOR and bulk @memcpy.
// Reuses:
//   - `Poly1305` from `@cap/poly1305` (RFC 8439 MAC)
//   - `chachaBlock` from `@cap/chacha-rng` (RFC 8439 ChaCha20 core block)
//   - `ctEqBytes`, `zeroizeBytes` from `@cap/subtle-ct` (constant-time tag check & zeroization)

import { Poly1305 } from "../../poly1305/src/index.cp";
import { chachaBlock } from "../../rand_chacha/src/index.cp";
import { ctEqBytes, zeroizeBytes } from "../../subtle/src/index.cp";

type Cp20Vec16 = @Vector(16, u8);

pub const CHACHA20_KEY_LEN: usize = 32;
pub const CHACHA20_NONCE_LEN: usize = 12;
pub const CHACHA20_BLOCK_LEN: usize = 64;
pub const CHACHA20_TAG_LEN: usize = 16;
pub const XCHACHA20_NONCE_LEN: usize = 24;

fn cp20Add32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn cp20Rotl32(x: u32, bits: u32): u32 {
    const s1 = bits as u5;
    const s2 = ((32 as u32) - bits) as u5;
    return (x << s1) | (x >> s2);
}

fn cp20ReadU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << (8 as u5))
        | ((data[off + 2] as u32) << (16 as u5))
        | ((data[off + 3] as u32) << (24 as u5));
}

fn cp20WriteU32Le(out: []u8, off: usize, v: u32) {
    out[off] = (v & (0xFF as u32)) as u8;
    out[off + 1] = ((v >> (8 as u5)) & (0xFF as u32)) as u8;
    out[off + 2] = ((v >> (16 as u5)) & (0xFF as u32)) as u8;
    out[off + 3] = ((v >> (24 as u5)) & (0xFF as u32)) as u8;
}

fn cp20WriteU64Le(out: []u8, off: usize, v: u64) {
    out[off] = (v & (0xFF as u64)) as u8;
    out[off + 1] = ((v >> (8 as u6)) & (0xFF as u64)) as u8;
    out[off + 2] = ((v >> (16 as u6)) & (0xFF as u64)) as u8;
    out[off + 3] = ((v >> (24 as u6)) & (0xFF as u64)) as u8;
    out[off + 4] = ((v >> (32 as u6)) & (0xFF as u64)) as u8;
    out[off + 5] = ((v >> (40 as u6)) & (0xFF as u64)) as u8;
    out[off + 6] = ((v >> (48 as u6)) & (0xFF as u64)) as u8;
    out[off + 7] = ((v >> (56 as u6)) & (0xFF as u64)) as u8;
}

fn cp20QuarterRound(st: []u32, a: usize, b: usize, c: usize, d: usize) {
    let va = st[a];
    let vb = st[b];
    let vc = st[c];
    let vd = st[d];

    va = cp20Add32(va, vb);
    vd = cp20Rotl32(vd ^ va, 16 as u32);

    vc = cp20Add32(vc, vd);
    vb = cp20Rotl32(vb ^ vc, 12 as u32);

    va = cp20Add32(va, vb);
    vd = cp20Rotl32(vd ^ va, 8 as u32);

    vc = cp20Add32(vc, vd);
    vb = cp20Rotl32(vb ^ vc, 7 as u32);

    st[a] = va;
    st[b] = vb;
    st[c] = vc;
    st[d] = vd;
}

fn cp20UnpackKey(key: []const u8): [8]u32 {
    let k: [8]u32 = undefined;
    let i: usize = 0;
    while (i < 8) {
        k[i] = cp20ReadU32Le(key, i * 4);
        i = i + 1;
    }
    return k;
}

fn cp20UnpackNonce(nonce: []const u8): [3]u32 {
    let n: [3]u32 = undefined;
    n[0] = cp20ReadU32Le(nonce, 0);
    n[1] = cp20ReadU32Le(nonce, 4);
    n[2] = cp20ReadU32Le(nonce, 8);
    return n;
}

fn cp20ComputeAeadTag(otk32: []u8, aad: []const u8, ciphertext: []const u8, tag_out: []u8) {
    let st = Poly1305.init(otk32[0..32]);
    let chunk: [64]u8 = undefined;
    let zeros: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    // Feed AAD in 64-byte stack chunks (zero heap allocation, const-safe)
    let pos: usize = 0;
    while (pos < aad.len) {
        let take = aad.len - pos;
        if (take > 64) {
            take = 64;
        }
        @memcpy(chunk[0..take], aad[pos .. pos + take]);
        st.update(chunk[0..take]);
        pos = pos + take;
    }

    // Pad AAD to 16-byte boundary
    let aad_rem = aad.len % 16;
    if (aad_rem != 0) {
        let pad_len = (16 as usize) - aad_rem;
        st.update(zeros[0..pad_len]);
    }

    // Feed ciphertext in 64-byte stack chunks
    pos = 0;
    while (pos < ciphertext.len) {
        let take = ciphertext.len - pos;
        if (take > 64) {
            take = 64;
        }
        @memcpy(chunk[0..take], ciphertext[pos .. pos + take]);
        st.update(chunk[0..take]);
        pos = pos + take;
    }

    // Pad ciphertext to 16-byte boundary
    let ct_rem = ciphertext.len % 16;
    if (ct_rem != 0) {
        let pad_len = (16 as usize) - ct_rem;
        st.update(zeros[0..pad_len]);
    }

    // Feed 64-bit LE lengths of AAD and ciphertext
    let len_blk: [16]u8 = undefined;
    cp20WriteU64Le(len_blk[0..16], 0, aad.len as u64);
    cp20WriteU64Le(len_blk[0..16], 8, ciphertext.len as u64);
    st.update(len_blk[0..16]);

    st.finalize(tag_out[0..16]);
    zeroizeBytes(chunk[0..64]);
}

/// RFC 8439 Section 2.3: ChaCha20 64-byte block function.
/// Requires `key.len == 32`, `nonce.len == 12`, and `out.len >= 64`.
pub fn chacha20Block(key: []const u8, counter: u32, nonce: []const u8, out: []u8): bool {
    if (key.len != 32 || nonce.len != 12 || out.len < 64) {
        return false;
    }
    let k = cp20UnpackKey(key);
    let n = cp20UnpackNonce(nonce);
    chachaBlock(k, counter, n, 20 as u32, out[0..64]);
    return true;
}

/// RFC 8439 Section 2.4: IETF ChaCha20 stream encryption / decryption (XOR with keystream).
/// Requires `key.len == 32`, `nonce.len == 12`, and `out.len >= input.len`.
pub fn chacha20Xor(key: []const u8, counter: u32, nonce: []const u8, input: []const u8, out: []u8): bool {
    if (key.len != 32 || nonce.len != 12 || out.len < input.len) {
        return false;
    }
    if (input.len == 0) {
        return true;
    }

    let k = cp20UnpackKey(key);
    let n = cp20UnpackNonce(nonce);
    let block: [64]u8 = undefined;
    let ctr = counter;
    let off: usize = 0;

    while (off < input.len) {
        chachaBlock(k, ctr, n, 20 as u32, block[0..64]);
        ctr = cp20Add32(ctr, 1 as u32);

        let take = input.len - off;
        if (take > 64) {
            take = 64;
        }
        let i: usize = 0;
        while (i + 16 <= take) {
            const va: Cp20Vec16 = input[off + i .. off + i + 16][0..16].*;
            const vb: Cp20Vec16 = block[i .. i + 16][0..16].*;
            const vx: [16]u8 = va ^ vb;
            @memcpy(out[off + i .. off + i + 16], vx[0..16]);
            i = i + 16;
        }
        while (i < take) {
            out[off + i] = input[off + i] ^ block[i];
            i = i + 1;
        }
        off = off + take;
    }

    zeroizeBytes(block[0..64]);
    return true;
}

/// RFC 8439 Section 2.6: Poly1305 one-time key generation (counter = 0, first 32 bytes).
/// Requires `key.len == 32`, `nonce.len == 12`, and `out.len >= 32`.
pub fn poly1305KeyGen(key: []const u8, nonce: []const u8, out: []u8): bool {
    if (key.len != 32 || nonce.len != 12 || out.len < 32) {
        return false;
    }
    let block: [64]u8 = undefined;
    if (!chacha20Block(key, 0 as u32, nonce, block[0..64])) {
        return false;
    }
    @memcpy(out[0..32], block[0..32]);
    zeroizeBytes(block[0..64]);
    return true;
}

/// RFC 8439 Section 2.8: ChaCha20-Poly1305 AEAD encryption.
/// Requires `key.len == 32`, `nonce.len == 12`, `ciphertext.len >= plaintext.len`, `tag.len >= 16`.
pub fn aeadEncrypt(
    key: []const u8,
    nonce: []const u8,
    aad: []const u8,
    plaintext: []const u8,
    ciphertext: []u8,
    tag: []u8
): bool {
    if (key.len != 32 || nonce.len != 12 || ciphertext.len < plaintext.len || tag.len < 16) {
        return false;
    }

    let otk: [32]u8 = undefined;
    if (!poly1305KeyGen(key, nonce, otk[0..32])) {
        return false;
    }

    if (!chacha20Xor(key, 1 as u32, nonce, plaintext, ciphertext[0..plaintext.len])) {
        zeroizeBytes(otk[0..32]);
        return false;
    }

    cp20ComputeAeadTag(otk[0..32], aad, ciphertext[0..plaintext.len], tag[0..16]);
    zeroizeBytes(otk[0..32]);
    return true;
}

/// RFC 8439 Section 2.8: ChaCha20-Poly1305 AEAD decryption with constant-time tag verification.
/// On authentication failure, `plaintext[0..ciphertext.len]` is zeroized and `false` is returned.
pub fn aeadDecrypt(
    key: []const u8,
    nonce: []const u8,
    aad: []const u8,
    ciphertext: []const u8,
    tag: []const u8,
    plaintext: []u8
): bool {
    if (key.len != 32 || nonce.len != 12 || tag.len != 16 || plaintext.len < ciphertext.len) {
        return false;
    }

    let otk: [32]u8 = undefined;
    if (!poly1305KeyGen(key, nonce, otk[0..32])) {
        return false;
    }

    let expected_tag: [16]u8 = undefined;
    cp20ComputeAeadTag(otk[0..32], aad, ciphertext, expected_tag[0..16]);
    zeroizeBytes(otk[0..32]);

    const tag_ok = ctEqBytes(expected_tag[0..16], tag).toBool();
    zeroizeBytes(expected_tag[0..16]);

    if (!tag_ok) {
        if (ciphertext.len > 0) {
            zeroizeBytes(plaintext[0..ciphertext.len]);
        }
        return false;
    }

    return chacha20Xor(key, 1 as u32, nonce, ciphertext, plaintext[0..ciphertext.len]);
}

/// draft-irtf-cfrg-xchacha Section 2.2: HChaCha20 subkey derivation (32-byte key + 16-byte nonce -> 32-byte subkey).
pub fn xchacha20HChaCha20(key: []const u8, nonce16: []const u8, out_subkey: []u8): bool {
    if (key.len != 32 || nonce16.len != 16 || out_subkey.len < 32) {
        return false;
    }

    let st: [16]u32 = undefined;
    // RFC 8439 constants: "expand 32-byte k"
    st[0] = 0x61707865 as u32;
    st[1] = 0x3320646E as u32;
    st[2] = 0x79622D32 as u32;
    st[3] = 0x6B206574 as u32;

    let i: usize = 0;
    while (i < 8) {
        st[4 + i] = cp20ReadU32Le(key, i * 4);
        i = i + 1;
    }

    st[12] = cp20ReadU32Le(nonce16, 0);
    st[13] = cp20ReadU32Le(nonce16, 4);
    st[14] = cp20ReadU32Le(nonce16, 8);
    st[15] = cp20ReadU32Le(nonce16, 12);

    let r: u32 = 0 as u32;
    while (r < 10) {
        // Column rounds
        cp20QuarterRound(st[0..16], 0, 4, 8, 12);
        cp20QuarterRound(st[0..16], 1, 5, 9, 13);
        cp20QuarterRound(st[0..16], 2, 6, 10, 14);
        cp20QuarterRound(st[0..16], 3, 7, 11, 15);
        // Diagonal rounds
        cp20QuarterRound(st[0..16], 0, 5, 10, 15);
        cp20QuarterRound(st[0..16], 1, 6, 11, 12);
        cp20QuarterRound(st[0..16], 2, 7, 8, 13);
        cp20QuarterRound(st[0..16], 3, 4, 9, 14);
        r = r + 1;
    }

    // Output state[0..4] || state[12..16] without adding initial state
    cp20WriteU32Le(out_subkey, 0, st[0]);
    cp20WriteU32Le(out_subkey, 4, st[1]);
    cp20WriteU32Le(out_subkey, 8, st[2]);
    cp20WriteU32Le(out_subkey, 12, st[3]);
    cp20WriteU32Le(out_subkey, 16, st[12]);
    cp20WriteU32Le(out_subkey, 20, st[13]);
    cp20WriteU32Le(out_subkey, 24, st[14]);
    cp20WriteU32Le(out_subkey, 28, st[15]);

    return true;
}

/// draft-irtf-cfrg-xchacha Section 2.3: XChaCha20-Poly1305 AEAD encryption (192-bit / 24-byte nonce).
pub fn xchacha20Poly1305Encrypt(
    key: []const u8,
    nonce24: []const u8,
    aad: []const u8,
    plaintext: []const u8,
    ciphertext: []u8,
    tag: []u8
): bool {
    if (key.len != 32 || nonce24.len != 24 || ciphertext.len < plaintext.len || tag.len < 16) {
        return false;
    }

    let subkey: [32]u8 = undefined;
    if (!xchacha20HChaCha20(key, nonce24[0..16], subkey[0..32])) {
        return false;
    }

    let chacha_nonce: [12]u8 = undefined;
    chacha_nonce[0] = 0;
    chacha_nonce[1] = 0;
    chacha_nonce[2] = 0;
    chacha_nonce[3] = 0;
    @memcpy(chacha_nonce[4..12], nonce24[16..24]);

    const ok = aeadEncrypt(subkey[0..32], chacha_nonce[0..12], aad, plaintext, ciphertext, tag);
    zeroizeBytes(subkey[0..32]);
    return ok;
}

/// draft-irtf-cfrg-xchacha Section 2.3: XChaCha20-Poly1305 AEAD decryption (192-bit / 24-byte nonce).
/// On authentication failure, `plaintext[0..ciphertext.len]` is zeroized and `false` is returned.
pub fn xchacha20Poly1305Decrypt(
    key: []const u8,
    nonce24: []const u8,
    aad: []const u8,
    ciphertext: []const u8,
    tag: []const u8,
    plaintext: []u8
): bool {
    if (key.len != 32 || nonce24.len != 24 || tag.len != 16 || plaintext.len < ciphertext.len) {
        return false;
    }

    let subkey: [32]u8 = undefined;
    if (!xchacha20HChaCha20(key, nonce24[0..16], subkey[0..32])) {
        return false;
    }

    let chacha_nonce: [12]u8 = undefined;
    chacha_nonce[0] = 0;
    chacha_nonce[1] = 0;
    chacha_nonce[2] = 0;
    chacha_nonce[3] = 0;
    @memcpy(chacha_nonce[4..12], nonce24[16..24]);

    const ok = aeadDecrypt(subkey[0..32], chacha_nonce[0..12], aad, ciphertext, tag, plaintext);
    zeroizeBytes(subkey[0..32]);
    return ok;
}

export {
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_BLOCK_LEN,
    CHACHA20_TAG_LEN,
    XCHACHA20_NONCE_LEN,
    chacha20Block,
    chacha20Xor,
    poly1305KeyGen,
    aeadEncrypt,
    aeadDecrypt,
    xchacha20HChaCha20,
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
};
