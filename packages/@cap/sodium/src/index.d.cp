// Type definitions for CP

import {  } from "std";
export fn sodium_init(): i32;
export fn sodium_version_string(): [*:0]&u8!?;
export fn randombytes_buf(buf: [*]u8!, size: usize);
export fn crypto_sign_ed25519_keypair(pk: [*]u8!, sk: [*]u8!): i32;
export fn crypto_sign_ed25519_detached(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32;
export fn crypto_sign_ed25519_verify_detached(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32;
export fn crypto_aead_xchacha20poly1305_ietf_encrypt(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32;
export fn crypto_aead_xchacha20poly1305_ietf_decrypt(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32;
export fn crypto_generichash(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32;
export error SodiumError { InitFailed, InvalidKeyLength, InvalidNonceLength, InvalidSignatureLength, InvalidHashLength, BufferTooSmall, CiphertextTooShort, KeypairFailed, SignFailed, EncryptFailed, AuthFailed, HashFailed }

export class Ed25519Keypair {
    pk: [32]u8,
    sk: [64]u8,
}

export class Ed25519Signature {
    bytes: [64]u8,
}

export class Sodium {
    fn init(): !void;
    fn versionString(): string;
    fn randombytesBuf(out: []u8);
    fn ed25519Keypair(): !Ed25519Keypair;
    fn ed25519SignDetached(msg: &[]u8, sk: &[]u8): !Ed25519Signature;
    fn ed25519VerifyDetached(sig: &[]u8, msg: &[]u8, pk: &[]u8): bool;
    fn aeadEncrypt(msg: &[]u8, aad: &[]u8, nonce: &[]u8, key: &[]u8, out: []u8): !&[]u8;
    fn aeadDecrypt(ct: &[]u8, aad: &[]u8, nonce: &[]u8, key: &[]u8, out: []u8): !&[]u8;
    fn generichash(msg: &[]u8, out: []u8): !void;
}

