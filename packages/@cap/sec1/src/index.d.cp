// Type definitions for CP

import { OID_TAG, OID_PRIME256V1, ObjectIdentifier } from "../../const-oid/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
import { zeroizeBytes } from "../../subtle-ct/src/index.cp";
export class Sec1LenDecode {
    ok: bool,
    val_len: usize,
    next_pos: usize,
}

export fn sec1ZeroPointBuf(): [133]u8;
export fn sec1ZeroPrivBuf(): [66]u8;
export fn sec1DerLenSize(len: usize): usize;
export fn sec1WriteDerLen(out: []u8, pos: usize, len: usize): usize;
export fn sec1ReadDerLen(der: &[]u8, pos: usize): Sec1LenDecode;
export fn sec1StrEqBytes(s: string, b: &[]u8): bool;
export class PointResult {
    ok: bool,
    point: EncodedPoint,
    err: i64,
}

export class EncodedPoint {
    buf: [133]u8,
    len: usize,
    coord_size: usize,
    fn identity(): EncodedPoint;
    fn fromUncompressed(x: &[]u8, y: &[]u8): PointResult;
    fn fromCompressed(x: &[]u8, y_is_odd: bool): PointResult;
    fn fromBytes(bytes: &[]u8): PointResult;
    fn fromBytesForCoord(bytes: &[]u8, expected_coord_size: usize): PointResult;
    fn tag(): u8;
    fn isIdentity(): bool;
    fn isCompressed(): bool;
    fn isUncompressed(): bool;
    fn xBytes(out: []u8): usize;
    fn yBytes(out: []u8): usize;
    fn toBytes(out: []u8): usize;
    fn compress(): PointResult;
    fn eq(other: EncodedPoint): bool;
}

export class EcPrivateKey {
    priv_key: [66]u8,
    priv_len: usize,
    has_curve: bool,
    curve_oid: ObjectIdentifier,
    has_pub_key: bool,
    pub_key: EncodedPoint,
    fn empty(): EcPrivateKey;
    fn zeroize(_: auto);
}

export class EcPrivateKeyResult {
    ok: bool,
    key: EcPrivateKey,
    err: i64,
}

export fn makeEcPrivateKey(priv_bytes: &[]u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult;
export fn encodeEcPrivateKeyDer(key: EcPrivateKey, out: []u8): i64;
export fn decodeEcPrivateKeyDer(der: &[]u8): EcPrivateKeyResult;
export fn encodeEcPrivateKeyPem(key: EcPrivateKey, crlf: bool, out: []u8): i64;
export fn decodeEcPrivateKeyPem(pem: string): EcPrivateKeyResult;
