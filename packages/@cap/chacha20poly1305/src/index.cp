// @cap/chacha20poly1305 — Pure CP zero-allocation RFC 8439 ChaCha20-Poly1305 AEAD
// Clean-room MIT parity with Rust crates.io `chacha20poly1305` v0.10.
// Reuses `@cap/chacha20-crate`, `@cap/poly1305`, and `@cap/subtle-ct`.

import { chacha20Block, ChaCha20Cipher } from "../../chacha20-crate/src/index.cp";
import { Poly1305 } from "../../poly1305/src/index.cp";
import { ctEqBytes, zeroizeBytes } from "../../subtle-ct/src/index.cp";

pub const AEAD_KEY_LEN: usize = 32;
pub const AEAD_NONCE_LEN: usize = 12;
pub const AEAD_TAG_LEN: usize = 16;

fn aeadWriteLe64(out: []u8, off: usize, v: u64) {
    out[off] = (v & (0xFF as u64)) as u8;
    out[off + (1 as usize)] = ((v >> 8) & (0xFF as u64)) as u8;
    out[off + (2 as usize)] = ((v >> 16) & (0xFF as u64)) as u8;
    out[off + (3 as usize)] = ((v >> 24) & (0xFF as u64)) as u8;
    out[off + (4 as usize)] = ((v >> 32) & (0xFF as u64)) as u8;
    out[off + (5 as usize)] = ((v >> 40) & (0xFF as u64)) as u8;
    out[off + (6 as usize)] = ((v >> 48) & (0xFF as u64)) as u8;
    out[off + (7 as usize)] = ((v >> 56) & (0xFF as u64)) as u8;
}

fn aeadPad16Len(len: usize): usize {
    let rem: usize = len % (16 as usize);
    if (rem == (0 as usize)) {
        return 0;
    }
    return (16 as usize) - rem;
}

fn aeadComputeTag(key: []u8, nonce: []u8, aad: []u8, ciphertext: []u8, tag_out16: []u8): bool {
    let otk_block: [64]u8 = undefined;
    if (!chacha20Block(key, 0 as u32, nonce, otk_block[0..64])) {
        return false;
    }

    let poly = Poly1305.init(otk_block[0..32]);
    let zeros: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    if (aad.len > (0 as usize)) {
        poly.update(aad);
        let pad_aad: usize = aeadPad16Len(aad.len);
        if (pad_aad > (0 as usize)) {
            poly.update(zeros[0..pad_aad]);
        }
    }

    if (ciphertext.len > (0 as usize)) {
        poly.update(ciphertext);
        let pad_ct: usize = aeadPad16Len(ciphertext.len);
        if (pad_ct > (0 as usize)) {
            poly.update(zeros[0..pad_ct]);
        }
    }

    let lens: [16]u8 = undefined;
    aeadWriteLe64(lens[0..16], 0 as usize, aad.len as u64);
    aeadWriteLe64(lens[0..16], 8 as usize, ciphertext.len as u64);
    poly.update(lens[0..16]);
    poly.finalize(tag_out16[0..16]);

    zeroizeBytes(otk_block[0..64]);
    return true;
}

fn aeadCtEq16(a: []u8, b: []u8): bool {
    return ctEqBytes(a[0..AEAD_TAG_LEN], b[0..AEAD_TAG_LEN]).toBool();
}

pub struct ChaCha20Poly1305 {
    key: [32]u8,

    pub static fn new(key: []u8): ChaCha20Poly1305? {
        if (key.len != AEAD_KEY_LEN) {
            return null;
        }
        let k_arr: [32]u8 = undefined;
        let i: usize = 0;
        while (i < AEAD_KEY_LEN) {
            k_arr[i] = key[i];
            i += 1;
        }
        return ChaCha20Poly1305 { key: k_arr };
    }

    pub fn encryptInPlaceDetached(nonce: []u8, aad: []u8, buffer: []u8, tag_out16: []u8): bool {
        if (nonce.len != AEAD_NONCE_LEN || tag_out16.len < AEAD_TAG_LEN) {
            return false;
        }
        let kbuf: [32]u8 = _.key;
        let maybe_cipher = ChaCha20Cipher.newWithCounter(kbuf[0..32], nonce, 1 as u32);
        if (maybe_cipher == null) {
            zeroizeBytes(kbuf[0..32]);
            return false;
        }
        let cipher = maybe_cipher.?;
        if (buffer.len > (0 as usize)) {
            cipher.applyKeystream(buffer);
        }
        let ok = aeadComputeTag(kbuf[0..32], nonce, aad, buffer, tag_out16[0..16]);
        zeroizeBytes(kbuf[0..32]);
        return ok;
    }

    pub fn decryptInPlaceDetached(nonce: []u8, aad: []u8, buffer: []u8, tag16: []u8): bool {
        if (nonce.len != AEAD_NONCE_LEN || tag16.len != AEAD_TAG_LEN) {
            return false;
        }
        let kbuf: [32]u8 = _.key;
        let expected_tag: [16]u8 = undefined;
        if (!aeadComputeTag(kbuf[0..32], nonce, aad, buffer, expected_tag[0..16])) {
            zeroizeBytes(kbuf[0..32]);
            return false;
        }
        let valid = aeadCtEq16(expected_tag[0..16], tag16);
        zeroizeBytes(expected_tag[0..16]);
        if (!valid) {
            zeroizeBytes(kbuf[0..32]);
            return false;
        }

        let maybe_cipher = ChaCha20Cipher.newWithCounter(kbuf[0..32], nonce, 1 as u32);
        if (maybe_cipher == null) {
            zeroizeBytes(kbuf[0..32]);
            return false;
        }
        let cipher = maybe_cipher.?;
        if (buffer.len > (0 as usize)) {
            cipher.applyKeystream(buffer);
        }
        zeroizeBytes(kbuf[0..32]);
        return true;
    }
}
