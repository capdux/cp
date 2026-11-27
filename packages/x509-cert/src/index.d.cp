// Type definitions for CP

import { TAG_INTEGER, TAG_UTF8_STRING, TAG_PRINTABLE_STRING, TAG_UTC_TIME, TAG_GENERALIZED_TIME, TAG_SEQUENCE, TAG_SET, DerReader, DerWriter } from "../../der-asn1/src/index.cp";
import { OID_COMMON_NAME, OID_COUNTRY_NAME, OID_ORGANIZATION_NAME, ObjectIdentifier } from "../../const-oid/src/index.cp";
import { AlgorithmIdentifier, SubjectPublicKeyInfo, encodeAlgorithmId, decodeAlgorithmId, encodeSpkiDer, decodeSpkiDer } from "../../spki/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";
export fn x509Zero20(): [20]u8;
export fn x509Zero64(): [64]u8;
export fn x509Zero8(): [8]u8;
export fn x509Zero128(): [128]u8;
export fn x509BytesEq(a: &[]u8, b: &[]u8): bool;
export fn x509StrEqBytes(s: string, b: &[]u8): bool;
export class X509Validity {
    not_before: [20]u8,
    not_before_len: usize,
    not_before_generalized: bool,
    not_after: [20]u8,
    not_after_len: usize,
    not_after_generalized: bool,
    fn empty(): X509Validity;
    fn fromStrs(nb: string, na: string): X509Validity;
}

export class X509Name {
    cn: [64]u8,
    cn_len: usize,
    org: [64]u8,
    org_len: usize,
    country: [8]u8,
    country_len: usize,
    fn empty(): X509Name;
    fn init(cn: string, org: string, country: string): X509Name;
    fn eq(other: X509Name): bool;
}

export class X509Certificate {
    version: u8,
    serial: [20]u8,
    serial_len: usize,
    signature_alg: AlgorithmIdentifier,
    issuer: X509Name,
    validity: X509Validity,
    subject: X509Name,
    spki: SubjectPublicKeyInfo,
    signature: [128]u8,
    signature_len: usize,
    fn empty(): X509Certificate;
}

export class X509DecodeResult {
    ok: bool,
    cert: X509Certificate,
    err: i64,
}

export class X509NameDecode {
    ok: bool,
    name: X509Name,
}

export class X509ValidityDecode {
    ok: bool,
    validity: X509Validity,
}

export fn x509EncodeRdn(oid_str: string, val_tag: u8, val: &[]u8, out: []u8): usize;
export fn x509EncodeName(name: X509Name, out: []u8): usize;
export fn x509DecodeName(name_bytes: &[]u8): X509NameDecode;
export fn x509EncodeValidity(val: X509Validity, out: []u8): usize;
export fn x509DecodeValidity(val_bytes: &[]u8): X509ValidityDecode;
export fn encodeX509Der(cert: X509Certificate, out: []u8): i64;
export fn decodeX509Der(der: &[]u8): X509DecodeResult;
export fn encodeX509Pem(cert: X509Certificate, crlf: bool, out: []u8): i64;
export fn decodeX509Pem(pem: string): X509DecodeResult;
export fn isSelfSigned(cert: X509Certificate): bool;
