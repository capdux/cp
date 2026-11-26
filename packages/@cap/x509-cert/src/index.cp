// @cap/x509-cert — Pure CP zero-allocation RFC 5280 X.509 v1/v2/v3 Certificate DER & PEM codec
// Clean-room MIT implementation with parity to RustCrypto `x509-cert`.
// Reuses 4 official PKI packages with zero duplication:
//   - `@cap/der-asn1`    (`DerReader`, `DerWriter`, DER universal tags)
//   - `@cap/const-oid`   (`ObjectIdentifier`, DN attribute OIDs)
//   - `@cap/spki`        (`AlgorithmIdentifier`, `SubjectPublicKeyInfo`, DER codec)
//   - `@cap/pem-rfc7468` (`encodePem`, `decodePem` for `CERTIFICATE` PEM)

import {
    TAG_INTEGER,
    TAG_UTF8_STRING,
    TAG_PRINTABLE_STRING,
    TAG_UTC_TIME,
    TAG_GENERALIZED_TIME,
    TAG_SEQUENCE,
    TAG_SET,
    DerReader,
    DerWriter,
} from "../../der-asn1/src/index.cp";
import {
    OID_COMMON_NAME,
    OID_COUNTRY_NAME,
    OID_ORGANIZATION_NAME,
    ObjectIdentifier,
} from "../../const-oid/src/index.cp";
import {
    AlgorithmIdentifier,
    SubjectPublicKeyInfo,
    encodeAlgorithmId,
    decodeAlgorithmId,
    encodeSpkiDer,
    decodeSpkiDer,
} from "../../spki/src/index.cp";
import { encodePem, decodePem } from "../../pem-rfc7468/src/index.cp";

pub const X509_PEM_LABEL: string = "CERTIFICATE";
pub const X509_MAX_SERIAL_LEN: usize = 20;
pub const X509_MAX_SIG_LEN: usize = 128;

const X509_TAG_CTX0_EXPLICIT: u8 = 0xA0;
const X509_TAG_CTX1_IMPLICIT: u8 = 0x81;
const X509_TAG_CTX2_IMPLICIT: u8 = 0x82;
const X509_TAG_CTX3_EXPLICIT: u8 = 0xA3;

pub const X509_OK: i64 = 0;
pub const X509_ERR_BUFFER_TOO_SMALL: i64 = -1;
pub const X509_ERR_INVALID_DER: i64 = -2;
pub const X509_ERR_INVALID_VERSION: i64 = -3;
pub const X509_ERR_INVALID_SERIAL: i64 = -4;
pub const X509_ERR_INVALID_ALG: i64 = -5;
pub const X509_ERR_ALG_MISMATCH: i64 = -6;
pub const X509_ERR_INVALID_NAME: i64 = -7;
pub const X509_ERR_INVALID_VALIDITY: i64 = -8;
pub const X509_ERR_INVALID_SPKI: i64 = -9;
pub const X509_ERR_INVALID_SIGNATURE: i64 = -10;
pub const X509_ERR_INVALID_PEM: i64 = -11;
pub const X509_ERR_INVALID_PEM_LABEL: i64 = -12;
pub const X509_ERR_TRAILING_DATA: i64 = -13;

