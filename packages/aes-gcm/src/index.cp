// @cap/aes-gcm — Zero-allocation NIST SP 800-38D AES-128-GCM & AES-256-GCM AEAD
// Clean-room parity with crates.io:aes-gcm (Apache-2.0 OR MIT)
// Composes @cap/aes and @cap/ghash-gf128

import { Aes128, Aes256 } from "../../aes/src/index.cp";
import { Ghash } from "../../universal-hash/src/index.cp";

fn agcmZeroBlock(): [16]u8 {
    const z: u8 = 0;
    const blk: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
    return blk;
}

fn agcmMakeCounterBlock(nonce: [12]u8, counter: u32): [16]u8 {
    let blk: [16]u8 = agcmZeroBlock();
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        blk[i] = nonce[i];
    }
    blk[12] = u8((counter >> 24) & 0xFF);
    blk[13] = u8((counter >> 16) & 0xFF);
    blk[14] = u8((counter >> 8) & 0xFF);
    blk[15] = u8(counter & 0xFF);
    return blk;
}

fn agcmCtEq16(a: [16]u8, b: [16]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn agcmComputeTag(h: [16]u8, e_j0: [16]u8, aad: []const u8, ct: []const u8): [16]u8 {
    let h_buf: [16]u8 = h;
    let gh = Ghash.new(h_buf[0..16]);
    if (aad.len > 0) {
        gh.updatePadded(aad);
    }
    if (ct.len > 0) {
        gh.updatePadded(ct);
    }
    gh.updateLengths(u64(aad.len), u64(ct.len));
    let s: [16]u8 = agcmZeroBlock();
    gh.finalize(s[0..16]);

    let tag: [16]u8 = agcmZeroBlock();
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        tag[i] = s[i] ^ e_j0[i];
    }
    return tag;
}

pub struct Aes128Gcm {
    cipher: Aes128,
    hash_key: [16]u8,

    static fn new(key: [16]u8): Aes128Gcm {
        const c = Aes128.new(key);
        const h = c.encryptBlock(agcmZeroBlock());
        return Aes128Gcm{
            cipher: c,
            hash_key: h,
        };
    }

    fn gctr(nonce: [12]u8, input: []const u8, output: []u8) {
        let counter: u32 = 2;
        let pos: usize = 0;
        while (pos < input.len) {
            const ctr_blk = agcmMakeCounterBlock(nonce, counter);
            const ks = _.cipher.encryptBlock(ctr_blk);
            let rem: usize = input.len - pos;
            if (rem > 16) {
                rem = 16;
            }
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                output[pos + i] = input[pos + i] ^ ks[i];
            }
            pos += rem;
            counter = counter +% 1;
        }
    }

    fn encrypt(nonce: [12]u8, aad: []const u8, plaintext: []const u8, out_ct: []u8): [16]u8 {
        if (plaintext.len > 0) {
            _.gctr(nonce, plaintext, out_ct[0..plaintext.len]);
        }
        const j0 = agcmMakeCounterBlock(nonce, 1);
        const e_j0 = _.cipher.encryptBlock(j0);
        const ct_slice: []const u8 = if (plaintext.len > 0) out_ct[0..plaintext.len] else "";
        const tag = agcmComputeTag(_.hash_key, e_j0, aad, ct_slice);
        return tag;
    }

    fn decrypt(nonce: [12]u8, aad: []const u8, ciphertext: []const u8, tag: [16]u8, out_pt: []u8): bool {
        if (out_pt.len < ciphertext.len) {
            return false;
        }
        const j0 = agcmMakeCounterBlock(nonce, 1);
        const e_j0 = _.cipher.encryptBlock(j0);
        const expected = agcmComputeTag(_.hash_key, e_j0, aad, ciphertext);
        if (!agcmCtEq16(expected, tag)) {
            return false;
        }
        if (ciphertext.len > 0) {
            _.gctr(nonce, ciphertext, out_pt[0..ciphertext.len]);
        }
        return true;
    }
}

pub struct Aes256Gcm {
    cipher: Aes256,
    hash_key: [16]u8,

    static fn new(key: [32]u8): Aes256Gcm {
        const c = Aes256.new(key);
        const h = c.encryptBlock(agcmZeroBlock());
        return Aes256Gcm{
            cipher: c,
            hash_key: h,
        };
    }

    fn gctr(nonce: [12]u8, input: []const u8, output: []u8) {
        let counter: u32 = 2;
        let pos: usize = 0;
        while (pos < input.len) {
            const ctr_blk = agcmMakeCounterBlock(nonce, counter);
            const ks = _.cipher.encryptBlock(ctr_blk);
            let rem: usize = input.len - pos;
            if (rem > 16) {
                rem = 16;
            }
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                output[pos + i] = input[pos + i] ^ ks[i];
            }
            pos += rem;
            counter = counter +% 1;
        }
    }

    fn encrypt(nonce: [12]u8, aad: []const u8, plaintext: []const u8, out_ct: []u8): [16]u8 {
        if (plaintext.len > 0) {
            _.gctr(nonce, plaintext, out_ct[0..plaintext.len]);
        }
        const j0 = agcmMakeCounterBlock(nonce, 1);
        const e_j0 = _.cipher.encryptBlock(j0);
        const ct_slice: []const u8 = if (plaintext.len > 0) out_ct[0..plaintext.len] else "";
        const tag = agcmComputeTag(_.hash_key, e_j0, aad, ct_slice);
        return tag;
    }

    fn decrypt(nonce: [12]u8, aad: []const u8, ciphertext: []const u8, tag: [16]u8, out_pt: []u8): bool {
        if (out_pt.len < ciphertext.len) {
            return false;
        }
        const j0 = agcmMakeCounterBlock(nonce, 1);
        const e_j0 = _.cipher.encryptBlock(j0);
        const expected = agcmComputeTag(_.hash_key, e_j0, aad, ciphertext);
        if (!agcmCtEq16(expected, tag)) {
            return false;
        }
        if (ciphertext.len > 0) {
            _.gctr(nonce, ciphertext, out_pt[0..ciphertext.len]);
        }
        return true;
    }
}
