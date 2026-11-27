// Type definitions for CP

import { OID_TAG, OID_RSA_ENCRYPTION, OID_EC_PUBLIC_KEY, OID_PRIME256V1, OID_ED25519, ObjectIdentifier } from "../../const-oid/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
export class SpkiLenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

export fn spkiZeroKeyBytes(): [160]u8;
export fn spkiDerLenSize(len: usize): usize;
export fn spkiWriteDerLen(out: []u8, pos: usize, len: usize): usize;
export fn spkiReadDerLen(der: &[]u8, pos: usize): SpkiLenDecode;
export fn spkiStrEqBytes(s: string, b: &[]u8): bool;
export class AlgorithmIdentifier {
    oid: ObjectIdentifier,
    has_params: bool,
    param_is_null: bool,
    param_oid: ObjectIdentifier,
    has_param_oid: bool,
    fn empty(): AlgorithmIdentifier;
    fn fromOid(oid: ObjectIdentifier): AlgorithmIdentifier;
    fn withNull(oid: ObjectIdentifier): AlgorithmIdentifier;
    fn withParamOid(oid: ObjectIdentifier, param_oid: ObjectIdentifier): AlgorithmIdentifier;
    fn eq(other: AlgorithmIdentifier): bool;
}

export class AlgIdDecodeResult {
    ok: bool,
    alg: AlgorithmIdentifier,
    consumed: usize,
    err: i64,
}

export class SubjectPublicKeyInfo {
    algorithm: AlgorithmIdentifier,
    key_bytes: [160]u8,
    key_len: usize,
    unused_bits: u8,
    fn empty(): SubjectPublicKeyInfo;
    fn ed25519(pub_key: &[]u8): SpkiDecodeResult;
    fn p256(pub_key: &[]u8): SpkiDecodeResult;
    fn rsa(pub_key_der: &[]u8): SpkiDecodeResult;
}

export class SpkiDecodeResult {
    ok: bool,
    spki: SubjectPublicKeyInfo,
    err: i64,
}

export fn encodeAlgorithmId(alg: AlgorithmIdentifier, out: []u8): i64;
export fn spkiDecodeAlgIdSlice(der: &[]u8, start_pos: usize): AlgIdDecodeResult;
export fn decodeAlgorithmId(der: &[]u8): AlgIdDecodeResult;
export fn encodeSpkiDer(spki: SubjectPublicKeyInfo, out: []u8): i64;
export fn decodeSpkiDer(der: &[]u8): SpkiDecodeResult;
export fn encodeSpkiPem(spki: SubjectPublicKeyInfo, crlf: bool, out: []u8): i64;
export fn decodeSpkiPem(pem: string): SpkiDecodeResult;
export fn fingerprintSpki(der: &[]u8): u64;