fn x509Zero20(): [20]u8 {
    let b: [20]u8 = undefined;
    let i: usize = 0;
    while (i < 20) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn x509Zero64(): [64]u8 {
    let b: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn x509Zero8(): [8]u8 {
    let b: [8]u8 = undefined;
    let i: usize = 0;
    while (i < 8) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn x509Zero128(): [128]u8 {
    let b: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) {
        b[i] = 0 as u8;
        i = i + 1;
    }
    return b;
}

fn x509BytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn x509StrEqBytes(s: string, b: []const u8): bool {
    if (s.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// RFC 5280 §4.1.2.5 Validity (UTCTime `0x17` or GeneralizedTime `0x18`).
pub struct X509Validity {
    pub not_before: [20]u8,
    pub not_before_len: usize,
    pub not_before_generalized: bool,
    pub not_after: [20]u8,
    pub not_after_len: usize,
    pub not_after_generalized: bool,

    pub static fn empty(): X509Validity {
        return X509Validity {
            not_before: x509Zero20(),
            not_before_len: 0,
            not_before_generalized: false,
            not_after: x509Zero20(),
            not_after_len: 0,
            not_after_generalized: false,
        };
    }

    /// Construct validity from ASCII UTCTime (13 chars, e.g. "260101000000Z")
    /// or GeneralizedTime (15 chars, e.g. "20500101000000Z").
    pub static fn fromStrs(nb: string, na: string): X509Validity {
        let nb_buf = x509Zero20();
        let nb_len = nb.len;
        if (nb_len > 20) {
            nb_len = 20;
        }
        let i: usize = 0;
        while (i < nb_len) {
            nb_buf[i] = nb[i];
            i = i + 1;
        }

        let na_buf = x509Zero20();
        let na_len = na.len;
        if (na_len > 20) {
            na_len = 20;
        }
        i = 0;
        while (i < na_len) {
            na_buf[i] = na[i];
            i = i + 1;
        }

        return X509Validity {
            not_before: nb_buf,
            not_before_len: nb_len,
            not_before_generalized: nb_len >= 15,
            not_after: na_buf,
            not_after_len: na_len,
            not_after_generalized: na_len >= 15,
        };
    }
}

/// RFC 5280 §4.1.2.4 / §4.1.2.6 Distinguished Name (CN / O / C attributes).
pub struct X509Name {
    pub cn: [64]u8,
    pub cn_len: usize,
    pub org: [64]u8,
    pub org_len: usize,
    pub country: [8]u8,
    pub country_len: usize,

    pub static fn empty(): X509Name {
        return X509Name {
            cn: x509Zero64(),
            cn_len: 0,
            org: x509Zero64(),
            org_len: 0,
            country: x509Zero8(),
            country_len: 0,
        };
    }

    pub static fn init(cn: string, org: string, country: string): X509Name {
        let cn_buf = x509Zero64();
        let cn_len = cn.len;
        if (cn_len > 64) {
            cn_len = 64;
        }
        let i: usize = 0;
        while (i < cn_len) {
            cn_buf[i] = cn[i];
            i = i + 1;
        }

        let org_buf = x509Zero64();
        let org_len = org.len;
        if (org_len > 64) {
            org_len = 64;
        }
        i = 0;
        while (i < org_len) {
            org_buf[i] = org[i];
            i = i + 1;
        }

        let c_buf = x509Zero8();
        let c_len = country.len;
        if (c_len > 8) {
            c_len = 8;
        }
        i = 0;
        while (i < c_len) {
            c_buf[i] = country[i];
            i = i + 1;
        }

        return X509Name {
            cn: cn_buf,
            cn_len: cn_len,
            org: org_buf,
            org_len: org_len,
            country: c_buf,
            country_len: c_len,
        };
    }

    pub fn eq(other: X509Name): bool {
        if (!x509BytesEq(_.cn[0.._.cn_len], other.cn[0..other.cn_len])) {
            return false;
        }
        if (!x509BytesEq(_.org[0.._.org_len], other.org[0..other.org_len])) {
            return false;
        }
        if (!x509BytesEq(_.country[0.._.country_len], other.country[0..other.country_len])) {
            return false;
        }
        return true;
    }
}

/// RFC 5280 §4.1 X.509 Certificate (v1 / v2 / v3).
pub struct X509Certificate {
    pub version: u8,
    pub serial: [20]u8,
    pub serial_len: usize,
    pub signature_alg: AlgorithmIdentifier,
    pub issuer: X509Name,
    pub validity: X509Validity,
    pub subject: X509Name,
    pub spki: SubjectPublicKeyInfo,
    pub signature: [128]u8,
    pub signature_len: usize,

    pub static fn empty(): X509Certificate {
        return X509Certificate {
            version: 3 as u8,
            serial: x509Zero20(),
            serial_len: 0,
            signature_alg: AlgorithmIdentifier.empty(),
            issuer: X509Name.empty(),
            validity: X509Validity.empty(),
            subject: X509Name.empty(),
            spki: SubjectPublicKeyInfo.empty(),
            signature: x509Zero128(),
            signature_len: 0,
        };
    }
}

pub struct X509DecodeResult {
    pub ok: bool,
    pub cert: X509Certificate,
    pub err: i64,
}

struct X509NameDecode {
    ok: bool,
    name: X509Name,
}

struct X509ValidityDecode {
    ok: bool,
    validity: X509Validity,
}

fn x509EncodeRdn(oid_str: string, val_tag: u8, val: []const u8, out: []u8): usize {
    const oid_res = ObjectIdentifier.fromStr(oid_str);
    if (!oid_res.ok) {
        return 0;
    }
    let atv_buf: [128]u8 = undefined;
    let atv_w = DerWriter.new(atv_buf[0..128]);
    if (!atv_w.writeOid(oid_res.oid)) {
        return 0;
    }
    if (!atv_w.writeTlv(val_tag, val)) {
        return 0;
    }

    let seq_buf: [144]u8 = undefined;
    let seq_w = DerWriter.new(seq_buf[0..144]);
    if (!seq_w.writeSequence(atv_w.written())) {
        return 0;
    }

    let set_w = DerWriter.new(out);
    if (!set_w.writeTlv(TAG_SET, seq_w.written())) {
        return 0;
    }
    return set_w.len();
}

fn x509EncodeName(name: X509Name, out: []u8): usize {
    let inner: [256]u8 = undefined;
    let pos: usize = 0;

    if (name.country_len > 0) {
        const n = x509EncodeRdn(OID_COUNTRY_NAME, TAG_PRINTABLE_STRING, name.country[0..name.country_len], inner[pos..256]);
        if (n == 0) {
            return 0;
        }
        pos = pos + n;
    }
    if (name.org_len > 0) {
        const n = x509EncodeRdn(OID_ORGANIZATION_NAME, TAG_UTF8_STRING, name.org[0..name.org_len], inner[pos..256]);
        if (n == 0) {
            return 0;
        }
        pos = pos + n;
    }
    if (name.cn_len > 0) {
        const n = x509EncodeRdn(OID_COMMON_NAME, TAG_UTF8_STRING, name.cn[0..name.cn_len], inner[pos..256]);
        if (n == 0) {
            return 0;
        }
        pos = pos + n;
    }

    let w = DerWriter.new(out);
    if (!w.writeSequence(inner[0..pos])) {
        return 0;
    }
    return w.len();
}

fn x509DecodeName(name_bytes: []const u8): X509NameDecode {
    let r = DerReader.new(name_bytes);
    let name = X509Name.empty();

    const oid_cn = ObjectIdentifier.fromStr(OID_COMMON_NAME).oid;
    const oid_org = ObjectIdentifier.fromStr(OID_ORGANIZATION_NAME).oid;
    const oid_c = ObjectIdentifier.fromStr(OID_COUNTRY_NAME).oid;

    while (!r.isEmpty()) {
        const rdn_tlv = r.readTlv();
        if (!rdn_tlv.ok || rdn_tlv.tag != TAG_SET) {
            return X509NameDecode { ok: false, name: X509Name.empty() };
        }
        let rdn_r = DerReader.new(rdn_tlv.value);
        while (!rdn_r.isEmpty()) {
            const atv_seq = rdn_r.readSequence();
            if (!atv_seq.ok) {
                return X509NameDecode { ok: false, name: X509Name.empty() };
            }
            let atv_r = atv_seq.reader;
            const oid_res = atv_r.readOid();
            if (!oid_res.ok) {
                return X509NameDecode { ok: false, name: X509Name.empty() };
            }
            const val_tlv = atv_r.readTlv();
            if (!val_tlv.ok || !atv_r.isEmpty()) {
                return X509NameDecode { ok: false, name: X509Name.empty() };
            }

            if (oid_res.oid.eq(oid_cn)) {
                let copy_len = val_tlv.value.len;
                if (copy_len > 64) {
                    copy_len = 64;
                }
                let i: usize = 0;
                while (i < copy_len) {
                    name.cn[i] = val_tlv.value[i];
                    i = i + 1;
                }
                name.cn_len = copy_len;
            } else if (oid_res.oid.eq(oid_org)) {
                let copy_len = val_tlv.value.len;
                if (copy_len > 64) {
                    copy_len = 64;
                }
                let i: usize = 0;
                while (i < copy_len) {
                    name.org[i] = val_tlv.value[i];
                    i = i + 1;
                }
                name.org_len = copy_len;
            } else if (oid_res.oid.eq(oid_c)) {
                let copy_len = val_tlv.value.len;
                if (copy_len > 8) {
                    copy_len = 8;
                }
                let i: usize = 0;
                while (i < copy_len) {
                    name.country[i] = val_tlv.value[i];
                    i = i + 1;
                }
                name.country_len = copy_len;
            }
        }
    }

    return X509NameDecode { ok: true, name: name };
}

fn x509EncodeValidity(val: X509Validity, out: []u8): usize {
    if (val.not_before_len == 0 || val.not_before_len > 20 || val.not_after_len == 0 || val.not_after_len > 20) {
        return 0;
    }
    let inner: [64]u8 = undefined;
    let iw = DerWriter.new(inner[0..64]);

    let nb_tag = TAG_UTC_TIME;
    if (val.not_before_generalized) {
        nb_tag = TAG_GENERALIZED_TIME;
    }
    let na_tag = TAG_UTC_TIME;
    if (val.not_after_generalized) {
        na_tag = TAG_GENERALIZED_TIME;
    }

    if (!iw.writeTlv(nb_tag, val.not_before[0..val.not_before_len])) {
        return 0;
    }
    if (!iw.writeTlv(na_tag, val.not_after[0..val.not_after_len])) {
        return 0;
    }

    let w = DerWriter.new(out);
    if (!w.writeSequence(iw.written())) {
        return 0;
    }
    return w.len();
}

fn x509DecodeValidity(val_bytes: []const u8): X509ValidityDecode {
    let r = DerReader.new(val_bytes);
    const nb = r.readTlv();
    if (!nb.ok || (nb.tag != TAG_UTC_TIME && nb.tag != TAG_GENERALIZED_TIME)) {
        return X509ValidityDecode { ok: false, validity: X509Validity.empty() };
    }
    if (nb.value.len == 0 || nb.value.len > 20) {
        return X509ValidityDecode { ok: false, validity: X509Validity.empty() };
    }

    const na = r.readTlv();
    if (!na.ok || (na.tag != TAG_UTC_TIME && na.tag != TAG_GENERALIZED_TIME)) {
        return X509ValidityDecode { ok: false, validity: X509Validity.empty() };
    }
    if (na.value.len == 0 || na.value.len > 20 || !r.isEmpty()) {
        return X509ValidityDecode { ok: false, validity: X509Validity.empty() };
    }

    let nb_buf = x509Zero20();
    let i: usize = 0;
    while (i < nb.value.len) {
        nb_buf[i] = nb.value[i];
        i = i + 1;
    }
    let na_buf = x509Zero20();
    i = 0;
    while (i < na.value.len) {
        na_buf[i] = na.value[i];
        i = i + 1;
    }

    return X509ValidityDecode {
        ok: true,
        validity: X509Validity {
            not_before: nb_buf,
            not_before_len: nb.value.len,
            not_before_generalized: nb.tag == TAG_GENERALIZED_TIME,
            not_after: na_buf,
            not_after_len: na.value.len,
            not_after_generalized: na.tag == TAG_GENERALIZED_TIME,
        },
    };
}

/// Encode an RFC 5280 X.509 Certificate into canonical DER bytes.
/// Returns bytes written on success, or a negative `X509_ERR_*` code.
pub fn encodeX509Der(cert: X509Certificate, out: []u8): i64 {
    if (cert.version < (1 as u8) || cert.version > (3 as u8)) {
        return X509_ERR_INVALID_VERSION;
    }
    if (cert.serial_len == 0 || cert.serial_len > X509_MAX_SERIAL_LEN) {
        return X509_ERR_INVALID_SERIAL;
    }
    if (cert.signature_len > X509_MAX_SIG_LEN) {
        return X509_ERR_INVALID_SIGNATURE;
    }

    let tbs_buf: [1024]u8 = undefined;
    let tbs_w = DerWriter.new(tbs_buf[0..1024]);

    // 1. version [0] EXPLICIT Version DEFAULT v1 (omitted for v1 per DER DEFAULT rule)
    if (cert.version > (1 as u8)) {
        let ver_int: [4]u8 = undefined;
        let vw = DerWriter.new(ver_int[0..4]);
        if (!vw.writeU64((cert.version - (1 as u8)) as u64)) {
            return X509_ERR_INVALID_VERSION;
        }
        if (!tbs_w.writeTlv(X509_TAG_CTX0_EXPLICIT, vw.written())) {
            return X509_ERR_BUFFER_TOO_SMALL;
        }
    }

    // 2. serialNumber INTEGER (unsigned, prepend 0x00 if MSB has bit 7 set)
    let ser_buf: [21]u8 = undefined;
    let ser_pos: usize = 0;
    if ((cert.serial[0] & (0x80 as u8)) != (0 as u8)) {
        ser_buf[0] = 0 as u8;
        ser_pos = 1;
    }
    let i: usize = 0;
    while (i < cert.serial_len) {
        ser_buf[ser_pos + i] = cert.serial[i];
        i = i + 1;
    }
    if (!tbs_w.writeTlv(TAG_INTEGER, ser_buf[0..(ser_pos + cert.serial_len)])) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }

    // 3. signature AlgorithmIdentifier (reuses @cap/spki)
    let alg_buf: [192]u8 = undefined;
    const alg_len = encodeAlgorithmId(cert.signature_alg, alg_buf[0..192]);
    if (alg_len < 0) {
        return X509_ERR_INVALID_ALG;
    }
    const alg_ulen = alg_len as usize;
    if (tbs_w.remaining() < alg_ulen) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }
    i = 0;
    while (i < alg_ulen) {
        tbs_w.buf[tbs_w.pos + i] = alg_buf[i];
        i = i + 1;
    }
    tbs_w.pos = tbs_w.pos + alg_ulen;

    // 4. issuer Name
    let name_buf: [320]u8 = undefined;
    const issuer_len = x509EncodeName(cert.issuer, name_buf[0..320]);
    if (issuer_len == 0 || tbs_w.remaining() < issuer_len) {
        return X509_ERR_INVALID_NAME;
    }
    i = 0;
    while (i < issuer_len) {
        tbs_w.buf[tbs_w.pos + i] = name_buf[i];
        i = i + 1;
    }
    tbs_w.pos = tbs_w.pos + issuer_len;

    // 5. validity Validity
    let val_buf: [80]u8 = undefined;
    const val_len = x509EncodeValidity(cert.validity, val_buf[0..80]);
    if (val_len == 0 || tbs_w.remaining() < val_len) {
        return X509_ERR_INVALID_VALIDITY;
    }
    i = 0;
    while (i < val_len) {
        tbs_w.buf[tbs_w.pos + i] = val_buf[i];
        i = i + 1;
    }
    tbs_w.pos = tbs_w.pos + val_len;

    // 6. subject Name
    const subj_len = x509EncodeName(cert.subject, name_buf[0..320]);
    if (subj_len == 0 || tbs_w.remaining() < subj_len) {
        return X509_ERR_INVALID_NAME;
    }
    i = 0;
    while (i < subj_len) {
        tbs_w.buf[tbs_w.pos + i] = name_buf[i];
        i = i + 1;
    }
    tbs_w.pos = tbs_w.pos + subj_len;

    // 7. subjectPublicKeyInfo (reuses @cap/spki)
    let spki_buf: [384]u8 = undefined;
    const spki_len = encodeSpkiDer(cert.spki, spki_buf[0..384]);
    if (spki_len < 0) {
        return X509_ERR_INVALID_SPKI;
    }
    const spki_ulen = spki_len as usize;
    if (tbs_w.remaining() < spki_ulen) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }
    i = 0;
    while (i < spki_ulen) {
        tbs_w.buf[tbs_w.pos + i] = spki_buf[i];
        i = i + 1;
    }
    tbs_w.pos = tbs_w.pos + spki_ulen;

    // Assemble outer Certificate SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue }
    let inner_buf: [1408]u8 = undefined;
    let iw = DerWriter.new(inner_buf[0..1408]);
    if (!iw.writeSequence(tbs_w.written())) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }
    if (iw.remaining() < alg_ulen) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }
    i = 0;
    while (i < alg_ulen) {
        iw.buf[iw.pos + i] = alg_buf[i];
        i = i + 1;
    }
    iw.pos = iw.pos + alg_ulen;

    if (!iw.writeBitString(0 as u8, cert.signature[0..cert.signature_len])) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }

    let out_w = DerWriter.new(out);
    if (!out_w.writeSequence(iw.written())) {
        return X509_ERR_BUFFER_TOO_SMALL;
    }
    return out_w.len() as i64;
}

