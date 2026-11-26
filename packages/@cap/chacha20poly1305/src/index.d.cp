// Type definitions for CP

import { chacha20Block, ChaCha20Cipher } from "../../chacha20-crate/src/index.cp";
import { Poly1305 } from "../../poly1305/src/index.cp";
import { ctEqBytes, zeroizeBytes } from "../../subtle-ct/src/index.cp";
export fn aeadWriteLe64(out: []u8, off: usize, v: u64);
export fn aeadPad16Len(len: usize): usize;
export fn aeadComputeTag(key: []u8, nonce: []u8, aad: []u8, ciphertext: []u8, tag_out16: []u8): bool;
export fn aeadCtEq16(a: []u8, b: []u8): bool;
export class ChaCha20Poly1305 {
    key: [32]u8,
    fn new(key: []u8): ChaCha20Poly1305?;
    fn encryptInPlaceDetached(nonce: []u8, aad: []u8, buffer: []u8, tag_out16: []u8): bool;
    fn decryptInPlaceDetached(nonce: []u8, aad: []u8, buffer: []u8, tag16: []u8): bool;
}

