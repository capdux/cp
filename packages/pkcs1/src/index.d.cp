// Type definitions for CP

import { TAG_INTEGER, DerReader, DerWriter } from "../../der/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle/src/index.cp";
import { bytesEq } from "cap:bin";
export fn pk1WriteBigUint(w: &mut DerWriter, raw: &[]u8): bool;
export fn pk1ReadBigUint(r: &mut DerReader, dst: []u8, len_out: &mut usize): i64;
export class RsaPublicKey {
    n: [512]u8,
    n_len: usize,
    e: u64,
    fn empty(): RsaPublicKey;
    fn new(n_bytes: &[]u8, e: u64): RsaPublicKeyResult;
}

export class RsaPublicKeyResult {
    ok: bool,
    key: RsaPublicKey,
    err: i64,
}

export class RsaPrivateKey {
    version: u8,
    n: [512]u8,
    n_len: usize,
    e: u64,
    d: [512]u8,
    d_len: usize,
    p: [256]u8,
    p_len: usize,
    q: [256]u8,
    q_len: usize,
    dp: [256]u8,
    dp_len: usize,
    dq: [256]u8,
    dq_len: usize,
    qinv: [256]u8,
    qinv_len: usize,
    fn empty(): RsaPrivateKey;
    fn publicKey(): RsaPublicKey;
    fn zeroize(_: auto);
}

export class RsaPrivateKeyResult {
    ok: bool,
    key: RsaPrivateKey,
    err: i64,
}

export fn modulusBitLen(pub_key: RsaPublicKey): usize;
export fn encodeRsaPublicKeyDer(pub_key: RsaPublicKey, out: []u8): i64;
export fn decodeRsaPublicKeyDer(der: &[]u8): RsaPublicKeyResult;
export fn encodeRsaPublicKeyPem(pub_key: RsaPublicKey, crlf: bool, out: []u8): i64;
export fn decodeRsaPublicKeyPem(pem: string): RsaPublicKeyResult;
export fn encodeRsaPrivateKeyDer(priv_key: RsaPrivateKey, out: []u8): i64;
export fn decodeRsaPrivateKeyDer(der: &[]u8): RsaPrivateKeyResult;
export fn encodeRsaPrivateKeyPem(priv_key: RsaPrivateKey, crlf: bool, out: []u8): i64;
export fn decodeRsaPrivateKeyPem(pem: string): RsaPrivateKeyResult;