/// Decode an RFC 5280 X.509 Certificate from DER bytes (zero heap allocation).
pub fn decodeX509Der(der: []const u8): X509DecodeResult {
    const empty_cert = X509Certificate.empty();
    let top_r = DerReader.new(der);
    const cert_seq = top_r.readSequence();
    if (!cert_seq.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_DER };
    }
    if (!top_r.isEmpty()) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_TRAILING_DATA };
    }

    let cert_r = cert_seq.reader;

    // 1. TBSCertificate SEQUENCE
    const tbs_seq = cert_r.readSequence();
    if (!tbs_seq.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_DER };
    }
    let tbs_r = tbs_seq.reader;

    // 1a. Optional version [0] EXPLICIT INTEGER (DEFAULT v1 = 1)
    let version: u8 = 1 as u8;
    if (!tbs_r.isEmpty() && tbs_r.peekTag() == X509_TAG_CTX0_EXPLICIT) {
        const ver_tlv = tbs_r.readTlv();
        if (!ver_tlv.ok) {
            return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_VERSION };
        }
        let ver_r = DerReader.new(ver_tlv.value);
        const ver_u64 = ver_r.readU64();
        if (!ver_u64.ok || !ver_r.isEmpty() || ver_u64.value > (2 as u64)) {
            return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_VERSION };
        }
        version = (ver_u64.value + (1 as u64)) as u8;
    }

    // 1b. serialNumber INTEGER
    const ser_tlv = tbs_r.readTlv();
    if (!ser_tlv.ok || ser_tlv.tag != TAG_INTEGER || ser_tlv.value.len == 0) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_SERIAL };
    }
    let ser_start: usize = 0;
    if (ser_tlv.value.len > 1 && ser_tlv.value[0] == (0 as u8)) {
        ser_start = 1;
    }
    const ser_len = ser_tlv.value.len - ser_start;
    if (ser_len == 0 || ser_len > X509_MAX_SERIAL_LEN) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_SERIAL };
    }
    let serial = x509Zero20();
    let i: usize = 0;
    while (i < ser_len) {
        serial[i] = ser_tlv.value[ser_start + i];
        i = i + 1;
    }

    // 1c. signature AlgorithmIdentifier (reuses @cap/spki decodeAlgorithmId)
    const tbs_alg_start = tbs_r.pos;
    const tbs_alg_tlv = tbs_r.readTlv();
    if (!tbs_alg_tlv.ok || tbs_alg_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_ALG };
    }
    const tbs_alg_res = decodeAlgorithmId(tbs_r.buf[tbs_alg_start..tbs_r.pos]);
    if (!tbs_alg_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_ALG };
    }

    // 1d. issuer Name
    const issuer_tlv = tbs_r.readTlv();
    if (!issuer_tlv.ok || issuer_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_NAME };
    }
    const issuer_res = x509DecodeName(issuer_tlv.value);
    if (!issuer_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_NAME };
    }

    // 1e. validity Validity
    const val_tlv = tbs_r.readTlv();
    if (!val_tlv.ok || val_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_VALIDITY };
    }
    const val_res = x509DecodeValidity(val_tlv.value);
    if (!val_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_VALIDITY };
    }

    // 1f. subject Name
    const subj_tlv = tbs_r.readTlv();
    if (!subj_tlv.ok || subj_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_NAME };
    }
    const subj_res = x509DecodeName(subj_tlv.value);
    if (!subj_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_NAME };
    }

    // 1g. subjectPublicKeyInfo (reuses @cap/spki decodeSpkiDer)
    const spki_start = tbs_r.pos;
    const spki_tlv = tbs_r.readTlv();
    if (!spki_tlv.ok || spki_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_SPKI };
    }
    const spki_res = decodeSpkiDer(tbs_r.buf[spki_start..tbs_r.pos]);
    if (!spki_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_SPKI };
    }

    // 1h. Skip optional [1] issuerUniqueID, [2] subjectUniqueID, [3] extensions
    if (!tbs_r.isEmpty() && tbs_r.peekTag() == X509_TAG_CTX1_IMPLICIT) {
        if (!tbs_r.readTlv().ok) {
            return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_DER };
        }
    }
    if (!tbs_r.isEmpty() && tbs_r.peekTag() == X509_TAG_CTX2_IMPLICIT) {
        if (!tbs_r.readTlv().ok) {
            return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_DER };
        }
    }
    if (!tbs_r.isEmpty() && tbs_r.peekTag() == X509_TAG_CTX3_EXPLICIT) {
        if (!tbs_r.readTlv().ok) {
            return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_DER };
        }
    }
    if (!tbs_r.isEmpty()) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_TRAILING_DATA };
    }

    // 2. Outer signatureAlgorithm AlgorithmIdentifier
    const outer_alg_start = cert_r.pos;
    const outer_alg_tlv = cert_r.readTlv();
    if (!outer_alg_tlv.ok || outer_alg_tlv.tag != TAG_SEQUENCE) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_ALG };
    }
    const outer_alg_res = decodeAlgorithmId(cert_r.buf[outer_alg_start..cert_r.pos]);
    if (!outer_alg_res.ok) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_ALG };
    }
    if (!outer_alg_res.alg.eq(tbs_alg_res.alg)) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_ALG_MISMATCH };
    }

    // 3. Outer signatureValue BIT STRING
    const sig_bs = cert_r.readBitString();
    if (!sig_bs.ok || sig_bs.unused_bits != (0 as u8) || sig_bs.bytes.len > X509_MAX_SIG_LEN) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_SIGNATURE };
    }
    if (!cert_r.isEmpty()) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_TRAILING_DATA };
    }

    let sig_buf = x509Zero128();
    i = 0;
    while (i < sig_bs.bytes.len) {
        sig_buf[i] = sig_bs.bytes[i];
        i = i + 1;
    }

    return X509DecodeResult {
        ok: true,
        cert: X509Certificate {
            version: version,
            serial: serial,
            serial_len: ser_len,
            signature_alg: outer_alg_res.alg,
            issuer: issuer_res.name,
            validity: val_res.validity,
            subject: subj_res.name,
            spki: spki_res.spki,
            signature: sig_buf,
            signature_len: sig_bs.bytes.len,
        },
        err: X509_OK,
    };
}

