// Type definitions for CP

import { Poly1305 } from "../../poly1305/src/index.cp";
import { chachaBlock } from "../../chacha-rng/src/index.cp";
import { ctEqBytes, zeroizeBytes } from "../../subtle-ct/src/index.cp";
export fn cp20Add32(a: u32, b: u32): u32;
export fn cp20Rotl32(x: u32, bits: u32): u32;
export fn cp20ReadU32Le(data: &[]u8, off: usize): u32;
export fn cp20WriteU32Le(out: []u8, off: usize, v: u32);
export fn cp20WriteU64Le(out: []u8, off: usize, v: u64);
export fn cp20QuarterRound(st: []u32, a: usize, b: usize, c: usize, d: usize);
export fn cp20UnpackKey(key: &[]u8): [8]u32;
export fn cp20UnpackNonce(nonce: &[]u8): [3]u32;
export fn cp20ComputeAeadTag(otk32: []u8, aad: &[]u8, ciphertext: &[]u8, tag_out: []u8);
export fn chacha20Block(key: &[]u8, counter: u32, nonce: &[]u8, out: []u8): bool;
export fn chacha20Xor(key: &[]u8, counter: u32, nonce: &[]u8, input: &[]u8, out: []u8): bool;
export fn poly1305KeyGen(key: &[]u8, nonce: &[]u8, out: []u8): bool;
export fn aeadEncrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;
export fn aeadDecrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;
export fn xchacha20HChaCha20(key: &[]u8, nonce16: &[]u8, out_subkey: []u8): bool;
export fn xchacha20Poly1305Encrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;
export fn xchacha20Poly1305Decrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;
