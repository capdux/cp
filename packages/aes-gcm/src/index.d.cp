// Type definitions for CP

import { Aes128, Aes256 } from "../../aes/src/index.cp";
import { Ghash } from "../../ghash-gf128/src/index.cp";
export fn agcmZeroBlock(): [16]u8;
export fn agcmMakeCounterBlock(nonce: [12]u8, counter: u32): [16]u8;
export fn agcmCtEq16(a: [16]u8, b: [16]u8): bool;
export fn agcmComputeTag(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8): [16]u8;
export class Aes128Gcm {
    cipher: Aes128,
    hash_key: [16]u8,
    fn new(key: [16]u8): Aes128Gcm;
    fn gctr(nonce: [12]u8, input: &[]u8, output: []u8);
    fn encrypt(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8): [16]u8;
    fn decrypt(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8): bool;
}

export class Aes256Gcm {
    cipher: Aes256,
    hash_key: [16]u8,
    fn new(key: [32]u8): Aes256Gcm;
    fn gctr(nonce: [12]u8, input: &[]u8, output: []u8);
    fn encrypt(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8): [16]u8;
    fn decrypt(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8): bool;
}