/// Encode an X.509 Certificate into RFC 7468 PEM (`-----BEGIN CERTIFICATE-----`).
/// Reuses `@cap/pem-rfc7468`.
pub fn encodeX509Pem(cert: X509Certificate, crlf: bool, out: []u8): i64 {
    let der_buf: [1536]u8 = undefined;
    const dlen = encodeX509Der(cert, der_buf[0..1536]);
    if (dlen < 0) {
        return dlen;
    }
    return encodePem(X509_PEM_LABEL, der_buf[0..(dlen as usize)], crlf, out);
}

/// Decode an RFC 7468 `CERTIFICATE` PEM block into an `X509Certificate`.
/// Strictly validates that the PEM label is `"CERTIFICATE"`.
pub fn decodeX509Pem(pem: string): X509DecodeResult {
    const empty_cert = X509Certificate.empty();
    let label_buf: [64]u8 = undefined;
    let label_len: usize = 0;
    let der_buf: [1536]u8 = undefined;

    const dlen = decodePem(pem, label_buf[0..64], &label_len, der_buf[0..1536]);
    if (dlen < 0) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_PEM };
    }
    if (!x509StrEqBytes(X509_PEM_LABEL, label_buf[0..label_len])) {
        return X509DecodeResult { ok: false, cert: empty_cert, err: X509_ERR_INVALID_PEM_LABEL };
    }
    return decodeX509Der(der_buf[0..(dlen as usize)]);
}

/// Return `true` if `cert.issuer` equals `cert.subject` (self-issued / self-signed DN match).
pub fn isSelfSigned(cert: X509Certificate): bool {
    return cert.issuer.eq(cert.subject);
}

export {
    X509_PEM_LABEL,
    X509_MAX_SERIAL_LEN,
    X509_MAX_SIG_LEN,
    X509_OK,
    X509_ERR_BUFFER_TOO_SMALL,
    X509_ERR_INVALID_DER,
    X509_ERR_INVALID_VERSION,
    X509_ERR_INVALID_SERIAL,
    X509_ERR_INVALID_ALG,
    X509_ERR_ALG_MISMATCH,
    X509_ERR_INVALID_NAME,
    X509_ERR_INVALID_VALIDITY,
    X509_ERR_INVALID_SPKI,
    X509_ERR_INVALID_SIGNATURE,
    X509_ERR_INVALID_PEM,
    X509_ERR_INVALID_PEM_LABEL,
    X509_ERR_TRAILING_DATA,
    X509Validity,
    X509Name,
    X509Certificate,
    X509DecodeResult,
    encodeX509Der,
    decodeX509Der,
    encodeX509Pem,
    decodeX509Pem,
    isSelfSigned,
};
