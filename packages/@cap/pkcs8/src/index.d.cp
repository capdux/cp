// Type definitions for CP

import { OID_ED25519, OID_EC_PUBLIC_KEY, OID_PRIME256V1, ObjectIdentifier } from "../../const-oid/src/index.cp";
import { AlgorithmIdentifier, encodeAlgorithmId, decodeAlgorithmId } from "../../spki/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle-ct/src/index.cp";
export class Pk8LenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

export fn pk8ZeroPrivBuf(): [160]u8;
export fn pk8ZeroPubBuf(): [64]u8;
export fn pk8DerLenSize(n: usize): usize;
export fn pk8WriteDerLen(out: []u8, pos: usize, n: usize): usize;
export fn pk8ReadDerLen(der: &[]u8, pos: usize): Pk8LenDecode;
export fn pk8StrEqBytes(s: string, b: &[]u8): bool;
export class PrivateKeyInfo {
    version: u8,
    algorithm: AlgorithmIdentifier,
    private_key: [160]u8,
    private_key_len: usize,
    has_public_key: bool,
    public_key: [64]u8,
    public_key_len: usize,
    fn empty(): PrivateKeyInfo;
    fn ed25519(seed32: &[]u8): Pkcs8DecodeResult;
    fn ed25519V2(seed32: &[]u8, pub32: &[]u8): Pkcs8DecodeResult;
    fn p256(ec_priv_der: &[]u8): Pkcs8DecodeResult;
    fn zeroize(_: auto);
}

export class Pkcs8DecodeResult {
    ok: bool,
    info: PrivateKeyInfo,
    err: i64,
}

export fn encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64;
export fn decodePkcs8Der(der: &[]u8): Pkcs8DecodeResult;
export fn encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64;
export fn decodePkcs8Pem(pem: string): Pkcs8DecodeResult;
